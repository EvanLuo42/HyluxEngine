/// @file
/// @brief EditorBootstrap implementation.

#include "Bootstrap/EditorBootstrap.h"

#include "Bootstrap/BootstrapProgress.h"

#include <utility>

namespace Hylux::Editor
{

void EditorBootstrap::AddStep(std::unique_ptr<IBootstrapStep> step)
{
    if (step)
    {
        steps_.push_back(std::move(step));
    }
}

bool EditorBootstrap::Run(BootstrapContext& context)
{
    const std::size_t total = steps_.size();
    for (std::size_t i = 0; i < total; ++i)
    {
        const auto& step = steps_[i];
        if (context.progress != nullptr)
        {
            const int percent = total > 0 ? static_cast<int>((i * 100) / total) : 0;
            context.progress->SetStep(QString::fromStdString(step->Name()), percent);
        }
        if (!step->Run(context))
        {
            return false;
        }
    }
    if (context.progress != nullptr)
    {
        context.progress->SetPercent(100);
    }
    return true;
}

} // namespace Hylux::Editor
