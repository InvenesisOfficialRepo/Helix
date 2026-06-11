#pragma once

#include <QAbstractListModel>
#include <QDate>
#include <QDateTime>
#include <QString>
#include <QVector>

#include "domain/Status.h"

struct ProjectItem {
    QString batchId;      // uuid string
    QString projectCode;
    QString testCode;
    Status status = Status::Pending;
    QDate scheduledFor;
    QDateTime createdAt;
    int pendingCount = 0;
    int sentCount = 0;
    bool hasReport = false;
};

class ProjectModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        BatchIdRole = Qt::UserRole + 1,
        ProjectCodeRole,
        TestCodeRole,
        StatusRole,
        StatusTextRole,
        ScheduledForRole,
        ScheduledForTextRole,
        CreatedAtRole,
        CreatedAtTextRole,
        PendingCountRole,
        SentCountRole,
        SubtitleRole,
        HasReportRole
    };

    explicit ProjectModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(QVector<ProjectItem> items);

private:
    QVector<ProjectItem> items_;
};
