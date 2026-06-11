#pragma once

#include <QString>
#include <QtSql/QSqlDatabase>

#include "DbConfig.h"

class DatabaseManager
{
public:
    static bool openConnection(const DbConfig& cfg, const QString& connectionName, QString* errorOut);
    static QSqlDatabase db(const QString& connectionName);
    static void closeConnection(const QString& connectionName);
};
