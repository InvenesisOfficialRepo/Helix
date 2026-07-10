#pragma once

#include <QString>
#include <QVariantMap>
#include <QList>

namespace import_module {

class ImportDataAccess {
public:
    ImportDataAccess();
    ~ImportDataAccess() = default;

    // Returns the number of successfully inserted rows, or -1 on error.
    int importData(const QString& tableName, const QList<QVariantMap>& mappedRows, QString* errOut);

private:
    QString generateNextBottleId() const;
    QString generateNextSolutionId() const;
    
    bool insertBottles(const QList<QVariantMap>& rows, QString* errOut);
    bool insertSolutions(const QList<QVariantMap>& rows, QString* errOut);
};

} // namespace import_module
