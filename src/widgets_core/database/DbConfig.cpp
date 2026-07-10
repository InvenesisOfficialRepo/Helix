#include "DbConfig.h"
#include <QProcessEnvironment>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMap>

bool loadDbConfigFromEnv(DbConfig &cfg, QString &error)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QMap<QString, QString> fallbackEnv;

    // Helper to parse a .env file
    auto loadDotEnv = [&](const QString &path) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty() || line.startsWith("#")) continue;
                int idx = line.indexOf('=');
                if (idx > 0) {
                    QString key = line.left(idx).trimmed();
                    QString val = line.mid(idx + 1).trimmed();
                    fallbackEnv[key] = val;
                }
            }
        }
    };

    // Try loading from app directory and home directory
    loadDotEnv(QCoreApplication::applicationDirPath() + "/.env");
    loadDotEnv(QDir::homePath() + "/.helix.env");

    error.clear();

    auto getRequired = [&](const QString &key) -> QString {
        QString value = env.value(key);
        if (value.isEmpty()) {
            value = fallbackEnv.value(key);
        }
        
        if (value.isEmpty()) {
            if (!error.isEmpty())
                error += '\n';
            error += QString("Missing environment variable: %1").arg(key);
        }
        return value;
    };

    // Required values
    cfg.host     = getRequired("INV_DB_HOST");
    cfg.dbName   = getRequired("INV_DB_NAME");
    cfg.user     = getRequired("INV_DB_USER");
    cfg.password = getRequired("INV_DB_PASSWORD");

    // Port: optional, default 5432 if not set
    QString portStr = env.value("INV_DB_PORT");
    if (portStr.isEmpty()) {
        portStr = fallbackEnv.value("INV_DB_PORT");
    }
    
    if (portStr.isEmpty()) {
        cfg.port = 5432;  // default PostgreSQL port
    } else {
        bool ok = false;
        int p = portStr.toInt(&ok);
        if (!ok) {
            if (!error.isEmpty())
                error += '\n';
            error += "Environment variable INV_DB_PORT is not a valid integer.";
            // fall back to default
            cfg.port = 5432;
        } else {
            cfg.port = p;
        }
    }

    // If 'error' is still empty, everything is fine
    return error.isEmpty();
}
