/// @file
/// @brief Tests for Hylux::Guid (Zero/Generate/Parse/ToString/Hash/comparisons).

#include "Core/Guid.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
#include <string>

using namespace Hylux;

TEST_SUITE_BEGIN("Core");

TEST_CASE("Guid::Zero: 16 zero bytes; IsZero true; Hash==0; ToString lowercase")
{
    constexpr Guid z = Guid::Zero();
    CHECK(z.IsZero());
    CHECK(z.Hash() == 0u);
    CHECK(z.ToString() == "00000000-0000-0000-0000-000000000000");
}

TEST_CASE("Guid::Generate: produces a v4 / RFC variant GUID; multiple values are unique")
{
    const Guid a = Guid::Generate();
    const Guid b = Guid::Generate();
    CHECK_FALSE(a.IsZero());
    CHECK_FALSE(b.IsZero());
    CHECK(a != b);
    // v4 lives in the top nibble of byte 6.
    CHECK((a.bytes[6] >> 4) == 0x4);
    // RFC variant bits 10xx in the top two bits of byte 8.
    CHECK((a.bytes[8] & 0xC0) == 0x80);
}

TEST_CASE("Guid::Parse: round-trips through ToString")
{
    const Guid g = Guid::Generate();
    const auto str = g.ToString();
    const auto parsed = Guid::Parse(str);
    REQUIRE(parsed.has_value());
    CHECK(*parsed == g);
    CHECK(parsed->ToString() == str);
}

TEST_CASE("Guid::Parse: rejects wrong length / missing hyphens / non-hex / uppercase OK")
{
    CHECK_FALSE(Guid::Parse("").has_value());
    CHECK_FALSE(Guid::Parse("not-a-guid").has_value());
    CHECK_FALSE(Guid::Parse("0000000000000000-0000-0000-0000-000000000000").has_value());
    CHECK_FALSE(Guid::Parse("00000000_0000_0000_0000_000000000000").has_value());
    CHECK_FALSE(Guid::Parse("zzzz0000-0000-0000-0000-000000000000").has_value());
    CHECK(Guid::Parse("DEADBEEF-CAFE-1234-9876-FEDCBA987654").has_value());
}

TEST_CASE("Guid: ToString of specific bytes matches canonical layout")
{
    Guid g{};
    for (std::size_t i = 0; i < Guid::kSize; ++i) g.bytes[i] = static_cast<std::uint8_t>(i);
    CHECK(g.ToString() == "00010203-0405-0607-0809-0a0b0c0d0e0f");
}

TEST_CASE("Guid::Hash: same bytes -> same hash; different bytes typically differ")
{
    const auto a = Guid::Generate();
    Guid copy = a;
    CHECK(a.Hash() == copy.Hash());
    const auto b = Guid::Generate();
    CHECK(a.Hash() != b.Hash());
}

TEST_CASE("Guid: equality and lexicographic <=> ordering")
{
    Guid a{};
    Guid b{};
    a.bytes[0] = 1;
    CHECK_FALSE(a == b);
    CHECK(b < a);
    CHECK(a > b);
    CHECK(a >= b);
    CHECK(b <= a);
    CHECK(a != b);
}

TEST_CASE("std::hash<Guid> delegates to Hash()")
{
    const Guid g = Guid::Generate();
    CHECK(std::hash<Guid>{}(g) == static_cast<std::size_t>(g.Hash()));
}

TEST_SUITE_END();
