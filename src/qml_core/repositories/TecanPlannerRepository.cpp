#include "TecanPlannerRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

namespace {

QStringList parsePgArray(const QVariant& val)
{
    if (val.typeId() == QMetaType::QStringList) {
        return val.toStringList();
    }
    QString str = val.toString().trimmed();
    if (str.startsWith('{') && str.endsWith('}')) {
        str = str.mid(1, str.length() - 2);
        if (str.isEmpty()) return {};

        QStringList list;
        QString current;
        bool inQuotes = false;
        for (int i = 0; i < str.length(); ++i) {
            QChar ch = str.at(i);
            if (ch == '"') {
                inQuotes = !inQuotes;
            } else if (ch == ',' && !inQuotes) {
                list.append(current.trimmed());
                current.clear();
            } else {
                current.append(ch);
            }
        }
        if (!current.isEmpty()) {
            list.append(current.trimmed());
        }
        return list;
    }
    if (!str.isEmpty()) {
        return {str};
    }
    return {};
}

} // anonymous namespace

TecanPlannerRepository::TecanPlannerRepository(QSqlDatabase db)
    : db_(std::move(db))
{
}

QVector<TecanAssayBatchDto> TecanPlannerRepository::loadDailyAssayBatches(const QDate& date, QString* errorOut)
{
    if (errorOut) errorOut->clear();
    QVector<TecanAssayBatchDto> out;

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return {};
    }

    QSqlQuery q(db_);
    q.prepare(R"SQL(
        SELECT
          COALESCE(NULLIF(TRIM(test_code), ''), requested_tests, 'UNKNOWN') AS assay_code,
          COALESCE(NULLIF(TRIM(solvent), ''), 'DMSO') AS assay_solvent,
          COALESCE(NULLIF(TRIM(species), ''), 'Unknown') AS assay_species,
          COALESCE(NULLIF(TRIM(project_code), ''), 'N/A') AS assay_project,
          COUNT(*) AS compound_count,
          array_agg(request_id::text ORDER BY request_id) AS request_ids,
          array_agg(COALESCE(NULLIF(TRIM(compound_name), ''), 'Unnamed') ORDER BY compound_name) AS compound_names
        FROM public.test_requests
        WHERE scheduled_for = :day
          AND done = FALSE
        GROUP BY 
          COALESCE(NULLIF(TRIM(test_code), ''), requested_tests, 'UNKNOWN'),
          COALESCE(NULLIF(TRIM(solvent), ''), 'DMSO'),
          COALESCE(NULLIF(TRIM(species), ''), 'Unknown'),
          COALESCE(NULLIF(TRIM(project_code), ''), 'N/A')
        ORDER BY assay_code, assay_solvent, assay_species, assay_project
    )SQL");
    q.bindValue(":day", date);

    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        qWarning() << "[TecanPlannerRepository] query failed:" << (errorOut ? *errorOut : q.lastError().text());
        return {};
    }

    while (q.next()) {
        TecanAssayBatchDto b;
        b.testCode = q.value(0).toString();
        b.solvent = q.value(1).toString();
        b.species = q.value(2).toString();
        b.projectCode = q.value(3).toString();
        b.compoundCount = q.value(4).toInt();
        b.requestIds = parsePgArray(q.value(5));
        b.compoundNames = parsePgArray(q.value(6));

        out.push_back(std::move(b));
    }

    return out;
}

void TecanPlannerRepository::loadDateSummary(const QDate& date, int* outBatchCount, int* outCompoundCount, QString* errorOut)
{
    if (errorOut) errorOut->clear();
    if (outBatchCount) *outBatchCount = 0;
    if (outCompoundCount) *outCompoundCount = 0;

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database connection is not open.";
        return;
    }

    QSqlQuery q(db_);
    q.prepare(R"SQL(
        SELECT
          COUNT(DISTINCT (
            COALESCE(NULLIF(TRIM(test_code), ''), requested_tests, 'UNKNOWN'),
            COALESCE(NULLIF(TRIM(solvent), ''), 'DMSO'),
            COALESCE(NULLIF(TRIM(species), ''), 'Unknown'),
            COALESCE(NULLIF(TRIM(project_code), ''), 'N/A')
          )) AS batch_count,
          COUNT(*) AS total_compounds
        FROM public.test_requests
        WHERE scheduled_for = :day
          AND done = FALSE
    )SQL");
    q.bindValue(":day", date);

    if (!q.exec()) {
        if (errorOut) *errorOut = q.lastError().text();
        return;
    }

    if (q.next()) {
        if (outBatchCount) *outBatchCount = q.value(0).toInt();
        if (outCompoundCount) *outCompoundCount = q.value(1).toInt();
    }
}
