#include "PlateViewWidget.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#include <QHeaderView>
#include <QGroupBox>
#include <QColor>
#include <QBrush>
#include <QPen>

class WellGraphicsItem : public QGraphicsEllipseItem {
public:
    WellGraphicsItem(int r, int c, const WellContent &content, double size, const QString &highlightFilter, const WorkstationTokens &tokens, QGraphicsItem *parent = nullptr)
        : QGraphicsEllipseItem(0, 0, size, size, parent), m_row(r), m_col(c), m_content(content) {

        setAcceptHoverEvents(true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);

        QColor baseColor = PlateModel::getQuadrantColor(content.quadrant);
        if (content.isEmpty()) {
            setBrush(QBrush(QColor(tokens.surfaceInput)));
            setPen(QPen(QColor(tokens.border), 1));
        } else {
            setBrush(QBrush(baseColor));
            setPen(QPen(baseColor.lighter(115), 1.2));
        }

        if (!highlightFilter.trimmed().isEmpty() && !content.isEmpty()) {
            if (content.compoundName.contains(highlightFilter.trimmed(), Qt::CaseInsensitive)) {
                setPen(QPen(QColor(tokens.accent), 2.5));
            }
        }

        QString wellId = PlateModel::getWellId(r, c);
        if (content.isEmpty()) {
            setToolTip(QString("Well %1: [Empty]").arg(wellId));
        } else {
            setToolTip(QString("Well %1\nCompound: %2\nQuadrant: %3\nSource Well: %4")
                .arg(wellId)
                .arg(content.compoundName)
                .arg(PlateModel::getQuadrantName(content.quadrant))
                .arg(content.sourceWellId));
        }
    }

    int row() const { return m_row; }
    int col() const { return m_col; }

private:
    int m_row;
    int m_col;
    WellContent m_content;
};

// ----------------------------------------------------
// PlateGraphicsView
// ----------------------------------------------------
PlateGraphicsView::PlateGraphicsView(QWidget *parent)
    : QGraphicsView(parent) {
    m_tokens = StyleHelper::getTokens(BrightnessLevel::Dark, AccentColor::Blue, DensityLevel::Comfortable);
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTokens(m_tokens);
}

void PlateGraphicsView::setTokens(const WorkstationTokens &tokens) {
    m_tokens = tokens;
    setBackgroundBrush(QBrush(QColor(m_tokens.surfaceWindow)));
    setStyleSheet(QString("border: 1px solid %1; border-radius: %2px;").arg(m_tokens.border).arg(m_tokens.radius));
    rebuildScene();
}

void PlateGraphicsView::setModel(const PlateModel &model, const QString &highlightFilter, const WorkstationTokens &tokens) {
    m_model = model;
    m_highlightFilter = highlightFilter;
    m_tokens = tokens;
    rebuildScene();
}

void PlateGraphicsView::rebuildScene() {
    m_scene->clear();
    if (m_model.rowCount() == 0 || m_model.colCount() == 0) return;

    double wellSize = (m_model.format() == PlateFormat::Plate1536) ? 12.0 : 22.0;
    double spacing = (m_model.format() == PlateFormat::Plate1536) ? 3.0 : 5.0;
    double marginX = (m_model.format() == PlateFormat::Plate1536) ? 36.0 : 40.0;
    double marginY = (m_model.format() == PlateFormat::Plate1536) ? 26.0 : 30.0;

    QFont font("Inter", (m_model.format() == PlateFormat::Plate1536) ? 6 : 8, QFont::Bold);

    // Column Headers
    for (int c = 0; c < m_model.colCount(); ++c) {
        double x = marginX + c * (wellSize + spacing);
        QGraphicsTextItem *text = m_scene->addText(QString::number(c + 1), font);
        text->setDefaultTextColor(QColor(m_tokens.accent));
        text->setPos(x + (wellSize - text->boundingRect().width()) / 2.0, 0);
    }

    // Row Headers
    for (int r = 0; r < m_model.rowCount(); ++r) {
        double y = marginY + r * (wellSize + spacing);
        QGraphicsTextItem *text = m_scene->addText(PlateModel::getRowLabel(r), font);
        text->setDefaultTextColor(QColor(m_tokens.accent));
        text->setPos(0, y + (wellSize - text->boundingRect().height()) / 2.0);
    }

    // Wells
    for (int r = 0; r < m_model.rowCount(); ++r) {
        for (int c = 0; c < m_model.colCount(); ++c) {
            double x = marginX + c * (wellSize + spacing);
            double y = marginY + r * (wellSize + spacing);

            WellContent wc = m_model.well(r, c);
            WellGraphicsItem *item = new WellGraphicsItem(r, c, wc, wellSize, m_highlightFilter, m_tokens);
            item->setPos(x, y);
            m_scene->addItem(item);
        }
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-10, -10, 10, 10));
}

void PlateGraphicsView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) zoomIn();
        else zoomOut();
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void PlateGraphicsView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
}

void PlateGraphicsView::zoomIn() {
    scale(1.2, 1.2);
    m_zoomFactor *= 1.2;
}

void PlateGraphicsView::zoomOut() {
    scale(1.0 / 1.2, 1.0 / 1.2);
    m_zoomFactor /= 1.2;
}

void PlateGraphicsView::resetZoom() {
    resetTransform();
    m_zoomFactor = 1.0;
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

// ----------------------------------------------------
// PlateViewWidget
// ----------------------------------------------------
PlateViewWidget::PlateViewWidget(QWidget *parent)
    : QWidget(parent), m_model(PlateFormat::Plate384) {
    m_tokens = StyleHelper::getTokens(BrightnessLevel::Dark, AccentColor::Blue, DensityLevel::Comfortable);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(6);

    // Top Bar: Search Filter & Zoom Controls
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->setContentsMargins(0, 0, 0, 0);

    QLabel *searchLabel = new QLabel("Filter:", this);
    searchLabel->setStyleSheet("font-size: 11px; font-weight: 600; color: #71717A;");

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search compound name...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PlateViewWidget::onSearchFilterChanged);

    m_zoomInBtn = new QPushButton("+", this);
    m_zoomInBtn->setToolTip("Zoom In (Ctrl + Wheel)");
    m_zoomInBtn->setFixedWidth(28);
    connect(m_zoomInBtn, &QPushButton::clicked, this, &PlateViewWidget::onZoomIn);

    m_zoomOutBtn = new QPushButton("-", this);
    m_zoomOutBtn->setToolTip("Zoom Out");
    m_zoomOutBtn->setFixedWidth(28);
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &PlateViewWidget::onZoomOut);

    m_resetZoomBtn = new QPushButton("Fit", this);
    m_resetZoomBtn->setToolTip("Fit Plate to Screen");
    m_resetZoomBtn->setFixedWidth(36);
    connect(m_resetZoomBtn, &QPushButton::clicked, this, &PlateViewWidget::onResetZoom);

    topBar->addWidget(searchLabel);
    topBar->addWidget(m_searchEdit, 1);
    topBar->addWidget(m_zoomInBtn);
    topBar->addWidget(m_zoomOutBtn);
    topBar->addWidget(m_resetZoomBtn);

    // Main Content: View Tabs + Side Inspector Panel
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(6);

    m_viewTabs = new QTabWidget(this);

    m_graphicsView = new PlateGraphicsView(this);
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(7);
    m_tableWidget->setHorizontalHeaderLabels({ "Well", "Row", "Column", "Quadrant", "Volume (ul)", "Source Well", "Compound Name" });
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &PlateViewWidget::onTableSelectionChanged);

    m_viewTabs->addTab(m_graphicsView, "Visual Grid View");
    m_viewTabs->addTab(m_tableWidget, "Table List View");

    // Side Inspector & Statistics Panel
    QVBoxLayout *sideLayout = new QVBoxLayout();
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(6);

    QGroupBox *inspectorGroup = new QGroupBox("Well Inspector", this);
    QVBoxLayout *inspLayout = new QVBoxLayout(inspectorGroup);
    inspLayout->setSpacing(4);

    m_selectedWellLabel = new QLabel("Selected Well: --", this);
    m_selectedWellLabel->setStyleSheet(QString("font-weight: bold; font-size: 12px; color: %1;").arg(m_tokens.accent));

    m_selectedQuadLabel = new QLabel("Quadrant: --", this);
    m_selectedSrcWellLabel = new QLabel("Source Well: --", this);
    m_selectedVolumeLabel = new QLabel("Volume: --", this);
    m_selectedCompoundLabel = new QLabel("Compound: --", this);
    m_selectedCompoundLabel->setWordWrap(true);
    m_selectedCompoundLabel->setStyleSheet(QString("font-weight: bold; color: %1;").arg(m_tokens.textPrimary));

    inspLayout->addWidget(m_selectedWellLabel);
    inspLayout->addWidget(m_selectedQuadLabel);
    inspLayout->addWidget(m_selectedSrcWellLabel);
    inspLayout->addWidget(m_selectedVolumeLabel);
    inspLayout->addWidget(m_selectedCompoundLabel);
    inspLayout->addStretch();

    QGroupBox *statsGroup = new QGroupBox("Plate Summary", this);
    QVBoxLayout *statsLayout = new QVBoxLayout(statsGroup);
    statsLayout->setSpacing(3);

    m_statTotalLabel = new QLabel("Total Wells: 0 / 0", this);
    m_statTotalLabel->setStyleSheet(QString("font-weight: bold; color: %1;").arg(m_tokens.accent));

    m_statQ1Label = new QLabel("Q1 (A01): 0", this);
    m_statQ1Label->setStyleSheet("color: #3B7A57; font-weight: 600;");

    m_statQ2Label = new QLabel("Q2 (A02): 0", this);
    m_statQ2Label->setStyleSheet("color: #4A6984; font-weight: 600;");

    m_statQ3Label = new QLabel("Q3 (B01): 0", this);
    m_statQ3Label->setStyleSheet("color: #9A6B38; font-weight: 600;");

    m_statQ4Label = new QLabel("Q4 (B02): 0", this);
    m_statQ4Label->setStyleSheet("color: #6B4C8A; font-weight: 600;");

    m_statEmptyLabel = new QLabel("Empty Wells: 0", this);
    m_statEmptyLabel->setStyleSheet(QString("color: %1;").arg(m_tokens.textMuted));

    statsLayout->addWidget(m_statTotalLabel);
    statsLayout->addWidget(m_statQ1Label);
    statsLayout->addWidget(m_statQ2Label);
    statsLayout->addWidget(m_statQ3Label);
    statsLayout->addWidget(m_statQ4Label);
    statsLayout->addWidget(m_statEmptyLabel);

    sideLayout->addWidget(inspectorGroup, 1);
    sideLayout->addWidget(statsGroup, 1);

    QWidget *sideWidget = new QWidget(this);
    sideWidget->setLayout(sideLayout);
    sideWidget->setFixedWidth(190);

    contentLayout->addWidget(m_viewTabs, 1);
    contentLayout->addWidget(sideWidget);

    mainLayout->addLayout(topBar);
    mainLayout->addLayout(contentLayout, 1);

    setFormat(PlateFormat::Plate384);
}

void PlateViewWidget::setTokens(const WorkstationTokens &tokens) {
    m_tokens = tokens;
    m_graphicsView->setTokens(m_tokens);
    m_selectedWellLabel->setStyleSheet(QString("font-weight: bold; font-size: 12px; color: %1;").arg(m_tokens.accent));
    m_selectedCompoundLabel->setStyleSheet(QString("font-weight: bold; color: %1;").arg(m_tokens.textPrimary));
    m_statTotalLabel->setStyleSheet(QString("font-weight: bold; color: %1;").arg(m_tokens.accent));
    m_statEmptyLabel->setStyleSheet(QString("color: %1;").arg(m_tokens.textMuted));
    updateTable();
}

void PlateViewWidget::setFormat(PlateFormat format) {
    m_model.resize(format);
    m_graphicsView->setModel(m_model, m_searchEdit->text(), m_tokens);
    m_graphicsView->resetZoom();
    updateTable();
    updateStats();
    selectWell(-1, -1);
}

void PlateViewWidget::setPlateModel(const PlateModel &model) {
    m_model = model;
    m_graphicsView->setModel(m_model, m_searchEdit->text(), m_tokens);
    m_graphicsView->resetZoom();
    updateTable();
    updateStats();
}

void PlateViewWidget::clear() {
    m_model.clear();
    m_graphicsView->setModel(m_model, QString(), m_tokens);
    m_tableWidget->setRowCount(0);
    updateStats();
    selectWell(-1, -1);
}

void PlateViewWidget::onSearchFilterChanged(const QString &text) {
    m_graphicsView->setModel(m_model, text, m_tokens);
    updateTable();
}

void PlateViewWidget::onZoomIn() { m_graphicsView->zoomIn(); }
void PlateViewWidget::onZoomOut() { m_graphicsView->zoomOut(); }
void PlateViewWidget::onResetZoom() { m_graphicsView->resetZoom(); }

void PlateViewWidget::onWellClicked(int row, int col) {
    selectWell(row, col);
}

void PlateViewWidget::selectWell(int r, int c) {
    if (r < 0 || c < 0 || r >= m_model.rowCount() || c >= m_model.colCount()) {
        m_selectedWellLabel->setText("Selected Well: --");
        m_selectedQuadLabel->setText("Quadrant: --");
        m_selectedSrcWellLabel->setText("Source Well: --");
        m_selectedVolumeLabel->setText("Volume: --");
        m_selectedCompoundLabel->setText("Compound: --");
        return;
    }

    WellContent w = m_model.well(r, c);
    m_selectedWellLabel->setText(QString("Selected Well: %1").arg(PlateModel::getWellId(r, c)));
    if (w.quadrant != QuadrantId::None) {
        m_selectedQuadLabel->setText(QString("Quadrant: %1").arg(PlateModel::getQuadrantName(w.quadrant)));
        m_selectedSrcWellLabel->setText(QString("Source Well: %1").arg(w.sourceWellId));
        m_selectedVolumeLabel->setText(QString("Volume: %1 %2").arg(QString::number(w.volume, 'f', 1)).arg(w.volumeUnit));
        m_selectedCompoundLabel->setText(QString("Compound: %1").arg(w.compoundName));
    } else {
        m_selectedQuadLabel->setText("Quadrant: Empty");
        m_selectedSrcWellLabel->setText("Source Well: --");
        m_selectedVolumeLabel->setText("Volume: --");
        m_selectedCompoundLabel->setText("Compound: [Empty]");
    }
}

void PlateViewWidget::updateTable() {
    m_tableWidget->setRowCount(0);
    if (m_model.rowCount() == 0) return;

    QString filter = m_searchEdit->text().trimmed();

    for (int r = 0; r < m_model.rowCount(); ++r) {
        for (int c = 0; c < m_model.colCount(); ++c) {
            WellContent w = m_model.well(r, c);

            if (!filter.isEmpty() && !w.compoundName.contains(filter, Qt::CaseInsensitive)) {
                continue;
            }

            int rowIdx = m_tableWidget->rowCount();
            m_tableWidget->insertRow(rowIdx);

            QTableWidgetItem *itemWell = new QTableWidgetItem(PlateModel::getWellId(r, c));
            QTableWidgetItem *itemRow = new QTableWidgetItem(PlateModel::getRowLabel(r));
            QTableWidgetItem *itemCol = new QTableWidgetItem(QString::number(c + 1));
            QTableWidgetItem *itemQuad = new QTableWidgetItem(w.quadrant == QuadrantId::None ? "-" : QString("Q%1").arg(static_cast<int>(w.quadrant)));
            QTableWidgetItem *itemVol = new QTableWidgetItem(w.quadrant == QuadrantId::None ? "-" : QString("%1 %2").arg(QString::number(w.volume, 'f', 1)).arg(w.volumeUnit));
            QTableWidgetItem *itemSrcWell = new QTableWidgetItem(w.sourceWellId.isEmpty() ? "-" : w.sourceWellId);
            QTableWidgetItem *itemComp = new QTableWidgetItem(w.compoundName.isEmpty() ? "[Empty]" : w.compoundName);

            if (w.quadrant != QuadrantId::None) {
                QColor qColor = PlateModel::getQuadrantColor(w.quadrant);
                itemQuad->setForeground(QBrush(qColor));
            }

            m_tableWidget->setItem(rowIdx, 0, itemWell);
            m_tableWidget->setItem(rowIdx, 1, itemRow);
            m_tableWidget->setItem(rowIdx, 2, itemCol);
            m_tableWidget->setItem(rowIdx, 3, itemQuad);
            m_tableWidget->setItem(rowIdx, 4, itemVol);
            m_tableWidget->setItem(rowIdx, 5, itemSrcWell);
            m_tableWidget->setItem(rowIdx, 6, itemComp);
        }
    }
}

void PlateViewWidget::onTableSelectionChanged() {
    int row = m_tableWidget->currentRow();
    if (row >= 0 && row < m_tableWidget->rowCount()) {
        QString wellId = m_tableWidget->item(row, 0)->text();
        int r = -1, c = -1;
        if (PlateModel::parseWellId(wellId, r, c)) {
            selectWell(r, c);
        }
    }
}

void PlateViewWidget::updateStats() {
    int total = m_model.totalWellCount();
    int filled = m_model.filledWellCount();

    int q1 = 0, q2 = 0, q3 = 0, q4 = 0;
    for (int r = 0; r < m_model.rowCount(); ++r) {
        for (int c = 0; c < m_model.colCount(); ++c) {
            WellContent w = m_model.well(r, c);
            switch (w.quadrant) {
            case QuadrantId::Q1: q1++; break;
            case QuadrantId::Q2: q2++; break;
            case QuadrantId::Q3: q3++; break;
            case QuadrantId::Q4: q4++; break;
            default: break;
            }
        }
    }

    m_statTotalLabel->setText(QString("Total Wells: %1 / %2").arg(filled).arg(total));
    m_statQ1Label->setText(QString("Q1 (A01): %1").arg(q1));
    m_statQ2Label->setText(QString("Q2 (A02): %1").arg(q2));
    m_statQ3Label->setText(QString("Q3 (B01): %1").arg(q3));
    m_statQ4Label->setText(QString("Q4 (B02): %1").arg(q4));
    m_statEmptyLabel->setText(QString("Empty Wells: %1").arg(total - filled));
}
