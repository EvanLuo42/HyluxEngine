/// @file
/// @brief Tests for Hylux::VirtualPath: Normalize, Split, ToLowerAscii, HashLowerAscii,
///        IsValidMountPrefix.

#include "Core/IO/Virtual/VirtualPath.h"
#include "Core/Utils/Hash.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("IO::Virtual");

using namespace Hylux;

TEST_CASE("VirtualPath::Normalize: empty / missing-slash / leading-slash variants")
{
    CHECK(VirtualPath::Normalize("") == "");
    CHECK(VirtualPath::Normalize("Engine/Foo") == "");
    CHECK(VirtualPath::Normalize("/") == "");
    CHECK(VirtualPath::Normalize("/Engine") == "/Engine");
    CHECK(VirtualPath::Normalize("/Engine/Foo") == "/Engine/Foo");
}

TEST_CASE("VirtualPath::Normalize: backslashes fold to '/'")
{
    CHECK(VirtualPath::Normalize("\\Engine\\Foo") == "/Engine/Foo");
    CHECK(VirtualPath::Normalize("/Engine\\Sub/Foo") == "/Engine/Sub/Foo");
}

TEST_CASE("VirtualPath::Normalize: collapses runs of '/' and trailing slash")
{
    CHECK(VirtualPath::Normalize("/Engine//Foo") == "/Engine/Foo");
    CHECK(VirtualPath::Normalize("/Engine/Foo/") == "/Engine/Foo");
    CHECK(VirtualPath::Normalize("///") == "");
}

TEST_CASE("VirtualPath::Normalize: '.' segments skipped; '..' pops; escape rejected")
{
    CHECK(VirtualPath::Normalize("/Engine/./Foo") == "/Engine/Foo");
    CHECK(VirtualPath::Normalize("/Engine/Foo/..") == "/Engine");
    CHECK(VirtualPath::Normalize("/Engine/Foo/../Bar") == "/Engine/Bar");
    CHECK(VirtualPath::Normalize("/Engine/..") == "");
    CHECK(VirtualPath::Normalize("/..") == "");
}

TEST_CASE("VirtualPath::Normalize: preserves case")
{
    CHECK(VirtualPath::Normalize("/Engine/Foo") == "/Engine/Foo");
    CHECK(VirtualPath::Normalize("/engine/foo") == "/engine/foo");
}

TEST_CASE("VirtualPath::Split: returns false for short or malformed paths")
{
    std::string_view prefix, sub;
    CHECK_FALSE(VirtualPath::Split("", prefix, sub));
    CHECK_FALSE(VirtualPath::Split("/", prefix, sub));
    CHECK_FALSE(VirtualPath::Split("Engine/foo", prefix, sub));
    CHECK_FALSE(VirtualPath::Split("//Foo", prefix, sub));
    CHECK_FALSE(VirtualPath::Split("/Engine", prefix, sub));
}

TEST_CASE("VirtualPath::Split: extracts the leading '/Prefix/' and the remainder")
{
    std::string_view prefix, sub;
    REQUIRE(VirtualPath::Split("/Engine/Foo/Bar", prefix, sub));
    CHECK(prefix == "/Engine/");
    CHECK(sub == "Foo/Bar");

    REQUIRE(VirtualPath::Split("/Game/Maps/Level.umap", prefix, sub));
    CHECK(prefix == "/Game/");
    CHECK(sub == "Maps/Level.umap");

    // Bare prefix.
    REQUIRE(VirtualPath::Split("/Engine/", prefix, sub));
    CHECK(prefix == "/Engine/");
    CHECK(sub.empty());
}

TEST_CASE("VirtualPath::ToLowerAscii: folds A-Z only; preserves length; non-ASCII unchanged")
{
    CHECK(VirtualPath::ToLowerAscii("HELLO") == "hello");
    CHECK(VirtualPath::ToLowerAscii("Hello-World!") == "hello-world!");
    const std::string in{static_cast<char>(0xC3), static_cast<char>(0xA9)};  // é UTF-8
    const std::string out = VirtualPath::ToLowerAscii(in);
    CHECK(out.size() == 2);
    CHECK(static_cast<unsigned char>(out[0]) == 0xC3);
}

TEST_CASE("VirtualPath::HashLowerAscii: equivalent to FNV1a64 over folded bytes; case-insensitive")
{
    const auto a = VirtualPath::HashLowerAscii("/Engine/Foo");
    const auto b = VirtualPath::HashLowerAscii("/engine/FOO");
    CHECK(a == b);

    const auto expected = Hash::Fnv1a64(VirtualPath::ToLowerAscii("/Engine/Foo"));
    CHECK(a == expected);
}

TEST_CASE("VirtualPath::IsValidMountPrefix: accepts /Name/; rejects empty / single-slash / backslash")
{
    CHECK(VirtualPath::IsValidMountPrefix("/Engine/"));
    CHECK(VirtualPath::IsValidMountPrefix("/Game/"));
    CHECK(VirtualPath::IsValidMountPrefix("/A/"));
    CHECK(VirtualPath::IsValidMountPrefix("/A/B/"));

    CHECK_FALSE(VirtualPath::IsValidMountPrefix(""));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("/"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("//"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("Engine/"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("/Engine"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("/Engine\\"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("/A//"));
}

TEST_SUITE_END();
