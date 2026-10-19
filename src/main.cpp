#include "app/AppController.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>

#include <QDebug>

#include <cstdlib>
#include <utility>

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
    QCommandLineOption checkQmlOption(QStringLiteral("check-qml"),
                                      QStringLiteral("Check QML startup across all views and exit"));
    arguments.addOption(checkQmlOption);
    arguments.process(app);

    pigeon::AppController controller;
    if (arguments.isSet(databaseOption))
        controller.selectDatabase(arguments.value(databaseOption));

    QQmlApplicationEngine engine;
    QList<QQmlError> qmlWarnings;
    QObject::connect(&engine, &QQmlEngine::warnings, &engine,
                     [&qmlWarnings](const QList<QQmlError> &warnings) {
                         qmlWarnings.append(warnings);
                     });
    engine.rootContext()->setContextProperty(QStringLiteral("readerController"), &controller);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Pigeon/Gui/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return EXIT_FAILURE;

    if (arguments.isSet(checkQmlOption)) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        if (!window)
            return EXIT_FAILURE;

        // Exercise every page and both layout widths. QML property errors
        // from lazily instantiated view delegates are also reported here.
        for (int page = 0; page < 5; ++page) {
            QTimer::singleShot(100 * (page + 1), window, [window, page] {
                window->setWidth(page % 2 ? 700 : 1240);
                window->setHeight(page % 2 ? 600 : 780);
                window->setProperty("currentPage", page);
            });
        }
        QTimer::singleShot(750, &app, &QCoreApplication::quit);
        const int result = app.exec();
        for (const QQmlError &warning : std::as_const(qmlWarnings))
            qCritical().noquote() << warning.toString();
        return result == 0 && qmlWarnings.isEmpty() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return app.exec();
}
