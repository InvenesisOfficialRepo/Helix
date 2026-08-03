#include "CsvParser.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

char CsvParser::detectDelimiter(const QString &firstLine) {
    int comma = firstLine.count(',');
    int semi = firstLine.count(';');
    int tab = firstLine.count('\t');

    if (semi > comma && semi > tab) return ';';
    if (tab > comma && tab > semi) return '\t';
    return ',';
}

QStringList CsvParser::splitCsvLine(const QString &line, char delimiter) {
    QStringList result;
    QString current;
    bool inQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar c = line.at(i);
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == delimiter && !inQuotes) {
            result.append(current.trimmed());
            current.clear();
        } else {
            current.append(c);
        }
    }
    result.append(current.trimmed());

    // Clean surrounding quotes
    for (int i = 0; i < result.size(); ++i) {
        QString item = result[i];
        if (item.startsWith('"') && item.endsWith('"') && item.length() >= 2) {
            result[i] = item.mid(1, item.length() - 2);
        }
    }
    return result;
}

CsvParseResult CsvParser::parseFile(const QString &filePath, PlateFormat expectedFormat) {
    CsvParseResult result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = "Could not open file: " + file.errorString();
        return result;
    }

    QTextStream in(&file);
    QStringList lines;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            lines.append(line);
        }
    }
    file.close();

    if (lines.isEmpty()) {
        result.errorMessage = "CSV file is empty.";
        return result;
    }

    char delim = detectDelimiter(lines.first());
    int expectedRows = PlateModel::getRowCount(expectedFormat);
    int expectedCols = PlateModel::getColCount(expectedFormat);

    result.compoundMatrix.resize(expectedRows);
    for (int r = 0; r < expectedRows; ++r) {
        result.compoundMatrix[r].resize(expectedCols);
    }

    // Try parsing as Matrix format first
    bool isMatrix = false;
    QStringList headerTokens = splitCsvLine(lines.first(), delim);

    // If first line has column numbers (like 1..12 or 1..24) or first cell is empty/label
    if (lines.size() >= expectedRows) {
        // Inspect row 1 label
        QStringList row1Tokens = splitCsvLine(lines.size() > 1 ? lines.at(1) : lines.first(), delim);
        if (!row1Tokens.isEmpty()) {
            int rIdx = PlateModel::getRowIndexFromLabel(row1Tokens.first());
            if (rIdx >= 0 && rIdx < expectedRows) {
                isMatrix = true;
            }
        }
    }

    if (isMatrix) {
        int startRowIdx = 0;
        // Check if header line is column numbers
        if (!headerTokens.isEmpty()) {
            int firstColIdx = PlateModel::getRowIndexFromLabel(headerTokens.first());
            if (firstColIdx < 0) {
                // Header line is column numbers (e.g. 1..12)
                startRowIdx = 1;
            }
        }

        for (int lineIdx = startRowIdx; lineIdx < lines.size(); ++lineIdx) {
            QStringList tokens = splitCsvLine(lines.at(lineIdx), delim);
            if (tokens.isEmpty()) continue;

            int r = PlateModel::getRowIndexFromLabel(tokens.first());
            if (r < 0 || r >= expectedRows) continue;

            for (int cIdx = 1; cIdx < tokens.size(); ++cIdx) {
                int c = cIdx - 1;
                if (c >= expectedCols) break;

                QString val = tokens.at(cIdx).trimmed();
                if (!val.isEmpty()) {
                    result.compoundMatrix[r][c] = val;
                    result.filledWells++;
                }
            }
        }
        result.success = true;
        result.detectedFormat = expectedFormat;
        return result;
    }

    // Otherwise try parsing as List format (Well, Compound, Barcode, Volume)
    int wellCol = -1;
    int compoundCol = -1;
    int barcodeCol = -1;
    int volumeCol = -1;
    int volumeUnitCol = -1;
    for (int i = 0; i < headerTokens.size(); ++i) {
        QString tok = headerTokens.at(i).toLower();
        if (tok.contains("well") || tok.contains("position") || tok.contains("id")) {
            wellCol = i;
        } else if (tok.contains("compound") || tok.contains("sample") || tok.contains("name") || tok.contains("content")) {
            compoundCol = i;
        } else if (tok.contains("barcode") || tok.contains("container")) {
            barcodeCol = i;
        } else if (tok == "volume" || tok.startsWith("vol")) {
            if (!tok.contains("unit")) volumeCol = i;
            else volumeUnitCol = i;
        } else if (tok.contains("volumeunit") || tok.contains("volunit")) {
            volumeUnitCol = i;
        }
    }

    if (wellCol >= 0 && compoundCol >= 0) {
        for (int lineIdx = 1; lineIdx < lines.size(); ++lineIdx) {
            QStringList tokens = splitCsvLine(lines.at(lineIdx), delim);
            if (tokens.size() > qMax(wellCol, compoundCol)) {
                QString wellStr = tokens.at(wellCol);
                QString compStr = tokens.at(compoundCol);
                if (barcodeCol >= 0 && barcodeCol < tokens.size() && result.detectedBarcode.isEmpty()) {
                    result.detectedBarcode = tokens.at(barcodeCol).trimmed();
                }
                if (volumeCol >= 0 && volumeCol < tokens.size() && result.detectedVolume == 0.0) {
                    bool ok = false;
                    double v = tokens.at(volumeCol).toDouble(&ok);
                    if (ok && v > 0.0) {
                        result.detectedVolume = v;
                    }
                }
                if (volumeUnitCol >= 0 && volumeUnitCol < tokens.size() && !tokens.at(volumeUnitCol).isEmpty()) {
                    result.detectedVolumeUnit = tokens.at(volumeUnitCol).trimmed();
                }

                int r = -1, c = -1;
                if (PlateModel::parseWellId(wellStr, r, c)) {
                    if (r >= 0 && r < expectedRows && c >= 0 && c < expectedCols) {
                        result.compoundMatrix[r][c] = compStr;
                        if (!compStr.isEmpty()) result.filledWells++;
                    }
                }
            }
        }
        result.success = true;
        result.detectedFormat = expectedFormat;
        return result;
    }

    // Fallback: Matrix assuming line 0 is row A if no header line
    for (int r = 0; r < qMin(lines.size(), expectedRows); ++r) {
        QStringList tokens = splitCsvLine(lines.at(r), delim);
        for (int c = 0; c < qMin(tokens.size(), expectedCols); ++c) {
            QString val = tokens.at(c);
            if (!val.isEmpty()) {
                result.compoundMatrix[r][c] = val;
                result.filledWells++;
            }
        }
    }

    result.success = true;
    result.detectedFormat = expectedFormat;
    return result;
}

bool CsvParser::saveMatrixCsv(const QString &filePath, const PlateModel &model, const QString &barcode) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    QString bc = barcode.trimmed();
    if (!bc.isEmpty()) {
        out << "\"Containerbarcode: " << bc << "\"\n";
    }

    // Header line: "", "01", "02", ...
    out << "\"Row/Col\"";
    for (int c = 0; c < model.colCount(); ++c) {
        out << ",\"" << PlateModel::getColLabel(c) << "\"";
    }
    out << "\n";

    for (int r = 0; r < model.rowCount(); ++r) {
        out << "\"" << PlateModel::getRowLabel(r) << "\"";
        for (int c = 0; c < model.colCount(); ++c) {
            out << ",\"" << model.well(r, c).compoundName << "\"";
        }
        out << "\n";
    }

    file.close();
    return true;
}

bool CsvParser::saveListCsv(const QString &filePath, const PlateModel &model, const QString &barcode) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    QString bc = barcode.trimmed();
    if (bc.isEmpty()) {
        bc = "PLATE_001";
    }

    out << "Containerbarcode,Samplealias,Containerposition,Volume,VolumeUnit,Concentration,ConcentrationUnit,UserdefValue1,UserdefValue2,UserdefValue3,UserdefValue4,UserdefValue5\n";

    for (int r = 0; r < model.rowCount(); ++r) {
        for (int c = 0; c < model.colCount(); ++c) {
            WellContent w = model.well(r, c);
            QString wellPos = PlateModel::getWellId(r, c);
            QString sampleAlias = w.compoundName;
            QString volStr = QString::number(w.volume, 'f', 4);
            QString unitStr = w.volumeUnit.isEmpty() ? "ul" : w.volumeUnit;

            out << bc << ","
                << sampleAlias << ","
                << wellPos << ","
                << volStr << "," << unitStr << ",0,uM,"
                << (w.quadrant == QuadrantId::None ? "" : QString("Q%1").arg(static_cast<int>(w.quadrant))) << ","
                << w.sourceWellId << ",0,NA,NA\n";
        }
    }

    file.close();
    return true;
}
