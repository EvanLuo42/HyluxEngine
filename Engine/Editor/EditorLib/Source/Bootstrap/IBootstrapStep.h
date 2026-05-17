/// @file
/// @brief One step in the editor's pre-launch sequence. Steps report status through
///        BootstrapProgress; returning false aborts the launch.

#pragma once

#include <filesystem>
#include <string>

namespace Hylux
{
class Engine;
} // namespace Hylux

namespace Hylux::Editor
{

class BootstrapProgress;

/// @brief Mutable shared state handed to every step in order.
struct BootstrapContext
{
    Engine*               engine{nullptr};
    BootstrapProgress*    progress{nullptr};
    std::filesystem::path repoRoot;
    std::filesystem::path executableDir;
};

class IBootstrapStep
{
public:
    virtual ~IBootstrapStep() = default;

    [[nodiscard]] virtual std::string Name() const = 0;

    /// @brief Returns true to continue, false to abort the launch.
    virtual bool Run(BootstrapContext& context) = 0;
};

} // namespace Hylux::Editor
