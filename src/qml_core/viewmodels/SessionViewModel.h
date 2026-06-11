#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class SessionViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    Q_PROPERTY(QString role READ role NOTIFY roleChanged)
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY isAuthenticatedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool canSend READ canSend NOTIFY canSendChanged)

public:
    explicit SessionViewModel(QObject* parent = nullptr);

    QString username() const { return username_; }
    QString role() const { return role_; }
    bool isAuthenticated() const { return isAuthenticated_; }
    bool busy() const { return busy_; }
    QString errorMessage() const { return errorMessage_; }

    bool canSend() const;

    Q_INVOKABLE void login(const QString& username, const QString& password);
    Q_INVOKABLE void logout();

signals:
    void usernameChanged();
    void roleChanged();
    void isAuthenticatedChanged();
    void busyChanged();
    void errorMessageChanged();
    void canSendChanged();

    // AppContext wires this to DbWorker::authenticate
    void loginRequested(const QString& username, const QString& password);

public slots:
    // AppContext wires DbWorker::authenticateDone to this
    void handleAuthResult(const QString& username, bool success, const QString& role, const QString& error);

private:
    void setBusy(bool b);
    void setError(const QString& msg);

    QString username_;
    QString role_;
    bool isAuthenticated_ = false;
    bool busy_ = false;
    QString errorMessage_;

    // Adjust to your roles
    QStringList sendRoles_ = {"admin", "logistics"};
};
