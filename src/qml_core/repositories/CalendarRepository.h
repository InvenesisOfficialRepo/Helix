#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QVariantMap>

#include "models/CalendarEventsModel.h"

class CalendarRepository
{
public:
    explicit CalendarRepository(QSqlDatabase db);

    // Returns map: "YYYY-MM-DD" -> count (scheduled compounds that day)
    QVariantMap loadMonthCounts(const QDate& startInclusive,
                                const QDate& endExclusive,
                                QString* errorOut);

    // Returns events grouped by batch for that day
    QVector<CalendarEventItem> loadDayEvents(const QDate& day, QString* errorOut);

private:
    QSqlDatabase db_;
};
