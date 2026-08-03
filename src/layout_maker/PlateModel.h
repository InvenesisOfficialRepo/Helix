#ifndef PLATEMODEL_H
#define PLATEMODEL_H

#include <QString>
#include <QVector>
#include <QColor>

enum class PlateFormat {
    Plate96,
    Plate384,
    Plate1536
};

enum class QuadrantId {
    None = 0,
    Q1 = 1,
    Q2 = 2,
    Q3 = 3,
    Q4 = 4
};

enum class PlateOrientation {
    Normal_0 = 0,
    Rotate_180,
    Rotate_90_CW,
    Rotate_90_CCW,
    Flip_Horizontal,
    Flip_Vertical
};

struct WellContent {
    QString compoundName;
    QuadrantId quadrant = QuadrantId::None;
    QString sourceWellId;
    double volume = 0.0;
    QString volumeUnit = "ul";

    bool isEmpty() const {
        return compoundName.trimmed().isEmpty();
    }
};

class PlateModel {
public:
    PlateModel(PlateFormat format = PlateFormat::Plate384);

    void resize(PlateFormat format);
    PlateFormat format() const { return m_format; }

    int rowCount() const;
    int colCount() const;
    int totalWellCount() const;
    int filledWellCount() const;

    WellContent well(int r, int c) const;
    void setWell(int r, int c, const WellContent &content);
    void clear();

    static int getRowCount(PlateFormat fmt);
    static int getColCount(PlateFormat fmt);
    static QString getRowLabel(int rowIndex);
    static int getRowIndexFromLabel(const QString &label);
    static QString getColLabel(int colIndex);
    static QString getWellId(int r, int c);
    static bool parseWellId(const QString &wellId, int &outRow, int &outCol);
    static QColor getQuadrantColor(QuadrantId q);
    static QString getQuadrantName(QuadrantId q);
    static QString getQuadrantStartWell(QuadrantId q);
    static QString getOrientationName(PlateOrientation ori);

    static QVector<QVector<QString>> transformMatrix(const QVector<QVector<QString>> &input, PlateOrientation orientation, int targetRows, int targetCols);

private:
    PlateFormat m_format;
    int m_rows;
    int m_cols;
    QVector<QVector<WellContent>> m_grid;
};

#endif // PLATEMODEL_H
