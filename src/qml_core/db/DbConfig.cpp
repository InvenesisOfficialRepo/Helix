#include "DbConfig.h"
#include <QProcessEnvironment>

bool loadDbConfigFromEnv(DbConfig &cfg, QString &error)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    error.clear();

    auto getRequired = [&](const QString &key) -> QString {
        const QString value = env.value(key);
        if (value.isEmpty()) {
            if (!error.isEmpty())
                error += '\n';
            error += QString("Missing environment variable: %1").arg(key);
        }
        return value;
    };

    cfg.host     = getRequired("INV_DB_HOST");
    cfg.dbName   = getRequired("INV_DB_NAME");
    cfg.user     = getRequired("INV_DB_USER");
    cfg.password = getRequired("INV_DB_PASSWORD");

    const QString portStr = env.value("INV_DB_PORT");
    if (portStr.isEmpty()) {
        cfg.port = 5432;
    } else {
        bool ok = false;
        const int p = portStr.toInt(&ok);
        if (!ok) {
            if (!error.isEmpty())
                error += '\n';
            error += "Environment variable INV_DB_PORT is not a valid integer.";
            cfg.port = 5432;
        } else {
            cfg.port = p;
        }
    }

    return error.isEmpty();
}
