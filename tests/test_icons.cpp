#include "app/AppIconProvider.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

class IconTests final : public QObject {
    Q_OBJECT

private slots:
    void installedDesktopIcon();
    void missingIconAndBoundedSize();
    void relativeThemeIcon();
    void domainFallbackAndBrowserPriority();
};

void IconTests::installedDesktopIcon()
{
#if !defined(Q_OS_LINUX)
    QSKIP("Desktop-entry matching is Linux-specific; other platforms use the fallback");
#endif

    // A private desktop entry verifies real lookup without depending on the
    // user's installed applications, theme, or browser version.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QDir().mkpath(directory.path() + "/applications"));
    QImage original(32, 32, QImage::Format_ARGB32_Premultiplied);
    original.fill(QColor("#D32167"));
    const QString iconPath = directory.path() + "/test.png";
    QVERIFY(original.save(iconPath));

    QFile desktop(directory.path() + "/applications/pigeon-test.desktop");
    QVERIFY(desktop.open(QIODevice::WriteOnly));
    desktop.write(("[Desktop Entry]\nType=Application\nName=Pigeon test\n"
                   "Exec=/opt/pigeon-icon-test\nStartupWMClass=PigeonIconTest\nIcon="
                   + iconPath + "\n[Desktop Action unrelated]\nIcon=invalid\n").toUtf8());
    desktop.close();

    const QByteArray oldDataHome = qgetenv("XDG_DATA_HOME");
    qputenv("XDG_DATA_HOME", directory.path().toUtf8());
    pigeon::AppIconProvider provider;
    qputenv("XDG_DATA_HOME", oldDataHome);

    QSize size;
    const auto image = provider.requestImage("pigeon-icon-test", &size, QSize(32, 32));
    QCOMPARE(size, QSize(32, 32));
    QCOMPARE(image.pixelColor(16, 16), QColor("#D32167"));
    QCOMPARE(provider.requestImage("PigeonIconTest", nullptr, QSize(32, 32)), image);
    QCOMPARE(provider.requestImage("Pigeon%20test", nullptr, QSize(32, 32)), image);
    QCOMPARE(provider.requestImage("pigeon-icon-test", nullptr, QSize(32, 32)), image);
}

void IconTests::relativeThemeIcon()
{
#if !defined(Q_OS_LINUX)
    QSKIP("Desktop-entry matching is Linux-specific");
#endif

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString theme = directory.path() + "/icons/hicolor";
    QVERIFY(QDir().mkpath(theme + "/32x32/apps"));
    QVERIFY(QDir().mkpath(directory.path() + "/applications"));
    QImage original(32, 32, QImage::Format_ARGB32_Premultiplied);
    original.fill(QColor("#25B34D"));
    QVERIFY(original.save(theme + "/32x32/apps/pigeon-theme-test.png"));

    QFile index(theme + "/index.theme");
    QVERIFY(index.open(QIODevice::WriteOnly));
    index.write("[Icon Theme]\nName=Hicolor\nDirectories=32x32/apps\n"
                "[32x32/apps]\nSize=32\nContext=Applications\nType=Fixed\n");
    index.close();

    QFile desktop(directory.path() + "/applications/pigeon-theme-test.desktop");
    QVERIFY(desktop.open(QIODevice::WriteOnly));
    desktop.write("[Desktop Entry]\nName=Pigeon theme test\n"
                  "Exec=pigeon-themed-process\nIcon=pigeon-theme-test\n");
    desktop.close();

    const QByteArray oldDataHome = qgetenv("XDG_DATA_HOME");
    qputenv("XDG_DATA_HOME", directory.path().toUtf8());
    pigeon::AppIconProvider provider;
    qputenv("XDG_DATA_HOME", oldDataHome);

    const auto image = provider.requestImage("pigeon-themed-process", nullptr, QSize(32, 32));
    QCOMPARE(image.pixelColor(16, 16), QColor("#25B34D"));
}

void IconTests::domainFallbackAndBrowserPriority()
{
#if !defined(Q_OS_LINUX)
    QSKIP("Desktop-entry matching is Linux-specific");
#endif

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QDir().mkpath(directory.path() + "/applications"));

    auto entry = [&](const QString &fileName, const QString &name, const QString &command,
                     const QString &color) {
        QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor(color));
        const QString path = directory.path() + '/' + fileName + ".png";
        if (!image.save(path))
            return false;

        QFile file(directory.path() + "/applications/" + fileName + ".desktop");
        if (!file.open(QIODevice::WriteOnly))
            return false;

        return file.write(("[Desktop Entry]\nName=" + name + "\nExec=" + command
                           + "\nIcon=" + path + '\n').toUtf8()) > 0;
    };

    // Create the PWA first to reproduce the browser executable collision.
    QVERIFY(entry("a-site", "ChatGPT", "pigeon-test-browser --app-id=chatgpt", "#D32167"));
    QVERIFY(entry("z-browser", "Pigeon test browser", "pigeon-test-browser", "#2563EB"));

    const QByteArray oldDataHome = qgetenv("XDG_DATA_HOME");
    qputenv("XDG_DATA_HOME", directory.path().toUtf8());
    pigeon::AppIconProvider provider;
    qputenv("XDG_DATA_HOME", oldDataHome);

    const auto fallback = provider.resolve("pigeon-uninstalled-client", "chatgpt.com");
    QVERIFY(fallback.value("site").toBool());
    const QString url = fallback.value("url").toString();
    const auto image = provider.requestImage(url.mid(QString("image://appicons/").size()), nullptr, QSize(32, 32));
    QCOMPARE(image.pixelColor(16, 16), QColor("#D32167"));

    const auto browser = provider.resolve("pigeon-test-browser", "chatgpt.com");
    QVERIFY(!browser.value("site").toBool());
    const auto browserImage = provider.requestImage("pigeon-test-browser", nullptr, QSize(32, 32));
    QCOMPARE(browserImage.pixelColor(16, 16), QColor("#2563EB"));
    QVERIFY(!provider.resolve("pigeon-uninstalled-client", "unrelated.invalid").value("site").toBool());
    QVERIFY(!provider.resolve("pigeon-uninstalled-client", "evilchatgpt.com").value("site").toBool());
    QCOMPARE(provider.resolve("pigeon-uninstalled-client", "chatgpt.com"), fallback);
}

void IconTests::missingIconAndBoundedSize()
{
    pigeon::AppIconProvider provider;
    QSize size;
    const QImage image = provider.requestImage("pigeon-icon-definitely-not-installed", &size, QSize(4096, 4096));
    QVERIFY(!image.isNull());
    QCOMPARE(size, QSize(128, 128));
    QVERIFY(image.pixelColor(64, 64).alpha() > 0);
    QVERIFY(!provider.requestImage("", nullptr, QSize()).isNull());
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    IconTests tests;

    return QTest::qExec(&tests, argc, argv);
}

#include "test_icons.moc"
