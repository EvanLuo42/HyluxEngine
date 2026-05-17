/// @file
/// @brief Bitflag operator generator for scoped enums. HYLUX_ENABLE_BITFLAGS(EnumT) emits
///        the standard set of bitwise operators plus HasAny / HasAll / IsEmpty helpers
///        at the call-site namespace, found at call sites via ADL on EnumT. Use once per
///        enum, right after its definition.

#pragma once

#include <type_traits>

#define HYLUX_ENABLE_BITFLAGS(EnumT)                                                          \
    [[nodiscard]] constexpr EnumT operator|(EnumT a, EnumT b) noexcept                        \
    {                                                                                         \
        using U = std::underlying_type_t<EnumT>;                                              \
        return static_cast<EnumT>(static_cast<U>(a) | static_cast<U>(b));                     \
    }                                                                                         \
    [[nodiscard]] constexpr EnumT operator&(EnumT a, EnumT b) noexcept                        \
    {                                                                                         \
        using U = std::underlying_type_t<EnumT>;                                              \
        return static_cast<EnumT>(static_cast<U>(a) & static_cast<U>(b));                     \
    }                                                                                         \
    [[nodiscard]] constexpr EnumT operator^(EnumT a, EnumT b) noexcept                        \
    {                                                                                         \
        using U = std::underlying_type_t<EnumT>;                                              \
        return static_cast<EnumT>(static_cast<U>(a) ^ static_cast<U>(b));                     \
    }                                                                                         \
    [[nodiscard]] constexpr EnumT operator~(EnumT a) noexcept                                 \
    {                                                                                         \
        using U = std::underlying_type_t<EnumT>;                                              \
        return static_cast<EnumT>(~static_cast<U>(a));                                        \
    }                                                                                         \
    constexpr EnumT& operator|=(EnumT& a, EnumT b) noexcept { a = a | b; return a; }          \
    constexpr EnumT& operator&=(EnumT& a, EnumT b) noexcept { a = a & b; return a; }          \
    constexpr EnumT& operator^=(EnumT& a, EnumT b) noexcept { a = a ^ b; return a; }          \
    [[nodiscard]] constexpr bool HasAny(EnumT value, EnumT mask) noexcept                     \
    {                                                                                         \
        using U = std::underlying_type_t<EnumT>;                                              \
        return (static_cast<U>(value) & static_cast<U>(mask)) != U{0};                        \
    }                                                                                         \
    [[nodiscard]] constexpr bool HasAll(EnumT value, EnumT mask) noexcept                     \
    {                                                                                         \
        using U = std::underlying_type_t<EnumT>;                                              \
        return (static_cast<U>(value) & static_cast<U>(mask)) == static_cast<U>(mask);        \
    }                                                                                         \
    [[nodiscard]] constexpr bool IsEmpty(EnumT value) noexcept                                \
    {                                                                                         \
        return static_cast<std::underlying_type_t<EnumT>>(value) == 0;                        \
    }
