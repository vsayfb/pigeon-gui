#include "app/SettingsController.h"
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class SettingsTest : public QObject {
    Q_OBJECT
private slots:
    void multiDelete() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString root = temporary.path();
        QFile cli(root + "/pigeon");
        QVERIFY(cli.open(QIODevice::WriteOnly));
        cli.write(R"SH(#!/bin/sh
printf '%s\n' "$3" >> "$PIGEON_INSTALL_ROOT/deletions"
if [ -f "$PIGEON_INSTALL_ROOT/fail" ] && [ "$3" = '04c000020a04c633641401bb11' ]; then
    echo 'Deletion failed' >&2
    exit 1
fi
printf 'Deleted.\n'
)SH");
        cli.close();
        QVERIFY(cli.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QFile database(root + "/history.db");
        QVERIFY(database.open(QIODevice::WriteOnly));
        database.close();
        const QByteArray oldPath = qgetenv("PATH");
        const QByteArray oldRoot = qgetenv("PIGEON_INSTALL_ROOT");
        qputenv("PATH", root.toUtf8() + ':' + oldPath);
        qputenv("PIGEON_INSTALL_ROOT", root.toUtf8());
        pigeon::SettingsController settings;
        QSignalSpy result(&settings, &pigeon::SettingsController::historyResult);
        QSignalSpy deleted(&settings, &pigeon::SettingsController::historyDeleted);
        const QString a = "04c000020a04c633641401bb11";
        const QString b = "04c000020a04c633641401bc11";
        const QString c = "04c000020a04c633641401bd11";
        settings.deleteConversations({a, b, a}, database.fileName());
        // A second operation must not replace an in-flight batch.
        settings.clearHistory(database.fileName());
        QTRY_COMPARE(result.count(), 1);
        QVERIFY(!settings.busy());
        QVERIFY(result.last().at(0).toBool());
        QVERIFY(result.last().at(1).toString().contains("2 deletions"));
        QCOMPARE(deleted.count(), 1);
        QFile log(root + "/deletions");
        QVERIFY(log.open(QIODevice::ReadOnly));
        QCOMPARE(log.readAll(), (a + "\n" + b + "\n").toUtf8());
        log.close();
        QVERIFY(log.remove());

        QFile failure(root + "/fail");
        QVERIFY(failure.open(QIODevice::WriteOnly));
        failure.close();
        settings.deleteConversations({b, a, c}, database.fileName());
        QTRY_COMPARE(result.count(), 2);
        QVERIFY(!settings.busy());
        QVERIFY(!result.last().at(0).toBool());
        QVERIFY(result.last().at(1).toString().contains("1 of 3"));
        QCOMPARE(deleted.count(), 2); // Refresh even after partial success.
        QVERIFY(log.open(QIODevice::ReadOnly));
        QCOMPARE(log.readAll(), (b + "\n" + a + "\n").toUtf8());
        log.close();
        QVERIFY(log.remove());

        settings.deleteConversations({b, "not-an-identity"}, database.fileName());
        QCOMPARE(result.count(), 3);
        QVERIFY(!result.last().at(0).toBool());
        QVERIFY(!log.exists()); // Validate the entire batch before deleting any row.
        settings.deleteConversations({}, database.fileName());
        QCOMPARE(result.count(), 4);
        QVERIFY(!log.exists());
        qputenv("PATH", oldPath);
        if (oldRoot.isNull())
            qunsetenv("PIGEON_INSTALL_ROOT");
        else
            qputenv("PIGEON_INSTALL_ROOT", oldRoot);
    }

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
[ "$PIGEON_GUI_AUTH" = 1 ] || [ "$PIGEON_GUI_AUTH" = 0 ] || exit 3
printf '%s\n' "$PIGEON_GUI_AUTH" > "$PIGEON_INSTALL_ROOT/auth-mode"
case "$1" in
  autostart)
    case "$2" in
      status) printf 'Start at boot: '; cat "$PIGEON_INSTALL_ROOT/boot" ;;
      on|off)
        if [ -e "$PIGEON_INSTALL_ROOT/cancel" ]; then echo 'Authorization cancelled' >&2; exit 1; fi
        printf '%s\n' "$2" > "$PIGEON_INSTALL_ROOT/boot"
        printf 'Start at boot: %s\n' "$2" ;;
    esac ;;
  history)
    if [ -e "$PIGEON_INSTALL_ROOT/history-needs-auth" ] && [ "$PIGEON_GUI_AUTH" = 1 ]; then exit 3; fi
    printf 'Recorded traffic deleted.\n' ;;
  database) printf '%s\n' "$2" > "$PIGEON_INSTALL_ROOT/database.conf"; echo 'Database location saved.' ;;
esac
)SH");
        cli.close();
        QVERIFY(cli.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QFile state(root + "/boot");
        QVERIFY(state.open(QIODevice::WriteOnly));
        state.write("on\n");
        state.close();
        QFile policyKit(bin + "/pkexec");
        QVERIFY(policyKit.open(QIODevice::WriteOnly));
        policyKit.write("#!/bin/sh\nexit 0\n");
        policyKit.close();
        QVERIFY(policyKit.setPermissions(QFile::ReadOwner | QFile::ExeOwner));
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
        QVERIFY(QDir().mkpath(root + "/new location"));
        QFile history(path);
        QVERIFY(history.open(QIODevice::WriteOnly));
        history.close();
        QSignalSpy deleted(&settings, &pigeon::SettingsController::historyDeleted);
        QSignalSpy deletionResult(&settings, &pigeon::SettingsController::historyResult);
        settings.deleteConversation("04c000020a04c633641401bb11", path);
        QTRY_VERIFY(!settings.busy());
        QCOMPARE(deleted.count(), 1);
        QVERIFY(deletionResult.last().at(0).toBool());
        // A terminal can return immediately while the privileged command is pending.
        QVERIFY(policyKit.remove());
        QVERIFY(cancel.remove());
        QFile terminal(bin + "/x-terminal-emulator");
        QVERIFY(terminal.open(QIODevice::WriteOnly));
        terminal.write("#!/bin/sh\nshift\n\"$@\" &\nexit 0\n");
        terminal.close();
        QVERIFY(terminal.setPermissions(QFile::ReadOwner | QFile::ExeOwner));
        qputenv("PATH", bin.toUtf8());
        settings.setAutostart(true);
        QTRY_VERIFY(!settings.busy());
        QVERIFY(settings.autostart());
        QFile mode(root + "/auth-mode");
        QVERIFY(mode.open(QIODevice::ReadOnly));
        QCOMPARE(mode.readAll().trimmed(), QByteArray("0"));
        QVERIFY(settings.available());
        QFile needsAuth(root + "/history-needs-auth");
        QVERIFY(needsAuth.open(QIODevice::WriteOnly));
        needsAuth.close();
        settings.clearHistory(path);
        QTRY_VERIFY(!settings.busy());
        QCOMPARE(deleted.count(), 2);
        QVERIFY(deletionResult.last().at(0).toBool());
        settings.deleteConversations({"04c000020a04c633641401bb11", "04c000020a04c633641401bc11"}, path);
        QTRY_VERIFY(!settings.busy());
        QCOMPARE(deleted.count(), 3);
        QCOMPARE(deletionResult.count(), 3);
        QVERIFY(deletionResult.last().at(0).toBool());
        QVERIFY(deletionResult.last().at(1).toString().contains("2 deletions"));
        qputenv("PATH", oldPath);
        qunsetenv("PIGEON_INSTALL_ROOT");
    }
};
QTEST_GUILESS_MAIN(SettingsTest)
#include "test_settings.moc"
