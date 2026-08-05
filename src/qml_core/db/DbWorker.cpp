#include "DbWorker.h"

#include <QMetaType>
#include <QUuid>
#include <QtSql/QSqlDatabase>

#include "DatabaseManager.h"
#include "auth/AuthRepository.h"
#include "repositories/DashboardRepository.h"

#include "repositories/BatchRepository.h"
#include "domain/BatchDetailDtos.h"

#include "repositories/CalendarRepository.h"
#include "repositories/TecanPlannerRepository.h"

#include "repositories/DoneSyncRepository.h"

#include "repositories/ExcelImportRepository.h"

#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>

// Ensure DTOs can cross threads via queued signals
static const int _dashboardMetaTypes = []() -> int {
    qRegisterMetaType<BatchDto>("BatchDto");
    qRegisterMetaType<QVector<BatchDto>>("QVector<BatchDto>");
    qRegisterMetaType<TestCardDto>("TestCardDto");
    qRegisterMetaType<QVector<TestCardDto>>("QVector<TestCardDto>");
    return 0;
}();

static const int _batchDetailMetaTypes = []() -> int {
    qRegisterMetaType<CompoundDto>("CompoundDto");
    qRegisterMetaType<QVector<CompoundDto>>("QVector<CompoundDto>");
    qRegisterMetaType<BatchDetailDto>("BatchDetailDto");
    return 0;
}();

static const int _calendarMeta = []() -> int {
    qRegisterMetaType<QVector<CalendarEventItem>>("QVector<CalendarEventItem>");
    qRegisterMetaType<QVariantMap>("QVariantMap");
    return 0;
}();

static const int _tecanPlannerMeta = []() -> int {
    qRegisterMetaType<TecanAssayBatchDto>("TecanAssayBatchDto");
    qRegisterMetaType<QVector<TecanAssayBatchDto>>("QVector<TecanAssayBatchDto>");
    return 0;
}();

DbWorker::DbWorker(QObject* parent)
    : QObject(parent)
{
    (void)_dashboardMetaTypes;
}

void DbWorker::setConfig(const DbConfig& cfg, const QString& configError)
{
    cfg_ = cfg;
    cfgError_ = configError;
}

void DbWorker::onThreadStarted()
{
    connectionName_ = QString("dbworker_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    if (!cfgError_.isEmpty()) {
        ready_ = false;
        lastError_ = cfgError_;
        emit dbReadyChanged(false, lastError_);
        return;
    }

    QString err;
    if (!DatabaseManager::openConnection(cfg_, connectionName_, &err)) {
        ready_ = false;
        lastError_ = err.isEmpty() ? "Failed to open database connection." : err;
        emit dbReadyChanged(false, lastError_);
        return;
    }

    ready_ = true;
    lastError_.clear();
    emit dbReadyChanged(true, QString{});
}

void DbWorker::authenticate(const QString& username, const QString& password)
{
    if (!ready_) {
        emit authenticateDone(username, false, QString{}, lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit authenticateDone(username, false, QString{}, "Database connection is not open.");
        return;
    }

    AuthRepository repo(db);
    const AuthResult res = repo.authenticate(username, password);

    emit authenticateDone(username, res.success, res.role, res.error);
}

void DbWorker::requestDashboardSnapshot()
{
    if (!ready_) {
        emit dashboardSnapshotFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit dashboardSnapshotFailed("Database connection is not open.");
        return;
    }

    DashboardRepository repo(db);
    QString err;
    const auto snapshot = repo.loadDashboardSnapshot(&err);
    if (!err.isEmpty()) {
        emit dashboardSnapshotFailed(err);
        return;
    }

    emit dashboardSnapshotReady(snapshot);
}


void DbWorker::requestBatchDetail(const QString& batchId)
{
    if (!ready_) {
        emit batchDetailFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit batchDetailFailed("Database connection is not open.");
        return;
    }

    BatchRepository repo(db);
    QString err;
    const BatchDetailDto dto = repo.loadBatchDetail(batchId, &err);
    if (!err.isEmpty()) {
        emit batchDetailFailed(err);
        return;
    }

    emit batchDetailReady(dto);
}

void DbWorker::sendSelectedCompounds(const QString& batchId, const QStringList& compoundIds, const QDate& date)
{
    if (!ready_) {
        emit sendSelectedFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit sendSelectedFailed("Database connection is not open.");
        return;
    }

    BatchRepository repo(db);
    QString err;
    const bool ok = repo.sendSelectedCompounds(batchId, compoundIds, date, &err);
    if (!ok) {
        emit sendSelectedFailed(err.isEmpty() ? "Send failed." : err);
        return;
    }

    emit sendSelectedDone();
}

void DbWorker::requestCalendarMonthSummary(int year, int month0)
{
    if (!ready_) {
        emit calendarMonthSummaryFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit calendarMonthSummaryFailed("Database connection is not open.");
        return;
    }

    const QDate start(year, month0 + 1, 1);
    const QDate end = start.addMonths(1);

    CalendarRepository repo(db);
    QString err;
    const QVariantMap counts = repo.loadMonthCounts(start, end, &err);
    if (!err.isEmpty()) {
        emit calendarMonthSummaryFailed(err);
        return;
    }

    emit calendarMonthSummaryReady(counts);
}

void DbWorker::requestCalendarDayEvents(const QDate& day)
{
    if (!ready_) {
        emit calendarDayEventsFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit calendarDayEventsFailed("Database connection is not open.");
        return;
    }

    CalendarRepository repo(db);
    QString err;
    const auto items = repo.loadDayEvents(day, &err);
    if (!err.isEmpty()) {
        emit calendarDayEventsFailed(err);
        return;
    }

    emit calendarDayEventsReady(items);
}

void DbWorker::requestTecanPlannerBatches(const QDate& date)
{
    if (!ready_) {
        emit tecanPlannerBatchesFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit tecanPlannerBatchesFailed("Database connection is not open.");
        return;
    }

    TecanPlannerRepository repo(db);
    QString err;
    const auto items = repo.loadDailyAssayBatches(date, &err);
    if (!err.isEmpty()) {
        emit tecanPlannerBatchesFailed(err);
        return;
    }

    emit tecanPlannerBatchesReady(items);
}

void DbWorker::requestTecanPlannerSummary(const QDate& today)
{
    if (!ready_) {
        emit tecanPlannerSummaryFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit tecanPlannerSummaryFailed("Database connection is not open.");
        return;
    }

    TecanPlannerRepository repo(db);
    QString err;
    int batchCount = 0;
    int compoundCount = 0;
    repo.loadDateSummary(today, &batchCount, &compoundCount, &err);
    if (!err.isEmpty()) {
        emit tecanPlannerSummaryFailed(err);
        return;
    }

    emit tecanPlannerSummaryReady(batchCount, compoundCount);
}

void DbWorker::syncDoneStatuses()
{
    if (!ready_) {
        emit syncDoneStatusesFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit syncDoneStatusesFailed("Database connection is not open.");
        return;
    }

    DoneSyncRepository repo(db);
    QString err;
    int compUpd = 0;
    int batchUpd = 0;

    if (!repo.syncDoneFromTestRequests(&compUpd, &batchUpd, &err)) {
        emit syncDoneStatusesFailed(err.isEmpty() ? "Sync failed." : err);
        return;
    }

    emit syncDoneStatusesDone(compUpd, batchUpd);
}

void DbWorker::importExcel(const QString& filePath, const QString& createdByUser)
{
    if (!ready_) {
        emit importExcelFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit importExcelFailed("Database connection is not open.");
        return;
    }

    ExcelImportRepository repo(db);

    QString info, err;
    if (!repo.importFile(filePath, createdByUser, &info, &err)) {
        emit importExcelFailed(err.isEmpty() ? "Excel import failed." : err);
        return;
    }

    emit importExcelFinished(info.isEmpty() ? "Import finished." : info);
}

void DbWorker::requestDoneDashboardSnapshot()
{
    if (!ready_) {
        emit doneDashboardSnapshotFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit doneDashboardSnapshotFailed("Database connection is not open.");
        return;
    }

    DashboardRepository repo(db);
    QString err;
    const auto snapshot = repo.loadDoneDashboardSnapshot(&err);
    if (!err.isEmpty()) {
        emit doneDashboardSnapshotFailed(err);
        return;
    }

    emit doneDashboardSnapshotReady(snapshot);
}

void DbWorker::requestArchiveDashboardSnapshot()
{
    if (!ready_) {
        emit archiveDashboardSnapshotFailed(lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit archiveDashboardSnapshotFailed("Database connection is not open.");
        return;
    }

    DashboardRepository repo(db);
    QString err;
    const auto snapshot = repo.loadArchiveDashboardSnapshot(&err);
    if (!err.isEmpty()) {
        emit archiveDashboardSnapshotFailed(err);
        return;
    }

    emit archiveDashboardSnapshotReady(snapshot);
}

void DbWorker::publishReports(const QString& batchId, const QString& pdfPath, const QString& htmlPath)
{
    if (!ready_) {
        emit publishReportsFinished(false, lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit publishReportsFinished(false, "Database connection is not open.");
        return;
    }

    // Read PDF file (if specified)
    QByteArray pdfData;
    QString pdfName;
    if (!pdfPath.isEmpty()) {
        QFile file(pdfPath);
        if (!file.open(QIODevice::ReadOnly)) {
            emit publishReportsFinished(false, "Failed to open PDF file: " + file.errorString());
            return;
        }
        pdfData = file.readAll();
        pdfName = QFileInfo(pdfPath).fileName();
        file.close();
    }

    // Read HTML file (if specified)
    QString htmlData;
    QString htmlName;
    if (!htmlPath.isEmpty()) {
        QFile file(htmlPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            emit publishReportsFinished(false, "Failed to open HTML file: " + file.errorString());
            return;
        }
        htmlData = QString::fromUtf8(file.readAll());
        htmlName = QFileInfo(htmlPath).fileName();
        file.close();
    }

    if (pdfName.isEmpty() && htmlName.isEmpty()) {
        emit publishReportsFinished(false, "No files specified for publication.");
        return;
    }

    // Now insert or update in public.batch_reports
    QSqlQuery q(db);
    q.prepare(R"SQL(
        INSERT INTO public.batch_reports (
            batch_id, 
            report_pdf_name, 
            report_pdf_data, 
            report_html_name, 
            report_html_data,
            created_at
        )
        VALUES (
            :batch_id::uuid, 
            NULLIF(:pdf_name, ''), 
            :pdf_data, 
            NULLIF(:html_name, ''), 
            NULLIF(:html_data, ''),
            now()
        )
        ON CONFLICT (batch_id) DO UPDATE SET
            report_pdf_name = COALESCE(NULLIF(EXCLUDED.report_pdf_name, ''), public.batch_reports.report_pdf_name),
            report_pdf_data = COALESCE(EXCLUDED.report_pdf_data, public.batch_reports.report_pdf_data),
            report_html_name = COALESCE(NULLIF(EXCLUDED.report_html_name, ''), public.batch_reports.report_html_name),
            report_html_data = COALESCE(NULLIF(EXCLUDED.report_html_data, ''), public.batch_reports.report_html_data),
            created_at = now();
    )SQL");

    q.bindValue(":batch_id", batchId);
    q.bindValue(":pdf_name", pdfName);
    q.bindValue(":pdf_data", pdfData.isEmpty() ? QVariant(QMetaType(QMetaType::QByteArray)) : QVariant(pdfData));
    q.bindValue(":html_name", htmlName);
    q.bindValue(":html_data", htmlData.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : QVariant(htmlData));

    if (!q.exec()) {
        emit publishReportsFinished(false, "Database query failed: " + q.lastError().text());
        return;
    }

    emit publishReportsFinished(true, QString{});
}

void DbWorker::publishResults(const QString& batchId, const QString& pdfPath, const QVariantMap& analysisResults, const QVariantList& rawPlates)
{
    if (!ready_) {
        emit publishReportsFinished(false, lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit publishReportsFinished(false, "Database connection is not open.");
        return;
    }

    // Read PDF file (mandatory for this workflow)
    QByteArray pdfData;
    QString pdfName;
    if (!pdfPath.isEmpty()) {
        QFile file(pdfPath);
        if (!file.open(QIODevice::ReadOnly)) {
            emit publishReportsFinished(false, "Failed to open PDF file: " + file.errorString());
            return;
        }
        pdfData = file.readAll();
        pdfName = QFileInfo(pdfPath).fileName();
        file.close();
    } else {
        emit publishReportsFinished(false, "PDF report file is required for publishing results.");
        return;
    }

    // Begin transaction
    if (!db.transaction()) {
        emit publishReportsFinished(false, "Failed to begin database transaction.");
        return;
    }

    QSqlQuery q(db);

    // Ensure tables exist
    bool ok = q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS public.compound_efficacies (
            efficacy_id integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
            batch_id uuid NOT NULL REFERENCES public.tracked_test_batches(batch_id) ON DELETE CASCADE,
            compound_name character varying(255) NOT NULL,
            dose double precision NOT NULL,
            dose_unit character varying(10) NOT NULL,
            efficacy double precision NOT NULL,
            is_standard boolean DEFAULT false,
            is_qc boolean DEFAULT false,
            created_at timestamp without time zone DEFAULT CURRENT_TIMESTAMP
        );
    )SQL");
    if (!ok) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to create compound_efficacies table: " + q.lastError().text());
        return;
    }

    ok = q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS public.batch_raw_plates (
            plate_id integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
            batch_id uuid NOT NULL REFERENCES public.tracked_test_batches(batch_id) ON DELETE CASCADE,
            plate_name character varying(255) NOT NULL,
            csv_name character varying(255) NOT NULL,
            csv_data bytea NOT NULL,
            created_at timestamp without time zone DEFAULT CURRENT_TIMESTAMP
        );
    )SQL");
    if (!ok) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to create batch_raw_plates table: " + q.lastError().text());
        return;
    }

    ok = q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS public.batch_analysis_results (
            batch_id uuid PRIMARY KEY REFERENCES public.tracked_test_batches(batch_id) ON DELETE CASCADE,
            analysis_json jsonb NOT NULL,
            created_at timestamp without time zone DEFAULT CURRENT_TIMESTAMP
        );
    )SQL");
    if (!ok) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to create batch_analysis_results table: " + q.lastError().text());
        return;
    }

    // 1) Clear old efficacies
    q.prepare("DELETE FROM public.compound_efficacies WHERE batch_id = :batch_id::uuid");
    q.bindValue(":batch_id", batchId);
    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to clear old compound efficacies: " + q.lastError().text());
        return;
    }

    // 2) Insert new compound efficacies
    QVariantList efficacies = analysisResults.value("compounds").toList();
    for (const QVariant &v : efficacies) {
        QVariantMap cmp = v.toMap();
        QString compName = cmp.value("compound_name").toString();
        bool isStd = cmp.value("is_standard").toBool();
        bool isQc = cmp.value("is_qc").toBool();
        QVariantList drList = cmp.value("dose_responses").toList();

        for (const QVariant &drVal : drList) {
            QVariantMap dr = drVal.toMap();
            double dose = dr.value("dose").toDouble();
            double efficacy = dr.value("efficacy").toDouble();

            q.prepare(R"SQL(
                INSERT INTO public.compound_efficacies (batch_id, compound_name, dose, dose_unit, efficacy, is_standard, is_qc, created_at)
                VALUES (:batch_id::uuid, :compound_name, :dose, 'uM', :efficacy, :is_standard, :is_qc, now())
            )SQL");
            q.bindValue(":batch_id", batchId);
            q.bindValue(":compound_name", compName);
            q.bindValue(":dose", dose);
            q.bindValue(":efficacy", efficacy);
            q.bindValue(":is_standard", isStd);
            q.bindValue(":is_qc", isQc);

            if (!q.exec()) {
                db.rollback();
                emit publishReportsFinished(false, "Failed to insert compound efficacy record: " + q.lastError().text());
                return;
            }
        }
    }

    // 3) Clear old raw CSV plates
    q.prepare("DELETE FROM public.batch_raw_plates WHERE batch_id = :batch_id::uuid");
    q.bindValue(":batch_id", batchId);
    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to clear old raw CSV plates: " + q.lastError().text());
        return;
    }

    // 4) Insert raw CSV plates
    for (const QVariant &v : rawPlates) {
        QVariantMap pm = v.toMap();
        QString plateName = pm.value("plate_name").toString();
        QString csvName = pm.value("csv_name").toString();
        QString csvPath = pm.value("csv_path").toString();

        QFile csvFile(csvPath);
        if (!csvFile.open(QIODevice::ReadOnly)) {
            db.rollback();
            emit publishReportsFinished(false, "Failed to read raw test plate CSV: " + csvFile.errorString());
            return;
        }
        QByteArray csvData = csvFile.readAll();
        csvFile.close();

        q.prepare(R"SQL(
            INSERT INTO public.batch_raw_plates (batch_id, plate_name, csv_name, csv_data, created_at)
            VALUES (:batch_id::uuid, :plate_name, :csv_name, :csv_data, now())
        )SQL");
        q.bindValue(":batch_id", batchId);
        q.bindValue(":plate_name", plateName);
        q.bindValue(":csv_name", csvName);
        q.bindValue(":csv_data", csvData);

        if (!q.exec()) {
            db.rollback();
            emit publishReportsFinished(false, "Failed to insert raw plate CSV data: " + q.lastError().text());
            return;
        }
    }

    // Save batch analysis results JSON
    q.prepare("DELETE FROM public.batch_analysis_results WHERE batch_id = :batch_id::uuid");
    q.bindValue(":batch_id", batchId);
    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to clear old batch analysis results: " + q.lastError().text());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromVariant(analysisResults);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    QString jsonStr = QString::fromUtf8(jsonBytes);

    q.prepare(R"SQL(
        INSERT INTO public.batch_analysis_results (batch_id, analysis_json)
        VALUES (:batch_id::uuid, :analysis_json::jsonb)
    )SQL");
    q.bindValue(":batch_id", batchId);
    q.bindValue(":analysis_json", jsonStr);
    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to insert batch analysis results JSON: " + q.lastError().text());
        return;
    }

    // 5) Insert/Update PDF report in public.batch_reports (omit HTML, as requested)
    q.prepare(R"SQL(
        INSERT INTO public.batch_reports (
            batch_id, 
            report_pdf_name, 
            report_pdf_data, 
            report_html_name, 
            report_html_data,
            created_at
        )
        VALUES (
            :batch_id::uuid, 
            NULLIF(:pdf_name, ''), 
            :pdf_data, 
            NULL, 
            NULL,
            now()
        )
        ON CONFLICT (batch_id) DO UPDATE SET
            report_pdf_name = EXCLUDED.report_pdf_name,
            report_pdf_data = EXCLUDED.report_pdf_data,
            report_html_name = NULL,
            report_html_data = NULL,
            created_at = now();
    )SQL");
    q.bindValue(":batch_id", batchId);
    q.bindValue(":pdf_name", pdfName);
    q.bindValue(":pdf_data", pdfData);

    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to save PDF report in batch_reports: " + q.lastError().text());
        return;
    }

    // 6) Update tracked_test_batches status to done
    q.prepare(R"SQL(
        UPDATE public.tracked_test_batches 
        SET status = 'done', done_at = now() 
        WHERE batch_id = :batch_id::uuid
    )SQL");
    q.bindValue(":batch_id", batchId);

    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to update batch status to done: " + q.lastError().text());
        return;
    }

    // Commit transaction
    if (!db.commit()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to commit database transaction.");
        return;
    }

    emit publishReportsFinished(true, QString{});
}

void DbWorker::rejectAndRedoBatch(const QString& batchId)
{
    if (!ready_) {
        emit publishReportsFinished(false, lastError_.isEmpty() ? "Database not ready." : lastError_);
        return;
    }

    QSqlDatabase db = DatabaseManager::db(connectionName_);
    if (!db.isValid() || !db.isOpen()) {
        emit publishReportsFinished(false, "Database connection is not open.");
        return;
    }

    if (!db.transaction()) {
        emit publishReportsFinished(false, "Failed to begin database transaction.");
        return;
    }

    QSqlQuery q(db);

    // Delete batch analysis results
    q.prepare("DELETE FROM public.batch_analysis_results WHERE batch_id = :batch_id::uuid");
    q.bindValue(":batch_id", batchId);
    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to clear batch analysis results: " + q.lastError().text());
        return;
    }

    // 1) Delete compound efficacies
    q.prepare("DELETE FROM public.compound_efficacies WHERE batch_id = :batch_id::uuid");
    q.bindValue(":batch_id", batchId);
    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to clear compound efficacies: " + q.lastError().text());
        return;
    }

    // 2) Delete raw CSV plates
    q.prepare("DELETE FROM public.batch_raw_plates WHERE batch_id = :batch_id::uuid");
    q.bindValue(":batch_id", batchId);
    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to clear raw CSV plates: " + q.lastError().text());
        return;
    }

    // 3) Delete PDF reports
    q.prepare("DELETE FROM public.batch_reports WHERE batch_id = :batch_id::uuid");
    q.bindValue(":batch_id", batchId);
    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to clear PDF reports: " + q.lastError().text());
        return;
    }

    // 4) Set status to sent, done_at to NULL
    q.prepare("UPDATE public.tracked_test_batches SET status = 'sent', done_at = NULL WHERE batch_id = :batch_id::uuid");
    q.bindValue(":batch_id", batchId);
    if (!q.exec()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to update batch status: " + q.lastError().text());
        return;
    }

    if (!db.commit()) {
        db.rollback();
        emit publishReportsFinished(false, "Failed to commit database transaction.");
        return;
    }

    emit publishReportsFinished(true, QString{});
}

void DbWorker::shutdown()
{
    if (!connectionName_.isEmpty()) {
        DatabaseManager::closeConnection(connectionName_);
        connectionName_.clear();
    }
    ready_ = false;
}
