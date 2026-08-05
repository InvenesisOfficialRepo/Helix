#pragma once

#include <QObject>
#include <QDate>
#include <QVariantList>
#include <QString>

#include "domain/TecanPlannerDtos.h"

class DbWorker;

class TecanPlannerViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDate selectedDate READ selectedDate WRITE setSelectedDate NOTIFY selectedDateChanged)
    Q_PROPERTY(QVariantList batches READ batches NOTIFY batchesChanged)
    Q_PROPERTY(int batchCount READ batchCount NOTIFY batchCountChanged)
    Q_PROPERTY(int totalCompounds READ totalCompounds NOTIFY totalCompoundsChanged)
    Q_PROPERTY(int todayBatchCount READ todayBatchCount NOTIFY todaySummaryChanged)
    Q_PROPERTY(int todayCompoundCount READ todayCompoundCount NOTIFY todaySummaryChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit TecanPlannerViewModel(QObject *parent = nullptr);

    void setDbWorker(DbWorker* worker);

    QDate selectedDate() const { return selectedDate_; }
    Q_INVOKABLE void setSelectedDate(const QDate& d);

    QVariantList batches() const { return batchesList_; }
    int batchCount() const { return batchesList_.size(); }
    int totalCompounds() const { return totalCompounds_; }
    int todayBatchCount() const { return todayBatchCount_; }
    int todayCompoundCount() const { return todayCompoundCount_; }
    bool busy() const { return busy_; }
    QString errorMessage() const { return errorMessage_; }

    Q_INVOKABLE void reload();
    Q_INVOKABLE void reloadForDate(const QDate& date);
    Q_INVOKABLE void nextDay();
    Q_INVOKABLE void prevDay();
    Q_INVOKABLE void setToday();
    Q_INVOKABLE void refreshTodaySummary();

signals:
    void selectedDateChanged();
    void batchesChanged();
    void batchCountChanged();
    void totalCompoundsChanged();
    void todaySummaryChanged();
    void busyChanged();
    void errorMessageChanged();

    // VM -> DbWorker signals
    void requestPlannerBatches(const QDate& date);
    void requestTodaySummary(const QDate& today);

private slots:
    void onPlannerBatchesReady(const QVector<TecanAssayBatchDto>& items);
    void onPlannerBatchesFailed(const QString& error);

    void onTodaySummaryReady(int batchCount, int compoundCount);
    void onTodaySummaryFailed(const QString& error);

private:
    void setBusy(bool v);
    void setError(const QString& msg);

    DbWorker* worker_ = nullptr;
    QDate selectedDate_;
    QVector<TecanAssayBatchDto> rawBatches_;
    QVariantList batchesList_;
    int totalCompounds_ = 0;
    int todayBatchCount_ = 0;
    int todayCompoundCount_ = 0;
    bool busy_ = false;
    QString errorMessage_;
};
