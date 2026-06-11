#pragma once

#include <QObject>
#include <QDate>
#include <QVariantMap>

#include "models/CalendarEventsModel.h"

class DbWorker;

class CalendarViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDate selectedDate READ selectedDate WRITE setSelectedDate NOTIFY selectedDateChanged)
    Q_PROPERTY(CalendarEventsModel* eventsModel READ eventsModel CONSTANT)

    // Month marks: "YYYY-MM-DD" -> count
    Q_PROPERTY(QVariantMap monthDayCounts READ monthDayCounts NOTIFY monthDayCountsChanged)

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)



public:
    explicit CalendarViewModel(QObject *parent = nullptr);

    void setDbWorker(DbWorker* worker);

    QDate selectedDate() const;
    void setSelectedDate(const QDate &d);

    CalendarEventsModel* eventsModel() const;

    QVariantMap monthDayCounts() const;
    bool busy() const;
    QString errorMessage() const;

    Q_INVOKABLE void reloadForSelectedDate();
    Q_INVOKABLE void setVisibleMonth(int year, int month0); // 0=Jan..11=Dec
    Q_INVOKABLE void refreshVisibleMonth();

signals:
    void selectedDateChanged();
    void monthDayCountsChanged();
    void busyChanged();
    void errorMessageChanged();

    // UI -> worker
    void requestMonthSummary(int year, int month0);
    void requestDayEvents(const QDate& day);

private slots:
    void onMonthSummaryReady(const QVariantMap& counts);
    void onMonthSummaryFailed(const QString& error);

    void onDayEventsReady(const QVector<CalendarEventItem>& items);
    void onDayEventsFailed(const QString& error);

private:
    void setBusy(bool v);
    void setError(const QString& msg);

    int visibleYear_ = 0;
    int visibleMonth0_ = 0; // 0=Jan..11=Dec

    DbWorker* worker_ = nullptr;

    QDate selectedDate_;
    CalendarEventsModel* events_ = nullptr;

    QVariantMap monthDayCounts_;
    bool busy_ = false;
    QString errorMessage_;
};
