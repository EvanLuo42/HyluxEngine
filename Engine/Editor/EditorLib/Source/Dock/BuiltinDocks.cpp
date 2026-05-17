/// @file
/// @brief Built-in dock factories implementation.

#include "Dock/BuiltinDocks.h"

#include "Dock/DockRegistry.h"
#include "Dock/IDockFactory.h"
#include "ViewModels/ContentBrowserViewModel.h"
#include "ViewModels/DetailsViewModel.h"
#include "ViewModels/LogViewModel.h"
#include "ViewModels/OutlinerViewModel.h"

#include <memory>

namespace Hylux::Editor
{
namespace
{

class LogDockFactory final : public IDockFactory
{
public:
    explicit LogDockFactory(QmlLogSink* sink) : sink_(sink) {}

    [[nodiscard]] std::string Id() const override { return "hylux.log"; }
    [[nodiscard]] std::string Title() const override { return "Log"; }
    [[nodiscard]] std::string QmlSource() const override
    {
        return "qrc:/qt/qml/Hylux/Editor/Docks/LogDock.qml";
    }
    [[nodiscard]] DockArea DefaultArea() const override { return DockArea::Bottom; }

    [[nodiscard]] std::unique_ptr<QObject> CreateViewModel(EditorContext& context) override
    {
        return std::make_unique<LogViewModel>(context, sink_);
    }

private:
    QmlLogSink* sink_ = nullptr;
};

class OutlinerDockFactory final : public IDockFactory
{
public:
    [[nodiscard]] std::string Id() const override { return "hylux.outliner"; }
    [[nodiscard]] std::string Title() const override { return "Outliner"; }
    [[nodiscard]] std::string QmlSource() const override
    {
        return "qrc:/qt/qml/Hylux/Editor/Docks/OutlinerDock.qml";
    }
    [[nodiscard]] DockArea DefaultArea() const override { return DockArea::Left; }

    [[nodiscard]] std::unique_ptr<QObject> CreateViewModel(EditorContext& context) override
    {
        return std::make_unique<OutlinerViewModel>(context);
    }
};

class DetailsDockFactory final : public IDockFactory
{
public:
    [[nodiscard]] std::string Id() const override { return "hylux.details"; }
    [[nodiscard]] std::string Title() const override { return "Details"; }
    [[nodiscard]] std::string QmlSource() const override
    {
        return "qrc:/qt/qml/Hylux/Editor/Docks/DetailsDock.qml";
    }
    [[nodiscard]] DockArea DefaultArea() const override { return DockArea::Right; }

    [[nodiscard]] std::unique_ptr<QObject> CreateViewModel(EditorContext& context) override
    {
        return std::make_unique<DetailsViewModel>(context);
    }
};

class ContentBrowserDockFactory final : public IDockFactory
{
public:
    [[nodiscard]] std::string Id() const override { return "hylux.content"; }
    [[nodiscard]] std::string Title() const override { return "Content Browser"; }
    [[nodiscard]] std::string QmlSource() const override
    {
        return "qrc:/qt/qml/Hylux/Editor/Docks/ContentBrowserDock.qml";
    }
    [[nodiscard]] DockArea DefaultArea() const override { return DockArea::Bottom; }

    [[nodiscard]] std::unique_ptr<QObject> CreateViewModel(EditorContext& context) override
    {
        return std::make_unique<ContentBrowserViewModel>(context);
    }
};

} // namespace

void RegisterBuiltinDocks(DockRegistry& registry, QmlLogSink* logSink)
{
    registry.Register(std::make_unique<LogDockFactory>(logSink));
    registry.Register(std::make_unique<OutlinerDockFactory>());
    registry.Register(std::make_unique<DetailsDockFactory>());
    registry.Register(std::make_unique<ContentBrowserDockFactory>());
}

} // namespace Hylux::Editor
