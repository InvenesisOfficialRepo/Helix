#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QVariantMap>
#include "ImportSchema.h"

namespace import_module {

struct ParsedData {
    QStringList headers;
    QList<QVariantList> rows;
};

class SmartImporter {
public:
    SmartImporter();
    ~SmartImporter() = default;

    // 1. Read raw data from file
    bool loadFile(const QString& filePath, QString* errOut = nullptr);
    
    // 2. Access parsed raw data
    const ParsedData& getRawData() const { return m_parsedData; }
    
    // 3. Auto-map headers to a given schema
    // Returns a map of Schema dbColumnName -> File Header Index (-1 if not mapped)
    QMap<QString, int> autoMapHeaders(const ImportTableSchema& schema) const;

private:
    bool loadXlsx(const QString& filePath, QString* errOut);
    bool loadCsv(const QString& filePath, QString* errOut);
    
    QString normalizeString(const QString& str) const;
    int calculateSimilarity(const QString& s1, const QString& s2) const;

    ParsedData m_parsedData;
};

} // namespace import_module
