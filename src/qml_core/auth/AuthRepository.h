#pragma once

#include <QString>
#include <QtSql/QSqlDatabase>

struct AuthResult
{
    bool success = false;
    QString role;
    QString error;
};

class AuthRepository
{
public:
    explicit AuthRepository(const QSqlDatabase& db);

    AuthResult authenticate(const QString& username, const QString& password);

private:
    QSqlDatabase db_;
};
