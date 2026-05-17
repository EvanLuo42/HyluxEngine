/// @file
/// @brief Bootstrap step that runs the editor shader compiler in incremental mode and
///        feeds per-file progress into BootstrapProgress so the splash visualises it.

#pragma once

#include "Bootstrap/IBootstrapStep.h"

namespace Hylux::Editor
{

class BootstrapShaderCompileStep final : public IBootstrapStep
{
public:
    [[nodiscard]] std::string Name() const override { return "Compiling shaders"; }
    bool                      Run(BootstrapContext& context) override;
};

} // namespace Hylux::Editor
