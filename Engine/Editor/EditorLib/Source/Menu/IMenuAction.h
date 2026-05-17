/// @file
/// @brief Descriptor for a single menu action registered with MenuRegistry. Actions are
///        addressed by slash-separated paths ("File/Open Project", "Window/Outliner");
///        the registry materialises the tree implied by all registered paths.

#pragma once

#include <functional>
#include <string>

namespace Hylux::Editor
{

/// @brief Plain-old-data describing one menu leaf. `run` is invoked on the Qt main
///        thread when the user activates the entry; it may submit closures to
///        EngineLoop::SubmitCommand for engine-side work.
struct MenuActionDesc
{
    std::string           path;
    std::string           shortcut;
    std::function<void()> run;
    std::function<bool()> isEnabled;
    int                   order = 0;
};

} // namespace Hylux::Editor
