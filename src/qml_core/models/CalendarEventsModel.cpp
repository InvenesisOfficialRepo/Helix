#include "CalendarEventsModel.h"

CalendarEventsModel::CalendarEventsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CalendarEventsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return items_.size();
}

QVariant CalendarEventsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size())
        return {};

    const auto &it = items_.at(index.row());
    switch (role) {
    case BatchIdRole:       return it.batchId;
    case ProjectCodeRole:   return it.projectCode;
    case TestCodeRole:      return it.testCode;
    case ScheduledForRole:  return it.scheduledFor;
    case PendingCountRole:  return it.pendingCount;
    case SentCountRole:     return it.sentCount;
    case TitleRole:         return it.title;
    default:                return {};
    }
}

QHash<int, QByteArray> CalendarEventsModel::roleNames() const
{
    return {
        {BatchIdRole, "batchId"},
        {ProjectCodeRole, "projectCode"},
        {TestCodeRole, "testCode"},
        {ScheduledForRole, "scheduledFor"},
        {PendingCountRole, "pendingCount"},
        {SentCountRole, "sentCount"},
        {TitleRole, "title"}
    };
}

void CalendarEventsModel::setItems(QVector<CalendarEventItem> items)
{
    beginResetModel();
    items_ = std::move(items);
    endResetModel();
}

void CalendarEventsModel::clear()
{
    beginResetModel();
    items_.clear();
    endResetModel();
}
