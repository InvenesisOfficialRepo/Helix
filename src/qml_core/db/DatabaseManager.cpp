#include "DatabaseManager.h"

#include <QtSql/QSqlError>
#include <QMutex>
#include <QMutexLocker>

static QMutex dbMutex;

bool DatabaseManager::openConnection(const DbConfig& cfg, const QString& connectionName, QString* errorOut)
{
    if (errorOut) errorOut->clear();

    if (cfg.host.isEmpty() || cfg.dbName.isEmpty() || cfg.user.isEmpty()) {
        if (errorOut) *errorOut = "DbConfig is incomplete (host/dbName/user required).";
        return false;
    }

    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase existing = QSqlDatabase::database(connectionName, false);
        if (existing.isOpen())
            return true;

        if (!existing.open()) {
            if (errorOut) *errorOut = existing.lastError().text();
            return false;
        }
        return true;
    }

    QMutexLocker locker(&dbMutex);
    if (QSqlDatabase::contains(connectionName)) {
        locker.unlock();
        return openConnection(cfg, connectionName, errorOut);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", connectionName);
    db.setHostName(cfg.host);
    db.setPort(cfg.port);
    db.setDatabaseName(cfg.dbName);
    db.setUserName(cfg.user);
    db.setPassword(cfg.password);

    if (!db.open()) {
        if (errorOut) *errorOut = db.lastError().text();
        return false;
    }

    return true;
}

QSqlDatabase DatabaseManager::db(const QString& connectionName)
{
    return QSqlDatabase::database(connectionName, false);
}

void DatabaseManager::closeConnection(const QString& connectionName)
{
    if (!QSqlDatabase::contains(connectionName))
        return;

    {
        QSqlDatabase db = QSqlDatabase::database(connectionName, false);
        if (db.isValid())
            db.close();
    }
    
    QMutexLocker locker(&dbMutex);
    QSqlDatabase::removeDatabase(connectionName);
}
