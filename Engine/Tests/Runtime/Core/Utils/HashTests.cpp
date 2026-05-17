/// @file
/// @brief Tests for Core/Utils/Hash.h FNV-1a helpers.

#include "Core/Utils/Hash.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Core::Utils");

#include <string_view>

using namespace Hylux;

TEST_CASE("Hash::Fnv1a64: empty input returns the seed unchanged")
{
    static_assert(Hash::Fnv1a64("", 0) == Hash::Fnv1a64Offset);
    static_assert(Hash::Fnv1a64(std::string_view{""}) == Hash::Fnv1a64Offset);
    constexpr auto seeded = Hash::Fnv1a64(std::string_view{""}, 0xDEADBEEFull);
    static_assert(seeded == 0xDEADBEEFull);
}

TEST_CASE("Hash::Fnv1a64: known vectors")
{
    static_assert(Hash::Fnv1a64(std::string_view{"a"}) == 0xaf63dc4c8601ec8cull);
    static_assert(Hash::Fnv1a64(std::string_view{"foobar"}) == 0x85944171f73967e8ull);
}

TEST_CASE("Hash::Fnv1a64: non-ASCII bytes use unsigned interpretation")
{
    const char bytes[] = {static_cast<char>(0xFF), static_cast<char>(0x80)};
    const auto a = Hash::Fnv1a64(bytes, 2);
    const auto b = Hash::Fnv1a64(bytes, 2, Hash::Fnv1a64Offset);
    CHECK(a == b);
    // Manually compute: ((offset ^ 0xFF) * prime) ^ 0x80) * prime
    constexpr std::uint64_t expected =
        ((Hash::Fnv1a64Offset ^ 0xFFull) * Hash::Fnv1a64Prime ^ 0x80ull) * Hash::Fnv1a64Prime;
    CHECK(a == expected);
}

TEST_CASE("Hash::Fnv1a64: custom seed produces a different result than default")
{
    const auto seeded = Hash::Fnv1a64(std::string_view{"abc"}, 1ull);
    const auto def    = Hash::Fnv1a64(std::string_view{"abc"});
    CHECK(seeded != def);
}

TEST_CASE("Hash::Fnv1a32: empty returns offset; known vector for \"a\"")
{
    static_assert(Hash::Fnv1a32(std::string_view{""}) == Hash::Fnv1a32Offset);
    static_assert(Hash::Fnv1a32(std::string_view{"a"}) == 0xe40c292cu);
}

TEST_CASE("Hash::Fnv1a64(const char*, length): length covering embedded NUL")
{
    const char raw[] = {'a', '\0', 'b'};
    const auto withNul = Hash::Fnv1a64(raw, 3);
    const auto noNul   = Hash::Fnv1a64(std::string_view{"a"});
    CHECK(withNul != noNul);
}

TEST_SUITE_END();
