#include "viewmodels/WallboardViewModel.h"

#include <QDateTime>
#include <QMetaObject>

#include "db/DbWorker.h"

WallboardViewModel::WallboardViewModel(QObject* parent)
    : QObject(parent)
{
}

void WallboardViewModel::setDbWorker(DbWorker* worker)
{
    if (worker_ == worker)
        return;

    if (worker_) {
        disconnect(worker_, nullptr, this, nullptr);
    }

    worker_ = worker;

    if (!worker_)
        return;

    // DbWorker lives in worker thread -> always queued
    connect(worker_, &DbWorker::calendarRangeEventsReady,
            this, &WallboardViewModel::onRangeReady,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::calendarRangeEventsFailed,
            this, &WallboardViewModel::onRangeFailed,
            Qt::QueuedConnection);
}

QDate WallboardViewModel::mondayOfWeek(const QDate& d)
{
    // QDate::dayOfWeek(): Monday=1 ... Sunday=7
    return d.addDays(1 - d.dayOfWeek());
}

void WallboardViewModel::refresh()
{
    if (!worker_) {
        errorMessage_ = "DbWorker not set.";
        emit errorMessageChanged();
        return;
    }

    errorMessage_.clear();
    emit errorMessageChanged();

    lastRequestedStartMonday_ = mondayOfWeek(QDate::currentDate());
    const QDate endExclusive = lastRequestedStartMonday_.addDays(14);

    // Call worker method in its thread
    QMetaObject::invokeMethod(worker_, "requestCalendarRangeEvents",
                              Qt::QueuedConnection,
                              Q_ARG(QDate, lastRequestedStartMonday_),
                              Q_ARG(QDate, endExclusive));
}

void WallboardViewModel::onRangeReady(const QVector<CalendarEventItem>& items)
{
    dayModel_.setTwoWeekData(lastRequestedStartMonday_, items);

    lastRefreshIso_ = QDateTime::currentDateTime().toString(Qt::ISODate);
    emit lastRefreshIsoChanged();
}

void WallboardViewModel::onRangeFailed(const QString& error)
{
    errorMessage_ = error;
    emit errorMessageChanged();
}
