#include "UpdateManager.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QDebug>

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
{
}

UpdateManager::~UpdateManager()
{
    if (m_currentReply) {
        m_currentReply->abort();
    }
    if (m_tempFile.isOpen()) {
        m_tempFile.close();
    }
}

void UpdateManager::checkForUpdates()
{
    if (m_isChecking || m_isDownloading) return;
    setChecking(true);

    // Fetch the version JSON directly from raw GitHub to bypass caching
    QUrl url("https://raw.githubusercontent.com/InvenesisOfficialRepo/Helix/main/version.json");
    QNetworkRequest request(url);
    
    // Ensure no caching
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    m_currentReply = m_networkManager.get(request);

    connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        m_currentReply->deleteLater();
        setChecking(false);

        if (m_currentReply->error() != QNetworkReply::NoError) {
            emit errorOccurred("Failed to reach update server: " + m_currentReply->errorString());
            m_currentReply = nullptr;
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(m_currentReply->readAll());
        m_currentReply = nullptr;
        
        if (!doc.isObject()) {
            emit errorOccurred("Invalid update manifest format.");
            return;
        }

        QJsonObject obj = doc.object();
        m_latestVersion = obj.value("latest_version").toString();
        m_releaseNotes = obj.value("release_notes").toString();
        m_downloadUrl = QUrl(obj.value("download_url").toString());

        QVersionNumber current = QVersionNumber::fromString(QCoreApplication::applicationVersion());
        QVersionNumber remote = QVersionNumber::fromString(m_latestVersion);

        qInfo() << "Helix Update Check: Local version =" << current.toString() 
                << "| Remote version =" << remote.toString();

        if (!remote.isNull() && !current.isNull() && QVersionNumber::compare(remote, current) > 0) {
            emit updateAvailable(m_latestVersion, m_releaseNotes);
        } else {
            emit noUpdateAvailable();
        }
    });
}

void UpdateManager::startDownload()
{
    if (m_isDownloading || !m_downloadUrl.isValid()) return;
    setDownloading(true);
    setProgress(0.0);

    // Setup temporary storage for downloaded installer
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_tempFilePath = tempDir + "/Helix_Setup.exe";
    
    // Remove old download if exists
    if (QFile::exists(m_tempFilePath)) {
        QFile::remove(m_tempFilePath);
    }

    m_tempFile.setFileName(m_tempFilePath);
    if (!m_tempFile.open(QIODevice::WriteOnly)) {
        setDownloading(false);
        emit errorOccurred("Could not write to temporary directory.");
        return;
    }

    QNetworkRequest request(m_downloadUrl);
    m_currentReply = m_networkManager.get(request);

    connect(m_currentReply, &QNetworkReply::readyRead, this, [this]() {
        m_tempFile.write(m_currentReply->readAll());
    });

    connect(m_currentReply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesRead, qint64 totalBytes) {
        if (totalBytes > 0) {
            setProgress(static_cast<double>(bytesRead) / totalBytes);
        }
    });

    connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        m_currentReply->deleteLater();
        m_tempFile.close();
        setDownloading(false);

        if (m_currentReply->error() != QNetworkReply::NoError) {
            m_tempFile.remove();
            emit errorOccurred("Download failed: " + m_currentReply->errorString());
            m_currentReply = nullptr;
            return;
        }

        m_currentReply = nullptr;
        emit downloadFinished();
    });
}

void UpdateManager::installAndExit()
{
    if (m_tempFilePath.isEmpty() || !QFile::exists(m_tempFilePath)) {
        emit errorOccurred("No installer file found to run.");
        return;
    }

    qInfo() << "Helix Launcher: Starting installer detached:" << m_tempFilePath;

    // Launch installer detached from Helix process
    bool success = QProcess::startDetached(m_tempFilePath, QStringList());
    if (success) {
        QCoreApplication::quit();
    } else {
        emit errorOccurred("Could not launch installer. Please run it manually from Temp folder.");
    }
}

void UpdateManager::setChecking(bool checking) {
    if (m_isChecking != checking) {
        m_isChecking = checking;
        emit isCheckingChanged();
    }
}

void UpdateManager::setDownloading(bool downloading) {
    if (m_isDownloading != downloading) {
        m_isDownloading = downloading;
        emit isDownloadingChanged();
    }
}

void UpdateManager::setProgress(double progress) {
    if (m_downloadProgress != progress) {
        m_downloadProgress = progress;
        emit downloadProgressChanged();
    }
}
