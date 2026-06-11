#include "CalendarRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

CalendarRepository::CalendarRepository(QSqlDatabase db)
    : db_(std::move(db))
{
}

QVariantMap CalendarRepository::loadMonthCounts(const QDate& startInclusive,
                                                const QDate& endExclusive,
                                                QString* errorOut)
{
    if (errorOut) errorOut->clear();
    QVariantMap out;

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return {};
    }

    QSqlQuery q(db_);
    q.prepare(R"SQL(
        SELECT
          c.scheduled_for::date AS day,
          COUNT(*) AS total_count
        FROM public.tracked_test_compounds c
        JOIN public.tracked_test_batches b ON b.batch_id = c.batch_id
        WHERE c.scheduled_for >= :start
          AND c.scheduled_for <  :end
        GROUP BY c.scheduled_for::date
        ORDER BY day
    )SQL");
    q.bindValue(":start", startInclusive);
    q.bindValue(":end", endExclusive);

    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }

    while (q.next()) {
        const QDate day = q.value(0).toDate();
        const int count = q.value(1).toInt();
        out.insert(day.toString(Qt::ISODate), count);
    }

    return out;
}

QVector<CalendarEventItem> CalendarRepository::loadDayEvents(const QDate& day, QString* errorOut)
{
    if (errorOut) errorOut->clear();
    QVector<CalendarEventItem> out;

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return {};
    }

    QSqlQuery q(db_);
    q.prepare(R"SQL(
        SELECT
          b.batch_id::text,
          b.project_code,
          b.test_code,
          MIN(c.scheduled_for) AS scheduled_for,
          COUNT(*) FILTER (WHERE c.status='pending'::public.tracking_status) AS pending_count,
          COUNT(*) FILTER (WHERE c.status='sent'::public.tracking_status)    AS sent_count,
          COUNT(*) FILTER (WHERE c.status='done'::public.tracking_status)    AS done_count
        FROM public.tracked_test_batches b
        JOIN public.tracked_test_compounds c ON c.batch_id = b.batch_id
        WHERE c.scheduled_for = :day
        GROUP BY b.batch_id, b.project_code, b.test_code
        ORDER BY b.project_code, b.test_code
    )SQL");
    q.bindValue(":day", day);

    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }

    while (q.next()) {
        CalendarEventItem it;
        it.batchId = q.value(0).toString();
        it.projectCode = q.value(1).toString();
        it.testCode = q.value(2).toString();
        it.scheduledFor = q.value(3).toDate();
        it.pendingCount = q.value(4).toInt();
        it.sentCount = q.value(5).toInt();
        int doneCount = q.value(6).toInt();
        it.title = QString("%1 • %2 (Pending %3 / Sent %4 / Done %5)")
                       .arg(it.projectCode, it.testCode)
                       .arg(it.pendingCount)
                       .arg(it.sentCount)
                       .arg(doneCount);
        out.push_back(std::move(it));
    }

    return out;
}
