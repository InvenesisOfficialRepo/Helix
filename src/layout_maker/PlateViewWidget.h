#ifndef PLATEVIEWWIDGET_H
#define PLATEVIEWWIDGET_H

#include "PlateModel.h"
#include "StyleHelper.h"
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

class PlateGraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    explicit PlateGraphicsView(QWidget *parent = nullptr);
    void setModel(const PlateModel &model, const QString &highlightFilter = QString(), const WorkstationTokens &tokens = StyleHelper::getTokens(BrightnessLevel::Dark, AccentColor::Blue, DensityLevel::Comfortable));
    void setTokens(const WorkstationTokens &tokens);
    void zoomIn();
    void zoomOut();
    void resetZoom();

signals:
    void wellClicked(int row, int col);
    void wellHovered(int row, int col);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildScene();

    PlateModel m_model;
    QString m_highlightFilter;
    WorkstationTokens m_tokens;
    QGraphicsScene *m_scene;
    double m_zoomFactor = 1.0;
};

class PlateViewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlateViewWidget(QWidget *parent = nullptr);

    void setPlateModel(const PlateModel &model);
    void setFormat(PlateFormat format);
    void setTokens(const WorkstationTokens &tokens);
    void clear();

private slots:
    void onSearchFilterChanged(const QString &text);
    void onWellClicked(int row, int col);
    void onTableSelectionChanged();
    void onZoomIn();
    void onZoomOut();
    void onResetZoom();

private:
    void updateTable();
    void updateStats();
    void selectWell(int r, int c);

    PlateModel m_model;
    WorkstationTokens m_tokens;

    QLineEdit *m_searchEdit;
    QTabWidget *m_viewTabs;

    PlateGraphicsView *m_graphicsView;
    QTableWidget *m_tableWidget;

    // Zoom controls
    QPushButton *m_zoomInBtn;
    QPushButton *m_zoomOutBtn;
    QPushButton *m_resetZoomBtn;

    // Inspection Side Panel
    QLabel *m_selectedWellLabel;
    QLabel *m_selectedQuadLabel;
    QLabel *m_selectedSrcWellLabel;
    QLabel *m_selectedVolumeLabel;
    QLabel *m_selectedCompoundLabel;

    // Stats Labels
    QLabel *m_statTotalLabel;
    QLabel *m_statQ1Label;
    QLabel *m_statQ2Label;
    QLabel *m_statQ3Label;
    QLabel *m_statQ4Label;
    QLabel *m_statEmptyLabel;
};

#endif // PLATEVIEWWIDGET_H
