#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include <QCoreApplication>
#include <QIcon>
#include <QFile>
#include <QMessageBox>
#include <QFontDatabase>

#include "app/AppContext.h"
#include "database/Database.h"
#include "database/DbConfig.h"
#include "services/Logger.h"
#include "ThemeManager.h"
#include "update_mangement/UpdateManager.h"

#include <QSettings>

int main(int argc, char *argv[])
{
    // 1. Initialize QApplication (required for both QWidget and QGui/QML)
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // Initialize Logger
    Logger::init("invenesis_merge_app.log");
    qInfo() << "========================================";
    qInfo() << "Invenesis Unified Merge Application started";

    // Register custom fonts globally from Qt resources
    QList<QString> fontPaths = {
        ":/qt/qml/TestRequests/qml/fonts/Sen-Regular.ttf",
        ":/qt/qml/TestRequests/qml/fonts/Sen-Bold.ttf",
        ":/qt/qml/TestRequests/qml/fonts/MavenPro-Regular.ttf",
        ":/qt/qml/TestRequests/qml/fonts/MavenPro-Bold.ttf",
        ":/qt/qml/TestRequests/qml/fonts/MaterialSymbolsOutlined.ttf",
        ":/qt/qml/TestRequests/qml/fonts/Inter-VariableFont.ttf",
        ":/qt/qml/TestRequests/qml/fonts/Inter-Italic-VariableFont.ttf",
        ":/qt/qml/TestRequests/qml/fonts/JetBrainsMono-VariableFont.ttf",
        ":/qt/qml/TestRequests/qml/fonts/JetBrainsMono-Italic-VariableFont.ttf"
    };

    for (const QString &path : fontPaths) {
        int id = QFontDatabase::addApplicationFont(path);
        if (id == -1) {
            qWarning() << "Failed to load custom font from resource:" << path;
        } else {
            qInfo() << "Successfully loaded custom font from resource:" << path 
                    << "with families:" << QFontDatabase::applicationFontFamilies(id);
        }
    }

    // Set Application Details
    QCoreApplication::setOrganizationName("Invenesis");
    QCoreApplication::setOrganizationDomain("invenesis.com");
    QCoreApplication::setApplicationName("Helix");
    QCoreApplication::setApplicationVersion("1.2.9");

    // Load Window Icon
    QIcon appIcon(":/icons/resources/icons/Sphere.png");

    app.setWindowIcon(appIcon);

    // Set QML Quick controls style
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArray("Material"));

    // 2. Initialize the Studio Theme Manager and compile dynamic QSS
    ThemeManager::instance()->initialize(&app);

    // 3. Load DB config from environment variables (for Widgets GUI thread default connection)
    DbConfig cfg;
    QString error;
    if (!loadDbConfigFromEnv(cfg, error)) {
        QMessageBox::critical(nullptr,
                              QObject::tr("Configuration error"),
                              QObject::tr("Database configuration is invalid:\n%1").arg(error));
        return -1;
    }

    // Connect default database for Widget models in main GUI thread
    QString dbErr;
    if (!Database::connect(cfg.host,
                           cfg.port,
                           cfg.dbName,
                           cfg.user,
                           cfg.password,
                           &dbErr)) {
        QMessageBox::critical(nullptr,
                              QObject::tr("Database connection error"),
                              QObject::tr("Could not connect to the database:\n%1").arg(dbErr));
        return -1;
    }

    // Instantiate and register AppContext singleton
    AppContext appContext;

    // Instantiate and register UpdateManager singleton
    UpdateManager updateManager;

    // 4. Spin up the QML Engine
    QQmlApplicationEngine engine;

    qmlRegisterSingletonInstance("TestRequests", 1, 0, "App", &appContext);
    qmlRegisterSingletonInstance("TestRequests", 1, 0, "UpdateManager", &updateManager);

    // Load QML from the compiled QML resources
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/TestRequests/qml/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root objects";
        return -1;
    }

    int ret = app.exec();

    qInfo() << "Invenesis Unified Merge Application shutting down";
    Logger::cleanup();
    return ret;
}
