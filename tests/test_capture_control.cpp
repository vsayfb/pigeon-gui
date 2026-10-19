#include "app/CaptureController.h"

#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTemporaryDir>
#include <QTest>

using namespace pigeon;

class CaptureTests : public QObject {
    Q_OBJECT
private slots:
    void controlsAndErrors()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("capture.db");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        QLocalServer server;
        QVERIFY(server.listen(path + ".control"));
        QString current = "wifi0";
        QString mode = "auto";
        QByteArray lastCommand;
        connect(&server, &QLocalServer::newConnection, this, [&] {
            auto *socket = server.nextPendingConnection();
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QLocalSocket::readyRead, socket, [&, socket] {
                QByteArray bytes = socket->property("request").toByteArray() + socket->readAll();
                socket->setProperty("request", bytes);
                if (!bytes.endsWith('\n'))
                    return;
                lastCommand = bytes.trimmed();
                if (lastCommand == "SELECT bad0") {
                    socket->write("ERROR\tInterface unsupported; capture unchanged\n");
                } else {
                    if (lastCommand == "SELECT eth0") {
                        current = "eth0";
                        mode = "manual";
                    } else if (lastCommand == "AUTO") {
                        current = "wifi0";
                        mode = "auto";
                    }
                    socket->write("OK\t" + current.toUtf8() + "\t" + mode.toUtf8() + "\n");
                    if (lastCommand == "LIST") {
                        socket->write("wifi0\tactive\tsupported\neth0\tactive\tsupported\n");
                        socket->write("tun0\tactive\tunsupported\ndown0\tinactive\tunavailable\n");
                    }
                }
                socket->disconnectFromServer();
            });
        });

        CaptureController controller;
        controller.setDatabasePath(path);
        QTRY_VERIFY(!controller.busy());
        QCOMPARE(controller.currentInterface(), "wifi0");
        QVERIFY(controller.automatic());
        QCOMPARE(controller.interfaces().size(), 4);
        QVERIFY(!controller.interfaces().at(2).toMap().value("supported").toBool());
        QVERIFY(!controller.interfaces().at(3).toMap().value("active").toBool());

        controller.selectInterface("eth0");
        QTRY_VERIFY(!controller.busy());
        QCOMPARE(controller.currentInterface(), "eth0");
        QVERIFY(!controller.automatic());
        QCOMPARE(lastCommand, "LIST");
        controller.selectInterface("bad0");
        QTRY_VERIFY(!controller.busy());
        QVERIFY(controller.status().contains("unsupported"));
        QCOMPARE(controller.currentInterface(), "eth0");
        controller.selectInterface("");
        QTRY_VERIFY(!controller.busy());
        QCOMPARE(controller.currentInterface(), "wifi0");
        QVERIFY(controller.automatic());
        controller.selectInterface("eth0\nAUTO");
        QVERIFY(controller.status().contains("Invalid interface"));

        // Changing databases aborts pending control and clears old identity.
        controller.refresh();
        controller.setDatabasePath(directory.filePath("missing.db"));
        QVERIFY(!controller.busy());
        QVERIFY(controller.currentInterface().isEmpty());
        QVERIFY(controller.interfaces().isEmpty());
        QVERIFY(controller.status().contains("existing collector database"));
        controller.setDatabasePath(path);
        QTRY_VERIFY(!controller.busy());
        QCOMPARE(controller.currentInterface(), "wifi0");
    }

    void unavailableCollector()
    {
        QTemporaryDir directory;
        QFile file(directory.filePath("copy.db"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        CaptureController controller;
        controller.setDatabasePath(file.fileName());
        QTRY_VERIFY(!controller.busy());
        QVERIFY(controller.status().contains("unavailable"));
        QVERIFY(controller.currentInterface().isEmpty());
    }
};

QTEST_GUILESS_MAIN(CaptureTests)
#include "test_capture_control.moc"
