#include "ExcelImportRepository.h"

#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#ifdef USE_QXLSX
#include "xlsxdocument.h"
#include "xlsxcellrange.h"
#endif

static QString norm(QString s) { return s.trimmed().toLower(); }

struct ExcelRow {
    QString clientCode;     // "Client Code"
    QString projectCode;    // "Project N°" -> projects.project_code
    QString compoundName;   // "Sample ID"  -> tracked_test_compounds.compound_name
    QString testCode;       // "Invenesis Assay ID" -> tracked_test_batches.test_code
    QString species;        // "Species" -> tracked_test_compounds.species

    double startingDose_uM = 0.0; // "Starting Dose (µM)" -> starting_concentration
    double dilutionFold = 0.0;    // "Dilution Fold" -> dilution_steps (int) if integral, else store in notes
    int nbReplicates = 0;         // "Nb Replicates" -> number_of_replicate
    int nbDilutions = 0;          // "Nb Dilutions" -> number_of_dilutions
};

static bool parseXlsx(const QString& xlsxPath, QVector<ExcelRow>* rowsOut, QString* errorOut)
{
#ifndef USE_QXLSX
    Q_UNUSED(xlsxPath);
    Q_UNUSED(rowsOut);
    if (errorOut) *errorOut = "Excel import not available: build without QXlsx (USE_QXLSX=OFF).";
    return false;
#else
    QFileInfo fi(xlsxPath);
    if (!fi.exists()) {
        if (errorOut) *errorOut = "File not found: " + xlsxPath;
        return false;
    }
    if (!xlsxPath.toLower().endsWith(".xlsx")) {
        if (errorOut) *errorOut = "Not an .xlsx file: " + xlsxPath;
        return false;
    }

    QXlsx::Document doc(xlsxPath);
    const QStringList sheets = doc.sheetNames();
    if (sheets.isEmpty()) {
        if (errorOut) *errorOut = "Excel file contains no sheets.";
        return false;
    }
    doc.selectSheet(sheets.first());

    const QXlsx::CellRange range = doc.dimension();
    if (!range.isValid() || range.rowCount() < 2) {
        if (errorOut) *errorOut = "Excel sheet looks empty (no data rows).";
        return false;
    }

    // Header map: normalized header -> column index
    QHash<QString, int> col;
    const int headerRow = range.firstRow();
    for (int c = range.firstColumn(); c <= range.lastColumn(); ++c) {
        const QVariant v = doc.read(headerRow, c);
        if (!v.isValid()) continue;
        const QString h = norm(v.toString());
        if (!h.isEmpty()) col[h] = c;
    }

    auto require = [&](const QString& header) -> bool {
        if (!col.contains(norm(header))) {
            if (errorOut) *errorOut = "Missing required header: " + header;
            return false;
        }
        return true;
    };

    // required headers
    if (!require("Client Code")) return false;
    if (!require("Project N°")) return false;
    if (!require("Sample ID")) return false;
    if (!require("Invenesis Assay ID")) return false;
    if (!require("Species")) return false;
    if (!require("Starting Dose (µM)")) return false;
    if (!require("Dilution Fold")) return false;
    if (!require("Nb Replicates")) return false;
    if (!require("Nb Dilutions")) return false;

    auto readStr = [&](int r, const QString& h) -> QString {
        return doc.read(r, col[norm(h)]).toString().trimmed();
    };
    auto readDouble = [&](int r, const QString& h) -> double {
        return doc.read(r, col[norm(h)]).toDouble();
    };
    auto readInt = [&](int r, const QString& h) -> int {
        return doc.read(r, col[norm(h)]).toInt();
    };

    rowsOut->clear();

    for (int r = headerRow + 1; r <= range.lastRow(); ++r) {
        ExcelRow row;
        row.clientCode   = readStr(r, "Client Code");
        row.projectCode  = readStr(r, "Project N°");
        row.compoundName = readStr(r, "Sample ID");
        row.testCode     = readStr(r, "Invenesis Assay ID");
        row.species      = readStr(r, "Species");

        if (row.projectCode.isEmpty() && row.compoundName.isEmpty() && row.testCode.isEmpty() && row.species.isEmpty())
            continue;

        row.startingDose_uM = readDouble(r, "Starting Dose (µM)");
        row.dilutionFold    = readDouble(r, "Dilution Fold");
        row.nbReplicates    = readInt(r, "Nb Replicates");
        row.nbDilutions     = readInt(r, "Nb Dilutions");

        if (row.clientCode.isEmpty() || row.projectCode.isEmpty() || row.compoundName.isEmpty() || row.testCode.isEmpty()) {
            if (errorOut) *errorOut = QString("Invalid row %1: Client Code, Project N°, Sample ID and Invenesis Assay ID must be filled.").arg(r);
            return false;
        }

        rowsOut->push_back(row);
    }

    if (rowsOut->isEmpty()) {
        if (errorOut) *errorOut = "No data rows found in Excel.";
        return false;
    }

    return true;
#endif
}

ExcelImportRepository::ExcelImportRepository(QSqlDatabase db)
    : db_(db)
{
}

bool ExcelImportRepository::importFile(const QString& xlsxPath,
                                       const QString& createdByUser,
                                       QString* infoOut,
                                       QString* errorOut)
{
    if (infoOut) *infoOut = {};
    if (errorOut) *errorOut = {};

    if (!db_.isValid() || !db_.isOpen()) {
        if (errorOut) *errorOut = "Database is not open.";
        return false;
    }

    QVector<ExcelRow> rows;
    if (!parseXlsx(xlsxPath, &rows, errorOut))
        return false;

    // group by project+test
    QMap<QString, QVector<ExcelRow>> groups;
    for (const auto& r : rows) {
        const QString key = r.projectCode + "||" + r.testCode;
        groups[key].push_back(r);
    }

    if (!db_.transaction()) {
        if (errorOut) *errorOut = "Failed to start transaction: " + db_.lastError().text();
        return false;
    }

    auto rollback = [&](const QString& msg) {
        db_.rollback();
        if (errorOut) *errorOut = msg;
        return false;
    };

    // 1) Ensure project exists (create if missing; update client_code if conflict)
    // Your projects schema has client_code NOT NULL, so we must provide it on insert.
    QSqlQuery upsertProject(db_);
    upsertProject.prepare(R"(
        INSERT INTO projects (project_code, client_code)
        VALUES (:project_code, :client_code)
        ON CONFLICT (project_code)
        DO UPDATE SET client_code = EXCLUDED.client_code
    )");

    // 2) Insert new batches (we always create a NEW batch per group)
    QSqlQuery insertBatch(db_);
    if (createdByUser.trimmed().isEmpty()) {
        insertBatch.prepare(R"(
            INSERT INTO tracked_test_batches (project_code, test_code, status)
            VALUES (:project_code, :test_code, 'pending')
            RETURNING batch_id
        )");
    } else {
        insertBatch.prepare(R"(
            INSERT INTO tracked_test_batches (project_code, test_code, status, created_by)
            VALUES (:project_code, :test_code, 'pending', :created_by)
            RETURNING batch_id
        )");
    }

    // 3) Insert compounds
    QSqlQuery insertComp(db_);
    insertComp.prepare(R"(
        INSERT INTO tracked_test_compounds (
            batch_id,
            compound_name,
            status,
            starting_concentration,
            starting_concentration_unit,
            dilution_steps,
            number_of_dilutions,
            number_of_replicate,
            additional_notes,
            species
        )
        VALUES (
            :batch_id,
            :compound_name,
            'pending',
            :starting_concentration,
            'uM',
            :dilution_steps,
            :number_of_dilutions,
            :number_of_replicate,
            :additional_notes,
            :species
        )
    )");

    int batchesCreated = 0;
    int compoundsInserted = 0;

    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const QVector<ExcelRow>& gr = it.value();
        const ExcelRow& first = gr.first();

        // upsert project
        upsertProject.bindValue(":project_code", first.projectCode);
        upsertProject.bindValue(":client_code", first.clientCode);
        if (!upsertProject.exec())
            return rollback("Upsert into projects failed: " + upsertProject.lastError().text());

        // create batch
        insertBatch.bindValue(":project_code", first.projectCode);
        insertBatch.bindValue(":test_code", first.testCode);
        if (!createdByUser.trimmed().isEmpty())
            insertBatch.bindValue(":created_by", createdByUser.trimmed());

        if (!insertBatch.exec())
            return rollback("Insert batch failed: " + insertBatch.lastError().text());

        if (!insertBatch.next())
            return rollback("Insert batch did not return batch_id.");

        const QString batchId = insertBatch.value(0).toString();
        ++batchesCreated;

        // insert compounds
        for (const auto& r : gr) {
            // dilutionFold: your schema uses dilution_steps INTEGER.
            // If dilutionFold is non-integer (e.g. 2.5), store it in notes and set dilution_steps to 0.
            int dilutionStepsInt = 0;
            QString notes;

            const double rounded = qRound(r.dilutionFold);
            if (qFuzzyCompare(r.dilutionFold + 1.0, rounded + 1.0)) {
                dilutionStepsInt = static_cast<int>(rounded);
            } else {
                dilutionStepsInt = 0;
                notes = QString("Dilution Fold (non-integer): %1").arg(r.dilutionFold);
            }

            insertComp.bindValue(":batch_id", batchId);
            insertComp.bindValue(":compound_name", r.compoundName);
            insertComp.bindValue(":starting_concentration", r.startingDose_uM);
            insertComp.bindValue(":dilution_steps", dilutionStepsInt);
            insertComp.bindValue(":number_of_dilutions", r.nbDilutions);
            insertComp.bindValue(":number_of_replicate", r.nbReplicates);
            insertComp.bindValue(":additional_notes", notes);
            insertComp.bindValue(":species", r.species);

            if (!insertComp.exec())
                return rollback("Insert compound failed: " + insertComp.lastError().text());

            ++compoundsInserted;
        }
    }

    if (!db_.commit()) {
        if (errorOut) *errorOut = "Commit failed: " + db_.lastError().text();
        return false;
    }

    if (infoOut) {
        *infoOut = QString("Created %1 batch(es), inserted %2 compound(s).")
                       .arg(batchesCreated)
                       .arg(compoundsInserted);
    }

    return true;
}
