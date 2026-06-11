#include "DashboardViewModel.h"

#include <QFileInfo>

#include "db/DbWorker.h"
#include "models/TestCardModel.h"
#include "models/ProjectModel.h"

DashboardViewModel::DashboardViewModel(QObject *parent)
    : QObject(parent)
{
    model_ = new TestCardModel(this);
    doneModel_ = new TestCardModel(this);
    archiveModel_ = new TestCardModel(this);
}

void DashboardViewModel::setDbWorker(DbWorker* worker)
{
    if (worker_ == worker) return;

    worker_ = worker;
    if (!worker_) return;

    // UI -> worker (queued)
    connect(this, &DashboardViewModel::requestDashboardSnapshot,
            worker_, &DbWorker::requestDashboardSnapshot,
            Qt::QueuedConnection);

    connect(this, &DashboardViewModel::requestDoneDashboardSnapshot,
            worker_, &DbWorker::requestDoneDashboardSnapshot,
            Qt::QueuedConnection);

    connect(this, &DashboardViewModel::requestArchiveDashboardSnapshot,
            worker_, &DbWorker::requestArchiveDashboardSnapshot,
            Qt::QueuedConnection);

    connect(this, &DashboardViewModel::requestImportExcel,
            worker_, &DbWorker::importExcel,
            Qt::QueuedConnection);

    // worker -> UI (queued)
    connect(worker_, &DbWorker::dashboardSnapshotReady,
            this, &DashboardViewModel::onDashboardSnapshotReady,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::dashboardSnapshotFailed,
            this, &DashboardViewModel::onDashboardSnapshotFailed,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::doneDashboardSnapshotReady,
            this, &DashboardViewModel::onDoneDashboardSnapshotReady,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::doneDashboardSnapshotFailed,
            this, &DashboardViewModel::onDoneDashboardSnapshotFailed,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::archiveDashboardSnapshotReady,
            this, &DashboardViewModel::onArchiveDashboardSnapshotReady,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::archiveDashboardSnapshotFailed,
            this, &DashboardViewModel::onArchiveDashboardSnapshotFailed,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::importExcelFinished,
            this, &DashboardViewModel::onImportExcelFinished,
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::importExcelFailed,
            this, &DashboardViewModel::onImportExcelFailed,
            Qt::QueuedConnection);
}

TestCardModel* DashboardViewModel::testCardsModel() const { return model_; }
TestCardModel* DashboardViewModel::doneTestCardsModel() const { return doneModel_; }
TestCardModel* DashboardViewModel::archiveTestCardsModel() const { return archiveModel_; }
QString DashboardViewModel::errorMessage() const { return errorMessage_; }
bool DashboardViewModel::isBusy() const { return isBusy_; }

void DashboardViewModel::setBusy(bool v)
{
    if (isBusy_ == v) return;
    isBusy_ = v;
    emit busyChanged();
}

void DashboardViewModel::setError(const QString& msg)
{
    if (errorMessage_ == msg) return;
    errorMessage_ = msg;
    emit errorMessageChanged();
}

void DashboardViewModel::refresh()
{
    if (!worker_) {
        setError("DB worker not configured.");
        return;
    }
    if (isBusy_) return;

    setError(QString{});
    setBusy(true);
    pendingSnapshots_ = 1;
    emit requestDashboardSnapshot();
}

void DashboardViewModel::onDashboardSnapshotReady(const QVector<TestCardDto>& snapshot)
{
    QVector<TestCardItem> cards;
    cards.reserve(snapshot.size());

    for (const auto& c : snapshot) {
        auto projects = new ProjectModel(model_);
        QVector<ProjectItem> rows;
        rows.reserve(c.batches.size());

        for (const auto& b : c.batches) {
            ProjectItem r;
            r.batchId = b.batchId;
            r.projectCode = b.projectCode;
            r.testCode = b.testCode;
            r.status = b.status;
            r.scheduledFor = b.scheduledFor;
            r.createdAt = b.createdAt;
            r.pendingCount = b.pendingCount;
            r.sentCount = b.sentCount;
            r.hasReport = b.hasReport;
            rows.push_back(std::move(r));
        }
        projects->setItems(std::move(rows));

        TestCardItem item;
        item.testCode = c.testCode;
        item.pendingCount = c.pendingCount;
        item.sentCount = c.sentCount;
        item.projectsModel = projects;

        cards.push_back(std::move(item));
    }

    model_->setItems(std::move(cards));
    pendingSnapshots_--;
    if (pendingSnapshots_ <= 0) {
        setBusy(false);
    }
}

void DashboardViewModel::onDashboardSnapshotFailed(const QString& error)
{
    setError(error.isEmpty() ? "Failed to load dashboard." : error);
    pendingSnapshots_--;
    if (pendingSnapshots_ <= 0) {
        setBusy(false);
    }
}

void DashboardViewModel::openBatch(const QString& batchId)
{
    emit openBatchRequested(batchId);
}

void DashboardViewModel::importRequestExcel(const QUrl& fileUrl, const QString& createdByUser)
{
    if (!worker_) { setError("DB worker not configured."); return; }
    if (isBusy_) return;

    const QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) { setError("Invalid file URL."); return; }
    if (!QFileInfo::exists(path)) { setError("File not found: " + path); return; }
    if (!path.toLower().endsWith(".xlsx")) { setError("Please select an .xlsx file."); return; }

    setError(QString{});
    setBusy(true);

    emit requestImportExcel(path, createdByUser.trimmed());
}

void DashboardViewModel::onImportExcelFinished(const QString& message)
{
    Q_UNUSED(message);
    setBusy(false);
    // refresh to show new batches/cards
    refresh();
}

void DashboardViewModel::onImportExcelFailed(const QString& error)
{
    setError(error.isEmpty() ? "Excel import failed." : error);
    setBusy(false);
}

void DashboardViewModel::refreshDone()
{
    if (!worker_) {
        setError("DB worker not configured.");
        return;
    }
    if (isBusy_) return;

    setError(QString{});
    setBusy(true);
    pendingSnapshots_ = 2;
    emit requestDoneDashboardSnapshot();
    emit requestArchiveDashboardSnapshot();
}

void DashboardViewModel::onDoneDashboardSnapshotReady(const QVector<TestCardDto>& snapshot)
{
    QVector<TestCardItem> cards;
    cards.reserve(snapshot.size());

    for (const auto& c : snapshot) {
        auto projects = new ProjectModel(doneModel_);
        QVector<ProjectItem> rows;
        rows.reserve(c.batches.size());

        for (const auto& b : c.batches) {
            ProjectItem r;
            r.batchId = b.batchId;
            r.projectCode = b.projectCode;
            r.testCode = b.testCode;
            r.status = b.status;
            r.scheduledFor = b.scheduledFor;
            r.createdAt = b.createdAt;
            r.pendingCount = b.pendingCount;
            r.sentCount = b.sentCount;
            r.hasReport = b.hasReport;
            rows.push_back(std::move(r));
        }
        projects->setItems(std::move(rows));

        TestCardItem item;
        item.testCode = c.testCode;
        item.pendingCount = c.pendingCount;
        item.sentCount = c.sentCount;
        item.projectsModel = projects;

        cards.push_back(std::move(item));
    }

    doneModel_->setItems(std::move(cards));
    pendingSnapshots_--;
    if (pendingSnapshots_ <= 0) {
        setBusy(false);
    }
}

void DashboardViewModel::onDoneDashboardSnapshotFailed(const QString& error)
{
    setError(error.isEmpty() ? "Failed to load done dashboard." : error);
    pendingSnapshots_--;
    if (pendingSnapshots_ <= 0) {
        setBusy(false);
    }
}

void DashboardViewModel::onArchiveDashboardSnapshotReady(const QVector<TestCardDto>& snapshot)
{
    QVector<TestCardItem> cards;
    cards.reserve(snapshot.size());

    for (const auto& c : snapshot) {
        auto projects = new ProjectModel(archiveModel_);
        QVector<ProjectItem> rows;
        rows.reserve(c.batches.size());

        for (const auto& b : c.batches) {
            ProjectItem r;
            r.batchId = b.batchId;
            r.projectCode = b.projectCode;
            r.testCode = b.testCode;
            r.status = b.status;
            r.scheduledFor = b.scheduledFor;
            r.createdAt = b.createdAt;
            r.pendingCount = b.pendingCount;
            r.sentCount = b.sentCount;
            r.hasReport = b.hasReport;
            rows.push_back(std::move(r));
        }
        projects->setItems(std::move(rows));

        TestCardItem item;
        item.testCode = c.testCode;
        item.pendingCount = c.pendingCount;
        item.sentCount = c.sentCount;
        item.projectsModel = projects;

        cards.push_back(std::move(item));
    }

    archiveModel_->setItems(std::move(cards));
    pendingSnapshots_--;
    if (pendingSnapshots_ <= 0) {
        setBusy(false);
    }
}

void DashboardViewModel::onArchiveDashboardSnapshotFailed(const QString& error)
{
    setError(error.isEmpty() ? "Failed to load archive dashboard." : error);
    pendingSnapshots_--;
    if (pendingSnapshots_ <= 0) {
        setBusy(false);
    }
}
