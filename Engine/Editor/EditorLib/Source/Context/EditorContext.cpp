/// @file
/// @brief EditorContext implementation.

#include "Context/EditorContext.h"

#include "Engine/EngineLoop.h"

#include <utility>

namespace Hylux::Editor
{

EditorContext::EditorContext(EngineLoop&           loop,
                             MainThreadDispatcher& dispatcher,
                             DockRegistry&         dockRegistry,
                             DockHost&             dockHost,
                             MenuRegistry&         menuRegistry,
                             SelectionContext&     selection,
                             QObject*              parent)
    : QObject(parent),
      loop_(loop),
      dispatcher_(dispatcher),
      dockRegistry_(dockRegistry),
      dockHost_(dockHost),
      menuRegistry_(menuRegistry),
      selection_(selection)
{
}

void EditorContext::Submit(std::function<void(Engine&)> cmd)
{
    loop_.SubmitCommand(std::move(cmd));
}

} // namespace Hylux::Editor
