/// @file
/// @brief Strong typedef for a 64-bit parameter / pass name hash. Use MakeNameHash for
///        runtime hashing and StableNameHash for compile-time hashes captured at the call
///        site (matches the Shader::StablePassNameHash pattern).

#pragma once

#include "Core/Utils/Hash.h"

#include <cstdint>
#include <string_view>

namespace Hylux::Renderer
{

/// @brief 64-bit identifier derived from a UTF-8 parameter / pass name. The renderer treats
///        the bits as opaque; equality is the only operation.
enum class NameHash : std::uint64_t
{
    Invalid = 0,
};

/// @brief Runtime hash of a string view.
[[nodiscard]] inline NameHash MakeNameHash(std::string_view name) noexcept
{
    return static_cast<NameHash>(Hash::Fnv1a64(name));
}

/// @brief Compile-time hash; lets call sites encode a string constant as a NameHash literal.
[[nodiscard]] constexpr NameHash StableNameHash(std::string_view name) noexcept
{
    return static_cast<NameHash>(Hash::Fnv1a64(name));
}

[[nodiscard]] constexpr std::uint64_t ToU64(NameHash hash) noexcept
{
    return static_cast<std::uint64_t>(hash);
}

} // namespace Hylux::Renderer
