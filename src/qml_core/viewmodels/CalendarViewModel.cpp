#include "CalendarViewModel.h"

#include "db/DbWorker.h"

CalendarViewModel::CalendarViewModel(QObject *parent)
    : QObject(parent)
{
    events_ = new CalendarEventsModel(this);
    selectedDate_ = QDate::currentDate();
}

void CalendarViewModel::setDbWorker(DbWorker* worker)
{
    if (worker_ == worker) return;
    worker_ = worker;
    if (!worker_) return;

    // UI -> worker
    connect(this, &CalendarViewModel::requestMonthSummary,
            worker_, &DbWorker::requestCalendarMonthSummary,
            Qt::QueuedConnection);

    connect(this, &CalendarViewModel::requestDayEvents,
            worker_, &DbWorker::requestCalendarDayEvents,
            Qt::QueuedConnection);

    // worker -> UI
    connect(worker_, &DbWorker::calendarMonthSummaryReady,
            this, &CalendarViewModel::onMonthSummaryReady,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::calendarMonthSummaryFailed,
            this, &CalendarViewModel::onMonthSummaryFailed,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::calendarDayEventsReady,
            this, &CalendarViewModel::onDayEventsReady,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::calendarDayEventsFailed,
            this, &CalendarViewModel::onDayEventsFailed,
            Qt::QueuedConnection);

    // Initial load (current month + selected day)
    setVisibleMonth(selectedDate_.year(), selectedDate_.month() - 1);
    reloadForSelectedDate();
}

QDate CalendarViewModel::selectedDate() const { return selectedDate_; }

void CalendarViewModel::setSelectedDate(const QDate &d)
{
    if (selectedDate_ == d) return;
    selectedDate_ = d;
    emit selectedDateChanged();
}

CalendarEventsModel* CalendarViewModel::eventsModel() const { return events_; }

QVariantMap CalendarViewModel::monthDayCounts() const { return monthDayCounts_; }
bool CalendarViewModel::busy() const { return busy_; }
QString CalendarViewModel::errorMessage() const { return errorMessage_; }

void CalendarViewModel::setBusy(bool v)
{
    if (busy_ == v) return;
    busy_ = v;
    emit busyChanged();
}

void CalendarViewModel::setError(const QString& msg)
{
    if (errorMessage_ == msg) return;
    errorMessage_ = msg;
    emit errorMessageChanged();
}

void CalendarViewModel::setVisibleMonth(int year, int month0)
{
    if (!worker_) { setError("DB worker not configured (calendar)."); return; }

    visibleYear_ = year;
    visibleMonth0_ = month0;

    setError(QString{});
    emit requestMonthSummary(year, month0);
}

void CalendarViewModel::reloadForSelectedDate()
{
    if (!worker_) { setError("DB worker not configured (calendar)."); return; }
    setBusy(true);
    setError(QString{});
    events_->clear();
    emit requestDayEvents(selectedDate_);
}

void CalendarViewModel::onMonthSummaryReady(const QVariantMap& counts)
{
    monthDayCounts_ = counts;
    emit monthDayCountsChanged();
}

void CalendarViewModel::onMonthSummaryFailed(const QString& error)
{
    setError(error.isEmpty() ? "Failed to load month summary." : error);
}

void CalendarViewModel::onDayEventsReady(const QVector<CalendarEventItem>& items)
{
    events_->setItems(items);
    setBusy(false);
}

void CalendarViewModel::onDayEventsFailed(const QString& error)
{
    setBusy(false);
    setError(error.isEmpty() ? "Failed to load day events." : error);
}

void CalendarViewModel::refreshVisibleMonth()
{
    if (!worker_) return;

    // If never set, fall back to selected date’s month
    int y = visibleYear_;
    int m0 = visibleMonth0_;
    if (y == 0) {
        y = selectedDate_.year();
        m0 = selectedDate_.month() - 1;
    }

    emit requestMonthSummary(y, m0);
}
