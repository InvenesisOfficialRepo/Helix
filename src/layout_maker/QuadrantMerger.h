#ifndef QUADRANTMERGER_H
#define QUADRANTMERGER_H

#include "PlateModel.h"
#include "CsvParser.h"
#include <QMap>
#include <QString>

enum class ConversionMode {
    Mode96To384,
    Mode384To1536
};

struct QuadrantInputData {
    QuadrantId quadrant = QuadrantId::None;
    QString filePath;
    bool loaded = false;
    int filledCount = 0;
    PlateOrientation orientation = PlateOrientation::Normal_0;
    double transferVolume = 20.0;
    QString volumeUnit = "ul";
    QVector<QVector<QString>> compoundGrid;
};

class QuadrantMerger {
public:
    QuadrantMerger();

    void setConversionMode(ConversionMode mode);
    ConversionMode conversionMode() const { return m_mode; }

    PlateFormat sourceFormat() const;
    PlateFormat targetFormat() const;

    void setQuadrantInput(QuadrantId quad, const QString &filePath, const CsvParseResult &parseResult, PlateOrientation orientation = PlateOrientation::Normal_0, double volume = 20.0, const QString &unit = "ul");
    void setQuadrantOrientation(QuadrantId quad, PlateOrientation orientation);
    void setQuadrantVolume(QuadrantId quad, double volume, const QString &unit = "ul");
    void clearQuadrantInput(QuadrantId quad);
    void clearAll();

    bool hasInput(QuadrantId quad) const;
    QuadrantInputData quadrantInput(QuadrantId quad) const;

    PlateModel merge() const;

    static void calculateTargetPosition(QuadrantId q, int sourceRow, int sourceCol, int &targetRow, int &targetCol);

private:
    ConversionMode m_mode = ConversionMode::Mode96To384;
    QMap<QuadrantId, QuadrantInputData> m_inputs;
};

#endif // QUADRANTMERGER_H
