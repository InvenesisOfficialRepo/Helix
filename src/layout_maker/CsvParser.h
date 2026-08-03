#ifndef CSVPARSER_H
#define CSVPARSER_H

#include "PlateModel.h"
#include <QString>
#include <QVector>

struct CsvParseResult {
    bool success = false;
    QString errorMessage;
    PlateFormat detectedFormat = PlateFormat::Plate96;
    QVector<QVector<QString>> compoundMatrix; // [row][col] -> compound name
    int filledWells = 0;
    QString detectedBarcode;
    double detectedVolume = 0.0;
    QString detectedVolumeUnit = "ul";
};

class CsvParser {
public:
    static CsvParseResult parseFile(const QString &filePath, PlateFormat expectedFormat);
    static bool saveMatrixCsv(const QString &filePath, const PlateModel &model, const QString &barcode = QString());
    static bool saveListCsv(const QString &filePath, const PlateModel &model, const QString &barcode = QString());

private:
    static char detectDelimiter(const QString &firstLine);
    static QStringList splitCsvLine(const QString &line, char delimiter);
};

#endif // CSVPARSER_H
