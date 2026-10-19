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
    void filtersAndApplicationDrillDown();
    void pauseAndSupersededRefreshes();
    void retrySameDatabasePathAfterFailure();
};

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
             QStringLiteral("Unresolved"));
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
