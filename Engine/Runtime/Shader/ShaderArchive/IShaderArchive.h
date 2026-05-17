/// @file
/// @brief Abstract read-only view onto a shader archive. Lookup methods resolve a
///        composite key to a ShaderRecord whose bytecode and reflection pointers already
///        point into the archive's underlying storage (no copies).

#pragma once

#include "RHI/RHIEnums.h"
#include "Shader/ShaderId.h"
#include "Shader/ShaderReflection.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace Hylux::Shader
{

/// @brief Resolved archive entry returned by IShaderArchive::FindPass / FindMaterial.
///        All spans and pointers are owned by the archive and remain valid for the
///        archive's lifetime; consumers must not retain them across an archive reload.
struct ShaderRecord
{
    ShaderId                   id{};
    RHI::ShaderStage           stage{RHI::ShaderStage::None};
    RHI::ShaderBytecodeFormat  format{RHI::ShaderBytecodeFormat::Spirv};
    std::span<const std::byte> bytecode{};
    const ShaderReflection*    reflection{nullptr};
    std::string_view           entryPoint{};
    std::uint64_t              sourceHash{0};
};

/// @brief Abstract shader archive. Implementations may back themselves with a memory-
///        mapped file, a pak section, or test-fixture buffers; the contract is the same.
class IShaderArchive
{
public:
    virtual ~IShaderArchive() = default;

    IShaderArchive(const IShaderArchive&)            = delete;
    IShaderArchive& operator=(const IShaderArchive&) = delete;
    IShaderArchive(IShaderArchive&&)                 = delete;
    IShaderArchive& operator=(IShaderArchive&&)      = delete;

    /// @brief Resolves a pass shader key against the archive's pass index.
    [[nodiscard]] virtual std::optional<ShaderRecord> FindPass(const PassShaderKey& key) const = 0;

    /// @brief Resolves a material shader key against the archive's material index.
    [[nodiscard]] virtual std::optional<ShaderRecord>
        FindMaterial(const MaterialShaderKey& key) const = 0;

    /// @brief Returns the wire bytecode format every entry was compiled to (e.g. Spirv).
    [[nodiscard]] virtual RHI::ShaderBytecodeFormat BytecodeFormat() const noexcept = 0;

    /// @brief Returns the number of pass shader entries in the archive.
    [[nodiscard]] virtual std::size_t PassEntryCount() const noexcept = 0;

    /// @brief Returns the number of material shader entries in the archive.
    [[nodiscard]] virtual std::size_t MaterialEntryCount() const noexcept = 0;

    /// @brief Returns true if the archive entry identified by @p id is still present
    ///        after a reload. Used by ShaderModuleCache::Prune.
    [[nodiscard]] virtual bool ContainsId(ShaderId id) const = 0;

    /// @brief Returns a diagnostic identifier for the archive (e.g. backing blob path).
    [[nodiscard]] virtual std::string_view DebugName() const noexcept = 0;

protected:
    IShaderArchive() = default;
};

} // namespace Hylux::Shader
