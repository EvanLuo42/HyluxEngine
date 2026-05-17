/// @file
/// @brief DockRegistry implementation: vector storage with id lookup.

#include "Dock/DockRegistry.h"

#include <algorithm>
#include <utility>

namespace Hylux::Editor
{

void DockRegistry::Register(std::unique_ptr<IDockFactory> factory)
{
    if (!factory)
    {
        return;
    }
    factories_.push_back(std::move(factory));
}

IDockFactory* DockRegistry::Find(std::string_view id) const noexcept
{
    auto it = std::find_if(factories_.begin(), factories_.end(),
                           [id](const std::unique_ptr<IDockFactory>& f) { return f->Id() == id; });
    return it == factories_.end() ? nullptr : it->get();
}

} // namespace Hylux::Editor
