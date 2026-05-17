/// @file
/// @brief Thread-safe shader-binary mirror used by AftermathGpuCrashReporter to resolve
///        faulting shader hashes back to bytecode + source mapping in crash dumps.

#pragma once

#include "RHI/RHIEnums.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hylux::RHI::Vulkan
{

struct AftermathShaderEntry
{
    std::uint64_t          shaderHash{0};
    ShaderBytecodeFormat   format{ShaderBytecodeFormat::Spirv};
    ShaderStage            stage{ShaderStage::None};
    std::vector<std::byte> bytecode;
    std::vector<std::byte> sourceMapping;
    std::string            debugName;
};

/// @brief Hash-keyed registry of shader binaries. Append-only — re-registering a hash
///        replaces the prior entry.
class AftermathShaderDatabase
{
public:
    void Register(const AftermathShaderEntry& entry);

    /// @brief Returns a pointer to the stored entry, or nullptr if the hash is unknown.
    ///        The pointer remains valid until the next Register call for the same hash.
    [[nodiscard]] const AftermathShaderEntry* Find(std::uint64_t hash) const;

    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::uint64_t, AftermathShaderEntry> entries_;
};

} // namespace Hylux::RHI::Vulkan
