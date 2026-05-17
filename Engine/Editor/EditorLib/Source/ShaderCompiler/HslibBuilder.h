/// @file
/// @brief Editor-side writer for the .hslib shader archive format consumed by
///        Hylux::Shader::MappedShaderArchive. Accepts compiled SPIR-V bytecode plus
///        minimal reflection per (passNameHash, permutationKey, stage) entry; emits a
///        binary blob that matches Hylux::Shader::ArchiveFormat byte-for-byte.

#pragma once

#include "RHI/RHIEnums.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace Hylux::Editor
{

/// @brief One compiled pass-shader entry ready to be packed into the archive.
struct HslibPassEntryInput
{
    std::uint64_t              passNameHash{0};
    std::uint64_t              permutationKey{0};
    RHI::ShaderStage           stage{RHI::ShaderStage::None};
    std::string                entryPoint;          // e.g. "VSMain"
    std::vector<std::byte>     spirv;               // raw SPIR-V bytes

    // Reflection — kept minimal for stage 4b. Real reflection lands when Slang's
    // reflection API is wired through the compiler.
    std::uint32_t              pushConstantSize{0};
    std::uint32_t              pushConstantVisibility{0xFFFFFFFFu};   // ShaderStage::All
    std::uint64_t              pipelineLayoutHash{0};
    std::string                sourceFilePath;       // optional, stored in string pool
    std::uint64_t              sourceHash{0};
};

/// @brief Accumulator + serializer for the on-disk archive format.
class HslibBuilder
{
public:
    HslibBuilder() = default;

    HslibBuilder(const HslibBuilder&)            = delete;
    HslibBuilder& operator=(const HslibBuilder&) = delete;

    void AddPassEntry(HslibPassEntryInput entry);

    [[nodiscard]] std::size_t GetPassEntryCount() const noexcept { return passEntries_.size(); }

    /// @brief Serializes everything into the supplied destination file. Sorts the pass
    ///        index by (passNameHash, permutationKey, stage) so MappedShaderArchive's
    ///        binary search resolves correctly. Returns true on success.
    [[nodiscard]] bool WriteToFile(const std::filesystem::path& path) const;

private:
    std::vector<HslibPassEntryInput> passEntries_;
};

} // namespace Hylux::Editor
