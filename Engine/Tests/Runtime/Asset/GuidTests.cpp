/// @file
/// @brief Tests for Hylux::Asset::Guid.

#include "Asset/Guid.h"

#include <doctest/doctest.h>

#include <string>
#include <unordered_set>

using namespace Hylux::Asset;

TEST_CASE("Generate produces RFC-4122 v4 random GUIDs")
{
    Guid g = Guid::Generate();
    CHECK_FALSE(g.IsZero());

    const std::uint8_t versionNibble = static_cast<std::uint8_t>(g.bytes[6] >> 4);
    CHECK(versionNibble == 4);

    const std::uint8_t variantBits = static_cast<std::uint8_t>(g.bytes[8] & 0xC0u);
    CHECK(variantBits == 0x80u);
}

TEST_CASE("Generate yields distinct values across many calls")
{
    std::unordered_set<std::string> seen;
    for (int i = 0; i < 64; ++i)
    {
        seen.insert(Guid::Generate().ToString());
    }
    CHECK(seen.size() == 64);
}

TEST_CASE("Parse + ToString round-trip canonical form")
{
    const std::string kCanonical = "550e8400-e29b-41d4-a716-446655440000";
    auto              parsed     = Guid::Parse(kCanonical);
    REQUIRE(parsed.has_value());
    CHECK(parsed->ToString() == kCanonical);
}

TEST_CASE("Parse accepts uppercase hex")
{
    auto parsed = Guid::Parse("AABBCCDD-EEFF-1122-3344-556677889900");
    REQUIRE(parsed.has_value());
    CHECK(parsed->ToString() == std::string{"aabbccdd-eeff-1122-3344-556677889900"});
}

TEST_CASE("Parse rejects malformed input")
{
    CHECK_FALSE(Guid::Parse("").has_value());
    CHECK_FALSE(Guid::Parse("not-a-guid").has_value());
    CHECK_FALSE(Guid::Parse("550e8400e29b41d4a716446655440000").has_value());
    CHECK_FALSE(Guid::Parse("550e8400-e29b-41d4-a716-44665544000").has_value());
    CHECK_FALSE(Guid::Parse("zzzzzzzz-e29b-41d4-a716-446655440000").has_value());
}

TEST_CASE("IsZero detects the default-constructed value")
{
    Guid z;
    CHECK(z.IsZero());
    CHECK(Guid::Zero().IsZero());
    CHECK_FALSE(Guid::Generate().IsZero());
}

TEST_CASE("Equality compares bytes")
{
    auto a = Guid::Parse("00000000-0000-0000-0000-000000000001");
    auto b = Guid::Parse("00000000-0000-0000-0000-000000000001");
    auto c = Guid::Parse("00000000-0000-0000-0000-000000000002");
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(c.has_value());
    CHECK(*a == *b);
    CHECK(*a != *c);
}

TEST_CASE("std::hash specialization works in unordered_set")
{
    std::unordered_set<Guid> set;
    set.insert(Guid::Generate());
    set.insert(Guid::Generate());
    CHECK(set.size() == 2u);
}
