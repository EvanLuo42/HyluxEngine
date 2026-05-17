/// @file
/// @brief Sequential bootstrap runner. Executes each registered step in order, updating
///        BootstrapProgress between steps so the splash reflects the current activity.

#pragma once

#include "Bootstrap/IBootstrapStep.h"

#include <memory>
#include <vector>

namespace Hylux::Editor
{

class EditorBootstrap
{
public:
    EditorBootstrap()  = default;
    ~EditorBootstrap() = default;

    EditorBootstrap(const EditorBootstrap&)            = delete;
    EditorBootstrap& operator=(const EditorBootstrap&) = delete;

    void AddStep(std::unique_ptr<IBootstrapStep> step);

    /// @brief Runs every step in registration order. Returns false on the first failure.
    [[nodiscard]] bool Run(BootstrapContext& context);

private:
    std::vector<std::unique_ptr<IBootstrapStep>> steps_;
};

} // namespace Hylux::Editor
