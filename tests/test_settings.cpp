#include "app/SettingsController.h"
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class SettingsTest : public QObject {
    Q_OBJECT
private slots:
    void collectorSettings() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString root = temporary.path() + "/data";
        const QString bin = temporary.path() + "/bin";
        QVERIFY(QDir().mkpath(root));
        QVERIFY(QDir().mkpath(bin));
        QFile cli(bin + "/pigeon");
        QVERIFY(cli.open(QIODevice::WriteOnly));
        cli.write(R"SH(#!/bin/sh
[ "$PIGEON_GUI_AUTH" = 1 ] || exit 3
case "$1" in
  autostart)
    case "$2" in
      status) printf 'Start at boot: '; cat "$PIGEON_INSTALL_ROOT/boot" ;;
      on|off)
        if [ -e "$PIGEON_INSTALL_ROOT/cancel" ]; then echo 'Authorization cancelled' >&2; exit 1; fi
        printf '%s\n' "$2" > "$PIGEON_INSTALL_ROOT/boot"
        printf 'Start at boot: %s\n' "$2" ;;
    esac ;;
  database) printf '%s\n' "$2" > "$PIGEON_INSTALL_ROOT/database.conf"; echo 'Database location saved.' ;;
esac
)SH");
        cli.close();
        QVERIFY(cli.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QFile state(root + "/boot");
        QVERIFY(state.open(QIODevice::WriteOnly));
        state.write("on\n");
        state.close();
        const QByteArray oldPath = qgetenv("PATH");
        qputenv("PATH", bin.toUtf8() + ':' + oldPath);
        qputenv("PIGEON_INSTALL_ROOT", root.toUtf8());
        qunsetenv("PIGEON_DATABASE");
        pigeon::SettingsController settings;
        QCOMPARE(settings.databasePath(), root + "/pigeon.db");
        settings.refresh();
        QTRY_VERIFY(!settings.busy());
        QVERIFY(settings.available() && settings.bootSupported() && settings.autostart());
        settings.setAutostart(false);
        QTRY_VERIFY(!settings.busy());
        QVERIFY(!settings.autostart());
        QFile cancel(root + "/cancel");
        QVERIFY(cancel.open(QIODevice::WriteOnly));
        cancel.close();
        settings.setAutostart(true);
        QTRY_VERIFY(!settings.busy());
        QVERIFY(!settings.autostart());
        QVERIFY(settings.message().contains("cancelled"));
        settings.setDatabasePath("relative.db");
        QVERIFY(settings.message().contains("absolute"));
        QSignalSpy changed(&settings, &pigeon::SettingsController::databaseChanged);
        const QString path = root + "/new location/pigeon.db";
        settings.setDatabasePath(path);
        QTRY_VERIFY(!settings.busy());
        QCOMPARE(settings.databasePath(), path);
        QCOMPARE(changed.count(), 1);
        qputenv("PATH", oldPath);
        qunsetenv("PIGEON_INSTALL_ROOT");
    }
};
QTEST_GUILESS_MAIN(SettingsTest)
#include "test_settings.moc"
