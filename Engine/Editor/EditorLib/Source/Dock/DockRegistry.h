/// @file
/// @brief Pre-build registry of dock factories. Bootstrap populates this; DockHost reads
///        from it once to instantiate all ViewModels and wire up the QML MainWindow.

#pragma once

#include "Dock/IDockFactory.h"

#include <memory>
#include <string_view>
#include <vector>

namespace Hylux::Editor
{

class DockRegistry
{
public:
    DockRegistry()  = default;
    ~DockRegistry() = default;

    DockRegistry(const DockRegistry&)            = delete;
    DockRegistry& operator=(const DockRegistry&) = delete;

    void Register(std::unique_ptr<IDockFactory> factory);

    [[nodiscard]] const std::vector<std::unique_ptr<IDockFactory>>& Factories() const noexcept { return factories_; }

    [[nodiscard]] IDockFactory* Find(std::string_view id) const noexcept;

private:
    std::vector<std::unique_ptr<IDockFactory>> factories_;
};

} // namespace Hylux::Editor
