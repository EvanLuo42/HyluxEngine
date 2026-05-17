/// @file
/// @brief DockHost implementation.

#include "Dock/DockHost.h"

#include "Dock/DockRegistry.h"
#include "Dock/IDockFactory.h"

#include <QVariantMap>

#include <utility>

namespace Hylux::Editor
{

DockHost::DockHost(QObject* parent) : QObject(parent) {}

DockHost::~DockHost()
{
    TearDown();
}

void DockHost::Build(EditorContext& context, const DockRegistry& registry)
{
    TearDown();
    for (const auto& factory : registry.Factories())
    {
        Entry entry;
        entry.id          = factory->Id();
        entry.title       = factory->Title();
        entry.qmlSource   = factory->QmlSource();
        entry.defaultArea = static_cast<int>(factory->DefaultArea());
        entry.viewModel   = factory->CreateViewModel(context);
        if (entry.viewModel)
        {
            QObject* raw = entry.viewModel.get();
            raw->setParent(this);
            viewModelById_.emplace(entry.id, raw);
        }
        entries_.push_back(std::move(entry));
    }
}

void DockHost::TearDown()
{
    viewModelById_.clear();
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
    {
        it->viewModel.reset();
    }
    entries_.clear();
}

QVariantList DockHost::dockEntries() const
{
    QVariantList list;
    list.reserve(static_cast<int>(entries_.size()));
    for (const auto& entry : entries_)
    {
        QVariantMap map;
        map.insert("id", QString::fromStdString(entry.id));
        map.insert("title", QString::fromStdString(entry.title));
        map.insert("qmlSource", QString::fromStdString(entry.qmlSource));
        map.insert("defaultArea", entry.defaultArea);
        list.append(map);
    }
    return list;
}

QObject* DockHost::viewModel(const QString& id) const
{
    auto it = viewModelById_.find(id.toStdString());
    return it == viewModelById_.end() ? nullptr : it->second;
}

} // namespace Hylux::Editor
