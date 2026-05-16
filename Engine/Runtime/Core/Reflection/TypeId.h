/// @file
/// @brief 64-bit stable TypeId computed from a type's canonical name via FNV1a.

#pragma once

#include "Core/Reflection/TypeName.h"
#include "Core/Utils/Hash.h"

#include <cstdint>

namespace Hylux
{

/// @brief Strongly-typed 64-bit type identifier. Stable across processes and builds for a given T.
enum class TypeId : std::uint64_t
{
    Invalid = 0
};

/// @brief Computes the TypeId of T at compile time.
template<typename T> [[nodiscard]] constexpr TypeId TypeIdOf() noexcept
{
    return static_cast<TypeId>(Hash::Fnv1a64(TypeName<T>()));
}

/// @brief Returns true when the id is not the sentinel Invalid value.
[[nodiscard]] constexpr bool IsValid(TypeId id) noexcept
{
    return id != TypeId::Invalid;
}

/// @brief Returns the underlying 64-bit representation of the id.
[[nodiscard]] constexpr std::uint64_t ToU64(TypeId id) noexcept
{
    return static_cast<std::uint64_t>(id);
}

} // namespace Hylux
