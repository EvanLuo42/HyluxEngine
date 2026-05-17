/// @file
/// @brief FilesystemBlobStore implementation. Two backend variants are compiled
///        conditionally: Win32 (CreateFileW + CreateFileMappingW + MapViewOfFile) and
///        POSIX (open + mmap). A heap-load fallback covers zero-byte files and map
///        failures.

#include "Core/IO/Blob/FilesystemBlobStore.h"

#include <cstddef>
#include <cstring>
#include <fstream>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

namespace Hylux
{

namespace
{

/// @brief Heap-buffer IMappedBlob used when mmap is unavailable or the file is empty.
class HeapBlob final : public IMappedBlob
{
public:
    HeapBlob(std::vector<std::byte> bytes, std::string debugSource)
        : bytes_(std::move(bytes)), debugSource_(std::move(debugSource))
    {
    }

    [[nodiscard]] std::span<const std::byte> Bytes() const noexcept override
    {
        return {bytes_.data(), bytes_.size()};
    }

    [[nodiscard]] std::string_view DebugSource() const noexcept override { return debugSource_; }

private:
    std::vector<std::byte> bytes_;
    std::string            debugSource_;
};

#if defined(_WIN32)

/// @brief Win32 file-mapping-backed IMappedBlob. Closes view, mapping, and file in dtor.
class Win32MappedBlob final : public IMappedBlob
{
public:
    Win32MappedBlob(HANDLE file, HANDLE mapping, void* view, std::size_t size,
                    std::string debugSource)
        : file_(file), mapping_(mapping), view_(view), size_(size),
          debugSource_(std::move(debugSource))
    {
    }

    ~Win32MappedBlob() override
    {
        if (view_ != nullptr)
        {
            ::UnmapViewOfFile(view_);
        }
        if (mapping_ != nullptr)
        {
            ::CloseHandle(mapping_);
        }
        if (file_ != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(file_);
        }
    }

    [[nodiscard]] std::span<const std::byte> Bytes() const noexcept override
    {
        return {static_cast<const std::byte*>(view_), size_};
    }

    [[nodiscard]] std::string_view DebugSource() const noexcept override { return debugSource_; }

private:
    HANDLE      file_{INVALID_HANDLE_VALUE};
    HANDLE      mapping_{nullptr};
    void*       view_{nullptr};
    std::size_t size_{0};
    std::string debugSource_;
};

std::unique_ptr<IMappedBlob> OpenWin32(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto status = std::filesystem::status(path, ec);
    if (ec || !std::filesystem::is_regular_file(status))
    {
        return nullptr;
    }
    const auto fileSize = std::filesystem::file_size(path, ec);
    if (ec)
    {
        return nullptr;
    }

    const std::wstring wide = path.wstring();
    HANDLE file = ::CreateFileW(wide.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return nullptr;
    }

    if (fileSize == 0)
    {
        ::CloseHandle(file);
        return std::make_unique<HeapBlob>(std::vector<std::byte>{}, path.string());
    }

    HANDLE mapping = ::CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (mapping == nullptr)
    {
        ::CloseHandle(file);
        return nullptr;
    }

    void* view = ::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (view == nullptr)
    {
        ::CloseHandle(mapping);
        ::CloseHandle(file);
        return nullptr;
    }

    return std::make_unique<Win32MappedBlob>(file, mapping, view,
                                             static_cast<std::size_t>(fileSize), path.string());
}

#else

/// @brief POSIX mmap-backed IMappedBlob. Closes view and fd in dtor.
class PosixMappedBlob final : public IMappedBlob
{
public:
    PosixMappedBlob(int fd, void* view, std::size_t size, std::string debugSource)
        : fd_(fd), view_(view), size_(size), debugSource_(std::move(debugSource))
    {
    }

    ~PosixMappedBlob() override
    {
        if (view_ != MAP_FAILED && view_ != nullptr)
        {
            ::munmap(view_, size_);
        }
        if (fd_ >= 0)
        {
            ::close(fd_);
        }
    }

    [[nodiscard]] std::span<const std::byte> Bytes() const noexcept override
    {
        return {static_cast<const std::byte*>(view_), size_};
    }

    [[nodiscard]] std::string_view DebugSource() const noexcept override { return debugSource_; }

private:
    int         fd_{-1};
    void*       view_{nullptr};
    std::size_t size_{0};
    std::string debugSource_;
};

std::unique_ptr<IMappedBlob> OpenPosix(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto status = std::filesystem::status(path, ec);
    if (ec || !std::filesystem::is_regular_file(status))
    {
        return nullptr;
    }
    const auto fileSize = std::filesystem::file_size(path, ec);
    if (ec)
    {
        return nullptr;
    }

    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        return nullptr;
    }

    if (fileSize == 0)
    {
        ::close(fd);
        return std::make_unique<HeapBlob>(std::vector<std::byte>{}, path.string());
    }

    void* view = ::mmap(nullptr, static_cast<std::size_t>(fileSize), PROT_READ,
                        MAP_PRIVATE, fd, 0);
    if (view == MAP_FAILED)
    {
        ::close(fd);
        return nullptr;
    }

    return std::make_unique<PosixMappedBlob>(fd, view, static_cast<std::size_t>(fileSize),
                                             path.string());
}

#endif

std::unique_ptr<IMappedBlob> OpenHeapFallback(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream.is_open())
    {
        return nullptr;
    }
    const auto end = stream.tellg();
    if (end <= 0)
    {
        return std::make_unique<HeapBlob>(std::vector<std::byte>{}, path.string());
    }
    std::vector<std::byte> buffer(static_cast<std::size_t>(end));
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(buffer.data()),
                static_cast<std::streamsize>(buffer.size()));
    if (!stream)
    {
        return nullptr;
    }
    return std::make_unique<HeapBlob>(std::move(buffer), path.string());
}

} // namespace

FilesystemBlobStore::FilesystemBlobStore(std::filesystem::path root)
    : root_(std::move(root))
{
    debugName_ = "filesystem:" + root_.string();
}

std::filesystem::path FilesystemBlobStore::Resolve(std::string_view logicalKey) const
{
    return root_ / std::filesystem::path(logicalKey);
}

std::unique_ptr<IMappedBlob> FilesystemBlobStore::OpenMapped(std::string_view logicalKey)
{
    const auto path = Resolve(logicalKey);

#if defined(_WIN32)
    if (auto blob = OpenWin32(path))
    {
        return blob;
    }
#else
    if (auto blob = OpenPosix(path))
    {
        return blob;
    }
#endif

    return OpenHeapFallback(path);
}

bool FilesystemBlobStore::Exists(std::string_view logicalKey) const
{
    std::error_code ec;
    return std::filesystem::is_regular_file(Resolve(logicalKey), ec);
}

std::optional<std::filesystem::file_time_type>
FilesystemBlobStore::LastModified(std::string_view logicalKey) const
{
    std::error_code ec;
    const auto stamp = std::filesystem::last_write_time(Resolve(logicalKey), ec);
    if (ec)
    {
        return std::nullopt;
    }
    return stamp;
}

} // namespace Hylux
