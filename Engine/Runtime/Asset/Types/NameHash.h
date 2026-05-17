/// @file
/// @brief Strong typedef for a 64-bit parameter / pass name hash. Mirrors the FNV-1a
///        algorithm used elsewhere in the engine (Shader::StablePassNameHash, etc.) so
///        compile-time and runtime hashes of the same string compare equal.

#pragma once

#include "Core/Utils/Hash.h"

#include <cstdint>
#include <string_view>

namespace Hylux::Asset
{

/// @brief 64-bit identifier derived from a UTF-8 parameter / pass name. The renderer
///        treats the bits as opaque; equality is the only operation.
enum class NameHash : std::uint64_t
{
    Invalid = 0,
};

[[nodiscard]] inline NameHash MakeNameHash(std::string_view name) noexcept
{
    return static_cast<NameHash>(Hash::Fnv1a64(name));
}

[[nodiscard]] constexpr NameHash StableNameHash(std::string_view name) noexcept
{
    return static_cast<NameHash>(Hash::Fnv1a64(name));
}

[[nodiscard]] constexpr std::uint64_t ToU64(NameHash hash) noexcept
{
    return static_cast<std::uint64_t>(hash);
}

} // namespace Hylux::Asset
