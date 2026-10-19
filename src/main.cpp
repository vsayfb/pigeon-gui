#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

#include <cstdlib>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Pigeon"));
    QGuiApplication::setOrganizationName(QStringLiteral("Pigeon"));

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Pigeon/Gui/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    return app.exec();
}
