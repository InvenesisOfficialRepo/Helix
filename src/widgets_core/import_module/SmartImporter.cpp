#include "SmartImporter.h"

#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#ifdef USE_QXLSX
#include "xlsxdocument.h"
#include "xlsxcellrange.h"
#endif

namespace import_module {

SmartImporter::SmartImporter() {}

bool SmartImporter::loadFile(const QString& filePath, QString* errOut) {
    m_parsedData.headers.clear();
    m_parsedData.rows.clear();

    QFileInfo fi(filePath);
    if (!fi.exists()) {
        if (errOut) *errOut = "File not found: " + filePath;
        return false;
    }

    if (filePath.toLower().endsWith(".csv")) {
        return loadCsv(filePath, errOut);
    } else if (filePath.toLower().endsWith(".xlsx")) {
        return loadXlsx(filePath, errOut);
    } else {
        if (errOut) *errOut = "Unsupported file format. Please use .csv or .xlsx";
        return false;
    }
}

bool SmartImporter::loadXlsx(const QString& filePath, QString* errOut) {
#ifndef USE_QXLSX
    Q_UNUSED(filePath);
    if (errOut) *errOut = "Excel import not available: built without QXlsx support.";
    return false;
#else
    QXlsx::Document doc(filePath);
    const QStringList sheets = doc.sheetNames();
    if (sheets.isEmpty()) {
        if (errOut) *errOut = "Excel file contains no sheets.";
        return false;
    }
    doc.selectSheet(sheets.first());

    const QXlsx::CellRange range = doc.dimension();
    if (!range.isValid() || range.rowCount() < 1) {
        if (errOut) *errOut = "Excel sheet looks empty.";
        return false;
    }

    int headerRow = range.firstRow();
    
    // Extract headers
    for (int c = range.firstColumn(); c <= range.lastColumn(); ++c) {
        QString h = doc.read(headerRow, c).toString().trimmed();
        m_parsedData.headers.append(h);
    }

    // Extract rows
    for (int r = headerRow + 1; r <= range.lastRow(); ++r) {
        QVariantList rowData;
        bool hasData = false;
        for (int c = range.firstColumn(); c <= range.lastColumn(); ++c) {
            QVariant v = doc.read(r, c);
            rowData.append(v);
            if (v.isValid() && !v.toString().trimmed().isEmpty()) {
                hasData = true;
            }
        }
        if (hasData) {
            m_parsedData.rows.append(rowData);
        }
    }

    return true;
#endif
}

// Simple CSV parser that handles basic quotes
bool SmartImporter::loadCsv(const QString& filePath, QString* errOut) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errOut) *errOut = "Failed to open CSV file.";
        return false;
    }

    QTextStream in(&file);
    bool isHeader = true;

    // Detect delimiter from the first line
    QString firstLine = in.readLine();
    if (firstLine.isNull()) {
        if (errOut) *errOut = "CSV file is empty.";
        return false;
    }

    QChar delimiter = ',';
    if (firstLine.count(';') > firstLine.count(',')) {
        delimiter = ';';
    } else if (firstLine.count('\t') > firstLine.count(',')) {
        delimiter = '\t';
    }

    // Helper to parse a line respecting quotes
    auto parseLine = [delimiter](const QString& line) -> QStringList {
        QStringList result;
        QString current;
        bool inQuotes = false;
        for (int i = 0; i < line.length(); ++i) {
            QChar c = line[i];
            if (c == '"') {
                if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                    current.append('"');
                    ++i; // skip escaped quote
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (c == delimiter && !inQuotes) {
                result.append(current.trimmed());
                current.clear();
            } else {
                current.append(c);
            }
        }
        result.append(current.trimmed());
        return result;
    };

    m_parsedData.headers = parseLine(firstLine);

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;
        
        QStringList cols = parseLine(line);
        QVariantList rowData;
        for (const QString& col : cols) {
            rowData.append(col);
        }
        
        // Pad rowData if it has fewer columns than headers
        while (rowData.size() < m_parsedData.headers.size()) {
            rowData.append(QString());
        }
        
        m_parsedData.rows.append(rowData);
    }

    return true;
}

QString SmartImporter::normalizeString(const QString& str) const {
    QString norm = str.toLower();
    norm.remove(QRegularExpression("[^a-z0-9]"));
    return norm;
}

QMap<QString, int> SmartImporter::autoMapHeaders(const ImportTableSchema& schema) const {
    QMap<QString, int> mapping;
    
    // For each schema column, find the best header match
    for (const auto& col : schema.columns) {
        int bestMatchIdx = -1;
        
        // 1. Try exact alias match (normalized)
        for (int i = 0; i < m_parsedData.headers.size(); ++i) {
            QString fileHeader = normalizeString(m_parsedData.headers[i]);
            if (fileHeader.isEmpty()) continue;

            for (const QString& alias : col.aliases) {
                if (fileHeader == normalizeString(alias)) {
                    bestMatchIdx = i;
                    break;
                }
            }
            if (bestMatchIdx != -1) break;
        }

        // 2. If no exact match, try substring match (file header contains alias or vice versa)
        if (bestMatchIdx == -1) {
            for (int i = 0; i < m_parsedData.headers.size(); ++i) {
                QString fileHeader = normalizeString(m_parsedData.headers[i]);
                if (fileHeader.isEmpty()) continue;

                for (const QString& alias : col.aliases) {
                    QString normAlias = normalizeString(alias);
                    if (fileHeader.contains(normAlias) || normAlias.contains(fileHeader)) {
                        bestMatchIdx = i;
                        break;
                    }
                }
                if (bestMatchIdx != -1) break;
            }
        }
        
        mapping[col.dbColumnName] = bestMatchIdx;
    }

    return mapping;
}

} // namespace import_module
