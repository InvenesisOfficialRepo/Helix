#include "QuadrantMerger.h"

QuadrantMerger::QuadrantMerger() {
    clearAll();
}

void QuadrantMerger::setConversionMode(ConversionMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        clearAll();
    }
}

PlateFormat QuadrantMerger::sourceFormat() const {
    return (m_mode == ConversionMode::Mode96To384) ? PlateFormat::Plate96 : PlateFormat::Plate384;
}

PlateFormat QuadrantMerger::targetFormat() const {
    return (m_mode == ConversionMode::Mode96To384) ? PlateFormat::Plate384 : PlateFormat::Plate1536;
}

void QuadrantMerger::setQuadrantInput(QuadrantId quad, const QString &filePath, const CsvParseResult &parseResult, PlateOrientation orientation, double volume, const QString &unit) {
    QuadrantInputData data = m_inputs.value(quad);
    data.quadrant = quad;
    data.filePath = filePath;
    data.loaded = parseResult.success;
    data.filledCount = parseResult.filledWells;
    data.orientation = orientation;
    data.transferVolume = volume;
    data.volumeUnit = unit;
    data.compoundGrid = parseResult.compoundMatrix;
    m_inputs[quad] = data;
}

void QuadrantMerger::setQuadrantOrientation(QuadrantId quad, PlateOrientation orientation) {
    if (m_inputs.contains(quad)) {
        m_inputs[quad].orientation = orientation;
    }
}

void QuadrantMerger::setQuadrantVolume(QuadrantId quad, double volume, const QString &unit) {
    if (m_inputs.contains(quad)) {
        m_inputs[quad].transferVolume = volume;
        m_inputs[quad].volumeUnit = unit;
    }
}

void QuadrantMerger::clearQuadrantInput(QuadrantId quad) {
    QuadrantInputData data;
    data.quadrant = quad;
    m_inputs[quad] = data;
}

void QuadrantMerger::clearAll() {
    m_inputs.clear();
    m_inputs[QuadrantId::Q1] = QuadrantInputData{QuadrantId::Q1, "", false, 0, PlateOrientation::Normal_0, 20.0, "ul", {}};
    m_inputs[QuadrantId::Q2] = QuadrantInputData{QuadrantId::Q2, "", false, 0, PlateOrientation::Normal_0, 20.0, "ul", {}};
    m_inputs[QuadrantId::Q3] = QuadrantInputData{QuadrantId::Q3, "", false, 0, PlateOrientation::Normal_0, 20.0, "ul", {}};
    m_inputs[QuadrantId::Q4] = QuadrantInputData{QuadrantId::Q4, "", false, 0, PlateOrientation::Normal_0, 20.0, "ul", {}};
}

bool QuadrantMerger::hasInput(QuadrantId quad) const {
    return m_inputs.value(quad).loaded;
}

QuadrantInputData QuadrantMerger::quadrantInput(QuadrantId quad) const {
    return m_inputs.value(quad);
}

void QuadrantMerger::calculateTargetPosition(QuadrantId q, int sourceRow, int sourceCol, int &targetRow, int &targetCol) {
    int rowOffset = (q == QuadrantId::Q3 || q == QuadrantId::Q4) ? 1 : 0;
    int colOffset = (q == QuadrantId::Q2 || q == QuadrantId::Q4) ? 1 : 0;

    targetRow = 2 * sourceRow + rowOffset;
    targetCol = 2 * sourceCol + colOffset;
}

PlateModel QuadrantMerger::merge() const {
    PlateModel targetPlate(targetFormat());
    PlateFormat srcFmt = sourceFormat();

    int srcRows = PlateModel::getRowCount(srcFmt);
    int srcCols = PlateModel::getColCount(srcFmt);

    QList<QuadrantId> quads = { QuadrantId::Q1, QuadrantId::Q2, QuadrantId::Q3, QuadrantId::Q4 };

    for (QuadrantId q : quads) {
        QuadrantInputData input = m_inputs.value(q);
        if (!input.loaded || input.compoundGrid.isEmpty()) continue;

        // Apply deck orientation transformation to input matrix
        QVector<QVector<QString>> transformedGrid = PlateModel::transformMatrix(input.compoundGrid, input.orientation, srcRows, srcCols);

        for (int r = 0; r < qMin(transformedGrid.size(), srcRows); ++r) {
            for (int c = 0; c < qMin(transformedGrid[r].size(), srcCols); ++c) {
                QString val = transformedGrid[r][c].trimmed();
                if (!val.isEmpty()) {
                    int tr = 0, tc = 0;
                    calculateTargetPosition(q, r, c, tr, tc);

                    WellContent content;
                    content.compoundName = val;
                    content.quadrant = q;
                    content.sourceWellId = PlateModel::getWellId(r, c);
                    content.volume = input.transferVolume;
                    content.volumeUnit = input.volumeUnit;

                    targetPlate.setWell(tr, tc, content);
                }
            }
        }
    }

    return targetPlate;
}
