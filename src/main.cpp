#include "app/AppController.h"
#include "app/CaptureController.h"
#include "app/SettingsController.h"
#include "app/AppIconProvider.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QDir>
#include <QIcon>
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
    QGuiApplication::setDesktopFileName(QStringLiteral("pigeon-gui"));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/qt/qml/Pigeon/Gui/assets/pigeon.png")));

    QCommandLineParser arguments;
    arguments.setApplicationDescription(QStringLiteral("Read-only Pigeon SQLite viewer"));
    arguments.addHelpOption();
    QCommandLineOption databaseOption({QStringLiteral("d"), QStringLiteral("database")},
                                      QStringLiteral("Existing schema-v4/v5/v6/v7 SQLite database"),
                                      QStringLiteral("path"));
    arguments.addOption(databaseOption);
    QCommandLineOption checkQmlOption(QStringLiteral("check-qml"),
                                      QStringLiteral("Check QML startup across all views and exit"));
    arguments.addOption(checkQmlOption);
    arguments.addOption({QStringLiteral("preview-settings"), QStringLiteral("Open Settings in a preview")});
    QCommandLineOption previewOption(QStringLiteral("render-preview"),
                                     QStringLiteral("Render a window preview to a PNG and exit"),
                                     QStringLiteral("path"));
    arguments.addOption(previewOption);
    QCommandLineOption pageOption(QStringLiteral("preview-page"),
                                  QStringLiteral("Preview page (0–4)"), QStringLiteral("number"), QStringLiteral("1"));
    arguments.addOption(pageOption);
    QCommandLineOption widthOption(QStringLiteral("preview-width"),
                                   QStringLiteral("Preview width"), QStringLiteral("pixels"), QStringLiteral("1500"));
    arguments.addOption(widthOption);
    arguments.addOption({QStringLiteral("preview-dark"), QStringLiteral("Use dark theme for a preview")});
    QCommandLineOption delayOption(QStringLiteral("preview-delay"),
                                   QStringLiteral("Wait before rendering a preview"),
                                   QStringLiteral("milliseconds"), QStringLiteral("1800"));
    arguments.addOption(delayOption);
    arguments.addOption({QStringLiteral("preview-interfaces"), QStringLiteral("Open capture source dialog in a preview")});
    arguments.process(app);

    pigeon::SettingsController settingsController;
    pigeon::AppController controller;
    pigeon::CaptureController captureController;
    if (arguments.isSet(databaseOption)) {
        controller.selectDatabase(arguments.value(databaseOption));
    } else {
        controller.selectDatabase(settingsController.databasePath());
    }

    QObject::connect(&settingsController, &pigeon::SettingsController::databaseChanged,
                     &controller, &pigeon::AppController::selectDatabase);

    QQmlApplicationEngine engine;
    // Qt 6.4 does not include this resource root in its default import paths.
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    auto *appIcons = new pigeon::AppIconProvider;
    engine.addImageProvider(QStringLiteral("appicons"), appIcons);
    engine.rootContext()->setContextProperty(QStringLiteral("appIcons"),
        new pigeon::AppIconResolver(appIcons, &engine));
    QList<QQmlError> qmlWarnings;
    QObject::connect(&engine, &QQmlEngine::warnings, &engine,
                     [&qmlWarnings](const QList<QQmlError> &warnings) {
                         qmlWarnings.append(warnings);
                     });
    engine.rootContext()->setContextProperty(QStringLiteral("readerController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("captureController"), &captureController);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settingsController);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Pigeon/Gui/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return EXIT_FAILURE;

    if (arguments.isSet(previewOption)) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        if (!window)
            return EXIT_FAILURE;

        window->setProperty("darkTheme", arguments.isSet(QStringLiteral("preview-dark")));
        window->setWidth(qBound(620, arguments.value(widthOption).toInt(), 2400));
        window->setProperty("currentPage", qBound(0, arguments.value(pageOption).toInt(), 4));
        if (arguments.isSet(QStringLiteral("preview-interfaces"))) {
            QTimer::singleShot(200, window, [window] {
                auto *dialog = window->findChild<QObject *>(QStringLiteral("captureInterfaceDialog"));
                if (dialog)
                    QMetaObject::invokeMethod(dialog, "open");
            });
        }
        if (arguments.isSet(QStringLiteral("preview-settings"))) {
            QTimer::singleShot(200, window, [window] {
                auto *dialog = window->findChild<QObject *>(QStringLiteral("settingsDialog"));
                if (dialog)
                    QMetaObject::invokeMethod(dialog, "open");
            });
        }
        QTimer::singleShot(qBound(100, arguments.value(delayOption).toInt(), 60000), window, [&app, window, &arguments, &previewOption] {
            const bool saved = window->grabWindow().save(arguments.value(previewOption));
            app.exit(saved ? EXIT_SUCCESS : EXIT_FAILURE);
        });

        return app.exec();
    }

    if (arguments.isSet(checkQmlOption)) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        if (!window)
            return EXIT_FAILURE;

        // Exercise all pages in both themes, including narrow layouts.
        for (int step = 0; step < 10; ++step) {
            QTimer::singleShot(150 * (step + 1), window, [window, step] {
                window->setWidth(step % 2 ? 700 : 1500);
                window->setHeight(step % 2 ? 600 : 900);
                window->setProperty("darkTheme", step >= 5);
                window->setProperty("currentPage", step % 5);
            });
        }

        QTimer::singleShot(1550, window, [window] {
            auto *dialog = window->findChild<QObject *>(QStringLiteral("captureInterfaceDialog"));
            if (dialog)
                QMetaObject::invokeMethod(dialog, "open");
        });
        QTimer::singleShot(1650, window, [window] {
            auto *capture = window->findChild<QObject *>(QStringLiteral("captureInterfaceDialog"));
            if (capture)
                QMetaObject::invokeMethod(capture, "close");
            auto *settings = window->findChild<QObject *>(QStringLiteral("settingsDialog"));
            if (settings)
                QMetaObject::invokeMethod(settings, "open");
        });
        QTimer::singleShot(1950, &app, &QCoreApplication::quit);
        const int result = app.exec();
        for (const QQmlError &warning : std::as_const(qmlWarnings))
            qCritical().noquote() << warning.toString();
        return result == 0 && qmlWarnings.isEmpty() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return app.exec();
}
