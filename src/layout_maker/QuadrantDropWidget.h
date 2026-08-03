#ifndef QUADRANTDROPWIDGET_H
#define QUADRANTDROPWIDGET_H

#include "PlateModel.h"
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

class QuadrantDropWidget : public QFrame {
    Q_OBJECT
public:
    explicit QuadrantDropWidget(QuadrantId quadrant, QWidget *parent = nullptr);

    QuadrantId quadrant() const { return m_quadrant; }
    QString filePath() const { return m_filePath; }
    bool isLoaded() const { return m_isLoaded; }
    PlateOrientation orientation() const { return m_orientation; }
    double volume() const;

    void setLoadedFile(const QString &path, int filledWells, PlateFormat sourceFmt);
    void clear();
    void setSourceFormat(PlateFormat fmt);
    void setOrientation(PlateOrientation ori);
    void setVolume(double vol);

signals:
    void fileDropped(QuadrantId quad, const QString &filePath);
    void orientationChanged(QuadrantId quad, PlateOrientation orientation);
    void volumeChanged(QuadrantId quad, double volume);
    void clearRequested(QuadrantId quad);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onBrowseClicked();
    void onClearClicked();
    void onOrientationComboChanged(int index);
    void onVolumeSpinChanged(double val);

private:
    void updateStyle();

    QuadrantId m_quadrant;
    PlateFormat m_sourceFormat = PlateFormat::Plate96;
    PlateOrientation m_orientation = PlateOrientation::Normal_0;
    QString m_filePath;
    bool m_isLoaded = false;
    bool m_isDragHover = false;

    QLabel *m_badgeLabel;
    QLabel *m_startWellLabel;
    QLabel *m_statusIconLabel;
    QLabel *m_filenameLabel;
    QLabel *m_infoLabel;
    QComboBox *m_orientationCombo;
    QDoubleSpinBox *m_volumeSpin;
    QPushButton *m_browseBtn;
    QPushButton *m_clearBtn;
};

#endif // QUADRANTDROPWIDGET_H
