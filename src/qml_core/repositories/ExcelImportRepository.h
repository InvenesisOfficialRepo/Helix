#pragma once

#include <QSqlDatabase>
#include <QString>

class ExcelImportRepository
{
public:
    explicit ExcelImportRepository(QSqlDatabase db);

    // Creates:
    //  - projects row (if missing)
    //  - tracked_test_batches row per (project_code, test_code) group
    //  - tracked_test_compounds rows
    //
    // Runs in one transaction.
    bool importFile(const QString& xlsxPath,
                    const QString& createdByUser,   // optional: store in tracked_test_batches.created_by
                    QString* infoOut,
                    QString* errorOut);

private:
    QSqlDatabase db_;
};