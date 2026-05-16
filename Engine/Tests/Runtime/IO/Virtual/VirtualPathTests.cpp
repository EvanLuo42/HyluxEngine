/// @file
/// @brief Unit tests for VirtualPath normalization, splitting, and lower-ASCII hashing.

#include "Core/IO/Virtual/VirtualPath.h"

#include <doctest/doctest.h>

#include <ostream>

using namespace Hylux;

TEST_CASE("VirtualPath::Normalize collapses redundant separators and resolves dots")
{
    CHECK(VirtualPath::Normalize("/Engine/Shaders/Common.glsl") == "/Engine/Shaders/Common.glsl");
    CHECK(VirtualPath::Normalize("/A//B/../C/") == "/A/C");
    CHECK(VirtualPath::Normalize("/A/./B") == "/A/B");
    CHECK(VirtualPath::Normalize("/Engine/") == "/Engine");
    CHECK(VirtualPath::Normalize("\\Engine\\Shaders\\X") == "/Engine/Shaders/X");
}

TEST_CASE("VirtualPath::Normalize rejects invalid input")
{
    CHECK(VirtualPath::Normalize("").empty());
    CHECK(VirtualPath::Normalize("Engine/Shaders/x").empty());
    CHECK(VirtualPath::Normalize("/../etc/passwd").empty());
    CHECK(VirtualPath::Normalize("/A/../..").empty());
    CHECK(VirtualPath::Normalize("/").empty());
}

TEST_CASE("VirtualPath::Split separates prefix and sub-path")
{
    std::string_view prefix;
    std::string_view sub;

    REQUIRE(VirtualPath::Split("/Engine/Shaders/Common.glsl", prefix, sub));
    CHECK(prefix == "/Engine/");
    CHECK(sub    == "Shaders/Common.glsl");

    REQUIRE(VirtualPath::Split("/Engine/Shaders/", prefix, sub));
    CHECK(prefix == "/Engine/");
    CHECK(sub    == "Shaders/");

    CHECK_FALSE(VirtualPath::Split("/Engine", prefix, sub));
    CHECK_FALSE(VirtualPath::Split("", prefix, sub));
    CHECK_FALSE(VirtualPath::Split("Engine/foo", prefix, sub));
}

TEST_CASE("VirtualPath::HashLowerAscii is case-insensitive")
{
    const auto a = VirtualPath::HashLowerAscii("/Engine/Foo.glsl");
    const auto b = VirtualPath::HashLowerAscii("/engine/FOO.glsl");
    CHECK(a == b);

    const auto c = VirtualPath::HashLowerAscii("/Engine/Bar.glsl");
    CHECK(a != c);
}

TEST_CASE("VirtualPath::IsValidMountPrefix accepts /Name/ and multi-segment /A/B/")
{
    CHECK(VirtualPath::IsValidMountPrefix("/Engine/"));
    CHECK(VirtualPath::IsValidMountPrefix("/Game/"));
    CHECK(VirtualPath::IsValidMountPrefix("/A/B/"));
    CHECK(VirtualPath::IsValidMountPrefix("/Game/Localization/"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("Engine/"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("/Engine"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("/"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("//"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("/A//B/"));
    CHECK_FALSE(VirtualPath::IsValidMountPrefix("/A\\B/"));
}
