#include "ProjectModel.h"

#include <QVariant>

ProjectModel::ProjectModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ProjectModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return items_.size();
}

QVariant ProjectModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size())
        return {};

    const auto& it = items_.at(index.row());

    const QString schedText = it.scheduledFor.isValid()
                                  ? it.scheduledFor.toString(Qt::ISODate)
                                  : QString();

    const QString createdText = it.createdAt.isValid()
                                    ? it.createdAt.toString(Qt::ISODate)
                                    : QString();

    const QString subtitle = QString("%1 pending / %2 sent")
                                 .arg(it.pendingCount)
                                 .arg(it.sentCount);

    switch (role) {
    case BatchIdRole:          return it.batchId;
    case ProjectCodeRole:      return it.projectCode;
    case TestCodeRole:         return it.testCode;
    case StatusRole:           return static_cast<int>(it.status);
    case StatusTextRole:
        if (it.status == Status::Done && it.hasReport) {
            return QString("Published");
        }
        return statusToText(it.status);
    case ScheduledForRole:     return it.scheduledFor;
    case ScheduledForTextRole: return schedText;
    case CreatedAtRole:        return it.createdAt;
    case CreatedAtTextRole:    return createdText;
    case PendingCountRole:     return it.pendingCount;
    case SentCountRole:        return it.sentCount;
    case SubtitleRole:         return subtitle;
    case HasReportRole:        return it.hasReport;
    default:                   return {};
    }
}

QHash<int, QByteArray> ProjectModel::roleNames() const
{
    return {
        {BatchIdRole, "batchId"},
        {ProjectCodeRole, "projectCode"},
        {TestCodeRole, "testCode"},
        {StatusRole, "status"},
        {StatusTextRole, "statusText"},
        {ScheduledForRole, "scheduledFor"},
        {ScheduledForTextRole, "scheduledForText"},
        {CreatedAtRole, "createdAt"},
        {CreatedAtTextRole, "createdAtText"},
        {PendingCountRole, "pendingCount"},
        {SentCountRole, "sentCount"},
        {SubtitleRole, "subtitle"},
        {HasReportRole, "hasReport"}
    };
}

void ProjectModel::setItems(QVector<ProjectItem> items)
{
    beginResetModel();
    items_ = std::move(items);
    endResetModel();
}
