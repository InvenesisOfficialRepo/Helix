#ifndef LAYOUTMAKERWINDOW_H
#define LAYOUTMAKERWINDOW_H

#include <QMainWindow>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "QuadrantMerger.h"
#include "QuadrantDropWidget.h"
#include "PlateViewWidget.h"
#include "StyleHelper.h"

class LayoutMakerWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LayoutMakerWindow(QWidget *parent = nullptr);
    ~LayoutMakerWindow() = default;

private slots:
    void onModeChanged();
    void onThemeChanged();
    void onFileDropped(QuadrantId quad, const QString &filePath);
    void onOrientationChanged(QuadrantId quad, PlateOrientation orientation);
    void onVolumeChanged(QuadrantId quad, double volume);
    void onQuadrantClearRequested(QuadrantId quad);
    void onExportCsvClicked();
    void onGenerateSampleCsvsClicked();
    void onClearAllClicked();

private:
    void setupUi();
    void updateMergeResult();
    void applyWorkstationTheme();

    QuadrantMerger m_merger;

    // Workstation Theme Switchers
    QComboBox *m_brightnessCombo;
    QComboBox *m_accentCombo;
    QComboBox *m_densityCombo;

    // Assembly Mode Switcher
    QRadioButton *m_mode96To384Radio;
    QRadioButton *m_mode384To1536Radio;
    QButtonGroup *m_modeGroup;

    // Plate Barcode
    QLineEdit *m_barcodeEdit;

    QuadrantDropWidget *m_dropQ1;
    QuadrantDropWidget *m_dropQ2;
    QuadrantDropWidget *m_dropQ3;
    QuadrantDropWidget *m_dropQ4;

    PlateViewWidget *m_plateView;

    QPushButton *m_exportBtn;
    QPushButton *m_sampleGenBtn;
    QPushButton *m_clearAllBtn;

    QLabel *m_statusLabel;
};

#endif // LAYOUTMAKERWINDOW_H
