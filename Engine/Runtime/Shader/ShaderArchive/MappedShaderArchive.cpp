/// @file
/// @brief MappedShaderArchive implementation. Validates header, slices index and pool
///        regions out of the underlying mapped blob, then services lookups via
///        std::lower_bound. All returned pointers and spans alias into the mapped region.

#include "Shader/ShaderArchive/MappedShaderArchive.h"

#include "Shader/Diagnostics/ShaderLogCategories.h"

#include "Core/Logging/Logger.h"

#include <algorithm>
#include <cstring>
#include <tuple>

namespace Hylux::Shader
{

namespace
{

bool RangeIsInside(std::size_t totalSize, std::uint32_t offset, std::uint32_t size) noexcept
{
    const std::uint64_t end = static_cast<std::uint64_t>(offset) + static_cast<std::uint64_t>(size);
    return end <= totalSize;
}

template <typename T>
std::span<const T> SliceArray(std::span<const std::byte> blob, std::uint32_t offset,
                              std::uint32_t count)
{
    const std::size_t bytes = static_cast<std::size_t>(count) * sizeof(T);
    if (!RangeIsInside(blob.size(), offset, static_cast<std::uint32_t>(bytes)))
    {
        return {};
    }
    const auto* base = reinterpret_cast<const T*>(blob.data() + offset);
    return {base, count};
}

std::span<const std::byte> SliceBytes(std::span<const std::byte> blob, std::uint32_t offset,
                                      std::uint32_t size)
{
    if (!RangeIsInside(blob.size(), offset, size))
    {
        return {};
    }
    return blob.subspan(offset, size);
}

template <typename Entry>
auto PassKeyOf(const Entry& e) noexcept
{
    return std::tuple<std::uint64_t, std::uint64_t, std::uint32_t>{
        e.passNameHash, e.permutationKey, e.stage};
}

template <typename Entry>
auto MaterialKeyOf(const Entry& e) noexcept
{
    return std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint32_t>{
        e.materialAssetHash, e.passIdHash, e.permutationKey, e.stage};
}

} // namespace

std::unique_ptr<MappedShaderArchive>
MappedShaderArchive::Open(std::unique_ptr<IMappedBlob> blob)
{
    if (blob == nullptr)
    {
        HYLUX_LOG_ERROR(LogShaderArchive, "MappedShaderArchive::Open: null blob");
        return nullptr;
    }

    const auto bytes = blob->Bytes();
    if (bytes.size() < sizeof(ArchiveFormat::Header))
    {
        HYLUX_LOG_ERROR(LogShaderArchive,
                        "MappedShaderArchive::Open: blob {} too small ({} bytes) for header",
                        blob->DebugSource(), bytes.size());
        return nullptr;
    }

    ArchiveFormat::Header header{};
    std::memcpy(&header, bytes.data(), sizeof(header));

    if (header.magic != ArchiveFormat::kMagic)
    {
        HYLUX_LOG_ERROR(LogShaderArchive,
                        "MappedShaderArchive::Open: blob {} has wrong magic 0x{:08x}",
                        blob->DebugSource(), header.magic);
        return nullptr;
    }
    if (header.version != ArchiveFormat::kVersion)
    {
        HYLUX_LOG_ERROR(LogShaderArchive,
                        "MappedShaderArchive::Open: blob {} version {} unsupported (expected {})",
                        blob->DebugSource(), header.version, ArchiveFormat::kVersion);
        return nullptr;
    }

    auto archive = std::unique_ptr<MappedShaderArchive>(new MappedShaderArchive());
    archive->debugName_     = std::string(blob->DebugSource());
    archive->bytecodeFormat_ = static_cast<RHI::ShaderBytecodeFormat>(header.bytecodeFormat);

    archive->passEntries_ = SliceArray<ArchiveFormat::PassEntry>(
        bytes, header.passIndexOffset, header.passIndexCount);
    archive->materialEntries_ = SliceArray<ArchiveFormat::MaterialEntry>(
        bytes, header.materialIndexOffset, header.materialIndexCount);
    archive->blobPool_ = SliceBytes(bytes, header.blobPoolOffset, header.blobPoolSize);
    archive->reflectionPool_ =
        SliceBytes(bytes, header.reflectionPoolOffset, header.reflectionPoolSize);

    const auto stringBytes =
        SliceBytes(bytes, header.stringPoolOffset, header.stringPoolSize);
    archive->stringPool_ = {reinterpret_cast<const char*>(stringBytes.data()), stringBytes.size()};

    const bool passOk     = archive->passEntries_.size() == header.passIndexCount;
    const bool materialOk = archive->materialEntries_.size() == header.materialIndexCount;
    const bool blobOk     = archive->blobPool_.size() == header.blobPoolSize;
    const bool reflectionOk =
        archive->reflectionPool_.size() == header.reflectionPoolSize;
    const bool stringOk = archive->stringPool_.size() == header.stringPoolSize;

    if (!passOk || !materialOk || !blobOk || !reflectionOk || !stringOk)
    {
        HYLUX_LOG_ERROR(LogShaderArchive,
                        "MappedShaderArchive::Open: blob {} has truncated sections "
                        "(pass={}, material={}, blob={}, reflection={}, string={})",
                        blob->DebugSource(), passOk, materialOk, blobOk, reflectionOk, stringOk);
        return nullptr;
    }

    archive->idSet_.reserve(archive->passEntries_.size() + archive->materialEntries_.size());
    for (const auto& entry : archive->passEntries_)
    {
        const PassShaderKey k{entry.passNameHash, entry.permutationKey,
                              static_cast<RHI::ShaderStage>(entry.stage)};
        archive->idSet_.insert(MakeShaderId(k).value);
    }
    for (const auto& entry : archive->materialEntries_)
    {
        const MaterialShaderKey k{entry.materialAssetHash, entry.passIdHash,
                                  entry.permutationKey,
                                  static_cast<RHI::ShaderStage>(entry.stage)};
        archive->idSet_.insert(MakeShaderId(k).value);
    }

    archive->blob_ = std::move(blob);

    HYLUX_LOG_INFO(LogShaderArchive,
                   "MappedShaderArchive: opened {} ({} pass entries, {} material entries, "
                   "blob pool {} bytes)",
                   archive->debugName_, archive->passEntries_.size(),
                   archive->materialEntries_.size(), archive->blobPool_.size());

    return archive;
}

std::optional<ShaderRecord> MappedShaderArchive::FindPass(const PassShaderKey& key) const
{
    const auto target = std::tuple<std::uint64_t, std::uint64_t, std::uint32_t>{
        key.passNameHash, key.permutationKey, static_cast<std::uint32_t>(key.stage)};

    const auto it = std::lower_bound(
        passEntries_.begin(), passEntries_.end(), target,
        [](const ArchiveFormat::PassEntry& e, const auto& t) { return PassKeyOf(e) < t; });

    if (it == passEntries_.end() || PassKeyOf(*it) != target)
    {
        return std::nullopt;
    }
    return MakeRecordFromPass(*it);
}

std::optional<ShaderRecord>
MappedShaderArchive::FindMaterial(const MaterialShaderKey& key) const
{
    const auto target = std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint32_t>{
        key.materialAssetHash, key.passIdHash, key.permutationKey,
        static_cast<std::uint32_t>(key.stage)};

    const auto it = std::lower_bound(
        materialEntries_.begin(), materialEntries_.end(), target,
        [](const ArchiveFormat::MaterialEntry& e, const auto& t) { return MaterialKeyOf(e) < t; });

    if (it == materialEntries_.end() || MaterialKeyOf(*it) != target)
    {
        return std::nullopt;
    }
    return MakeRecordFromMaterial(*it);
}

ShaderRecord MappedShaderArchive::MakeRecordFromPass(const ArchiveFormat::PassEntry& e) const
{
    ShaderRecord rec{};
    const PassShaderKey k{e.passNameHash, e.permutationKey,
                          static_cast<RHI::ShaderStage>(e.stage)};
    rec.id         = MakeShaderId(k);
    rec.stage      = static_cast<RHI::ShaderStage>(e.stage);
    rec.format     = bytecodeFormat_;
    rec.bytecode   = SliceBytes(blobPool_, e.blobOffset, e.blobSize);
    rec.sourceHash = e.sourceHash;

    if (e.reflectionSize >= sizeof(ShaderReflection) &&
        RangeIsInside(reflectionPool_.size(), e.reflectionOffset, e.reflectionSize))
    {
        rec.reflection = reinterpret_cast<const ShaderReflection*>(
            reflectionPool_.data() + e.reflectionOffset);
    }

    if (e.entryPointStringOffset + e.entryPointStringSize <= stringPool_.size())
    {
        rec.entryPoint = std::string_view(stringPool_.data() + e.entryPointStringOffset,
                                          e.entryPointStringSize);
    }
    return rec;
}

ShaderRecord
MappedShaderArchive::MakeRecordFromMaterial(const ArchiveFormat::MaterialEntry& e) const
{
    ShaderRecord rec{};
    const MaterialShaderKey k{e.materialAssetHash, e.passIdHash, e.permutationKey,
                              static_cast<RHI::ShaderStage>(e.stage)};
    rec.id         = MakeShaderId(k);
    rec.stage      = static_cast<RHI::ShaderStage>(e.stage);
    rec.format     = bytecodeFormat_;
    rec.bytecode   = SliceBytes(blobPool_, e.blobOffset, e.blobSize);
    rec.sourceHash = e.sourceHash;

    if (e.reflectionSize >= sizeof(ShaderReflection) &&
        RangeIsInside(reflectionPool_.size(), e.reflectionOffset, e.reflectionSize))
    {
        rec.reflection = reinterpret_cast<const ShaderReflection*>(
            reflectionPool_.data() + e.reflectionOffset);
    }

    if (e.entryPointStringOffset + e.entryPointStringSize <= stringPool_.size())
    {
        rec.entryPoint = std::string_view(stringPool_.data() + e.entryPointStringOffset,
                                          e.entryPointStringSize);
    }
    return rec;
}

} // namespace Hylux::Shader
