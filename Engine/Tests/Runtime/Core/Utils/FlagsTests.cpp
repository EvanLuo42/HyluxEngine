/// @file
/// @brief Tests for HYLUX_ENABLE_BITFLAGS.

#include "Core/Utils/Flags.h"

#include <doctest/doctest.h>

#include <cstdint>

namespace HyluxFlagsTest
{

enum class Color : std::uint32_t
{
    None  = 0,
    Red   = 1u << 0,
    Green = 1u << 1,
    Blue  = 1u << 2,
    All   = Red | Green | Blue,
};

HYLUX_ENABLE_BITFLAGS(Color)

enum class Wide : std::uint64_t
{
    None = 0,
    Bit0 = 1ull << 0,
    Bit63 = 1ull << 63,
};

HYLUX_ENABLE_BITFLAGS(Wide)

} // namespace HyluxFlagsTest

TEST_SUITE_BEGIN("Core::Utils");

using namespace HyluxFlagsTest;

TEST_CASE("HYLUX_ENABLE_BITFLAGS: |, &, ^, ~ behave like the underlying type")
{
    constexpr Color rg = Color::Red | Color::Green;
    static_assert(static_cast<std::uint32_t>(rg) == 0b011u);

    constexpr Color blueOnly = Color::All & Color::Blue;
    static_assert(blueOnly == Color::Blue);

    constexpr Color xor1 = Color::Red ^ Color::Red;
    static_assert(xor1 == Color::None);

    constexpr auto inv = static_cast<std::uint32_t>(~Color::None);
    static_assert(inv == ~static_cast<std::uint32_t>(0));
}

TEST_CASE("HYLUX_ENABLE_BITFLAGS: |=, &=, ^= mutate in place")
{
    Color flags = Color::Red;
    flags |= Color::Blue;
    CHECK(flags == (Color::Red | Color::Blue));

    flags &= Color::Blue;
    CHECK(flags == Color::Blue);

    flags ^= Color::Blue;
    CHECK(flags == Color::None);
}

TEST_CASE("HYLUX_ENABLE_BITFLAGS: HasAny / HasAll / IsEmpty semantics")
{
    constexpr Color rg = Color::Red | Color::Green;
    static_assert(HasAny(rg, Color::Red));
    static_assert(HasAny(rg, Color::Blue) == false);
    static_assert(HasAll(rg, Color::Red | Color::Green));
    static_assert(HasAll(rg, Color::All) == false);
    static_assert(IsEmpty(Color::None));
    static_assert(IsEmpty(rg) == false);
}

TEST_CASE("HYLUX_ENABLE_BITFLAGS: works with 64-bit underlying type")
{
    Wide w = Wide::Bit0 | Wide::Bit63;
    CHECK(HasAny(w, Wide::Bit63));
    CHECK(HasAll(w, Wide::Bit0 | Wide::Bit63));
    w &= ~Wide::Bit0;
    CHECK_FALSE(HasAny(w, Wide::Bit0));
    CHECK(HasAny(w, Wide::Bit63));
}

TEST_SUITE_END();
