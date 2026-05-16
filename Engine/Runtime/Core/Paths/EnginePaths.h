/// @file
/// @brief Process-wide helpers for resolving the engine repo root, project content root, and
///        per-user app-data directory. Pure free functions; no subsystem, no Qt dependency, so
///        the editor, future game launcher, and tests all share the same discovery.

#pragma once

#include <filesystem>

namespace Hylux::EnginePaths
{

/// @brief Returns the absolute path to the engine repository root. Discovered by walking up from
///        the running executable's directory until a "vcpkg.json" file is found. Cached on the
///        first successful call. Returns an empty path if discovery fails (e.g. executable was
///        relocated outside any vcpkg-aware checkout).
[[nodiscard]] const std::filesystem::path& RepoRoot();

/// @brief Returns the project content root used as the "/Game/" mount in the editor. Placeholder
///        until a real project-config story lands: today this is just `RepoRoot()/Content`.
[[nodiscard]] const std::filesystem::path& ProjectContentRoot();

/// @brief Returns the per-user local app-data directory for Hylux (Windows:
///        `%LOCALAPPDATA%/HyluxEngine`). The directory is NOT created here — callers (or providers)
///        create it on first write. Returns an empty path if the platform API fails.
[[nodiscard]] const std::filesystem::path& UserAppDataRoot();

} // namespace Hylux::EnginePaths
