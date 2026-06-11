#include "DashboardRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

DashboardRepository::DashboardRepository(QSqlDatabase db)
    : db_(std::move(db))
{
}

QVector<TestCardDto> DashboardRepository::loadDashboardSnapshot(QString* errorOut)
{
    if (errorOut) errorOut->clear();

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return {};
    }

    // Cards: one row per test_code for non-done batches
    static const char* kCardsSql = R"SQL(
        SELECT
          b.test_code,
          COUNT(c.tracked_compound_id) FILTER (WHERE c.status = 'pending'::public.tracking_status) AS pending_count,
          COUNT(c.tracked_compound_id) FILTER (WHERE c.status = 'sent'::public.tracking_status)    AS sent_count
        FROM public.tracked_test_batches b
        LEFT JOIN public.tracked_test_compounds c ON c.batch_id = b.batch_id
        WHERE b.status <> 'done'::public.tracking_status
        GROUP BY b.test_code
        ORDER BY b.test_code;
    )SQL";

    QSqlQuery q(db_);
    if (!q.prepare(kCardsSql)) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }
    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }

    QVector<TestCardDto> cards;
    while (q.next()) {
        TestCardDto c;
        c.testCode = q.value(0).toString();
        c.pendingCount = q.value(1).toInt();
        c.sentCount = q.value(2).toInt();

        // Load batches for this test_code (ordered)
        QString err;
        c.batches = loadBatchesForTest(c.testCode, &err);
        if (!err.isEmpty()) {
            if (errorOut) *errorOut = err;
            return {};
        }

        cards.push_back(std::move(c));
    }

    return cards;
}

QVector<BatchDto> DashboardRepository::loadBatchesForTest(const QString& testCode, QString* errorOut)
{
    if (errorOut) errorOut->clear();

    static const char* kBatchesSql = R"SQL(
        SELECT
          b.batch_id,
          b.project_code,
          b.test_code,
          b.status::text,
          b.scheduled_for,
          b.created_at,
          COUNT(c.tracked_compound_id) FILTER (WHERE c.status = 'pending'::public.tracking_status) AS pending_count,
          COUNT(c.tracked_compound_id) FILTER (WHERE c.status = 'sent'::public.tracking_status)    AS sent_count
        FROM public.tracked_test_batches b
        LEFT JOIN public.tracked_test_compounds c ON c.batch_id = b.batch_id
        WHERE b.status <> 'done'::public.tracking_status
          AND b.test_code = :test
        GROUP BY b.batch_id, b.project_code, b.test_code, b.status, b.scheduled_for, b.created_at
        ORDER BY
          b.scheduled_for NULLS LAST,
          b.created_at DESC;
    )SQL";

    QSqlQuery q(db_);
    if (!q.prepare(kBatchesSql)) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }
    q.bindValue(":test", testCode);

    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }

    QVector<BatchDto> out;
    while (q.next()) {
        BatchDto b;
        b.batchId      = q.value(0).toString();
        b.projectCode  = q.value(1).toString();
        b.testCode     = q.value(2).toString();
        b.status       = statusFromDbValue(q.value(3).toString());
        b.scheduledFor = q.value(4).toDate();     // invalid if NULL
        b.createdAt    = q.value(5).toDateTime();
        b.pendingCount = q.value(6).toInt();
        b.sentCount    = q.value(7).toInt();
        out.push_back(std::move(b));
    }
    return out;
}

QVector<TestCardDto> DashboardRepository::loadDoneDashboardSnapshot(QString* errorOut)
{
    if (errorOut) errorOut->clear();

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return {};
    }

    // Cards: one row per test_code for done but unpublished batches
    static const char* kCardsSql = R"SQL(
        SELECT
          b.test_code,
          0 AS pending_count,
          0 AS sent_count
        FROM public.tracked_test_batches b
        WHERE b.status = 'done'::public.tracking_status
          AND NOT EXISTS (
            SELECT 1 FROM public.batch_reports r WHERE r.batch_id = b.batch_id
          )
          AND b.created_at >= date_trunc('year', CURRENT_DATE)
        GROUP BY b.test_code
        ORDER BY b.test_code;
    )SQL";

    QSqlQuery q(db_);
    if (!q.prepare(kCardsSql)) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }
    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }

    QVector<TestCardDto> cards;
    while (q.next()) {
        TestCardDto c;
        c.testCode = q.value(0).toString();
        c.pendingCount = q.value(1).toInt();
        c.sentCount = q.value(2).toInt();

        // Load done batches for this test_code (ordered)
        QString err;
        c.batches = loadDoneBatchesForTest(c.testCode, &err);
        if (!err.isEmpty()) {
            if (errorOut) *errorOut = err;
            return {};
        }

        cards.push_back(std::move(c));
    }

    return cards;
}

QVector<BatchDto> DashboardRepository::loadDoneBatchesForTest(const QString& testCode, QString* errorOut)
{
    if (errorOut) errorOut->clear();

    static const char* kBatchesSql = R"SQL(
        SELECT
          b.batch_id,
          b.project_code,
          b.test_code,
          b.status::text,
          b.scheduled_for,
          b.created_at,
          0 AS pending_count,
          0 AS sent_count,
          false AS has_report
        FROM public.tracked_test_batches b
        WHERE b.status = 'done'::public.tracking_status
          AND b.test_code = :test
          AND NOT EXISTS (
            SELECT 1 FROM public.batch_reports r WHERE r.batch_id = b.batch_id
          )
          AND b.created_at >= date_trunc('year', CURRENT_DATE)
        GROUP BY b.batch_id, b.project_code, b.test_code, b.status, b.scheduled_for, b.created_at
        ORDER BY
          b.scheduled_for NULLS LAST,
          b.created_at DESC;
    )SQL";

    QSqlQuery q(db_);
    if (!q.prepare(kBatchesSql)) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }
    q.bindValue(":test", testCode);

    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }

    QVector<BatchDto> out;
    while (q.next()) {
        BatchDto b;
        b.batchId      = q.value(0).toString();
        b.projectCode  = q.value(1).toString();
        b.testCode     = q.value(2).toString();
        b.status       = statusFromDbValue(q.value(3).toString());
        b.scheduledFor = q.value(4).toDate();     // invalid if NULL
        b.createdAt    = q.value(5).toDateTime();
        b.pendingCount = q.value(6).toInt();
        b.sentCount    = q.value(7).toInt();
        b.hasReport    = q.value(8).toBool();
        out.push_back(std::move(b));
    }
    return out;
}

QVector<TestCardDto> DashboardRepository::loadArchiveDashboardSnapshot(QString* errorOut)
{
    if (errorOut) errorOut->clear();

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return {};
    }

    // Cards: one row per test_code for done and published batches
    static const char* kCardsSql = R"SQL(
        SELECT
          b.test_code,
          0 AS pending_count,
          0 AS sent_count
        FROM public.tracked_test_batches b
        WHERE b.status = 'done'::public.tracking_status
          AND EXISTS (
            SELECT 1 FROM public.batch_reports r WHERE r.batch_id = b.batch_id
          )
          AND b.created_at >= date_trunc('year', CURRENT_DATE)
        GROUP BY b.test_code
        ORDER BY b.test_code;
    )SQL";

    QSqlQuery q(db_);
    if (!q.prepare(kCardsSql)) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }
    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }

    QVector<TestCardDto> cards;
    while (q.next()) {
        TestCardDto c;
        c.testCode = q.value(0).toString();
        c.pendingCount = q.value(1).toInt();
        c.sentCount = q.value(2).toInt();

        // Load archive batches for this test_code (ordered)
        QString err;
        c.batches = loadArchiveBatchesForTest(c.testCode, &err);
        if (!err.isEmpty()) {
            if (errorOut) *errorOut = err;
            return {};
        }

        cards.push_back(std::move(c));
    }

    return cards;
}

QVector<BatchDto> DashboardRepository::loadArchiveBatchesForTest(const QString& testCode, QString* errorOut)
{
    if (errorOut) errorOut->clear();

    static const char* kBatchesSql = R"SQL(
        SELECT
          b.batch_id,
          b.project_code,
          b.test_code,
          b.status::text,
          b.scheduled_for,
          b.created_at,
          0 AS pending_count,
          0 AS sent_count,
          true AS has_report
        FROM public.tracked_test_batches b
        WHERE b.status = 'done'::public.tracking_status
          AND b.test_code = :test
          AND EXISTS (
            SELECT 1 FROM public.batch_reports r WHERE r.batch_id = b.batch_id
          )
          AND b.created_at >= date_trunc('year', CURRENT_DATE)
        GROUP BY b.batch_id, b.project_code, b.test_code, b.status, b.scheduled_for, b.created_at
        ORDER BY
          b.scheduled_for NULLS LAST,
          b.created_at DESC;
    )SQL";

    QSqlQuery q(db_);
    if (!q.prepare(kBatchesSql)) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }
    q.bindValue(":test", testCode);

    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        return {};
    }

    QVector<BatchDto> out;
    while (q.next()) {
        BatchDto b;
        b.batchId      = q.value(0).toString();
        b.projectCode  = q.value(1).toString();
        b.testCode     = q.value(2).toString();
        b.status       = statusFromDbValue(q.value(3).toString());
        b.scheduledFor = q.value(4).toDate();     // invalid if NULL
        b.createdAt    = q.value(5).toDateTime();
        b.pendingCount = q.value(6).toInt();
        b.sentCount    = q.value(7).toInt();
        b.hasReport    = q.value(8).toBool();
        out.push_back(std::move(b));
    }
    return out;
}
