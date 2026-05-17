/// @file
/// @brief ViewModel powering LogDock. Polls a QmlLogSink on a Qt timer and exposes the
///        entries to QML as a QAbstractListModel; supports filter by level and category.

#pragma once

#include "ViewModels/EditorViewModel.h"

#include <QAbstractListModel>
#include <QString>

#include <cstdint>
#include <memory>

class QTimer;

namespace Hylux::Editor
{

class QmlLogSink;

class LogViewModel : public EditorViewModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel* entries READ Entries CONSTANT)
    Q_PROPERTY(int minLevel READ MinLevel WRITE SetMinLevel NOTIFY filtersChanged)
    Q_PROPERTY(QString categoryFilter READ CategoryFilter WRITE SetCategoryFilter NOTIFY filtersChanged)
public:
    explicit LogViewModel(EditorContext& context, QmlLogSink* sink, QObject* parent = nullptr);
    ~LogViewModel() override;

    [[nodiscard]] QAbstractListModel* Entries() noexcept;
    [[nodiscard]] int                 MinLevel() const noexcept { return minLevel_; }
    [[nodiscard]] QString             CategoryFilter() const noexcept { return categoryFilter_; }

    void SetMinLevel(int level);
    void SetCategoryFilter(const QString& category);

    Q_INVOKABLE void Clear();

Q_SIGNALS:
    void filtersChanged();

private Q_SLOTS:
    void PollSink();

private:
    class EntriesModel;

    QmlLogSink*                   sink_ = nullptr;
    std::unique_ptr<EntriesModel> entriesModel_;
    std::unique_ptr<QTimer>       pollTimer_;
    std::uint64_t                 lastSerial_     = 0;
    int                           minLevel_       = 0;
    QString                       categoryFilter_;
};

} // namespace Hylux::Editor
