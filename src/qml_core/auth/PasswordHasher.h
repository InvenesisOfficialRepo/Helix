#pragma once
#include <QString>

class PasswordHasher
{
public:
    static bool verifyPassword(const QString& enteredPassword, const QString& storedHash);
};
