#include "AuthRepository.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include "PasswordHasher.h"

AuthRepository::AuthRepository(const QSqlDatabase& db)
    : db_(db)
{
}

AuthResult AuthRepository::authenticate(const QString& username, const QString& password)
{
    AuthResult r;

    if (!db_.isValid() || !db_.isOpen()) {
        r.success = false;
        r.error = "Database is not open.";
        return r;
    }

    QSqlQuery q(db_);
    q.prepare("SELECT password_hash, role FROM users WHERE username = :u");
    q.bindValue(":u", username);

    if (!q.exec()) {
        r.success = false;
        r.error = QString("Database error: %1").arg(q.lastError().text());
        return r;
    }

    if (!q.next()) {
        // don’t leak user existence
        r.success = false;
        r.error = "Invalid username or password.";
        return r;
    }

    const QString storedHash = q.value(0).toString();
    const QString role = q.value(1).toString();

    if (!PasswordHasher::verifyPassword(password, storedHash)) {
        r.success = false;
        r.error = "Invalid username or password.";
        return r;
    }

    r.success = true;
    r.role = role;
    r.error.clear();
    return r;
}
