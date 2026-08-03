#include "LayoutMakerWindow.h"
#include "CsvParser.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QApplication>

LayoutMakerWindow::LayoutMakerWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("LayoutMaker - Workstation UI (Qt 6 C++)");
    resize(1280, 900);
    setMinimumSize(1000, 720);

    setupUi();
    applyWorkstationTheme();
}

void LayoutMakerWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(6);

    // Header Area: Title & Theme Switchers & Mode & Tools
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    QLabel *appTitle = new QLabel("LAYOUTMAKER", this);
    appTitle->setStyleSheet("font-size: 15px; font-weight: 900; letter-spacing: 1px;");

    // Workstation Theme Controls: Brightness / Accent / Density
    QGroupBox *themeBox = new QGroupBox("Workstation Theme", this);
    QHBoxLayout *themeLayout = new QHBoxLayout(themeBox);
    themeLayout->setContentsMargins(8, 2, 8, 2);
    themeLayout->setSpacing(6);

    m_brightnessCombo = new QComboBox(this);
    m_brightnessCombo->addItem("Darkest", static_cast<int>(BrightnessLevel::Darkest));
    m_brightnessCombo->addItem("Dark", static_cast<int>(BrightnessLevel::Dark));
    m_brightnessCombo->addItem("Medium", static_cast<int>(BrightnessLevel::Medium));
    m_brightnessCombo->addItem("Light", static_cast<int>(BrightnessLevel::Light));
    m_brightnessCombo->setCurrentIndex(1); // Dark default
    connect(m_brightnessCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LayoutMakerWindow::onThemeChanged);

    m_accentCombo = new QComboBox(this);
    m_accentCombo->addItem("Blue", static_cast<int>(AccentColor::Blue));
    m_accentCombo->addItem("Orange", static_cast<int>(AccentColor::Orange));
    m_accentCombo->addItem("Teal", static_cast<int>(AccentColor::Teal));
    m_accentCombo->addItem("Violet", static_cast<int>(AccentColor::Violet));
    connect(m_accentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LayoutMakerWindow::onThemeChanged);

    m_densityCombo = new QComboBox(this);
    m_densityCombo->addItem("Compact", static_cast<int>(DensityLevel::Compact));
    m_densityCombo->addItem("Comfortable", static_cast<int>(DensityLevel::Comfortable));
    connect(m_densityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LayoutMakerWindow::onThemeChanged);

    themeLayout->addWidget(new QLabel("Brightness:", this));
    themeLayout->addWidget(m_brightnessCombo);
    themeLayout->addWidget(new QLabel("Accent:", this));
    themeLayout->addWidget(m_accentCombo);
    themeLayout->addWidget(new QLabel("Density:", this));
    themeLayout->addWidget(m_densityCombo);

    // Assembly Mode Switcher
    QGroupBox *modeBox = new QGroupBox("Assembly Mode", this);
    QHBoxLayout *modeLayout = new QHBoxLayout(modeBox);
    modeLayout->setContentsMargins(8, 2, 8, 2);
    modeLayout->setSpacing(8);

    m_mode96To384Radio = new QRadioButton("4 × 96 ➔ 384", this);
    m_mode384To1536Radio = new QRadioButton("4 × 384 ➔ 1536", this);
    m_mode96To384Radio->setChecked(true);

    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->addButton(m_mode96To384Radio, static_cast<int>(ConversionMode::Mode96To384));
    m_modeGroup->addButton(m_mode384To1536Radio, static_cast<int>(ConversionMode::Mode384To1536));
    connect(m_modeGroup, &QButtonGroup::idClicked, this, &LayoutMakerWindow::onModeChanged);

    modeLayout->addWidget(m_mode96To384Radio);
    modeLayout->addWidget(m_mode384To1536Radio);

    // Plate Barcode Input
    QGroupBox *barcodeBox = new QGroupBox("Plate Barcode", this);
    QHBoxLayout *barcodeLayout = new QHBoxLayout(barcodeBox);
    barcodeLayout->setContentsMargins(8, 2, 8, 2);

    m_barcodeEdit = new QLineEdit(this);
    m_barcodeEdit->setPlaceholderText("e.g. PHC0009000");
    m_barcodeEdit->setFixedWidth(130);
    barcodeLayout->addWidget(m_barcodeEdit);

    m_exportBtn = new QPushButton("Export CSV...", this);
    m_exportBtn->setObjectName("primaryBtn");
    m_exportBtn->setCursor(Qt::PointingHandCursor);
    connect(m_exportBtn, &QPushButton::clicked, this, &LayoutMakerWindow::onExportCsvClicked);

    m_sampleGenBtn = new QPushButton("Load Sample CSVs", this);
    m_sampleGenBtn->setCursor(Qt::PointingHandCursor);
    connect(m_sampleGenBtn, &QPushButton::clicked, this, &LayoutMakerWindow::onGenerateSampleCsvsClicked);

    m_clearAllBtn = new QPushButton("Clear All", this);
    m_clearAllBtn->setCursor(Qt::PointingHandCursor);
    connect(m_clearAllBtn, &QPushButton::clicked, this, &LayoutMakerWindow::onClearAllClicked);

    headerLayout->addWidget(appTitle);
    headerLayout->addWidget(themeBox);
    headerLayout->addWidget(modeBox);
    headerLayout->addWidget(barcodeBox);
    headerLayout->addStretch();
    headerLayout->addWidget(m_sampleGenBtn);
    headerLayout->addWidget(m_exportBtn);
    headerLayout->addWidget(m_clearAllBtn);

    // Drop Zones Section (Compact 2x2 grid)
    QGridLayout *dropGrid = new QGridLayout();
    dropGrid->setSpacing(6);
    dropGrid->setContentsMargins(0, 0, 0, 0);

    m_dropQ1 = new QuadrantDropWidget(QuadrantId::Q1, this);
    m_dropQ2 = new QuadrantDropWidget(QuadrantId::Q2, this);
    m_dropQ3 = new QuadrantDropWidget(QuadrantId::Q3, this);
    m_dropQ4 = new QuadrantDropWidget(QuadrantId::Q4, this);

    connect(m_dropQ1, &QuadrantDropWidget::fileDropped, this, &LayoutMakerWindow::onFileDropped);
    connect(m_dropQ2, &QuadrantDropWidget::fileDropped, this, &LayoutMakerWindow::onFileDropped);
    connect(m_dropQ3, &QuadrantDropWidget::fileDropped, this, &LayoutMakerWindow::onFileDropped);
    connect(m_dropQ4, &QuadrantDropWidget::fileDropped, this, &LayoutMakerWindow::onFileDropped);

    connect(m_dropQ1, &QuadrantDropWidget::orientationChanged, this, &LayoutMakerWindow::onOrientationChanged);
    connect(m_dropQ2, &QuadrantDropWidget::orientationChanged, this, &LayoutMakerWindow::onOrientationChanged);
    connect(m_dropQ3, &QuadrantDropWidget::orientationChanged, this, &LayoutMakerWindow::onOrientationChanged);
    connect(m_dropQ4, &QuadrantDropWidget::orientationChanged, this, &LayoutMakerWindow::onOrientationChanged);

    connect(m_dropQ1, &QuadrantDropWidget::volumeChanged, this, &LayoutMakerWindow::onVolumeChanged);
    connect(m_dropQ2, &QuadrantDropWidget::volumeChanged, this, &LayoutMakerWindow::onVolumeChanged);
    connect(m_dropQ3, &QuadrantDropWidget::volumeChanged, this, &LayoutMakerWindow::onVolumeChanged);
    connect(m_dropQ4, &QuadrantDropWidget::volumeChanged, this, &LayoutMakerWindow::onVolumeChanged);

    connect(m_dropQ1, &QuadrantDropWidget::clearRequested, this, &LayoutMakerWindow::onQuadrantClearRequested);
    connect(m_dropQ2, &QuadrantDropWidget::clearRequested, this, &LayoutMakerWindow::onQuadrantClearRequested);
    connect(m_dropQ3, &QuadrantDropWidget::clearRequested, this, &LayoutMakerWindow::onQuadrantClearRequested);
    connect(m_dropQ4, &QuadrantDropWidget::clearRequested, this, &LayoutMakerWindow::onQuadrantClearRequested);

    dropGrid->addWidget(m_dropQ1, 0, 0);
    dropGrid->addWidget(m_dropQ2, 0, 1);
    dropGrid->addWidget(m_dropQ3, 1, 0);
    dropGrid->addWidget(m_dropQ4, 1, 1);

    // Result Visualization Area
    m_plateView = new PlateViewWidget(this);

    // Footer Status Bar
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->setContentsMargins(0, 0, 0, 0);

    m_statusLabel = new QLabel("Ready. Select or drag CSV layout files into the quadrant drop zones.", this);
    m_statusLabel->setStyleSheet("font-weight: 500; font-size: 11px;");

    actionLayout->addWidget(m_statusLabel);
    actionLayout->addStretch();

    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(dropGrid);
    mainLayout->addWidget(m_plateView, 1); // stretch factor to dominate vertical space
    mainLayout->addLayout(actionLayout);

    onModeChanged();
}

void LayoutMakerWindow::onThemeChanged() {
    applyWorkstationTheme();
}

void LayoutMakerWindow::applyWorkstationTheme() {
    BrightnessLevel b = static_cast<BrightnessLevel>(m_brightnessCombo->currentData().toInt());
    AccentColor a = static_cast<AccentColor>(m_accentCombo->currentData().toInt());
    DensityLevel d = static_cast<DensityLevel>(m_densityCombo->currentData().toInt());

    QString stylesheet = StyleHelper::getAppStylesheet(b, a, d);
    qApp->setStyleSheet(stylesheet);

    WorkstationTokens tokens = StyleHelper::getTokens(b, a, d);
    m_plateView->setTokens(tokens);
}

void LayoutMakerWindow::onModeChanged() {
    ConversionMode mode = static_cast<ConversionMode>(m_modeGroup->checkedId());
    m_merger.setConversionMode(mode);

    PlateFormat srcFmt = m_merger.sourceFormat();
    PlateFormat targetFmt = m_merger.targetFormat();

    m_dropQ1->setSourceFormat(srcFmt);
    m_dropQ2->setSourceFormat(srcFmt);
    m_dropQ3->setSourceFormat(srcFmt);
    m_dropQ4->setSourceFormat(srcFmt);

    m_dropQ1->clear();
    m_dropQ2->clear();
    m_dropQ3->clear();
    m_dropQ4->clear();

    m_plateView->setFormat(targetFmt);

    QString modeText = (mode == ConversionMode::Mode96To384) ? "96 ➔ 384 Wells" : "384 ➔ 1536 Wells";
    m_statusLabel->setText(QString("Target format: %1. Drop 4 CSV layouts.").arg(modeText));
}

void LayoutMakerWindow::onFileDropped(QuadrantId quad, const QString &filePath) {
    PlateFormat srcFmt = m_merger.sourceFormat();
    CsvParseResult result = CsvParser::parseFile(filePath, srcFmt);

    if (!result.success) {
        QMessageBox::warning(this, "CSV Parse Error",
            QString("Failed to read CSV for %1:\n%2").arg(PlateModel::getQuadrantName(quad)).arg(result.errorMessage));
        return;
    }

    QuadrantDropWidget *w = nullptr;
    switch (quad) {
    case QuadrantId::Q1: w = m_dropQ1; break;
    case QuadrantId::Q2: w = m_dropQ2; break;
    case QuadrantId::Q3: w = m_dropQ3; break;
    case QuadrantId::Q4: w = m_dropQ4; break;
    default: break;
    }

    double vol = (w ? w->volume() : 20.0);
    if (result.detectedVolume > 0.0) {
        vol = result.detectedVolume;
        if (w) w->setVolume(vol);
    }

    PlateOrientation ori = w ? w->orientation() : PlateOrientation::Normal_0;
    QString unit = result.detectedVolumeUnit.isEmpty() ? "ul" : result.detectedVolumeUnit;
    m_merger.setQuadrantInput(quad, filePath, result, ori, vol, unit);

    if (w) {
        w->setLoadedFile(filePath, result.filledWells, srcFmt);
    }

    if (m_barcodeEdit->text().trimmed().isEmpty() && !result.detectedBarcode.isEmpty()) {
        m_barcodeEdit->setText(result.detectedBarcode + "_Merged");
    }

    updateMergeResult();
}

void LayoutMakerWindow::onOrientationChanged(QuadrantId quad, PlateOrientation orientation) {
    m_merger.setQuadrantOrientation(quad, orientation);
    updateMergeResult();
}

void LayoutMakerWindow::onVolumeChanged(QuadrantId quad, double volume) {
    m_merger.setQuadrantVolume(quad, volume);
    updateMergeResult();
}

void LayoutMakerWindow::onQuadrantClearRequested(QuadrantId quad) {
    m_merger.clearQuadrantInput(quad);
    updateMergeResult();
}

void LayoutMakerWindow::updateMergeResult() {
    PlateModel resultPlate = m_merger.merge();
    m_plateView->setPlateModel(resultPlate);

    int filled = resultPlate.filledWellCount();
    int total = resultPlate.totalWellCount();
    m_statusLabel->setText(QString("Merged plate ready: %1 / %2 wells populated.").arg(filled).arg(total));
}

void LayoutMakerWindow::onExportCsvClicked() {
    PlateModel currentPlate = m_merger.merge();
    if (currentPlate.filledWellCount() == 0) {
        QMessageBox::information(this, "Export", "The current plate has no loaded compounds to export.");
        return;
    }

    QString barcode = m_barcodeEdit->text().trimmed();
    if (barcode.isEmpty()) {
        bool ok = false;
        QString defaultBc = (m_merger.conversionMode() == ConversionMode::Mode96To384) ? "PHC0009000" : "PHC0009500";
        barcode = QInputDialog::getText(this, "Target Plate Barcode Required",
            "Enter Barcode for the Output Plate Layout:",
            QLineEdit::Normal, defaultBc, &ok).trimmed();

        if (!ok || barcode.isEmpty()) {
            return;
        }
        m_barcodeEdit->setText(barcode);
    }

    QString defaultName = barcode + ".csv";
    QString filterList = "Fluent / HTS List CSV (*.csv);;Matrix Grid CSV (*.csv)";
    QString selectedFilter;
    QString filePath = QFileDialog::getSaveFileName(this, "Export Merged Plate Layout CSV", defaultName, filterList, &selectedFilter);

    if (filePath.isEmpty()) return;

    bool ok = false;
    if (selectedFilter.contains("Matrix")) {
        ok = CsvParser::saveMatrixCsv(filePath, currentPlate, barcode);
    } else {
        ok = CsvParser::saveListCsv(filePath, currentPlate, barcode);
    }

    if (ok) {
        QMessageBox::information(this, "Export Success", QString("Plate layout exported successfully!\n\nFile: %1\nPlate Barcode: %2").arg(filePath).arg(barcode));
    } else {
        QMessageBox::critical(this, "Export Failed", "Failed to write CSV file.");
    }
}

void LayoutMakerWindow::onGenerateSampleCsvsClicked() {
    QString dirPath = QDir::currentPath() + "/sample_layouts";
    QDir().mkpath(dirPath);

    PlateFormat srcFmt = m_merger.sourceFormat();
    int rows = PlateModel::getRowCount(srcFmt);
    int cols = PlateModel::getColCount(srcFmt);

    QList<QuadrantId> quads = { QuadrantId::Q1, QuadrantId::Q2, QuadrantId::Q3, QuadrantId::Q4 };
    QString prefix = (srcFmt == PlateFormat::Plate96) ? "96" : "384";

    for (QuadrantId q : quads) {
        QString filename = QString("%1/Sample_Q%2_%3well.csv").arg(dirPath).arg(static_cast<int>(q)).arg(prefix);
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "\"Row/Col\"";
            for (int c = 0; c < cols; ++c) {
                out << ",\"" << PlateModel::getColLabel(c) << "\"";
            }
            out << "\n";

            for (int r = 0; r < rows; ++r) {
                out << "\"" << PlateModel::getRowLabel(r) << "\"";
                for (int c = 0; c < cols; ++c) {
                    QString wellStr = PlateModel::getWellId(r, c);
                    QString compName = QString("Q%1_CMP_%2").arg(static_cast<int>(q)).arg(wellStr);
                    out << ",\"" << compName << "\"";
                }
                out << "\n";
            }
            file.close();
        }

        onFileDropped(q, filename);
    }

    m_statusLabel->setText("Generated and loaded 4 sample quadrant CSV layouts!");
}

void LayoutMakerWindow::onClearAllClicked() {
    m_merger.clearAll();
    m_dropQ1->clear();
    m_dropQ2->clear();
    m_dropQ3->clear();
    m_dropQ4->clear();
    m_plateView->setFormat(m_merger.targetFormat());
    m_statusLabel->setText("All inputs cleared.");
}
