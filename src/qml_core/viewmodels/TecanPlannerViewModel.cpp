#include "TecanPlannerViewModel.h"

#include <QDebug>
#include "db/DbWorker.h"

TecanPlannerViewModel::TecanPlannerViewModel(QObject *parent)
    : QObject(parent)
    , selectedDate_(QDate::currentDate())
{
}

void TecanPlannerViewModel::setDbWorker(DbWorker* worker)
{
    if (worker_ == worker) return;
    worker_ = worker;

    if (!worker_) return;

    // UI -> Worker
    connect(this, &TecanPlannerViewModel::requestPlannerBatches,
            worker_, &DbWorker::requestTecanPlannerBatches,
            Qt::QueuedConnection);

    connect(this, &TecanPlannerViewModel::requestTodaySummary,
            worker_, &DbWorker::requestTecanPlannerSummary,
            Qt::QueuedConnection);

    // Worker -> UI
    connect(worker_, &DbWorker::tecanPlannerBatchesReady,
            this, &TecanPlannerViewModel::onPlannerBatchesReady,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::tecanPlannerBatchesFailed,
            this, &TecanPlannerViewModel::onPlannerBatchesFailed,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::tecanPlannerSummaryReady,
            this, &TecanPlannerViewModel::onTodaySummaryReady,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::tecanPlannerSummaryFailed,
            this, &TecanPlannerViewModel::onTodaySummaryFailed,
            Qt::QueuedConnection);

    // Initial load
    refreshTodaySummary();
    reload();
}

void TecanPlannerViewModel::setSelectedDate(const QDate& d)
{
    if (selectedDate_ == d || !d.isValid()) return;
    selectedDate_ = d;
    emit selectedDateChanged();
    reload();
}

void TecanPlannerViewModel::reload()
{
    if (!worker_) return;
    setBusy(true);
    setError(QString());
    emit requestPlannerBatches(selectedDate_);
}

void TecanPlannerViewModel::reloadForDate(const QDate& date)
{
    setSelectedDate(date);
}

void TecanPlannerViewModel::nextDay()
{
    setSelectedDate(selectedDate_.addDays(1));
}

void TecanPlannerViewModel::prevDay()
{
    setSelectedDate(selectedDate_.addDays(-1));
}

void TecanPlannerViewModel::setToday()
{
    setSelectedDate(QDate::currentDate());
}

void TecanPlannerViewModel::refreshTodaySummary()
{
    if (!worker_) return;
    emit requestTodaySummary(QDate::currentDate());
}

void TecanPlannerViewModel::onPlannerBatchesReady(const QVector<TecanAssayBatchDto>& items)
{
    setBusy(false);
    rawBatches_ = items;

    QVariantList list;
    int total = 0;
    for (const auto& b : items) {
        list.append(b.toMap());
        total += b.compoundCount;
    }

    batchesList_ = list;
    totalCompounds_ = total;

    emit batchesChanged();
    emit batchCountChanged();
    emit totalCompoundsChanged();
}

void TecanPlannerViewModel::onPlannerBatchesFailed(const QString& error)
{
    setBusy(false);
    rawBatches_.clear();
    batchesList_.clear();
    totalCompounds_ = 0;
    emit batchesChanged();
    emit batchCountChanged();
    emit totalCompoundsChanged();
    setError(error);
}

void TecanPlannerViewModel::onTodaySummaryReady(int batchCount, int compoundCount)
{
    todayBatchCount_ = batchCount;
    todayCompoundCount_ = compoundCount;
    emit todaySummaryChanged();
}

void TecanPlannerViewModel::onTodaySummaryFailed(const QString& error)
{
    qWarning() << "[TecanPlannerViewModel] Today summary failed:" << error;
}

void TecanPlannerViewModel::setBusy(bool v)
{
    if (busy_ == v) return;
    busy_ = v;
    emit busyChanged();
}

void TecanPlannerViewModel::setError(const QString& msg)
{
    if (errorMessage_ == msg) return;
    errorMessage_ = msg;
    emit errorMessageChanged();
}
