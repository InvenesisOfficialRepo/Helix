#pragma once

#include <QObject>
#include <QString>

#include "DbConfig.h"
#include "domain/DashboardDtos.h"

#include "domain/BatchDetailDtos.h"

#include <QVariantMap>
#include "models/CalendarEventsModel.h"

class DbWorker : public QObject
{
    Q_OBJECT
public:
    explicit DbWorker(QObject* parent = nullptr);

    void setConfig(const DbConfig& cfg, const QString& configError);

public slots:
    void onThreadStarted();
    void authenticate(const QString& username, const QString& password);
    void requestDashboardSnapshot();
    void requestDoneDashboardSnapshot();
    void requestArchiveDashboardSnapshot();
    void requestBatchDetail(const QString& batchId);
    void sendSelectedCompounds(const QString& batchId, const QStringList& compoundIds, const QDate& date);
    void requestCalendarMonthSummary(int year, int month0); // 0=Jan..11=Dec
    void requestCalendarDayEvents(const QDate& day);
    void syncDoneStatuses();
    void importExcel(const QString& filePath, const QString& createdByUser);
    void publishReports(const QString& batchId, const QString& pdfPath, const QString& htmlPath);
    void publishResults(const QString& batchId, const QString& pdfPath, const QVariantMap& analysisResults, const QVariantList& rawPlates);
    void rejectAndRedoBatch(const QString& batchId);
    void shutdown();

signals:
    void dbReadyChanged(bool ready, const QString& error);

    void authenticateDone(const QString& username,
                          bool success,
                          const QString& role,
                          const QString& error);

    void dashboardSnapshotReady(const QVector<TestCardDto>& snapshot);
    void dashboardSnapshotFailed(const QString& error);

    void doneDashboardSnapshotReady(const QVector<TestCardDto>& snapshot);
    void doneDashboardSnapshotFailed(const QString& error);

    void archiveDashboardSnapshotReady(const QVector<TestCardDto>& snapshot);
    void archiveDashboardSnapshotFailed(const QString& error);

    void batchDetailReady(const BatchDetailDto& dto);
    void batchDetailFailed(const QString& error);

    void sendSelectedDone();
    void sendSelectedFailed(const QString& error);

    void calendarMonthSummaryReady(const QVariantMap& dayCounts);
    void calendarMonthSummaryFailed(const QString& error);

    void calendarDayEventsReady(const QVector<CalendarEventItem>& items);
    void calendarDayEventsFailed(const QString& error);

    void syncDoneStatusesDone(int compoundsUpdated, int batchesUpdated);
    void syncDoneStatusesFailed(const QString& error);

    void importExcelFinished(const QString& message);
    void importExcelFailed(const QString& error);

    void publishReportsFinished(bool success, const QString& error);

private:
    DbConfig cfg_;
    QString cfgError_;

    QString connectionName_;
    bool ready_ = false;
    QString lastError_;
};
