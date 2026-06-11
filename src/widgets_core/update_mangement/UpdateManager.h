#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QUrl>
#include <QVersionNumber>

class UpdateManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isChecking READ isChecking NOTIFY isCheckingChanged)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY isDownloadingChanged)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)

public:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager() override;

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void startDownload();
    Q_INVOKABLE void installAndExit();

    bool isChecking() const { return m_isChecking; }
    bool isDownloading() const { return m_isDownloading; }
    double downloadProgress() const { return m_downloadProgress; }

signals:
    void updateAvailable(const QString &latestVersion, const QString &notes);
    void noUpdateAvailable();
    void downloadFinished();
    void errorOccurred(const QString &errorMessage);
    
    void isCheckingChanged();
    void isDownloadingChanged();
    void downloadProgressChanged();

private:
    QNetworkAccessManager m_networkManager;
    QNetworkReply *m_currentReply = nullptr;
    QFile m_tempFile;
    
    QString m_latestVersion;
    QString m_releaseNotes;
    QUrl m_downloadUrl;
    QString m_tempFilePath;

    bool m_isChecking = false;
    bool m_isDownloading = false;
    double m_downloadProgress = 0.0;

    void setChecking(bool checking);
    void setDownloading(bool downloading);
    void setProgress(double progress);
};
