#pragma once

#include <QObject>
#include <QDate>

#include "domain/BatchDetailDtos.h"
#include "models/CompoundListModel.h"

class DbWorker;

class ProjectViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString batchId READ batchId NOTIFY batchChanged)
    Q_PROPERTY(QString projectCode READ projectCode NOTIFY batchChanged)
    Q_PROPERTY(QString testCode READ testCode NOTIFY batchChanged)
    Q_PROPERTY(QString overallStatus READ overallStatus NOTIFY batchChanged)
    Q_PROPERTY(QDate scheduledFor READ scheduledFor NOTIFY batchChanged)
    Q_PROPERTY(QString createdBy READ createdBy NOTIFY createdByChanged)
    Q_PROPERTY(CompoundListModel* compoundsModel READ compoundsModel CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QVariantMap analysisResults READ analysisResults NOTIFY analysisResultsChanged)
    Q_PROPERTY(bool hasResults READ hasResults NOTIFY hasResultsChanged)
    Q_PROPERTY(bool hasReport READ hasReport NOTIFY batchChanged)
    Q_PROPERTY(int estimatedTestPlates READ estimatedTestPlates NOTIFY estimatedTestPlatesChanged)

public:
    explicit ProjectViewModel(QObject *parent = nullptr);

    void setDbWorker(DbWorker* worker);

    QString batchId() const;
    QString projectCode() const;
    QString testCode() const;
    QString overallStatus() const;
    QDate scheduledFor() const;
    QString createdBy() const;

    CompoundListModel* compoundsModel() const;
    QString errorMessage() const;
    bool busy() const;

    QVariantMap analysisResults() const;
    bool hasResults() const;
    bool hasReport() const;
    int estimatedTestPlates() const;

    Q_INVOKABLE void loadBatch(const QString& batchId);
    Q_INVOKABLE void selectAllPending();
    Q_INVOKABLE void sendSelected(const QDate &date);
    Q_INVOKABLE void publishReports(const QUrl& pdfUrl, const QUrl& htmlUrl);
    Q_INVOKABLE void runAnalysis(const QVariantList& layoutCsvs, const QVariantList& rawDataCsvs, const QUrl& expJson, const QString& timepoint, const QVariantList& feedingDataCsvs = QVariantList());
    Q_INVOKABLE void publishResults(const QUrl& pdfUrl, const QVariantList& rawPlateUrls);
    Q_INVOKABLE void rejectAndRedoBatch();
    Q_INVOKABLE bool exportAnalysis(const QUrl& folderUrl);

signals:
    void batchChanged();
    void createdByChanged();
    void errorMessageChanged();
    void busyChanged();
    void publishFinished(bool success, const QString& errorMsg);
    void analysisResultsChanged();
    void hasResultsChanged();
    void estimatedTestPlatesChanged();

    // UI -> worker
    void requestBatchDetail(const QString& batchId);
    void requestSendSelected(const QString& batchId, const QStringList& compoundIds, const QDate& date);
    void requestPublishReports(const QString& batchId, const QString& pdfPath, const QString& htmlPath);
    void requestPublishResults(const QString& batchId, const QString& pdfPath, const QVariantMap& analysisResults, const QVariantList& rawPlates);
    void requestRejectAndRedoBatch(const QString& batchId);

    // NEW: ask AppContext to refresh dashboard (UI thread)
    void dashboardRefreshRequested();

private slots:
    void onBatchDetailReady(const BatchDetailDto& dto);
    void onBatchDetailFailed(const QString& error);
    void onSendDone();
    void onSendFailed(const QString& error);
    void onPublishFinished(bool success, const QString& error);
    void calculateEstimatedTestPlates();

private:
    void setBusy(bool v);
    void setError(const QString& msg);

    DbWorker* worker_ = nullptr;

    QString batchId_;
    QString projectCode_;
    QString testCode_;
    QString overallStatus_;
    QDate scheduledFor_;
    QString createdBy_;
    CompoundListModel* compounds_ = nullptr;
    QString errorMessage_;
    bool busy_ = false;

    QVariantMap analysisResults_;
    bool hasResults_ = false;
    bool hasReport_ = false;
    int estimatedTestPlates_ = 0;
};

