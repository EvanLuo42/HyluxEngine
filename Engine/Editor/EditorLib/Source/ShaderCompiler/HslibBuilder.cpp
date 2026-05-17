/// @file
/// @brief HslibBuilder implementation. Lays out header → pass index → material index →
///        string pool → reflection pool → blob pool, with 16-byte alignment between
///        sections so the runtime can cast structs out of the mapped blob directly.

#include "ShaderCompiler/HslibBuilder.h"

#include "Shader/ShaderArchive/ArchiveFormat.h"
#include "Shader/ShaderReflection.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <system_error>

namespace Hylux::Editor
{
namespace
{

[[nodiscard]] inline std::size_t AlignUp(std::size_t value, std::size_t alignment) noexcept
{
    return alignment <= 1 ? value : ((value + alignment - 1) & ~(alignment - 1));
}

/// @brief Appends raw bytes to the buffer; returns the offset at which they were written.
template<typename T>
inline std::size_t AppendBytes(std::vector<std::byte>& dst, const T* src, std::size_t count)
{
    const std::size_t offset    = dst.size();
    const std::size_t byteCount = sizeof(T) * count;
    dst.resize(offset + byteCount);
    std::memcpy(dst.data() + offset, src, byteCount);
    return offset;
}

inline void PadTo(std::vector<std::byte>& dst, std::size_t alignment)
{
    while ((dst.size() % alignment) != 0)
    {
        dst.push_back(std::byte{0});
    }
}

} // namespace

void HslibBuilder::AddPassEntry(HslibPassEntryInput entry)
{
    passEntries_.push_back(std::move(entry));
}

bool HslibBuilder::WriteToFile(const std::filesystem::path& path) const
{
    // 1. Sort pass entries by lookup key — MappedShaderArchive's binary search depends
    //    on (passNameHash, permutationKey, stage) ordering.
    std::vector<HslibPassEntryInput> entries = passEntries_;
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        if (a.passNameHash    != b.passNameHash)    return a.passNameHash    < b.passNameHash;
        if (a.permutationKey  != b.permutationKey)  return a.permutationKey  < b.permutationKey;
        return static_cast<std::uint32_t>(a.stage) < static_cast<std::uint32_t>(b.stage);
    });

    // 2. Build the variable-length pools first (string, reflection, blob) so we know
    //    their offsets relative to the front of the archive when laying out the header.
    std::vector<std::byte>                              stringPool;
    std::vector<Hylux::Shader::ShaderReflection>        reflectionRecords;
    std::vector<std::byte>                              blobPool;

    struct EntryLayout
    {
        std::uint32_t entryPointStringOffset;
        std::uint32_t entryPointStringSize;
        std::uint32_t reflectionOffset;
        std::uint32_t reflectionSize;
        std::uint32_t blobOffset;
        std::uint32_t blobSize;
    };
    std::vector<EntryLayout> layouts;
    layouts.reserve(entries.size());

    constexpr std::size_t kSectionAlignment = 16;

    for (const auto& entry : entries)
    {
        EntryLayout layout{};

        // Entry-point name into string pool.
        layout.entryPointStringOffset = static_cast<std::uint32_t>(stringPool.size());
        layout.entryPointStringSize   = static_cast<std::uint32_t>(entry.entryPoint.size());
        AppendBytes(stringPool, entry.entryPoint.data(), entry.entryPoint.size());
        stringPool.push_back(std::byte{0}); // NUL terminator for convenience

        // Reflection record. We write one ShaderReflection per entry; sourceFile path is
        // optional, stored in the string pool when present.
        Hylux::Shader::ShaderReflection refl{};
        refl.pushConstantSize       = entry.pushConstantSize;
        refl.pushConstantVisibility = entry.pushConstantVisibility;
        refl.pipelineLayoutHash     = entry.pipelineLayoutHash;
        refl.descriptorBindingOffset = 0;
        refl.descriptorBindingCount  = 0;
        refl.vertexAttributeOffset   = 0;
        refl.vertexAttributeCount    = 0;
        if (!entry.sourceFilePath.empty())
        {
            refl.sourceFileStringOffset = static_cast<std::uint32_t>(stringPool.size());
            refl.sourceFileStringSize   = static_cast<std::uint32_t>(entry.sourceFilePath.size());
            AppendBytes(stringPool, entry.sourceFilePath.data(), entry.sourceFilePath.size());
            stringPool.push_back(std::byte{0});
        }

        layout.reflectionOffset = static_cast<std::uint32_t>(reflectionRecords.size() *
                                                              sizeof(Hylux::Shader::ShaderReflection));
        layout.reflectionSize   = static_cast<std::uint32_t>(sizeof(Hylux::Shader::ShaderReflection));
        reflectionRecords.push_back(refl);

        // SPIR-V blob, padded to 4 bytes.
        layout.blobOffset = static_cast<std::uint32_t>(blobPool.size());
        layout.blobSize   = static_cast<std::uint32_t>(entry.spirv.size());
        AppendBytes(blobPool, entry.spirv.data(), entry.spirv.size());
        PadTo(blobPool, 4);

        layouts.push_back(layout);
    }

    PadTo(stringPool, kSectionAlignment);
    PadTo(blobPool,  kSectionAlignment);

    // 3. Build the pass index (no material entries for stage 4b).
    std::vector<Hylux::Shader::ArchiveFormat::PassEntry> passIndex;
    passIndex.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        const auto& e = entries[i];
        const auto& l = layouts[i];
        Hylux::Shader::ArchiveFormat::PassEntry pe{};
        pe.passNameHash           = e.passNameHash;
        pe.permutationKey         = e.permutationKey;
        pe.stage                  = static_cast<std::uint32_t>(e.stage);
        pe.reserved               = 0;
        pe.blobOffset             = l.blobOffset;
        pe.blobSize               = l.blobSize;
        pe.reflectionOffset       = l.reflectionOffset;
        pe.reflectionSize         = l.reflectionSize;
        pe.entryPointStringOffset = l.entryPointStringOffset;
        pe.entryPointStringSize   = l.entryPointStringSize;
        pe.sourceHash             = e.sourceHash;
        passIndex.push_back(pe);
    }

    // 4. Lay out the file: header → pass index → (empty) material index → string pool →
    //    reflection pool → blob pool, with section alignment between each.
    std::size_t cursor = AlignUp(sizeof(Hylux::Shader::ArchiveFormat::Header), kSectionAlignment);
    const std::size_t passIndexOffset = cursor;
    cursor += sizeof(Hylux::Shader::ArchiveFormat::PassEntry) * passIndex.size();
    cursor  = AlignUp(cursor, kSectionAlignment);

    const std::size_t materialIndexOffset = cursor; // count = 0
    cursor = AlignUp(cursor, kSectionAlignment);

    const std::size_t stringPoolOffset = cursor;
    cursor += stringPool.size();
    cursor  = AlignUp(cursor, kSectionAlignment);

    const std::size_t reflectionPoolOffset = cursor;
    cursor += reflectionRecords.size() * sizeof(Hylux::Shader::ShaderReflection);
    cursor  = AlignUp(cursor, kSectionAlignment);

    const std::size_t blobPoolOffset = cursor;
    cursor += blobPool.size();

    // 5. Compose the final buffer.
    std::vector<std::byte> archive(cursor, std::byte{0});

    Hylux::Shader::ArchiveFormat::Header header{};
    header.magic                = Hylux::Shader::ArchiveFormat::kMagic;
    header.version              = Hylux::Shader::ArchiveFormat::kVersion;
    header.bytecodeFormat       = static_cast<std::uint32_t>(RHI::ShaderBytecodeFormat::Spirv);
    header.reservedA            = 0;
    header.targetTripleHash     = 0; // Stage 4b: no per-target validation.
    header.passIndexOffset      = static_cast<std::uint32_t>(passIndexOffset);
    header.passIndexCount       = static_cast<std::uint32_t>(passIndex.size());
    header.materialIndexOffset  = static_cast<std::uint32_t>(materialIndexOffset);
    header.materialIndexCount   = 0;
    header.blobPoolOffset       = static_cast<std::uint32_t>(blobPoolOffset);
    header.blobPoolSize         = static_cast<std::uint32_t>(blobPool.size());
    header.reflectionPoolOffset = static_cast<std::uint32_t>(reflectionPoolOffset);
    header.reflectionPoolSize   = static_cast<std::uint32_t>(reflectionRecords.size() *
                                                              sizeof(Hylux::Shader::ShaderReflection));
    header.stringPoolOffset     = static_cast<std::uint32_t>(stringPoolOffset);
    header.stringPoolSize       = static_cast<std::uint32_t>(stringPool.size());

    std::memcpy(archive.data(), &header, sizeof(header));
    if (!passIndex.empty())
    {
        std::memcpy(archive.data() + passIndexOffset, passIndex.data(),
                    passIndex.size() * sizeof(Hylux::Shader::ArchiveFormat::PassEntry));
    }
    if (!stringPool.empty())
    {
        std::memcpy(archive.data() + stringPoolOffset, stringPool.data(), stringPool.size());
    }
    if (!reflectionRecords.empty())
    {
        std::memcpy(archive.data() + reflectionPoolOffset, reflectionRecords.data(),
                    reflectionRecords.size() * sizeof(Hylux::Shader::ShaderReflection));
    }
    if (!blobPool.empty())
    {
        std::memcpy(archive.data() + blobPoolOffset, blobPool.data(), blobPool.size());
    }

    // 6. Write atomically: temp file + rename.
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    auto temp = path;
    temp += ".tmp";

    {
        std::ofstream stream(temp, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            return false;
        }
        stream.write(reinterpret_cast<const char*>(archive.data()),
                     static_cast<std::streamsize>(archive.size()));
        if (!stream)
        {
            return false;
        }
    }

    std::filesystem::rename(temp, path, ec);
    if (ec)
    {
        std::filesystem::remove(path, ec);
        std::filesystem::rename(temp, path, ec);
        if (ec)
        {
            std::filesystem::remove(temp, ec);
            return false;
        }
    }
    return true;
}

} // namespace Hylux::Editor
