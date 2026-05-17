/// @file
/// @brief Bridge between the DockRegistry (C++) and the QML MainWindow that uses
///        KDDockWidgets. Owns ViewModel instances and exposes the dock list /
///        ViewModel lookup to QML; QML iterates and creates one KDDW.DockWidget per
///        entry, loading the factory's QML body and binding to viewModel(id).

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>
#include <unordered_map>

namespace Hylux::Editor
{

class DockRegistry;
class EditorContext;
class IDockFactory;

/// @brief Lives on the Qt main thread. Build() walks the registry, instantiates each
///        factory's ViewModel, and stores them under their dock id; QML then iterates
///        dockEntries() to populate the MainWindow layout.
class DockHost : public QObject
{
    Q_OBJECT
public:
    explicit DockHost(QObject* parent = nullptr);
    ~DockHost() override;

    DockHost(const DockHost&)            = delete;
    DockHost& operator=(const DockHost&) = delete;

    /// @brief Iterates the registry, creating ViewModels for each factory. Must be
    ///        called before QML reads dockEntries().
    void Build(EditorContext& context, const DockRegistry& registry);

    /// @brief Releases ViewModels in reverse construction order. Call before tearing
    ///        down EditorContext / engine loop.
    void TearDown();

    /// @brief Snapshot of registered docks for QML: each entry is
    ///        { id, title, qmlSource, defaultArea }.
    Q_INVOKABLE QVariantList dockEntries() const;

    /// @brief Returns the ViewModel registered under id, or nullptr. QML uses this in
    ///        the dock body QML to bind models / signals.
    Q_INVOKABLE QObject* viewModel(const QString& id) const;

private:
    struct Entry
    {
        std::string              id;
        std::string              title;
        std::string              qmlSource;
        int                      defaultArea = 0;
        std::unique_ptr<QObject> viewModel;
    };

    std::vector<Entry>                          entries_;
    std::unordered_map<std::string, QObject*>   viewModelById_;
};

} // namespace Hylux::Editor
