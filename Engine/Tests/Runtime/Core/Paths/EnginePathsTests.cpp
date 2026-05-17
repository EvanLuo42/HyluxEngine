/// @file
/// @brief Sanity tests for Core/Paths/EnginePaths.h. The values are environment-dependent so
///        these tests only assert structural invariants (idempotency, sub-path relationship).

#include "Core/Paths/EnginePaths.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Core::Paths");

using namespace Hylux;

TEST_CASE("EnginePaths::ExecutableDir: stable across calls")
{
    const auto& a = EnginePaths::ExecutableDir();
    const auto& b = EnginePaths::ExecutableDir();
    CHECK(&a == &b);
}

TEST_CASE("EnginePaths::RepoRoot: idempotent; either empty or containing vcpkg.json")
{
    const auto& a = EnginePaths::RepoRoot();
    const auto& b = EnginePaths::RepoRoot();
    CHECK(&a == &b);
    if (!a.empty())
    {
        CHECK(std::filesystem::exists(a / "vcpkg.json"));
    }
}

TEST_CASE("EnginePaths::ProjectContentRoot: derived from RepoRoot")
{
    const auto& root = EnginePaths::RepoRoot();
    const auto& content = EnginePaths::ProjectContentRoot();
    if (root.empty())
    {
        CHECK(content.empty());
    }
    else
    {
        CHECK(content == root / "Content");
    }
    CHECK(&content == &EnginePaths::ProjectContentRoot());
}

TEST_CASE("EnginePaths::UserAppDataRoot: idempotent (value depends on platform)")
{
    const auto& a = EnginePaths::UserAppDataRoot();
    const auto& b = EnginePaths::UserAppDataRoot();
    CHECK(&a == &b);
}

TEST_SUITE_END();
