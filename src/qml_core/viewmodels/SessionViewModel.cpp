#include "SessionViewModel.h"
#include "utils/UserSessionHelper.h"

SessionViewModel::SessionViewModel(QObject* parent)
    : QObject(parent)
{
}

bool SessionViewModel::canSend() const
{
    if (!isAuthenticated_)
        return false;

    const QString r = role_.trimmed().toLower();
    for (const QString& allowed : sendRoles_) {
        if (r == allowed.toLower())
            return true;
    }
    return false;
}

void SessionViewModel::login(const QString& username, const QString& password)
{
    if (busy_)
        return;

    setError(QString{});

    const QString u = username.trimmed();
    if (u.isEmpty() || password.isEmpty()) {
        setError("Please enter username and password.");
        return;
    }

    setBusy(true);
    emit loginRequested(u, password);
}

void SessionViewModel::logout()
{
    username_.clear();
    role_.clear();
    errorMessage_.clear();

    const bool wasAuth = isAuthenticated_;
    isAuthenticated_ = false;

    emit usernameChanged();
    emit roleChanged();
    emit errorMessageChanged();
    if (wasAuth)
        emit isAuthenticatedChanged();

    emit canSendChanged();
}

void SessionViewModel::handleAuthResult(const QString& username, bool success, const QString& role, const QString& error)
{
    setBusy(false);

    if (!success) {
        isAuthenticated_ = false;
        username_.clear();
        role_.clear();

        emit usernameChanged();
        emit roleChanged();
        emit isAuthenticatedChanged();
        setError(error.isEmpty() ? "Login failed." : error);
        emit canSendChanged();
        return;
    }

    username_ = username;
    role_ = role;
    isAuthenticated_ = true;

    SessionUtils::saveCurrentUsername(username);

    emit usernameChanged();
    emit roleChanged();
    emit isAuthenticatedChanged();
    setError(QString{});
    emit canSendChanged();
}

void SessionViewModel::setBusy(bool b)
{
    if (busy_ == b)
        return;
    busy_ = b;
    emit busyChanged();
}

void SessionViewModel::setError(const QString& msg)
{
    if (errorMessage_ == msg)
        return;
    errorMessage_ = msg;
    emit errorMessageChanged();
}
