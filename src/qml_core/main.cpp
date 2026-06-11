#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include <QCoreApplication>

#include "app/AppContext.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Invenesis");
    QCoreApplication::setOrganizationDomain("invenesis.com");
    QCoreApplication::setApplicationName("Test Request Tracking");

    QGuiApplication app(argc, argv);

    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArray("Material"));

    QQmlApplicationEngine engine;

    AppContext appContext;
    qmlRegisterSingletonInstance("TestRequests", 1, 0, "App", &appContext);

    // Load the QML entry point from the compiled QML module (qt_add_qml_module)
    engine.loadFromModule("TestRequests", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
