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

TEST_CASE("Hash::MixU8: matches one-byte Fnv1a64 step")
{
    const char one = static_cast<char>(0x42);
    CHECK(Hash::MixU8(Hash::Fnv1a64Offset, 0x42u) == Hash::Fnv1a64(&one, 1));
}

TEST_CASE("Hash::MixU32: equivalent to four little-endian bytes through Fnv1a64")
{
    constexpr std::uint32_t value = 0xDEADBEEFu;
    const char bytes[4] = {
        static_cast<char>(value & 0xFFu),
        static_cast<char>((value >>  8) & 0xFFu),
        static_cast<char>((value >> 16) & 0xFFu),
        static_cast<char>((value >> 24) & 0xFFu),
    };
    CHECK(Hash::MixU32(Hash::Fnv1a64Offset, value) == Hash::Fnv1a64(bytes, 4));
}

TEST_CASE("Hash::MixU64: equivalent to eight little-endian bytes through Fnv1a64")
{
    constexpr std::uint64_t value = 0x0123456789ABCDEFull;
    char bytes[8];
    std::uint64_t v = value;
    for (std::size_t i = 0; i < 8; ++i) { bytes[i] = static_cast<char>(v & 0xFFu); v >>= 8; }
    CHECK(Hash::MixU64(Hash::Fnv1a64Offset, value) == Hash::Fnv1a64(bytes, 8));
}

TEST_CASE("Hash::Combine: order-sensitive xor-and-prime over already-hashed inputs")
{
    constexpr std::uint64_t a = 0xAAAAAAAAAAAAAAAAull;
    constexpr std::uint64_t b = 0xBBBBBBBBBBBBBBBBull;
    const auto ab = Hash::Combine(Hash::Combine(Hash::Fnv1a64Offset, a), b);
    const auto ba = Hash::Combine(Hash::Combine(Hash::Fnv1a64Offset, b), a);
    CHECK(ab != ba);

    constexpr std::uint64_t expected =
        ((Hash::Fnv1a64Offset ^ a) * Hash::Fnv1a64Prime ^ b) * Hash::Fnv1a64Prime;
    CHECK(ab == expected);
}

TEST_CASE("Hash::MixTrivial: matches MixBytes over the value's bytes")
{
    struct Pod { std::uint32_t a; std::uint8_t b; std::uint64_t c; };
    Pod p{0x11223344u, 0x55, 0x6677889900AABBCCull};
    CHECK(Hash::MixTrivial(Hash::Fnv1a64Offset, p)
          == Hash::MixBytes(Hash::Fnv1a64Offset, &p, sizeof(p)));
}

TEST_SUITE_END();
