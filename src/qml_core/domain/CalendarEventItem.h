#pragma once

#include <QString>
#include <QDate>
#include <QMetaType>

struct CalendarEventItem
{
    QString batchId;
    QString projectCode;
    QString testCode;
    QDate   scheduledFor;
    int     pendingCount = 0;
    int     sentCount = 0;
    QString title;
};

Q_DECLARE_METATYPE(CalendarEventItem)
Q_DECLARE_METATYPE(QVector<CalendarEventItem>)