#include "app/AppController.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QUrl>

#include <cstdlib>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Pigeon"));
    QGuiApplication::setOrganizationName(QStringLiteral("Pigeon"));

    QCommandLineParser arguments;
    arguments.setApplicationDescription(QStringLiteral("Read-only Pigeon SQLite viewer"));
    arguments.addHelpOption();
    QCommandLineOption databaseOption({QStringLiteral("d"), QStringLiteral("database")},
                                      QStringLiteral("Existing schema-v4 SQLite database"),
                                      QStringLiteral("path"));
    arguments.addOption(databaseOption);
    arguments.process(app);

    pigeon::AppController controller;
    if (arguments.isSet(databaseOption))
        controller.selectDatabase(arguments.value(databaseOption));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("readerController"), &controller);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Pigeon/Gui/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    return app.exec();
}
