#pragma once

#include <QString>
#include <QSettings>

namespace SessionUtils {

inline QString getCurrentUsername() {
    // 1. Check DatabaseApp settings
    QSettings dbAppSettings("Invenesis", "DatabaseApp");
    QString user = dbAppSettings.value("lastUsername", "").toString().trimmed();
    if (!user.isEmpty() && user.compare("Unknown", Qt::CaseInsensitive) != 0) {
        return user;
    }

    // 2. Check DatabaseManager settings
    QSettings dbManagerSettings("Invenesis", "DatabaseManager");
    user = dbManagerSettings.value("lastUsername", "").toString().trimmed();
    if (!user.isEmpty() && user.compare("Unknown", Qt::CaseInsensitive) != 0) {
        return user;
    }

    // 3. Check Helix settings
    QSettings helixSettings("Invenesis", "Helix");
    user = helixSettings.value("lastUsername", "").toString().trimmed();
    if (!user.isEmpty() && user.compare("Unknown", Qt::CaseInsensitive) != 0) {
        return user;
    }

    // 4. Check default QSettings
    QSettings defaultSettings;
    user = defaultSettings.value("lastUsername", "").toString().trimmed();
    if (!user.isEmpty() && user.compare("Unknown", Qt::CaseInsensitive) != 0) {
        return user;
    }

    // 5. Check environment variable
    user = QString::fromLocal8Bit(qgetenv("USERNAME")).trimmed();
    if (user.isEmpty()) {
        user = QString::fromLocal8Bit(qgetenv("USER")).trimmed();
    }

    return user.isEmpty() ? "Unknown" : user;
}

inline void saveCurrentUsername(const QString& username) {
    const QString u = username.trimmed();
    if (u.isEmpty()) return;

    QSettings dbAppSettings("Invenesis", "DatabaseApp");
    dbAppSettings.setValue("lastUsername", u);

    QSettings dbManagerSettings("Invenesis", "DatabaseManager");
    dbManagerSettings.setValue("lastUsername", u);

    QSettings helixSettings("Invenesis", "Helix");
    helixSettings.setValue("lastUsername", u);

    QSettings defaultSettings;
    defaultSettings.setValue("lastUsername", u);
}

} // namespace SessionUtils
