#pragma once

#include <QObject>
#include <QThread>

#include "viewmodels/SessionViewModel.h"
#include "viewmodels/DashboardViewModel.h"
#include "viewmodels/ProjectViewModel.h"
#include "viewmodels/CalendarViewModel.h"

#include "db/DbConfig.h"

class DbWorker;
class MainWindow;
#include "services/FeedingAnalysisProcessor.h"
#include "ThemeManager.h"
#include <QPointer>

class AppContext : public QObject
{
    Q_OBJECT
    Q_PROPERTY(SessionViewModel* session READ session CONSTANT)
    Q_PROPERTY(DashboardViewModel* dashboard READ dashboard CONSTANT)
    Q_PROPERTY(ProjectViewModel* project READ project CONSTANT)
    Q_PROPERTY(CalendarViewModel* calendar READ calendar CONSTANT)
    Q_PROPERTY(ThemeManager* theme READ theme CONSTANT)
    Q_PROPERTY(FeedingAnalysisProcessor* feedingProcessor READ feedingProcessor CONSTANT)

    // Optional but very useful for LoginPage: shows missing env vars / DB open errors
    Q_PROPERTY(bool dbReady READ dbReady NOTIFY dbReadyChanged)
    Q_PROPERTY(QString dbError READ dbError NOTIFY dbErrorChanged)

public:
    explicit AppContext(QObject *parent = nullptr);
    ~AppContext() override;

    Q_INVOKABLE void launchWidgetsApp();

    SessionViewModel* session() const;
    DashboardViewModel* dashboard() const;
    ProjectViewModel* project() const;
    CalendarViewModel* calendar() const;
    ThemeManager* theme() const;
    FeedingAnalysisProcessor* feedingProcessor() const;

    bool dbReady() const { return dbReady_; }
    QString dbError() const { return dbError_; }

signals:
    void dbReadyChanged();
    void dbErrorChanged();

private slots:
    void onDbReadyChanged(bool ready, const QString& error);

private:
    SessionViewModel* session_ = nullptr;
    DashboardViewModel* dashboard_ = nullptr;
    ProjectViewModel* project_ = nullptr;
    CalendarViewModel* calendar_ = nullptr;
    FeedingAnalysisProcessor* feeding_ = nullptr;

    // Phase 2: worker thread
    QThread workerThread_;
    DbWorker* worker_ = nullptr;

    bool dbReady_ = false;
    QString dbError_;

    QPointer<MainWindow> widgetsMainWindow_;
};
