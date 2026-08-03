#include "AppContext.h"
#include "ui/databaseviewwindow.h"
#include "ThemeManager.h"
#include "services/FeedingAnalysisProcessor.h"
#include "LayoutMakerWindow.h"
#include <QMetaObject>
#include <QUuid>
#include <QTimer>

#include "db/DbWorker.h"

AppContext::AppContext(QObject *parent)
    : QObject(parent)
{
    session_ = new SessionViewModel(this);
    dashboard_ = new DashboardViewModel(this);
    project_ = new ProjectViewModel(this);
    calendar_ = new CalendarViewModel(this);
    datapointManager_ = new DatapointManagerViewModel(this);
    feeding_ = new FeedingAnalysisProcessor(this);

    // ---------------- Phase 1 wiring (keep) ----------------
    connect(session_, &SessionViewModel::isAuthenticatedChanged, this, [this] {
        if (session_->isAuthenticated())
            dashboard_->refresh();
    });

    connect(dashboard_, &DashboardViewModel::openBatchRequested,
            project_, &ProjectViewModel::loadBatch);

    connect(calendar_, &CalendarViewModel::selectedDateChanged,
            calendar_, &CalendarViewModel::reloadForSelectedDate);

    // ---------------- Phase 2: DB config + worker thread ----------------
    DbConfig cfg;
    QString cfgErr;
    const bool cfgOk = loadDbConfigFromEnv(cfg, cfgErr);

    worker_ = new DbWorker();
    worker_->setConfig(cfg, cfgOk ? QString{} : cfgErr);
    worker_->moveToThread(&workerThread_);

    // Phase 3: inject worker into dashboard VM so it can request DB jobs
    dashboard_->setDbWorker(worker_);

    //Phase 4:
    project_->setDbWorker(worker_);

    //Phase 5:
    calendar_->setDbWorker(worker_);

    connect(&workerThread_, &QThread::started, worker_, &DbWorker::onThreadStarted);

    // SessionViewModel -> DbWorker
    connect(session_, &SessionViewModel::loginRequested,
            worker_,  &DbWorker::authenticate,
            Qt::QueuedConnection);

    // DbWorker -> SessionViewModel
    connect(worker_, &DbWorker::authenticateDone,
            session_, &SessionViewModel::handleAuthResult,
            Qt::QueuedConnection);

    // DbWorker -> AppContext (db state)
    connect(worker_, &DbWorker::dbReadyChanged,
            this, &AppContext::onDbReadyChanged,
            Qt::QueuedConnection);

    connect(project_, &ProjectViewModel::dashboardRefreshRequested,
            dashboard_, &DashboardViewModel::refresh);

    workerThread_.start();

    // ---------------- Phase 6: Poll done-status sync ----------------
    auto syncTimer = new QTimer(this);
    syncTimer->setInterval(15000); // 15s (adjust to taste)
    syncTimer->setSingleShot(false);

    auto syncInFlight = new bool(false);

    // Timer -> worker sync (avoid overlaps)
    connect(syncTimer, &QTimer::timeout, this, [this, syncInFlight] {
        if (!session_->isAuthenticated()) return;
        if (!worker_) return;

        if (*syncInFlight) return;
        *syncInFlight = true;

        QMetaObject::invokeMethod(worker_, "syncDoneStatuses", Qt::QueuedConnection);
    });

    // Worker -> UI refresh
    connect(worker_, &DbWorker::syncDoneStatusesDone, this,
            [this, syncInFlight](int compoundsUpdated, int batchesUpdated) {
                *syncInFlight = false;

                if (compoundsUpdated <= 0 && batchesUpdated <= 0)
                    return; // nothing changed

                // Refresh dashboard + calendar marks/events
                dashboard_->refresh();
                calendar_->refreshVisibleMonth();
                calendar_->reloadForSelectedDate();
            },
            Qt::QueuedConnection);

    connect(worker_, &DbWorker::syncDoneStatusesFailed, this,
            [syncInFlight](const QString&) {
                *syncInFlight = false;
            },
            Qt::QueuedConnection);

    // Start/stop polling with auth state
    connect(session_, &SessionViewModel::isAuthenticatedChanged, this, [this, syncTimer, syncInFlight] {
        if (session_->isAuthenticated()) {
            syncTimer->start();
            // kick an immediate sync once at login
            if (!*syncInFlight) {
                *syncInFlight = true;
                QMetaObject::invokeMethod(worker_, "syncDoneStatuses", Qt::QueuedConnection);
            }
        } else {
            syncTimer->stop();
            *syncInFlight = false;
        }
    });

}

AppContext::~AppContext()
{
    if (worker_) {
        // Shut down worker + close DB connection safely in its own thread
        QMetaObject::invokeMethod(worker_, "shutdown", Qt::BlockingQueuedConnection);

        workerThread_.quit();
        workerThread_.wait();

        delete worker_;
        worker_ = nullptr;
    }
}

SessionViewModel* AppContext::session() const { return session_; }
DashboardViewModel* AppContext::dashboard() const { return dashboard_; }
ProjectViewModel* AppContext::project() const { return project_; }
CalendarViewModel* AppContext::calendar() const { return calendar_; }
DatapointManagerViewModel* AppContext::datapointManager() const { return datapointManager_; }
ThemeManager* AppContext::theme() const { return ThemeManager::instance(); }
FeedingAnalysisProcessor* AppContext::feedingProcessor() const { return feeding_; }

void AppContext::onDbReadyChanged(bool ready, const QString& error)
{
    if (dbReady_ != ready) {
        dbReady_ = ready;
        emit dbReadyChanged();
    }
    if (dbError_ != error) {
        dbError_ = error;
        emit dbErrorChanged();
    }
}

void AppContext::launchWidgetsApp()
{
    if (widgetsMainWindow_) {
        widgetsMainWindow_->raise();
        widgetsMainWindow_->activateWindow();
        return;
    }

    QString role = session_ ? session_->role() : QString();
    widgetsMainWindow_ = new MainWindow(role);
    widgetsMainWindow_->setAttribute(Qt::WA_DeleteOnClose);
    widgetsMainWindow_->show();
}

void AppContext::launchLayoutMaker()
{
    if (layoutMakerWindow_) {
        layoutMakerWindow_->raise();
        layoutMakerWindow_->activateWindow();
        return;
    }

    layoutMakerWindow_ = new LayoutMakerWindow();
    layoutMakerWindow_->setAttribute(Qt::WA_DeleteOnClose);
    layoutMakerWindow_->show();
}
