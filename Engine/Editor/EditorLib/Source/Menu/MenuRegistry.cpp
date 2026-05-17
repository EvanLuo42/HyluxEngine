/// @file
/// @brief MenuRegistry implementation: builds a tree of slash-separated paths and
///        renders snapshots into QVariantList for QML consumption.

#include "Menu/MenuRegistry.h"

#include <QVariantMap>

#include <algorithm>
#include <limits>
#include <sstream>
#include <utility>

namespace Hylux::Editor
{

struct MenuRegistry::Node
{
    std::string                        title;
    std::string                        fullPath;
    std::vector<std::unique_ptr<Node>> children;
    MenuActionDesc                     leaf;
    bool                               isLeaf = false;
    int                                order  = std::numeric_limits<int>::max();
};

MenuRegistry::MenuRegistry(QObject* parent) : QObject(parent), root_(std::make_unique<Node>()) {}

MenuRegistry::~MenuRegistry() = default;

std::vector<std::string> MenuRegistry::SplitPath(const std::string& path)
{
    std::vector<std::string> parts;
    std::string              current;
    for (const char c : path)
    {
        if (c == '/')
        {
            if (!current.empty())
            {
                parts.push_back(std::move(current));
                current.clear();
            }
        }
        else
        {
            current.push_back(c);
        }
    }
    if (!current.empty())
    {
        parts.push_back(std::move(current));
    }
    return parts;
}

MenuRegistry::Node* MenuRegistry::FindNode(const std::vector<std::string>& parts) noexcept
{
    Node* cursor = root_.get();
    for (const auto& part : parts)
    {
        auto it = std::find_if(cursor->children.begin(), cursor->children.end(),
                               [&part](const std::unique_ptr<Node>& n) { return n->title == part; });
        if (it == cursor->children.end())
        {
            return nullptr;
        }
        cursor = it->get();
    }
    return cursor;
}

const MenuRegistry::Node* MenuRegistry::FindNode(const std::vector<std::string>& parts) const noexcept
{
    const Node* cursor = root_.get();
    for (const auto& part : parts)
    {
        auto it = std::find_if(cursor->children.begin(), cursor->children.end(),
                               [&part](const std::unique_ptr<Node>& n) { return n->title == part; });
        if (it == cursor->children.end())
        {
            return nullptr;
        }
        cursor = it->get();
    }
    return cursor;
}

void MenuRegistry::Register(MenuActionDesc desc)
{
    const std::vector<std::string> parts = SplitPath(desc.path);
    if (parts.empty())
    {
        return;
    }
    Node*       cursor = root_.get();
    std::string accum;
    for (std::size_t i = 0; i < parts.size(); ++i)
    {
        if (!accum.empty())
        {
            accum.push_back('/');
        }
        accum += parts[i];

        auto it = std::find_if(cursor->children.begin(), cursor->children.end(),
                               [&parts, i](const std::unique_ptr<Node>& n) { return n->title == parts[i]; });
        if (it == cursor->children.end())
        {
            auto child       = std::make_unique<Node>();
            child->title     = parts[i];
            child->fullPath  = accum;
            cursor->children.push_back(std::move(child));
            it = std::prev(cursor->children.end());
        }
        cursor = it->get();
        cursor->order = std::min(cursor->order, desc.order);
    }
    cursor->isLeaf = true;
    cursor->leaf   = std::move(desc);
    Q_EMIT structureChanged();
}

void MenuRegistry::Clear()
{
    root_ = std::make_unique<Node>();
    Q_EMIT structureChanged();
}

QVariantMap MenuRegistry::NodeToVariant(const Node& node)
{
    QVariantMap map;
    map.insert("title", QString::fromStdString(node.title));
    map.insert("fullPath", QString::fromStdString(node.fullPath));
    map.insert("hasChildren", !node.children.empty());
    map.insert("isLeaf", node.isLeaf);
    if (node.isLeaf)
    {
        map.insert("shortcut", QString::fromStdString(node.leaf.shortcut));
        map.insert("isEnabled", node.leaf.isEnabled ? node.leaf.isEnabled() : true);
    }
    else
    {
        map.insert("shortcut", QString{});
        map.insert("isEnabled", true);
    }
    return map;
}

void MenuRegistry::SortChildren(std::vector<const Node*>& nodes)
{
    std::sort(nodes.begin(), nodes.end(), [](const Node* a, const Node* b) {
        if (a->order != b->order)
        {
            return a->order < b->order;
        }
        return a->title < b->title;
    });
}

QVariantList MenuRegistry::topLevelMenus() const
{
    std::vector<const Node*> nodes;
    nodes.reserve(root_->children.size());
    for (const auto& c : root_->children)
    {
        nodes.push_back(c.get());
    }
    SortChildren(nodes);

    QVariantList out;
    out.reserve(static_cast<int>(nodes.size()));
    for (const Node* n : nodes)
    {
        out.append(NodeToVariant(*n));
    }
    return out;
}

QVariantList MenuRegistry::itemsAt(const QString& path) const
{
    const Node* parent = FindNode(SplitPath(path.toStdString()));
    if (parent == nullptr)
    {
        return {};
    }
    std::vector<const Node*> nodes;
    nodes.reserve(parent->children.size());
    for (const auto& c : parent->children)
    {
        nodes.push_back(c.get());
    }
    SortChildren(nodes);

    QVariantList out;
    out.reserve(static_cast<int>(nodes.size()));
    for (const Node* n : nodes)
    {
        out.append(NodeToVariant(*n));
    }
    return out;
}

void MenuRegistry::trigger(const QString& fullPath)
{
    Node* node = FindNode(SplitPath(fullPath.toStdString()));
    if (node == nullptr || !node->isLeaf || !node->leaf.run)
    {
        return;
    }
    if (node->leaf.isEnabled && !node->leaf.isEnabled())
    {
        return;
    }
    node->leaf.run();
}

} // namespace Hylux::Editor
