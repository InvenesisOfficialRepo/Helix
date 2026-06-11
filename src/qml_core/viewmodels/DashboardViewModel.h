#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

#include "domain/DashboardDtos.h"

class DbWorker;
class TestCardModel;

class DashboardViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TestCardModel* testCardsModel READ testCardsModel CONSTANT)
    Q_PROPERTY(TestCardModel* doneTestCardsModel READ doneTestCardsModel CONSTANT)
    Q_PROPERTY(TestCardModel* archiveTestCardsModel READ archiveTestCardsModel CONSTANT)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit DashboardViewModel(QObject* parent = nullptr);

    void setDbWorker(DbWorker* worker);

    TestCardModel* testCardsModel() const;
    TestCardModel* doneTestCardsModel() const;
    TestCardModel* archiveTestCardsModel() const;
    bool isBusy() const;
    QString errorMessage() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshDone();
    Q_INVOKABLE void openBatch(const QString& batchId);

    // NEW: Excel import entry point used by QML
    Q_INVOKABLE void importRequestExcel(const QUrl& fileUrl, const QString& createdByUser);

signals:
    void busyChanged();
    void errorMessageChanged();

    void openBatchRequested(const QString& batchId);

    // UI -> worker (queued)
    void requestDashboardSnapshot();
    void requestDoneDashboardSnapshot();
    void requestArchiveDashboardSnapshot();

    // NEW: UI -> worker (queued)
    void requestImportExcel(const QString& filePath, const QString& createdByUser);

private slots:
    void onDashboardSnapshotReady(const QVector<TestCardDto>& snapshot);
    void onDashboardSnapshotFailed(const QString& error);

    void onDoneDashboardSnapshotReady(const QVector<TestCardDto>& snapshot);
    void onDoneDashboardSnapshotFailed(const QString& error);

    void onArchiveDashboardSnapshotReady(const QVector<TestCardDto>& snapshot);
    void onArchiveDashboardSnapshotFailed(const QString& error);

    // NEW: worker -> UI (queued)
    void onImportExcelFinished(const QString& message);
    void onImportExcelFailed(const QString& error);

private:
    void setBusy(bool v);
    void setError(const QString& msg);

    QPointer<DbWorker> worker_;
    TestCardModel* model_ = nullptr;
    TestCardModel* doneModel_ = nullptr;
    TestCardModel* archiveModel_ = nullptr;

    bool isBusy_ = false;
    QString errorMessage_;
    int pendingSnapshots_ = 0;
};
