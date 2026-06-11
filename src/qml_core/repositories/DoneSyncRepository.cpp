#include "DoneSyncRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

DoneSyncRepository::DoneSyncRepository(QSqlDatabase db)
    : db_(std::move(db))
{
}

bool DoneSyncRepository::syncDoneFromTestRequests(int* compoundsUpdated,
                                                  int* batchesUpdated,
                                                  QString* errorOut)
{
    if (compoundsUpdated) *compoundsUpdated = 0;
    if (batchesUpdated) *batchesUpdated = 0;
    if (errorOut) errorOut->clear();

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return false;
    }

    if (!db_.transaction()) {
        if (errorOut) *errorOut = "Failed to start transaction: " + db_.lastError().text();
        return false;
    }

    auto rollback = [&]() {
        db_.rollback();
    };

    // 1) Mark tracked compounds done where test_requests.done=true
    int updatedComp = 0;
    {
        QSqlQuery q(db_);
        q.prepare(R"SQL(
            WITH done_ids AS (
                SELECT DISTINCT tracked_compound_id
                FROM public.test_requests
                WHERE done = true
                  AND tracked_compound_id IS NOT NULL
            )
            UPDATE public.tracked_test_compounds c
            SET status  = 'done'::public.tracking_status,
                done_at = COALESCE(c.done_at, now())
            FROM done_ids d
            WHERE c.tracked_compound_id = d.tracked_compound_id
              AND c.status <> 'done'::public.tracking_status
        )SQL");

        if (!q.exec()) {
            if (errorOut) *errorOut = "Sync compounds failed: " + q.lastError().text();
            rollback();
            return false;
        }

        updatedComp = q.numRowsAffected();
        if (updatedComp < 0) updatedComp = 0; // defensive
    }

    // 2) Mark batches done if ALL compounds in batch are done (and batch has at least one compound)
    int updatedBatch = 0;
    {
        QSqlQuery q(db_);
        q.prepare(R"SQL(
            UPDATE public.tracked_test_batches b
            SET status = 'done'::public.tracking_status
            WHERE b.status <> 'done'::public.tracking_status
              AND EXISTS (
                  SELECT 1 FROM public.tracked_test_compounds c2
                  WHERE c2.batch_id = b.batch_id
              )
              AND NOT EXISTS (
                  SELECT 1 FROM public.tracked_test_compounds c
                  WHERE c.batch_id = b.batch_id
                    AND c.status <> 'done'::public.tracking_status
              )
        )SQL");

        if (!q.exec()) {
            if (errorOut) *errorOut = "Sync batches failed: " + q.lastError().text();
            rollback();
            return false;
        }

        updatedBatch = q.numRowsAffected();
        if (updatedBatch < 0) updatedBatch = 0; // defensive
    }

    if (!db_.commit()) {
        if (errorOut) *errorOut = "Commit failed: " + db_.lastError().text();
        rollback();
        return false;
    }

    if (compoundsUpdated) *compoundsUpdated = updatedComp;
    if (batchesUpdated) *batchesUpdated = updatedBatch;
    return true;
}
