/// @file
/// @brief IShaderArchive implementation that parses a memory-mapped .hslib blob. Holds a
///        strong reference to the IMappedBlob and exposes binary-search lookups against
///        the on-disk index tables.

#pragma once

#include "Core/IO/Blob/IMappedBlob.h"
#include "Shader/ShaderArchive/ArchiveFormat.h"
#include "Shader/ShaderArchive/IShaderArchive.h"

#include <memory>
#include <string>
#include <unordered_set>

namespace Hylux::Shader
{

/// @brief Concrete archive backed by an IMappedBlob. Constructed via Open(); returns
///        nullptr on header/version/range failures (errors are logged).
class MappedShaderArchive final : public IShaderArchive
{
public:
    /// @brief Validates the blob's header and constructs the archive. Returns nullptr if
    ///        the blob is malformed or the version is unsupported.
    [[nodiscard]] static std::unique_ptr<MappedShaderArchive>
        Open(std::unique_ptr<IMappedBlob> blob);

    ~MappedShaderArchive() override = default;

    [[nodiscard]] std::optional<ShaderRecord> FindPass(const PassShaderKey& key) const override;
    [[nodiscard]] std::optional<ShaderRecord>
        FindMaterial(const MaterialShaderKey& key) const override;
    [[nodiscard]] RHI::ShaderBytecodeFormat BytecodeFormat() const noexcept override
    {
        return bytecodeFormat_;
    }
    [[nodiscard]] std::size_t PassEntryCount() const noexcept override { return passEntries_.size(); }
    [[nodiscard]] std::size_t MaterialEntryCount() const noexcept override
    {
        return materialEntries_.size();
    }
    [[nodiscard]] bool ContainsId(ShaderId id) const override
    {
        return idSet_.find(id.value) != idSet_.end();
    }
    [[nodiscard]] std::string_view DebugName() const noexcept override { return debugName_; }

private:
    MappedShaderArchive() = default;

    [[nodiscard]] ShaderRecord MakeRecordFromPass(const ArchiveFormat::PassEntry&) const;
    [[nodiscard]] ShaderRecord MakeRecordFromMaterial(const ArchiveFormat::MaterialEntry&) const;

    std::unique_ptr<IMappedBlob>           blob_;
    std::string                            debugName_;
    RHI::ShaderBytecodeFormat              bytecodeFormat_{RHI::ShaderBytecodeFormat::Spirv};
    std::span<const ArchiveFormat::PassEntry>     passEntries_{};
    std::span<const ArchiveFormat::MaterialEntry> materialEntries_{};
    std::span<const std::byte>             blobPool_{};
    std::span<const std::byte>             reflectionPool_{};
    std::span<const char>                  stringPool_{};
    std::unordered_set<std::uint64_t>      idSet_{};
};

} // namespace Hylux::Shader
