/// @file
/// @brief Tests for the constexpr FNV-1a hashing helpers in Core/Utils/Hash.

#include "Core/Utils/Hash.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <string_view>

using namespace Hylux;

TEST_CASE("Fnv1a64 of empty string returns the offset basis")
{
    CHECK(Hash::Fnv1a64(std::string_view{}) == Hash::Fnv1a64Offset);
    CHECK(Hash::Fnv1a64(nullptr, 0) == Hash::Fnv1a64Offset);
}

TEST_CASE("Fnv1a64 matches the canonical reference vector")
{
    // Reference vector from the public FNV test suite.
    CHECK(Hash::Fnv1a64(std::string_view{"foobar"}) == 0x85944171f73967e8ULL);
}

TEST_CASE("Fnv1a64 is constexpr and stable across overloads")
{
    constexpr std::string_view text{"HyluxEngine"};
    constexpr std::uint64_t h1 = Hash::Fnv1a64(text);
    constexpr std::uint64_t h2 = Hash::Fnv1a64(text.data(), text.size());
    static_assert(h1 == h2, "Fnv1a64 view and raw overloads must agree at compile time");
    CHECK(h1 == h2);
}

TEST_CASE("Fnv1a64 chains: hashing in two halves with the second seeded by the first matches the whole")
{
    constexpr std::string_view whole{"abcdef"};
    const std::uint64_t h1 = Hash::Fnv1a64(std::string_view{"abc"});
    const std::uint64_t h2 = Hash::Fnv1a64(std::string_view{"def"}, h1);
    CHECK(h2 == Hash::Fnv1a64(whole));
}

TEST_CASE("Fnv1a64 distinguishes distinct inputs and is case-sensitive")
{
    CHECK(Hash::Fnv1a64(std::string_view{"abc"}) != Hash::Fnv1a64(std::string_view{"abd"}));
    CHECK(Hash::Fnv1a64(std::string_view{"abc"}) != Hash::Fnv1a64(std::string_view{"ABC"}));
}

TEST_CASE("Fnv1a32 of empty string returns the 32-bit offset basis")
{
    CHECK(Hash::Fnv1a32(std::string_view{}) == Hash::Fnv1a32Offset);
}

TEST_CASE("Fnv1a32 matches the canonical reference vector")
{
    CHECK(Hash::Fnv1a32(std::string_view{"foobar"}) == 0xbf9cf968u);
}

TEST_CASE("Fnv1a32 distinguishes distinct inputs")
{
    CHECK(Hash::Fnv1a32(std::string_view{"abc"}) != Hash::Fnv1a32(std::string_view{"abd"}));
    CHECK(Hash::Fnv1a32(std::string_view{"abc"}) != Hash::Fnv1a32(std::string_view{"ABC"}));
}

TEST_CASE("Fnv1a hashes bytes verbatim, including embedded NUL")
{
    constexpr char data[] = {'a', '\0', 'b'};
    const std::uint64_t hWithNul = Hash::Fnv1a64(data, sizeof(data));
    const std::uint64_t hShort   = Hash::Fnv1a64(std::string_view{"a"});
    CHECK(hWithNul != hShort);
}
