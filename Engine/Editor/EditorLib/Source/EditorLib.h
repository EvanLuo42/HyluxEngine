/// @file
/// @brief Public umbrella header for HyluxEditorLib — non-UI editor logic.

#pragma once

#include <string_view>

namespace Hylux
{

/// @brief Returns a build-time identifier string for the editor logic library.
[[nodiscard]] std::string_view GetEditorLibVersion() noexcept;

} // namespace Hylux
