#include "BatchRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>

BatchRepository::BatchRepository(QSqlDatabase db)
    : db_(std::move(db))
{
}

BatchDetailDto BatchRepository::loadBatchDetail(const QString& batchId, QString* errorOut)
{
    if (errorOut) errorOut->clear();

    BatchDetailDto out;
    out.batchId = batchId;

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return {};
    }

    // 1) Batch info
    {
        QSqlQuery q(db_);
        q.prepare(R"SQL(
            SELECT
              b.batch_id,
              b.project_code,
              b.test_code,
              b.status::text,
              b.scheduled_for,
              b.created_at,
              b.created_by,
              EXISTS (
                SELECT 1 FROM public.batch_reports r WHERE r.batch_id = :batch
              ) AS has_report,
              bar.analysis_json::text
            FROM public.tracked_test_batches b
            LEFT JOIN public.batch_analysis_results bar ON bar.batch_id = b.batch_id
            WHERE b.batch_id = :batch
        )SQL");
        q.bindValue(":batch", batchId);

        if (!q.exec()) {
            if (errorOut) *errorOut = q.lastError().text();
            return {};
        }
        if (!q.next()) {
            if (errorOut) *errorOut = "Batch not found: " + batchId;
            return {};
        }

        out.batchId     = q.value(0).toString();
        out.projectCode = q.value(1).toString();
        out.testCode    = q.value(2).toString();
        out.status      = statusFromDbValue(q.value(3).toString());
        out.scheduledFor = q.value(4).toDate();
        out.createdAt    = q.value(5).toDateTime();
        out.createdBy   = q.value(6).toString();
        out.hasReport   = q.value(7).toBool();

        QString jsonStr = q.value(8).toString();
        if (!jsonStr.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            out.analysisResults = doc.toVariant().toMap();
        }
    }

    // 2) Compounds for this batch
    {
        QSqlQuery q(db_);
        q.prepare(R"SQL(
            SELECT
              c.tracked_compound_id,
              c.compound_name,
              c.status::text,
              c.scheduled_for,
              c.robot_request_id,
              EXISTS (
                SELECT 1 
                FROM public.solutions s 
                WHERE LOWER(s.product_name) = LOWER(c.compound_name)
              ) AS has_solution,
              c.number_of_dilutions,
              c.number_of_replicate
            FROM public.tracked_test_compounds c
            WHERE c.batch_id = :batch
            ORDER BY c.created_at ASC
        )SQL");
        q.bindValue(":batch", batchId);

        if (!q.exec()) {
            if (errorOut) *errorOut = q.lastError().text();
            return {};
        }

        QVector<CompoundDto> compounds;
        while (q.next()) {
            CompoundDto c;
            c.trackedCompoundId = q.value(0).toString();
            c.compoundName      = q.value(1).toString();
            c.status            = statusFromDbValue(q.value(2).toString());
            c.scheduledFor      = q.value(3).toDate();
            c.robotRequestId    = q.value(4).toString();
            c.hasSolution       = q.value(5).toBool();
            c.numberOfDilutions = q.value(6).toInt();
            if (c.numberOfDilutions <= 0) c.numberOfDilutions = 3;
            c.numberOfReplicates = q.value(7).toInt();
            if (c.numberOfReplicates <= 0) c.numberOfReplicates = 3;
            compounds.push_back(std::move(c));
        }
        out.compounds = std::move(compounds);
    }

    return out;
}

bool BatchRepository::sendSelectedCompounds(const QString& batchId,
                                            const QStringList& trackedCompoundIds,
                                            const QDate& scheduledFor,
                                            QString* errorOut)
{
    if (errorOut) errorOut->clear();

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return false;
    }
    if (!scheduledFor.isValid()) {
        if (errorOut) *errorOut = "Invalid scheduled date.";
        return false;
    }
    if (trackedCompoundIds.isEmpty()) {
        if (errorOut) *errorOut = "No compounds selected.";
        return false;
    }

    if (!db_.transaction()) {
        if (errorOut) *errorOut = "Failed to start transaction: " + db_.lastError().text();
        return false;
    }

    auto rollback = [&]() {
        db_.rollback();
    };

    // Prepared queries
    QSqlQuery qUpdateCompound(db_);
    if (!qUpdateCompound.prepare(R"SQL(
        UPDATE public.tracked_test_compounds
        SET status = 'sent'::public.tracking_status,
            scheduled_for = :date,
            sent_at = now()
        WHERE tracked_compound_id = :cid
          AND batch_id = :batch
          AND status = 'pending'::public.tracking_status
    )SQL")) {
        if (errorOut) *errorOut = qUpdateCompound.lastError().text();
        rollback();
        return false;
    }

    QSqlQuery qInsertRequest(db_);
    if (!qInsertRequest.prepare(R"SQL(
        INSERT INTO public.test_requests(
            request_id,
            project_code,
            requested_tests,
            compound_name,
            starting_concentration,
            starting_concentration_unit,
            dilution_steps,
            dilution_steps_unit,
            number_of_dilutions,
            number_of_replicate,
            stock_concentration,
            stock_concentration_unit,
            concentrations_to_be_tested,
            additional_notes,
            done,
            test_code,
            tracked_compound_id,
            scheduled_for
        )
        SELECT
            :request_id,
            b.project_code,
            b.test_code,
            c.compound_name,
            c.starting_concentration,
            c.starting_concentration_unit,
            c.dilution_steps,
            c.dilution_steps_unit,
            c.number_of_dilutions,
            c.number_of_replicate,
            c.stock_concentration,
            c.stock_concentration_unit,
            c.concentrations_to_be_tested,
            c.additional_notes,
            false,
            b.test_code,
            c.tracked_compound_id,
            :date
        FROM public.tracked_test_batches b
        JOIN public.tracked_test_compounds c ON c.batch_id = b.batch_id
        WHERE b.batch_id = :batch
          AND c.tracked_compound_id = :cid
    )SQL")) {
        if (errorOut) *errorOut = qInsertRequest.lastError().text();
        rollback();
        return false;
    }

    QSqlQuery qLinkRobotId(db_);
    if (!qLinkRobotId.prepare(R"SQL(
        UPDATE public.tracked_test_compounds
        SET robot_request_id = :request_id
        WHERE tracked_compound_id = :cid
          AND batch_id = :batch
    )SQL")) {
        if (errorOut) *errorOut = qLinkRobotId.lastError().text();
        rollback();
        return false;
    }

    // Loop compounds (safe + simple in Qt)
    for (const QString& cid : trackedCompoundIds) {
        const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        // 1) update compound to sent (only if pending)
        qUpdateCompound.bindValue(":date", scheduledFor);
        qUpdateCompound.bindValue(":cid", cid);
        qUpdateCompound.bindValue(":batch", batchId);

        if (!qUpdateCompound.exec()) {
            if (errorOut) *errorOut = "Update compound failed: " + qUpdateCompound.lastError().text();
            rollback();
            return false;
        }

        // If it wasn’t pending, we skip inserting a request (prevents duplicates)
        if (qUpdateCompound.numRowsAffected() <= 0) {
            continue;
        }

        // 2) insert into test_requests
        qInsertRequest.bindValue(":request_id", requestId);
        qInsertRequest.bindValue(":date", scheduledFor);
        qInsertRequest.bindValue(":batch", batchId);
        qInsertRequest.bindValue(":cid", cid);

        if (!qInsertRequest.exec()) {
            if (errorOut) *errorOut = "Insert test_request failed: " + qInsertRequest.lastError().text();
            rollback();
            return false;
        }

        // 3) back-link robot_request_id on tracking compound
        qLinkRobotId.bindValue(":request_id", requestId);
        qLinkRobotId.bindValue(":cid", cid);
        qLinkRobotId.bindValue(":batch", batchId);

        if (!qLinkRobotId.exec()) {
            if (errorOut) *errorOut = "Update robot_request_id failed: " + qLinkRobotId.lastError().text();
            rollback();
            return false;
        }
    }

    // Update batch scheduled_for and set status to sent if no pending remain
    {
        QSqlQuery q(db_);
        q.prepare(R"SQL(
            UPDATE public.tracked_test_batches b
            SET scheduled_for = :date,
                status = CASE
                    WHEN EXISTS (
                        SELECT 1 FROM public.tracked_test_compounds c
                        WHERE c.batch_id = b.batch_id
                          AND c.status = 'pending'::public.tracking_status
                    )
                    THEN 'pending'::public.tracking_status
                    ELSE 'sent'::public.tracking_status
                END
            WHERE b.batch_id = :batch
              AND b.status <> 'done'::public.tracking_status
        )SQL");
        q.bindValue(":date", scheduledFor);
        q.bindValue(":batch", batchId);

        if (!q.exec()) {
            if (errorOut) *errorOut = "Update batch failed: " + q.lastError().text();
            rollback();
            return false;
        }
    }

    if (!db_.commit()) {
        if (errorOut) *errorOut = "Commit failed: " + db_.lastError().text();
        rollback();
        return false;
    }

    return true;
}
