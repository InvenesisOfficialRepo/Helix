#include "PasswordHasher.h"

#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>

extern "C" {
#include "ow-crypt.h"
}

bool PasswordHasher::verifyPassword(const QString& enteredPassword, const QString& storedHash)
{
    if (storedHash.isEmpty())
        return false;

    // Same as your QtBCrypt code
    const QByteArray pass = enteredPassword.toLocal8Bit();
    const QByteArray hash = storedHash.toLocal8Bit();

    // crypt() may not be re-entrant depending on implementation
    static QMutex cryptMutex;
    QMutexLocker lock(&cryptMutex);

    char* out = crypt(pass.constData(), hash.constData());
    if (!out)
        return false;

    return QByteArray(out) == hash;
}
