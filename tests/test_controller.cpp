#include "app/AppController.h"

#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

using namespace pigeon;

namespace {

bool makeFixture(const QString &path)
{
    const QString name = QUuid::createUuid().toString();
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        db.setDatabaseName(path);
        if (db.open()) {
            ok = true;
            const QStringList statements = {
                QStringLiteral("CREATE TABLE conversations ("
                    "us_ip BLOB,remote_ip BLOB,remote_port INTEGER,protocol INTEGER,"
                    "packets_out INTEGER,packets_in INTEGER,bytes_out INTEGER,bytes_in INTEGER,"
                    "first_seen INTEGER,last_seen INTEGER,remote_name TEXT,application TEXT)"),
                QStringLiteral("CREATE TABLE dns_cache (ip BLOB,name TEXT,observed_at INTEGER,expires_at INTEGER)"),
                QStringLiteral("PRAGMA user_version=4"),
                QStringLiteral("INSERT INTO conversations VALUES "
                    "(x'0a000001',x'08080808',443,6,1,1,10,70,10,90,'first.test','firefox')"),
                QStringLiteral("INSERT INTO conversations VALUES "
                    "(x'0a000001',x'01010101',443,6,1,1,30,5,10,95,'second.test','')"),
                QStringLiteral("INSERT INTO conversations VALUES "
                    "(x'20010db8000000000000000000000001',x'20010db8000000000000000000000002',53,17,1,1,0,45,10,99,'third.test','dns-client')")
            };
            for (const QString &sql : statements) {
                QSqlQuery q(db);
                if (!q.exec(sql)) {
                    qWarning() << q.lastError();
                    ok = false;
                    break;
                }
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(name);
    return ok;
}

} // namespace

class ControllerTests : public QObject {
    Q_OBJECT
private slots:
    void localNetworkRanges();
    void filtersAndApplicationDrillDown();
    void pauseAndSupersededRefreshes();
    void retrySameDatabasePathAfterFailure();
};

void ControllerTests::localNetworkRanges()
{
    AppController controller;
    QCOMPARE(controller.endpointName("", "127.0.0.1"), "This device");
    QCOMPARE(controller.endpointName("", "::1"), "This device");
    QCOMPARE(controller.endpointName("", "::ffff:127.0.0.1"), "This device");
    QCOMPARE(controller.endpointName("", "192.168.1.1"), "Local network");
    QCOMPARE(controller.endpointName("", "ff02::fb"), "Local network");
    QCOMPARE(controller.endpointName("", "8.8.8.8"), "-");
    QCOMPARE(controller.endpointName("router.test", "192.168.1.1"), "router.test");
    QCOMPARE(controller.endpointName("localhost", "127.0.0.1"), "localhost");
    const QStringList local = { "10.0.0.1", "172.16.0.1", "172.31.255.255", "192.168.1.1",
        "127.0.0.1", "169.254.1.1", "224.0.0.251", "239.255.255.250", "255.255.255.255",
        "::1", "fc00::1", "fdff::1", "fe80::1", "febf::1", "ff02::fb", "ff05::1", "ff18::1",
        "::ffff:192.168.1.1" };
    for (const auto &ip : local)
        QVERIFY2(!controller.addressScope(ip).isEmpty(), qPrintable(ip));
    const QStringList other = { "", "invalid", "0.0.0.0", "8.8.8.8", "100.64.0.1", "172.15.255.255",
        "172.32.0.0", "192.169.0.1", "224.0.1.1", "238.255.255.255", "240.0.0.1",
        "192.0.2.1", "::", "2001:db8::1", "2001:4860::8888", "fe7f::1", "fec0::1", "ff0e::1",
        "ff00::1", "::ffff:8.8.8.8" };
    for (const auto &ip : other)
        QVERIFY2(controller.addressScope(ip).isEmpty(), qPrintable(ip));
}

void ControllerTests::filtersAndApplicationDrillDown()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("view.db"));
    QVERIFY(makeFixture(path));

    AppController controller;
    controller.selectDatabase(path);
    QTRY_VERIFY_WITH_TIMEOUT(controller.hasSnapshot(), 3000);
    auto *top = static_cast<ConversationsModel *>(controller.topModel());
    auto *apps = static_cast<ApplicationsModel *>(controller.applicationsModel());
    QCOMPARE(top->rowCount(), 3);
    QCOMPARE(apps->rowCount(), 3);
    QCOMPARE(controller.recentSeconds(), 30);

    controller.setProtocol(6);
    controller.setFamily(4);
    controller.setSearch(QStringLiteral("first.test"));
    QTRY_COMPARE_WITH_TIMEOUT(top->rowCount(), 1, 3000);
    QCOMPARE(top->data(top->index(0, 0), ConversationsModel::ApplicationRole).toString(),
             QStringLiteral("firefox"));

    // Exact unresolved selection clears search, but keeps protocol/family.
    controller.selectApplication(QStringLiteral(""));
    QVERIFY(controller.applicationSelected());
    QVERIFY(controller.selectedApplication().isEmpty());
    QVERIFY(controller.search().isEmpty());
    QCOMPARE(controller.protocol(), 6);
    QCOMPARE(controller.family(), 4);
    QTRY_COMPARE_WITH_TIMEOUT(top->rowCount(), 1, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(top->data(top->index(0, 0), ConversationsModel::ApplicationRole).toString(),
                              QStringLiteral(""), 3000);
    QCOMPARE(apps->rowCount(), 3); // unfiltered aggregates

    const QString id = top->data(top->index(0, 0), ConversationsModel::IdentityRole).toString();
    controller.selectConversation(id, false);
    QCOMPARE(controller.selectedDetails().value(QStringLiteral("application")).toString(),
             QStringLiteral("Process not identified"));
    QCOMPARE(controller.selectedDetails().value(QStringLiteral("remoteIp")).toString(),
             QStringLiteral("1.1.1.1"));
    QCOMPARE(controller.selectedIdentity(), id);

    controller.setRankingSort(2);
    QTRY_VERIFY_WITH_TIMEOUT(controller.selectedDetails().value(
        QStringLiteral("inCurrentView")).toBool(), 3000);
    QCOMPARE(controller.selectedIdentity(), id);

    controller.clearFilters();
    QTRY_COMPARE_WITH_TIMEOUT(top->rowCount(), 3, 3000);
    QCOMPARE(controller.rankingSort(), 0);
    QCOMPARE(controller.liveSort(), 0);
    QCOMPARE(controller.recentSeconds(), 30);
    QVERIFY(!controller.applicationSelected());
    QCOMPARE(controller.protocol(), 0);
    QCOMPARE(controller.family(), 0);
    QCOMPARE(controller.search(), QString());
}

void ControllerTests::pauseAndSupersededRefreshes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("view.db"));
    QVERIFY(makeFixture(path));

    AppController controller;
    controller.selectDatabase(path);
    QTRY_VERIFY_WITH_TIMEOUT(controller.hasSnapshot(), 3000);
    auto *top = static_cast<ConversationsModel *>(controller.topModel());
    QCOMPARE(top->rowCount(), 3);

    controller.setPaused(true);
    controller.setProtocol(17);
    QCOMPARE(top->rowCount(), 3); // filters deferred while paused
    QTest::qWait(1200);
    QCOMPARE(top->rowCount(), 3);
    controller.setPaused(false);
    QTRY_COMPARE_WITH_TIMEOUT(top->rowCount(), 1, 3000);

    // Several fast changes while the worker is active must not publish the
    // superseded text filter's rows or counter baselines.
    controller.setProtocol(0);
    controller.setSearch(QStringLiteral("this-is-not-in-the-fixture"));
    controller.setSearch(QStringLiteral("second.test"));
    QTRY_COMPARE_WITH_TIMEOUT(top->rowCount(), 1, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(top->data(top->index(0, 0), ConversationsModel::ApplicationRole).toString(),
                              QStringLiteral(""), 3000);

    // Opening a new database must never display the previous one's contents.
    controller.selectDatabase(dir.filePath(QStringLiteral("missing.db")));
    QVERIFY(!controller.hasSnapshot());
    QCOMPARE(top->rowCount(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.error().isEmpty(), 3000);
    QVERIFY(!QFileInfo::exists(dir.filePath(QStringLiteral("missing.db"))));
}

void ControllerTests::retrySameDatabasePathAfterFailure()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("not-created-yet.db"));

    AppController controller;
    controller.selectDatabase(path);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.error().isEmpty(), 3000);
    QVERIFY(!controller.hasSnapshot());
    QVERIFY(!QFileInfo::exists(path)); // read-only attempts never create files

    QVERIFY(makeFixture(path));
    controller.retryRead();
    QTRY_VERIFY_WITH_TIMEOUT(controller.hasSnapshot(), 3000);
    QVERIFY(controller.error().isEmpty());
    QCOMPARE(controller.databasePath(), path);
    QCOMPARE(controller.topModel()->rowCount(), 3);

    controller.setPaused(true);
    QVERIFY(controller.paused());
    controller.retryRead();
    QTRY_VERIFY_WITH_TIMEOUT(!controller.paused(), 3000);
    QVERIFY(controller.hasSnapshot());
}

QTEST_GUILESS_MAIN(ControllerTests)
#include "test_controller.moc"
