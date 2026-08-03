#include "QuadrantDropWidget.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QFileInfo>

QuadrantDropWidget::QuadrantDropWidget(QuadrantId quadrant, QWidget *parent)
    : QFrame(parent), m_quadrant(quadrant) {
    setAcceptDrops(true);
    setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    setFixedHeight(68);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 6, 10, 6);
    mainLayout->setSpacing(10);

    // Left Column: Badge & Start Well
    QVBoxLayout *leftBox = new QVBoxLayout();
    leftBox->setSpacing(2);
    leftBox->setContentsMargins(0, 0, 0, 0);

    m_badgeLabel = new QLabel(PlateModel::getQuadrantName(m_quadrant).toUpper(), this);
    QColor qColor = PlateModel::getQuadrantColor(m_quadrant);
    m_badgeLabel->setStyleSheet(QString(
        "background-color: %1; color: #FFFFFF; font-weight: bold; font-size: 10px; "
        "padding: 2px 6px; border-radius: 2px;"
    ).arg(qColor.name()));

    m_startWellLabel = new QLabel(QString("Starts at %1").arg(PlateModel::getQuadrantStartWell(m_quadrant)), this);
    m_startWellLabel->setStyleSheet("color: #71717A; font-weight: bold; font-size: 10px;");

    leftBox->addWidget(m_badgeLabel);
    leftBox->addWidget(m_startWellLabel);

    // Middle Column: File status & Drop Text
    m_statusIconLabel = new QLabel("•", this);
    m_statusIconLabel->setStyleSheet("font-size: 14px; color: #71717A; font-weight: bold;");

    QVBoxLayout *centerBox = new QVBoxLayout();
    centerBox->setSpacing(1);
    centerBox->setContentsMargins(0, 0, 0, 0);

    m_filenameLabel = new QLabel("Drop CSV here or browse", this);
    m_filenameLabel->setStyleSheet("font-weight: 600; font-size: 11px; color: #D4D4D8;");

    m_infoLabel = new QLabel("No file loaded", this);
    m_infoLabel->setStyleSheet("color: #71717A; font-size: 10px;");

    centerBox->addWidget(m_filenameLabel);
    centerBox->addWidget(m_infoLabel);

    // Right Column: Transfer Volume, Fluent Deck Orientation Fix Combo & Browse / Clear buttons
    m_volumeSpin = new QDoubleSpinBox(this);
    m_volumeSpin->setRange(0.0, 5000.0);
    m_volumeSpin->setDecimals(1);
    m_volumeSpin->setSingleStep(1.0);
    m_volumeSpin->setValue(20.0);
    m_volumeSpin->setSuffix(" ul");
    m_volumeSpin->setToolTip("Transfer Volume per Well for this Quadrant");
    m_volumeSpin->setFixedWidth(70);
    connect(m_volumeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QuadrantDropWidget::onVolumeSpinChanged);

    m_orientationCombo = new QComboBox(this);
    m_orientationCombo->addItem("0° (Normal)", static_cast<int>(PlateOrientation::Normal_0));
    m_orientationCombo->addItem("180° Rotated", static_cast<int>(PlateOrientation::Rotate_180));
    m_orientationCombo->addItem("90° CW", static_cast<int>(PlateOrientation::Rotate_90_CW));
    m_orientationCombo->addItem("90° CCW", static_cast<int>(PlateOrientation::Rotate_90_CCW));
    m_orientationCombo->addItem("Flip Horiz", static_cast<int>(PlateOrientation::Flip_Horizontal));
    m_orientationCombo->addItem("Flip Vert", static_cast<int>(PlateOrientation::Flip_Vertical));
    m_orientationCombo->setToolTip("Fluent Deck Orientation Adjustment");
    connect(m_orientationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QuadrantDropWidget::onOrientationComboChanged);

    m_browseBtn = new QPushButton("Browse", this);
    m_browseBtn->setObjectName("iconBtn");
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    connect(m_browseBtn, &QPushButton::clicked, this, &QuadrantDropWidget::onBrowseClicked);

    m_clearBtn = new QPushButton("Clear", this);
    m_clearBtn->setObjectName("dangerBtn");
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    m_clearBtn->setVisible(false);
    connect(m_clearBtn, &QPushButton::clicked, this, &QuadrantDropWidget::onClearClicked);

    QHBoxLayout *rightBox = new QHBoxLayout();
    rightBox->setSpacing(6);
    rightBox->setContentsMargins(0, 0, 0, 0);
    rightBox->addWidget(m_volumeSpin);
    rightBox->addWidget(m_orientationCombo);
    rightBox->addWidget(m_browseBtn);
    rightBox->addWidget(m_clearBtn);

    mainLayout->addLayout(leftBox);
    mainLayout->addWidget(m_statusIconLabel);
    mainLayout->addLayout(centerBox, 1);
    mainLayout->addLayout(rightBox);

    updateStyle();
}

double QuadrantDropWidget::volume() const {
    return m_volumeSpin ? m_volumeSpin->value() : 20.0;
}

void QuadrantDropWidget::setVolume(double vol) {
    if (m_volumeSpin) {
        m_volumeSpin->blockSignals(true);
        m_volumeSpin->setValue(vol);
        m_volumeSpin->blockSignals(false);
    }
}

void QuadrantDropWidget::onVolumeSpinChanged(double val) {
    emit volumeChanged(m_quadrant, val);
}

void QuadrantDropWidget::setSourceFormat(PlateFormat fmt) {
    m_sourceFormat = fmt;
    if (!m_isLoaded) {
        m_infoLabel->setText("No file loaded");
    }
}

void QuadrantDropWidget::setOrientation(PlateOrientation ori) {
    m_orientation = ori;
    int index = m_orientationCombo->findData(static_cast<int>(ori));
    if (index >= 0) {
        m_orientationCombo->blockSignals(true);
        m_orientationCombo->setCurrentIndex(index);
        m_orientationCombo->blockSignals(false);
    }
}

void QuadrantDropWidget::onOrientationComboChanged(int index) {
    PlateOrientation ori = static_cast<PlateOrientation>(m_orientationCombo->itemData(index).toInt());
    m_orientation = ori;
    emit orientationChanged(m_quadrant, ori);
}

void QuadrantDropWidget::setLoadedFile(const QString &path, int filledWells, PlateFormat sourceFmt) {
    m_filePath = path;
    m_isLoaded = true;
    QFileInfo fi(path);

    int total = PlateModel::getRowCount(sourceFmt) * PlateModel::getColCount(sourceFmt);

    m_statusIconLabel->setText("•");
    m_statusIconLabel->setStyleSheet("font-size: 16px; color: #4fb06a; font-weight: bold;");

    m_filenameLabel->setText(fi.fileName());
    m_filenameLabel->setStyleSheet("font-weight: bold; font-size: 11px; color: #FAFAFA;");

    m_infoLabel->setText(QString("%1 / %2 wells").arg(filledWells).arg(total));
    m_infoLabel->setStyleSheet("color: #4c9be0; font-weight: 600; font-size: 10px;");

    m_clearBtn->setVisible(true);
    updateStyle();
}

void QuadrantDropWidget::clear() {
    m_filePath.clear();
    m_isLoaded = false;

    m_statusIconLabel->setText("•");
    m_statusIconLabel->setStyleSheet("font-size: 14px; color: #71717A; font-weight: bold;");

    m_filenameLabel->setText("Drop CSV here or browse");
    m_filenameLabel->setStyleSheet("font-weight: 600; font-size: 11px; color: #D4D4D8;");

    m_infoLabel->setText("No file loaded");
    m_infoLabel->setStyleSheet("color: #71717A; font-size: 10px;");

    m_clearBtn->setVisible(false);
    updateStyle();
}

void QuadrantDropWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty() && urls.first().toLocalFile().endsWith(".csv", Qt::CaseInsensitive)) {
            event->acceptProposedAction();
            m_isDragHover = true;
            updateStyle();
            return;
        }
    }
    event->ignore();
}

void QuadrantDropWidget::dragLeaveEvent(QDragLeaveEvent *event) {
    Q_UNUSED(event);
    m_isDragHover = false;
    updateStyle();
}

void QuadrantDropWidget::dropEvent(QDropEvent *event) {
    m_isDragHover = false;
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString path = urls.first().toLocalFile();
            event->acceptProposedAction();
            emit fileDropped(m_quadrant, path);
        }
    }
    updateStyle();
}

void QuadrantDropWidget::onBrowseClicked() {
    QString path = QFileDialog::getOpenFileName(this,
        QString("Select CSV for %1").arg(PlateModel::getQuadrantName(m_quadrant)),
        "", "CSV Files (*.csv);;All Files (*)");
    if (!path.isEmpty()) {
        emit fileDropped(m_quadrant, path);
    }
}

void QuadrantDropWidget::onClearClicked() {
    clear();
    emit clearRequested(m_quadrant);
}

void QuadrantDropWidget::updateStyle() {
    QColor qColor = PlateModel::getQuadrantColor(m_quadrant);
    QString style;

    if (m_isDragHover) {
        style = QString(
            "QuadrantDropWidget { "
            "  background-color: #18181B; "
            "  border: 1.5px dashed %1; "
            "  border-radius: 3px; "
            "}"
        ).arg(qColor.name());
    } else if (m_isLoaded) {
        style = QString(
            "QuadrantDropWidget { "
            "  background-color: #121215; "
            "  border: 1.5px solid %1; "
            "  border-radius: 3px; "
            "}"
        ).arg(qColor.name());
    } else {
        style = QString(
            "QuadrantDropWidget { "
            "  background-color: #09090B; "
            "  border: 1px dashed #27272A; "
            "  border-radius: 3px; "
            "}"
        );
    }
    setStyleSheet(style);
}
