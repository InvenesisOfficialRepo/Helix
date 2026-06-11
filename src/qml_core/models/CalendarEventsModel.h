#pragma once

#include <QAbstractListModel>
#include <QDate>
#include <QMetaType>
#include <QString>
#include <QVector>

struct CalendarEventItem {
    QString batchId;     // uuid string
    QString projectCode;
    QString testCode;
    QDate scheduledFor;
    int pendingCount = 0;
    int sentCount = 0;
    QString title;       // preformatted line used by QML
};

Q_DECLARE_METATYPE(CalendarEventItem)
Q_DECLARE_METATYPE(QVector<CalendarEventItem>)

class CalendarEventsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        BatchIdRole = Qt::UserRole + 1,
        ProjectCodeRole,
        TestCodeRole,
        ScheduledForRole,
        PendingCountRole,
        SentCountRole,
        TitleRole
    };

    explicit CalendarEventsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(QVector<CalendarEventItem> items);
    void clear();

private:
    QVector<CalendarEventItem> items_;
};
