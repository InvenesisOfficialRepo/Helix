#pragma once

#include <QObject>
#include <QString>
#include <QDate>

#include "models/TwoWeekDayModel.h"

class DbWorker;

class WallboardViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TwoWeekDayModel* dayModel READ dayModel CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString lastRefreshIso READ lastRefreshIso NOTIFY lastRefreshIsoChanged)

public:
    explicit WallboardViewModel(QObject* parent = nullptr);

    void setDbWorker(DbWorker* worker);

    TwoWeekDayModel* dayModel() { return &dayModel_; }
    QString errorMessage() const { return errorMessage_; }
    QString lastRefreshIso() const { return lastRefreshIso_; }

    Q_INVOKABLE void refresh();

signals:
    void errorMessageChanged();
    void lastRefreshIsoChanged();

private slots:
    void onRangeReady(const QVector<CalendarEventItem>& items);
    void onRangeFailed(const QString& error);

private:
    static QDate mondayOfWeek(const QDate& d);

    DbWorker* worker_ = nullptr;

    TwoWeekDayModel dayModel_;
    QString errorMessage_;
    QString lastRefreshIso_;

    QDate lastRequestedStartMonday_;
};
