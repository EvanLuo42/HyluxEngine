/// @file
/// @brief Unit tests for the VirtualFileSystem mount table: longest-prefix, priority overlay,
///        read fall-through, and write no-fall-through (with an in-test ReadOnly mock provider).

#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/IFileProvider.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"
#include "Core/IO/Virtual/VirtualFileSystem.h"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>

using namespace Hylux;

namespace
{

std::filesystem::path MakeTempDir()
{
    const auto base = std::filesystem::temp_directory_path();
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto dir = base / ("hylux-vfs-vfs-test-" + std::to_string(stamp));
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

void RemoveAllSilent(const std::filesystem::path& p)
{
    std::error_code ec;
    std::filesystem::remove_all(p, ec);
}

void WriteFile(const std::filesystem::path& p, std::string_view bytes)
{
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    std::ofstream out(p, std::ios::binary);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

std::string ReadAll(IFile& f)
{
    std::string out;
    std::array<char, 256> buf{};
    while (true)
    {
        const auto n = f.Read(buf.data(), buf.size());
        if (n == 0) break;
        out.append(buf.data(), static_cast<std::size_t>(n));
    }
    return out;
}

/// @brief Tiny in-memory, read-only IFile used by MemoryReadOnlyProvider for tests.
class MemoryReadOnlyFile final : public IFile
{
public:
    explicit MemoryReadOnlyFile(std::string data) : data_(std::move(data)) {}

    FileSize Read(void* buffer, FileSize bytes) override
    {
        const FileSize remaining = data_.size() - cursor_;
        const FileSize toCopy    = bytes < remaining ? bytes : remaining;
        std::memcpy(buffer, data_.data() + cursor_, static_cast<std::size_t>(toCopy));
        cursor_ += toCopy;
        return toCopy;
    }
    FileSize Write(const void*, FileSize) override { return 0; }
    bool     Seek(FileOffset offset, SeekOrigin origin) override
    {
        FileOffset target = 0;
        switch (origin)
        {
            case SeekOrigin::Begin:   target = offset; break;
            case SeekOrigin::Current: target = static_cast<FileOffset>(cursor_) + offset; break;
            case SeekOrigin::End:     target = static_cast<FileOffset>(data_.size()) + offset; break;
        }
        if (target < 0 || static_cast<FileSize>(target) > data_.size()) return false;
        cursor_ = static_cast<FileSize>(target);
        return true;
    }
    FileOffset Tell() const override { return static_cast<FileOffset>(cursor_); }
    void       Flush() override {}
    FileSize   Size() const override { return data_.size(); }
    bool       IsValid() const noexcept override { return true; }

private:
    std::string data_;
    FileSize    cursor_{0};
};

/// @brief In-memory provider for tests. Reports SupportsWrite()=false so we can verify the
///        VFS does NOT fall through writes to a lower-priority writable provider.
class MemoryReadOnlyProvider final : public IFileProvider
{
public:
    void Add(std::string subPath, std::string contents)
    {
        files_.emplace(std::move(subPath), std::move(contents));
    }

    std::unique_ptr<IFile> Open(std::string_view subPath, FileOpenMode mode) override
    {
        if (mode != FileOpenMode::Read) return nullptr;
        const auto it = files_.find(std::string(subPath));
        if (it == files_.end()) return nullptr;
        return std::make_unique<MemoryReadOnlyFile>(it->second);
    }
    bool Exists(std::string_view subPath) const override
    {
        return files_.find(std::string(subPath)) != files_.end();
    }
    FileStat Stat(std::string_view subPath) const override
    {
        const auto it = files_.find(std::string(subPath));
        if (it == files_.end()) return {};
        FileStat s;
        s.exists        = true;
        s.isRegularFile = true;
        s.size          = it->second.size();
        return s;
    }
    bool CreateDirectories(std::string_view) override { return false; }
    bool Remove(std::string_view) override { return false; }

    bool        SupportsWrite() const noexcept override { return false; }
    const char* DebugName() const noexcept override { return "MemoryReadOnly"; }

private:
    std::unordered_map<std::string, std::string> files_;
};

} // namespace

TEST_CASE("VirtualFileSystem rejects malformed paths and prefixes")
{
    VirtualFileSystem vfs;
    auto provider = std::make_shared<MemoryReadOnlyProvider>();
    provider->Add("x", "");

    CHECK(vfs.Mount("Engine/", provider, 0)   == MountId::Invalid);
    CHECK(vfs.Mount("/Engine",  provider, 0)  == MountId::Invalid);
    CHECK(vfs.Mount("/",        provider, 0)  == MountId::Invalid);
    CHECK(vfs.Mount("/A//B/",   provider, 0)  == MountId::Invalid);
    CHECK(vfs.Mount("/Engine/", nullptr, 0)   == MountId::Invalid);

    const auto id = vfs.Mount("/Engine/", provider, 0);
    CHECK(id != MountId::Invalid);

    CHECK(vfs.Open("Engine/x", FileOpenMode::Read) == nullptr);
    CHECK_FALSE(vfs.Exists("../etc/passwd"));
}

TEST_CASE("VirtualFileSystem longest-prefix wins")
{
    const auto tempDir = MakeTempDir();
    WriteFile(tempDir / "outer/foo.txt", "outer-foo");
    WriteFile(tempDir / "inner/foo.txt", "inner-foo");

    VirtualFileSystem vfs;
    vfs.Mount("/A/",     std::make_shared<LooseFileProvider>(tempDir / "outer"), 0);
    vfs.Mount("/A/Sub/", std::make_shared<LooseFileProvider>(tempDir / "inner"), 0);

    auto file = vfs.Open("/A/Sub/foo.txt", FileOpenMode::Read);
    REQUIRE(file != nullptr);
    CHECK(ReadAll(*file) == "inner-foo");

    auto outer = vfs.Open("/A/foo.txt", FileOpenMode::Read);
    REQUIRE(outer != nullptr);
    CHECK(ReadAll(*outer) == "outer-foo");

    RemoveAllSilent(tempDir);
}

TEST_CASE("VirtualFileSystem priority overlay shadows lower priority and falls through on miss")
{
    const auto tempDir = MakeTempDir();
    WriteFile(tempDir / "base/config.ini", "base");
    WriteFile(tempDir / "base/only-base.txt", "only-base");
    WriteFile(tempDir / "patch/config.ini", "patch");

    VirtualFileSystem vfs;
    vfs.Mount("/X/", std::make_shared<LooseFileProvider>(tempDir / "base"),  0);
    const auto patchId = vfs.Mount("/X/", std::make_shared<LooseFileProvider>(tempDir / "patch"), 10);

    auto overlay = vfs.Open("/X/config.ini", FileOpenMode::Read);
    REQUIRE(overlay != nullptr);
    CHECK(ReadAll(*overlay) == "patch");

    auto fallthrough = vfs.Open("/X/only-base.txt", FileOpenMode::Read);
    REQUIRE(fallthrough != nullptr);
    CHECK(ReadAll(*fallthrough) == "only-base");

    REQUIRE(vfs.Unmount(patchId));
    auto baseAgain = vfs.Open("/X/config.ini", FileOpenMode::Read);
    REQUIRE(baseAgain != nullptr);
    CHECK(ReadAll(*baseAgain) == "base");

    RemoveAllSilent(tempDir);
}

TEST_CASE("VirtualFileSystem Write does not fall through to lower-priority providers")
{
    const auto tempDir = MakeTempDir();
    VirtualFileSystem vfs;

    auto readOnly = std::make_shared<MemoryReadOnlyProvider>();
    readOnly->Add("file.txt", "ro-contents");

    auto writable = std::make_shared<LooseFileProvider>(tempDir);

    vfs.Mount("/X/", writable, 0);
    vfs.Mount("/X/", readOnly, 10);

    auto reader = vfs.Open("/X/file.txt", FileOpenMode::Read);
    REQUIRE(reader != nullptr);
    CHECK(ReadAll(*reader) == "ro-contents");

    auto writer = vfs.Open("/X/file.txt", FileOpenMode::Write);
    CHECK(writer == nullptr);

    CHECK_FALSE(std::filesystem::exists(tempDir / "file.txt"));

    RemoveAllSilent(tempDir);
}

TEST_CASE("VirtualFileSystem EnumerateMounts returns sorted snapshot")
{
    VirtualFileSystem vfs;
    auto p = std::make_shared<MemoryReadOnlyProvider>();
    vfs.Mount("/Engine/",  p, 0);
    vfs.Mount("/Game/",    p, 0);
    vfs.Mount("/Engine/",  p, 10);

    const auto info = vfs.EnumerateMounts();
    REQUIRE(info.size() == 3);
    CHECK(info[0].prefix == "/Engine/");
    CHECK(info[0].priority == 10);
}
