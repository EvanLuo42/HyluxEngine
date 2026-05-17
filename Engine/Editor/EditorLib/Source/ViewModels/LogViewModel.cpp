/// @file
/// @brief LogViewModel implementation: drains QmlLogSink on a Qt timer and exposes
///        entries via an inner QAbstractListModel filtered by level/category.

#include "ViewModels/LogViewModel.h"

#include "Logging/QmlLogSink.h"

#include <QTimer>

#include <algorithm>
#include <vector>

namespace Hylux::Editor
{

class LogViewModel::EntriesModel : public QAbstractListModel
{
public:
    enum Roles
    {
        SerialRole = Qt::UserRole + 1,
        LevelRole,
        CategoryRole,
        MessageRole,
        TimestampRole
    };

    explicit EntriesModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}

    [[nodiscard]] int rowCount(const QModelIndex& parent) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(entries_.size());
    }

    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(entries_.size()))
        {
            return {};
        }
        const auto& entry = entries_[static_cast<std::size_t>(index.row())];
        switch (role)
        {
        case SerialRole:    return static_cast<qulonglong>(entry.serial);
        case LevelRole:     return static_cast<int>(entry.level);
        case CategoryRole:  return QString::fromStdString(entry.category);
        case MessageRole:   return QString::fromStdString(entry.message);
        case TimestampRole: return static_cast<qlonglong>(entry.timestampNs);
        default:            return {};
        }
    }

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override
    {
        return {{SerialRole, "serial"},     {LevelRole, "level"},     {CategoryRole, "category"},
                {MessageRole, "message"},   {TimestampRole, "timestampNs"}};
    }

    void AppendBatch(const std::vector<LogEntrySnapshot>& batch)
    {
        if (batch.empty())
        {
            return;
        }
        const int firstNew = static_cast<int>(entries_.size());
        beginInsertRows(QModelIndex(), firstNew, firstNew + static_cast<int>(batch.size()) - 1);
        entries_.insert(entries_.end(), batch.begin(), batch.end());
        endInsertRows();
    }

    void Clear()
    {
        if (entries_.empty())
        {
            return;
        }
        beginResetModel();
        entries_.clear();
        endResetModel();
    }

private:
    std::vector<LogEntrySnapshot> entries_;
};

LogViewModel::LogViewModel(EditorContext& context, QmlLogSink* sink, QObject* parent)
    : EditorViewModel(context, parent), sink_(sink),
      entriesModel_(std::make_unique<EntriesModel>(this)), pollTimer_(std::make_unique<QTimer>(this))
{
    pollTimer_->setInterval(200);
    pollTimer_->setSingleShot(false);
    connect(pollTimer_.get(), &QTimer::timeout, this, &LogViewModel::PollSink);
    pollTimer_->start();
}

LogViewModel::~LogViewModel() = default;

QAbstractListModel* LogViewModel::Entries() noexcept
{
    return entriesModel_.get();
}

void LogViewModel::SetMinLevel(int level)
{
    if (minLevel_ == level)
    {
        return;
    }
    minLevel_ = level;
    Q_EMIT filtersChanged();
}

void LogViewModel::SetCategoryFilter(const QString& category)
{
    if (categoryFilter_ == category)
    {
        return;
    }
    categoryFilter_ = category;
    Q_EMIT filtersChanged();
}

void LogViewModel::Clear()
{
    entriesModel_->Clear();
}

void LogViewModel::PollSink()
{
    if (sink_ == nullptr)
    {
        return;
    }
    std::uint64_t newLast = lastSerial_;
    auto          batch   = sink_->Snapshot(lastSerial_, newLast);
    if (batch.empty())
    {
        return;
    }
    std::vector<LogEntrySnapshot> filtered;
    filtered.reserve(batch.size());
    const std::string categoryFilterUtf8 = categoryFilter_.toStdString();
    for (auto& entry : batch)
    {
        if (static_cast<int>(entry.level) < minLevel_)
        {
            continue;
        }
        if (!categoryFilterUtf8.empty() && entry.category != categoryFilterUtf8)
        {
            continue;
        }
        filtered.push_back(std::move(entry));
    }
    entriesModel_->AppendBatch(filtered);
    lastSerial_ = newLast;
}

} // namespace Hylux::Editor
