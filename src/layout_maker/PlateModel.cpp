#include "PlateModel.h"
#include <QRegularExpression>

PlateModel::PlateModel(PlateFormat format) {
    resize(format);
}

void PlateModel::resize(PlateFormat format) {
    m_format = format;
    m_rows = getRowCount(format);
    m_cols = getColCount(format);
    m_grid.resize(m_rows);
    for (int r = 0; r < m_rows; ++r) {
        m_grid[r].clear();
        m_grid[r].resize(m_cols);
    }
}

int PlateModel::rowCount() const { return m_rows; }
int PlateModel::colCount() const { return m_cols; }
int PlateModel::totalWellCount() const { return m_rows * m_cols; }

int PlateModel::filledWellCount() const {
    int count = 0;
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            if (!m_grid[r][c].isEmpty()) {
                count++;
            }
        }
    }
    return count;
}

WellContent PlateModel::well(int r, int c) const {
    if (r >= 0 && r < m_rows && c >= 0 && c < m_cols) {
        return m_grid[r][c];
    }
    return WellContent();
}

void PlateModel::setWell(int r, int c, const WellContent &content) {
    if (r >= 0 && r < m_rows && c >= 0 && c < m_cols) {
        m_grid[r][c] = content;
    }
}

void PlateModel::clear() {
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            m_grid[r][c] = WellContent();
        }
    }
}

int PlateModel::getRowCount(PlateFormat fmt) {
    switch (fmt) {
    case PlateFormat::Plate96:   return 8;
    case PlateFormat::Plate384:  return 16;
    case PlateFormat::Plate1536: return 32;
    }
    return 8;
}

int PlateModel::getColCount(PlateFormat fmt) {
    switch (fmt) {
    case PlateFormat::Plate96:   return 12;
    case PlateFormat::Plate384:  return 24;
    case PlateFormat::Plate1536: return 48;
    }
    return 12;
}

QString PlateModel::getRowLabel(int rowIndex) {
    if (rowIndex < 0) return QString();
    if (rowIndex < 26) {
        return QString(QChar('A' + rowIndex));
    }
    int first = (rowIndex / 26) - 1;
    int second = rowIndex % 26;
    return QString(QChar('A' + first)) + QString(QChar('A' + second));
}

int PlateModel::getRowIndexFromLabel(const QString &label) {
    QString clean = label.trimmed().toUpper();
    if (clean.isEmpty()) return -1;

    if (clean.length() == 1) {
        char ch = clean.at(0).toLatin1();
        if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    } else if (clean.length() == 2) {
        char ch1 = clean.at(0).toLatin1();
        char ch2 = clean.at(1).toLatin1();
        if (ch1 >= 'A' && ch1 <= 'Z' && ch2 >= 'A' && ch2 <= 'Z') {
            int first = ch1 - 'A' + 1;
            int second = ch2 - 'A';
            return first * 26 + second;
        }
    }
    return -1;
}

QString PlateModel::getColLabel(int colIndex) {
    return QString("%1").arg(colIndex + 1, 2, 10, QChar('0'));
}

QString PlateModel::getWellId(int r, int c) {
    return getRowLabel(r) + getColLabel(c);
}

bool PlateModel::parseWellId(const QString &wellId, int &outRow, int &outCol) {
    static const QRegularExpression regex("^([A-Za-z]{1,2})0*([1-9][0-9]*)$");
    QRegularExpressionMatch match = regex.match(wellId.trimmed());
    if (!match.hasMatch()) {
        return false;
    }
    QString rowStr = match.captured(1);
    int colNum = match.captured(2).toInt();

    int r = getRowIndexFromLabel(rowStr);
    int c = colNum - 1;
    if (r < 0 || c < 0) return false;

    outRow = r;
    outCol = c;
    return true;
}

QColor PlateModel::getQuadrantColor(QuadrantId q) {
    switch (q) {
    case QuadrantId::Q1: return QColor("#3B7A57"); // Muted Sage / Emerald
    case QuadrantId::Q2: return QColor("#4A6984"); // Muted Slate Blue
    case QuadrantId::Q3: return QColor("#9A6B38"); // Muted Warm Clay / Amber
    case QuadrantId::Q4: return QColor("#6B4C8A"); // Muted Plum / Violet
    case QuadrantId::None: default: return QColor("#27272A"); // Dark neutral
    }
}

QString PlateModel::getQuadrantName(QuadrantId q) {
    switch (q) {
    case QuadrantId::Q1: return "Q1 (A01)";
    case QuadrantId::Q2: return "Q2 (A02)";
    case QuadrantId::Q3: return "Q3 (B01)";
    case QuadrantId::Q4: return "Q4 (B02)";
    case QuadrantId::None: default: return "None";
    }
}

QString PlateModel::getQuadrantStartWell(QuadrantId q) {
    switch (q) {
    case QuadrantId::Q1: return "A01";
    case QuadrantId::Q2: return "A02";
    case QuadrantId::Q3: return "B01";
    case QuadrantId::Q4: return "B02";
    case QuadrantId::None: default: return "-";
    }
}

QString PlateModel::getOrientationName(PlateOrientation ori) {
    switch (ori) {
    case PlateOrientation::Normal_0:        return "Normal (0°)";
    case PlateOrientation::Rotate_180:      return "Rotate 180°";
    case PlateOrientation::Rotate_90_CW:    return "Rotate 90° CW";
    case PlateOrientation::Rotate_90_CCW:   return "Rotate 90° CCW";
    case PlateOrientation::Flip_Horizontal: return "Flip Horizontally";
    case PlateOrientation::Flip_Vertical:   return "Flip Vertically";
    }
    return "Normal (0°)";
}

QVector<QVector<QString>> PlateModel::transformMatrix(const QVector<QVector<QString>> &input, PlateOrientation orientation, int targetRows, int targetCols) {
    QVector<QVector<QString>> output(targetRows, QVector<QString>(targetCols));
    if (input.isEmpty()) return output;

    int inRows = input.size();
    int inCols = input[0].size();

    for (int r = 0; r < inRows; ++r) {
        for (int c = 0; c < inCols; ++c) {
            QString val = input[r][c];
            if (val.isEmpty()) continue;

            int tr = r;
            int tc = c;

            switch (orientation) {
            case PlateOrientation::Normal_0:
                tr = r;
                tc = c;
                break;
            case PlateOrientation::Rotate_180:
                tr = inRows - 1 - r;
                tc = inCols - 1 - c;
                break;
            case PlateOrientation::Flip_Horizontal:
                tr = r;
                tc = inCols - 1 - c;
                break;
            case PlateOrientation::Flip_Vertical:
                tr = inRows - 1 - r;
                tc = c;
                break;
            case PlateOrientation::Rotate_90_CW:
                // Map row r, col c to rotated positions
                tr = (c * targetRows) / inCols;
                tc = (inRows - 1 - r) * targetCols / inRows;
                break;
            case PlateOrientation::Rotate_90_CCW:
                tr = (inCols - 1 - c) * targetRows / inCols;
                tc = (r * targetCols) / inRows;
                break;
            }

            if (tr >= 0 && tr < targetRows && tc >= 0 && tc < targetCols) {
                output[tr][tc] = val;
            }
        }
    }

    return output;
}
