/// @file
/// @brief Central registry of editor menu actions. The QML MenuBar is data-driven off
///        this registry — docks, bootstrap, asset editors, etc. each call Register to
///        contribute entries. Q_INVOKABLE accessors expose the tree to QML.

#pragma once

#include "Menu/IMenuAction.h"

#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>
#include <string>
#include <vector>

namespace Hylux::Editor
{

/// @brief Lives on the Qt main thread. Tree is rebuilt lazily on the next QML query
///        after Register; consumers call topLevelMenus / itemsAt to enumerate and
///        trigger(path) to activate a leaf.
class MenuRegistry : public QObject
{
    Q_OBJECT
public:
    explicit MenuRegistry(QObject* parent = nullptr);
    ~MenuRegistry() override;

    MenuRegistry(const MenuRegistry&)            = delete;
    MenuRegistry& operator=(const MenuRegistry&) = delete;

    /// @brief Adds an entry. Paths are split on '/' to derive submenu nesting.
    void Register(MenuActionDesc desc);

    /// @brief Removes all entries. Used by tests and editor teardown.
    void Clear();

    /// @brief Top-level menu objects: [{ title, path, hasChildren }]. Ordered by the
    ///        minimum `order` of their contained leaves, ties broken alphabetically.
    Q_INVOKABLE QVariantList topLevelMenus() const;

    /// @brief Child entries at the given absolute path. Each entry contains:
    ///        { title, fullPath, shortcut, hasChildren, isEnabled }.
    Q_INVOKABLE QVariantList itemsAt(const QString& path) const;

    /// @brief Invokes the run callback registered for fullPath, on the calling thread.
    ///        No-op when the path is not a leaf or has no run callback.
    Q_INVOKABLE void trigger(const QString& fullPath);

Q_SIGNALS:
    /// @brief Emitted whenever Register / Clear mutates the tree. QML rebuilds menus.
    void structureChanged();

private:
    struct Node;
    [[nodiscard]] Node*       FindNode(const std::vector<std::string>& parts) noexcept;
    [[nodiscard]] const Node* FindNode(const std::vector<std::string>& parts) const noexcept;
    static std::vector<std::string> SplitPath(const std::string& path);
    static QVariantMap              NodeToVariant(const Node& node);
    static void                     SortChildren(std::vector<const Node*>& nodes);

    std::unique_ptr<Node> root_;
};

} // namespace Hylux::Editor
