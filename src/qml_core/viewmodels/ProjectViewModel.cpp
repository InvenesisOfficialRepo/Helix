#include "ProjectViewModel.h"

#include <QDebug>
#include <QUrl>

#include "db/DbWorker.h"
#include "domain/BatchDetailDtos.h"
#include "domain/Status.h"
#include "services/AnalysisPipeline.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>

ProjectViewModel::ProjectViewModel(QObject *parent)
    : QObject(parent)
{
    compounds_ = new CompoundListModel(this);
}

void ProjectViewModel::setDbWorker(DbWorker* worker)
{
    if (worker_ == worker) return;
    worker_ = worker;
    if (!worker_) return;

    // UI -> worker
    connect(this, &ProjectViewModel::requestBatchDetail,
            worker_, &DbWorker::requestBatchDetail,
            Qt::QueuedConnection);

    connect(this, &ProjectViewModel::requestSendSelected,
            worker_, &DbWorker::sendSelectedCompounds,
            Qt::QueuedConnection);

    connect(this, &ProjectViewModel::requestPublishReports,
            worker_, &DbWorker::publishReports,
            Qt::QueuedConnection);

    connect(this, &ProjectViewModel::requestPublishResults,
            worker_, &DbWorker::publishResults,
            Qt::QueuedConnection);

    connect(this, &ProjectViewModel::requestRejectAndRedoBatch,
            worker_, &DbWorker::rejectAndRedoBatch,
            Qt::QueuedConnection);

    // worker -> UI
    connect(worker_, &DbWorker::batchDetailReady,
            this, &ProjectViewModel::onBatchDetailReady,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::batchDetailFailed,
            this, &ProjectViewModel::onBatchDetailFailed,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::sendSelectedDone,
            this, &ProjectViewModel::onSendDone,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::sendSelectedFailed,
            this, &ProjectViewModel::onSendFailed,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::publishReportsFinished,
            this, &ProjectViewModel::onPublishFinished,
            Qt::QueuedConnection);
}

QString ProjectViewModel::batchId() const { return batchId_; }
QString ProjectViewModel::projectCode() const { return projectCode_; }
QString ProjectViewModel::testCode() const { return testCode_; }
QString ProjectViewModel::overallStatus() const { return overallStatus_; }
QDate ProjectViewModel::scheduledFor() const { return scheduledFor_; }
QString ProjectViewModel::createdBy() const { return createdBy_; }
CompoundListModel* ProjectViewModel::compoundsModel() const { return compounds_; }
QString ProjectViewModel::errorMessage() const { return errorMessage_; }
bool ProjectViewModel::busy() const { return busy_; }

void ProjectViewModel::setBusy(bool v)
{
    if (busy_ == v) return;
    busy_ = v;
    emit busyChanged();
}

void ProjectViewModel::setError(const QString& msg)
{
    if (errorMessage_ == msg) return;
    errorMessage_ = msg;
    emit errorMessageChanged();
}

void ProjectViewModel::loadBatch(const QString& batchId)
{
    batchId_ = batchId;
    emit batchChanged();

    // NEW: clear fields while loading (optional)
    createdBy_.clear();
    emit createdByChanged();

    // Clear analysis results
    analysisResults_ = QVariantMap();
    hasResults_ = false;
    hasReport_ = false;
    emit analysisResultsChanged();
    emit hasResultsChanged();

    if (!worker_) {
        setError("DB worker not configured.");
        return;
    }

    setError(QString{});
    setBusy(true);
    compounds_->setItems({}); // clear old
    emit requestBatchDetail(batchId_);
}

void ProjectViewModel::onBatchDetailReady(const BatchDetailDto& dto)
{
    // Populate header info
    batchId_ = dto.batchId;
    projectCode_ = dto.projectCode;
    testCode_ = dto.testCode;
    overallStatus_ = statusToText(dto.status);
    scheduledFor_ = dto.scheduledFor;

    // NEW: created by
    if (createdBy_ != dto.createdBy) {
        createdBy_ = dto.createdBy;
        emit createdByChanged();
    }

    hasReport_ = dto.hasReport;

    // Populate compounds
    QVector<CompoundItem> items;
    items.reserve(dto.compounds.size());
    for (const auto& c : dto.compounds) {
        CompoundItem it;
        it.trackedCompoundId = c.trackedCompoundId;
        it.compoundName = c.compoundName;
        it.status = c.status;
        it.selected = false;
        it.hasSolution = c.hasSolution;
        items.push_back(std::move(it));
    }
    compounds_->setItems(std::move(items));

    if (!dto.analysisResults.isEmpty()) {
        analysisResults_ = dto.analysisResults;
        hasResults_ = true;
    } else {
        analysisResults_ = QVariantMap();
        hasResults_ = false;
    }
    emit analysisResultsChanged();
    emit hasResultsChanged();

    setBusy(false);
    emit batchChanged();
}

void ProjectViewModel::onBatchDetailFailed(const QString& error)
{
    setBusy(false);
    setError(error.isEmpty() ? "Failed to load batch." : error);
}

void ProjectViewModel::selectAllPending()
{
    compounds_->selectAllPending();
}

void ProjectViewModel::sendSelected(const QDate &date)
{
    if (!worker_) {
        setError("DB worker not configured.");
        return;
    }
    if (!date.isValid()) {
        setError("Pick a valid date.");
        return;
    }

    const QStringList ids = compounds_->selectedIds();
    if (ids.isEmpty()) {
        setError("No pending compounds selected.");
        return;
    }

    setError(QString{});
    setBusy(true);

    emit requestSendSelected(batchId_, ids, date);
}

void ProjectViewModel::onSendDone()
{
    // refresh dashboard counts in background
    emit dashboardRefreshRequested();
    // After send, reload detail from DB (so status/scheduled_for reflect reality)
    emit requestBatchDetail(batchId_);
}

void ProjectViewModel::onSendFailed(const QString& error)
{
    setBusy(false);
    setError(error.isEmpty() ? "Send failed." : error);
}

void ProjectViewModel::publishReports(const QUrl& pdfUrl, const QUrl& htmlUrl)
{
    if (!worker_) {
        setError("DB worker not configured.");
        return;
    }
    if (batchId_.isEmpty()) {
        setError("No batch loaded.");
        return;
    }

    const QString pdfPath = pdfUrl.toLocalFile();
    const QString htmlPath = htmlUrl.toLocalFile();

    if (pdfPath.isEmpty() && htmlPath.isEmpty()) {
        setError("Please select at least one report file to publish.");
        return;
    }

    setError(QString{});
    setBusy(true);

    emit requestPublishReports(batchId_, pdfPath, htmlPath);
}

void ProjectViewModel::onPublishFinished(bool success, const QString& error)
{
    setBusy(false);
    if (!success) {
        setError(error.isEmpty() ? "Failed to publish reports." : error);
    } else {
        setError(QString{});
        emit dashboardRefreshRequested();
    }
    emit publishFinished(success, error);
}

QVariantMap ProjectViewModel::analysisResults() const { return analysisResults_; }
bool ProjectViewModel::hasResults() const { return hasResults_; }
bool ProjectViewModel::hasReport() const { return hasReport_; }

void ProjectViewModel::runAnalysis(const QVariantList& layoutCsvs, const QVariantList& rawDataCsvs, const QUrl& expJson, const QString& timepoint, const QVariantList& feedingDataCsvs)
{
    setError("");
    setBusy(true);

    AnalysisPipeline pipeline;
    QVariantMap res = pipeline.runAnalysis(layoutCsvs, rawDataCsvs, expJson, timepoint, batchId_, feedingDataCsvs);

    setBusy(false);
    if (res.value("success").toBool()) {
        analysisResults_ = res;
        hasResults_ = true;
        emit analysisResultsChanged();
        emit hasResultsChanged();
    } else {
        setError(res.value("error").toString());
    }
}

void ProjectViewModel::publishResults(const QUrl& pdfUrl, const QVariantList& rawPlateUrls)
{
    if (!worker_) {
        setError("DB worker not configured.");
        return;
    }
    if (batchId_.isEmpty()) {
        setError("No batch loaded.");
        return;
    }
    if (!hasResults_) {
        setError("No analysis results available to publish.");
        return;
    }

    const QString pdfPath = pdfUrl.toLocalFile();
    if (pdfPath.isEmpty()) {
        setError("Please select a PDF report file to publish.");
        return;
    }

    QVariantList rawPlates;
    for (int i = 0; i < rawPlateUrls.size(); ++i) {
        QUrl url = rawPlateUrls[i].toUrl();
        QString localPath = url.toLocalFile();
        if (!localPath.isEmpty()) {
            QVariantMap pm;
            pm["plate_name"] = QString("Test Plate %1").arg(i + 1);
            pm["csv_name"] = QFileInfo(localPath).fileName();
            pm["csv_path"] = localPath;
            rawPlates.append(pm);
        }
    }

    setError("");
    setBusy(true);

    emit requestPublishResults(batchId_, pdfPath, analysisResults_, rawPlates);
}

void ProjectViewModel::rejectAndRedoBatch()
{
    if (!worker_) {
        setError("DB worker not configured.");
        return;
    }
    if (batchId_.isEmpty()) {
        setError("No batch loaded.");
        return;
    }

    setError("");
    setBusy(true);

    emit requestRejectAndRedoBatch(batchId_);
}

bool ProjectViewModel::exportAnalysis(const QUrl& folderUrl)
{
    QString folderPath = folderUrl.toLocalFile();
    if (folderPath.isEmpty() || analysisResults_.isEmpty()) {
        return false;
    }

    QDir dir(folderPath);
    if (!dir.exists()) {
        if (!dir.mkpath(folderPath)) {
            return false;
        }
    }

    // 1. Copy the merged plate CSVs
    QVariantList mergedFiles = analysisResults_.value("merged_files").toList();
    for (const QVariant& f : mergedFiles) {
        QString srcPath;
        if (f.typeId() == QMetaType::QUrl) {
            srcPath = f.toUrl().toLocalFile();
        } else {
            QString str = f.toString();
            if (str.startsWith("file:///")) {
                srcPath = QUrl(str).toLocalFile();
            } else {
                srcPath = str;
            }
        }
        if (QFile::exists(srcPath)) {
            QString destName = QFileInfo(srcPath).fileName();
            QString destPath = dir.filePath(destName);
            if (QFile::exists(destPath)) {
                QFile::remove(destPath);
            }
            QFile::copy(srcPath, destPath);
        }
    }

    // 2. Write analysis_summary.csv
    QString summaryPath = dir.filePath("analysis_summary.csv");
    QFile summaryFile(summaryPath);
    if (summaryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&summaryFile);
        out << "Compound,Type,EC50,EC80,Status\n";
        QVariantList compounds = analysisResults_.value("compounds").toList();
        for (const QVariant& cVal : compounds) {
            QVariantMap c = cVal.toMap();
            QString name = c.value("compound_name").toString();
            QString type = c.value("is_standard").toBool() ? "Standard" : (c.value("is_qc").toBool() ? "QC" : "Test");
            double ec50 = c.value("ec50").toDouble();
            double ec80 = c.value("ec80").toDouble();
            QString status = c.value("status").toString();

            out << name << ","
                << type << ","
                << (ec50 > 0 ? QString::number(ec50, 'f', 6) : "N/A") << ","
                << (ec80 > 0 ? QString::number(ec80, 'f', 6) : "N/A") << ","
                << status << "\n";
        }
        summaryFile.close();
    } else {
        return false;
    }

    // 3. Write analysis_data.json
    QString jsonPath = dir.filePath("analysis_data.json");
    QFile jsonFile(jsonPath);
    if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc = QJsonDocument::fromVariant(analysisResults_);
        jsonFile.write(doc.toJson(QJsonDocument::Indented));
        jsonFile.close();
    } else {
        return false;
    }

    return true;
}


