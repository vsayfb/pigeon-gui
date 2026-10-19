#include "data/DatabaseReader.h"
#include "data/RateSampler.h"
#include "models/Models.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QPersistentModelIndex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

using namespace pigeon;

namespace {

bool execute(const QString &path, const QStringList &sql)
{
    const QString name = QUuid::createUuid().toString();
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        db.setDatabaseName(path);
        if (db.open()) {
            ok = true;
            for (const QString &command : sql) {
                QSqlQuery query(db);
                if (!query.exec(command)) {
                    qWarning() << query.lastError();
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

QString createFixture(const QString &path)
{
    const QStringList sql = {
        QStringLiteral("CREATE TABLE conversations ("
            "us_ip BLOB NOT NULL,remote_ip BLOB NOT NULL,remote_port INTEGER NOT NULL,"
            "protocol INTEGER NOT NULL,packets_out INTEGER NOT NULL,packets_in INTEGER NOT NULL,"
            "bytes_out INTEGER NOT NULL,bytes_in INTEGER NOT NULL,first_seen INTEGER NOT NULL,"
            "last_seen INTEGER NOT NULL,remote_name TEXT,application TEXT NOT NULL,"
            "PRIMARY KEY(us_ip,remote_ip,remote_port,protocol,application)) WITHOUT ROWID"),
        QStringLiteral("CREATE TABLE dns_cache (ip BLOB PRIMARY KEY,name TEXT,observed_at INTEGER,expires_at INTEGER)"),
        QStringLiteral("PRAGMA user_version=4"),
        QStringLiteral("INSERT INTO conversations VALUES "
            "(x'0a000001',x'08080808',443,6,7,8,9007199254740996,9007199254741000,10,90,'example.org','firefox')"),
        QStringLiteral("INSERT INTO conversations VALUES "
            "(x'0a000001',x'08080808',443,6,2,1,20,10,5,85,'example.org','')"),
        QStringLiteral("INSERT INTO conversations VALUES "
            "(x'20010db8000000000000000000000001',x'20010db8000000000000000000000002',"
            "53,17,0,1,0,30,70,95,NULL,'dns-client')")
    };
    return execute(path, sql) ? path : QString();
}

QString patchCounterSql(qint64 counter, qint64 packets = 8)
{
    return QStringLiteral("UPDATE conversations SET bytes_in=%1, packets_in=%2 "
                          "WHERE application='firefox'").arg(counter).arg(packets);
}

} // namespace

class DataTests : public QObject {
    Q_OBJECT
private slots:
    void exactWideIntegerFormatting();
    void missingAndUnsupportedDb();
    void readOnlySnapshotAndFilters();
    void exactRatesAndReset();
    void failedReadDoesNotModifySnapshot();
    void stableModelIndexes();
    void boundedRowHistory();
    void icmpVersionFiveAndFilters();
    void quicMetadataAndStableIdentity();
};

void DataTests::exactWideIntegerFormatting()
{
    Counter128 value;
    value.add(9223372036854775807ULL);
    value.add(9223372036854775807ULL);
    value.add(4);
    QCOMPARE(value.high, quint64(1));
    QCOMPARE(value.low, quint64(2));
    QCOMPARE(value.decimal(), QStringLiteral("18446744073709551618"));
    const Counter128 before = value;
    value.add(3);
    QCOMPARE(value.difference(before), 3.0);
}

void DataTests::missingAndUnsupportedDb()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("missing.db"));
    DatabaseReader reader;
    ReadOptions options;
    Snapshot snapshot;
    QString error;
    QVERIFY(!reader.read(path, options, 100, snapshot, error));
    QVERIFY(!QFileInfo::exists(path)); // never create an empty database
    QVERIFY(!error.isEmpty());
    QVERIFY(execute(path, {QStringLiteral("PRAGMA user_version=3")}));
    QVERIFY(!reader.read(path, options, 100, snapshot, error));
    QVERIFY(error.contains(QStringLiteral("version 4")));
    QVERIFY(execute(path, {QStringLiteral("PRAGMA user_version=4")}));
    QVERIFY(!reader.read(path, options, 100, snapshot, error)); // missing tables
    QVERIFY(error.contains(QStringLiteral("missing")));
}

void DataTests::readOnlySnapshotAndFilters()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = createFixture(dir.filePath(QStringLiteral("fixture.db")));
    QVERIFY(!path.isEmpty());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray before = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
    file.close();

    DatabaseReader reader;
    ReadOptions options;
    Snapshot snapshot;
    QString error;
    QVERIFY2(reader.read(path, options, 100, snapshot, error), qPrintable(error));
    QCOMPARE(snapshot.conversations, quint64(3));
    QCOMPARE(snapshot.remoteIps, quint64(2));
    QCOMPARE(snapshot.applications.size(), 3);
    QCOMPARE(snapshot.live.size(), 3);
    QCOMPARE(snapshot.top.size(), 3);
    QCOMPARE(snapshot.endpoints.size(), 2);
    QCOMPARE(snapshot.exactDown.low, quint64(9007199254741040ULL));
    QCOMPARE(snapshot.exactDown.decimal(), QStringLiteral("9007199254741040"));
    QVERIFY(snapshot.hasPackets);
    QCOMPARE(snapshot.latestPacket, qint64(95));
    QCOMPARE(snapshot.endpoints.first().firstSeen, qint64(70));
    QCOMPARE(snapshot.live.first().remoteIp, QStringLiteral("2001:db8::2"));

    options.search = QStringLiteral("8.8.8.8");
    options.applicationSelected = true;
    // Qt differentiates a null QString from a non-null empty QString. Both
    // must select the stored empty application, never match all or SQL NULL.
    for (const QString &unresolved : {QString(), QStringLiteral("")}) {
        options.application = unresolved;
        QVERIFY2(reader.read(path, options, 100, snapshot, error), qPrintable(error));
        QCOMPARE(snapshot.live.size(), 1);
        QCOMPARE(snapshot.top.size(), 1);
        QCOMPARE(snapshot.live.first().application, QStringLiteral(""));
        QCOMPARE(snapshot.applications.size(), 3); // NOT filtered by application
        QCOMPARE(snapshot.endpoints.size(), 1); // search, not application, applies
        QCOMPARE(snapshot.conversations, quint64(3)); // overview unfiltered
    }
    options.application = QStringLiteral("firefox");
    QVERIFY2(reader.read(path, options, 100, snapshot, error), qPrintable(error));
    QCOMPARE(snapshot.live.size(), 1);
    QCOMPARE(snapshot.live.first().application, QStringLiteral("firefox"));

    options.applicationSelected = false;
    options.search = QStringLiteral("2001:db8");
    options.family = 6;
    options.protocol = 6;
    QVERIFY2(reader.read(path, options, 100, snapshot, error), qPrintable(error));
    QCOMPARE(snapshot.live.size(), 0); // protocol applies
    QCOMPARE(snapshot.endpoints.size(), 1); // protocol does not apply

    reader.close();
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256), before);
}

void DataTests::exactRatesAndReset()
{
    QTemporaryDir dir;
    const QString path = createFixture(dir.filePath(QStringLiteral("fixture.db")));
    QVERIFY(!path.isEmpty());
    DatabaseReader reader;
    RateSampler sampler;
    ReadOptions options;
    Snapshot s;
    QString error;
    QVERIFY2(reader.read(path, options, 100, s, error), qPrintable(error));
    sampler.sample(s, 5.0);
    QVERIFY(!s.ratesValid);
    QVERIFY(!s.live.first().rateValid);
    QVERIFY(execute(path, {patchCounterSql(9007199254741007LL)}));
    QVERIFY2(reader.read(path, options, 101, s, error), qPrintable(error));
    sampler.sample(s, 5.4); // too soon; don't move baseline
    QVERIFY(!s.ratesValid);
    QVERIFY(execute(path, {patchCounterSql(9007199254741012LL)}));
    QVERIFY2(reader.read(path, options, 102, s, error), qPrintable(error));
    sampler.sample(s, 6.0);
    QVERIFY(s.ratesValid);
    QCOMPARE(s.rateDown, 12.0); // exact delta across 2^53
    bool found = false;
    for (const Conversation &row : s.live) {
        if (row.application == QStringLiteral("firefox")) {
            found = true;
            QVERIFY(row.rateValid);
            QCOMPARE(row.rateDown, 12.0);
        }
    }
    QVERIFY(found);
    QCOMPARE(s.history.size(), 1);

    QVERIFY(execute(path, {patchCounterSql(2, 1)})); // decreasing counters reset row + overview
    QVERIFY2(reader.read(path, options, 103, s, error), qPrintable(error));
    sampler.sample(s, 7.1);
    QVERIFY(!s.ratesValid);
    for (const Conversation &row : s.live)
        if (row.application == QStringLiteral("firefox"))
            QVERIFY(!row.rateValid);
}

void DataTests::boundedRowHistory()
{
    RateSampler sampler;
    Snapshot snapshot;
    Conversation row;
    row.application = QStringLiteral("history-test");
    row.bytesIn = 100;
    snapshot.live = {row};
    sampler.sample(snapshot, 0);
    QVERIFY(snapshot.live.first().rateHistory.isEmpty());

    for (int sample = 1; sample <= 35; ++sample) {
        row.bytesIn += quint64(sample);
        snapshot.live = {row};
        sampler.sample(snapshot, sample);
    }

    QCOMPARE(snapshot.live.first().rateHistory.size(), 30);
    QCOMPARE(snapshot.live.first().rateHistory.first(), 6.0);
    QCOMPARE(snapshot.live.first().rateHistory.last(), 35.0);
    ConversationsModel model;
    model.setRows(snapshot.live);
    QCOMPARE(model.data(model.index(0), ConversationsModel::RateHistoryRole).toList().size(), 30);

    row.bytesIn = 1; // Counter resets discard the old graph as well as the rate.
    snapshot.live = {row};
    sampler.sample(snapshot, 36);
    QVERIFY(snapshot.live.first().rateHistory.isEmpty());
    QVERIFY(!snapshot.live.first().rateValid);

    snapshot.live.clear();
    sampler.sample(snapshot, 37);
    snapshot.live = {row};
    sampler.sample(snapshot, 38);
    QVERIFY(snapshot.live.first().rateHistory.isEmpty());
}

void DataTests::icmpVersionFiveAndFilters()
{
    QTemporaryDir directory;
    const QString path = createFixture(directory.filePath(QStringLiteral("icmp.db")));
    QVERIFY(!path.isEmpty());
    QVERIFY(execute(path, {
        QStringLiteral("PRAGMA user_version=5"),
        QStringLiteral("INSERT INTO conversations VALUES(x'0a000001',x'08080808',0,1,1,1,4,4,10,95,NULL,'')"),
        QStringLiteral("INSERT INTO conversations VALUES(x'20010db8000000000000000000000001',"
                       "x'20010db8000000000000000000000002',0,58,1,1,4,4,10,95,NULL,'')")
    }));

    DatabaseReader reader;
    ReadOptions options;
    Snapshot snapshot;
    QString error;
    QVERIFY2(reader.read(path, options, 100, snapshot, error), qPrintable(error));
    QCOMPARE(snapshot.conversations, quint64(5));
    options.protocol = 1;
    QVERIFY2(reader.read(path, options, 100, snapshot, error), qPrintable(error));
    QCOMPARE(snapshot.live.size(), 1);
    QCOMPARE(snapshot.live.first().protocol, 1);
    QCOMPARE(snapshot.live.first().remotePort, 0);
    options.protocol = 58;
    QVERIFY2(reader.read(path, options, 100, snapshot, error), qPrintable(error));
    QCOMPARE(snapshot.live.size(), 1);
    QCOMPARE(snapshot.top.first().protocol, 58);
    QVERIFY(execute(path, {QStringLiteral("UPDATE conversations SET remote_port=1 WHERE protocol=58")}));
    QVERIFY(!reader.read(path, options, 100, snapshot, error));
    QCOMPARE(snapshot.top.first().remotePort, 0); // Failed reads preserve the last snapshot.
}

void DataTests::quicMetadataAndStableIdentity()
{
    QTemporaryDir directory;
    const QString path = createFixture(directory.filePath(QStringLiteral("quic.db")));
    QVERIFY(!path.isEmpty());
    QVERIFY(execute(path, {QStringLiteral("PRAGMA user_version=6"),
                          QStringLiteral("ALTER TABLE conversations ADD COLUMN quic INTEGER NOT NULL DEFAULT 0")}));
    DatabaseReader reader;
    ReadOptions options;
    options.protocol = 17;
    Snapshot before, after;
    QString error;
    QVERIFY2(reader.read(path, options, 100, before, error), qPrintable(error));
    QCOMPARE(before.live.size(), 1);
    QVERIFY(!before.live.first().quic);
    QVERIFY(execute(path, {QStringLiteral("UPDATE conversations SET quic=1 WHERE protocol=17")}));
    QVERIFY2(reader.read(path, options, 100, after, error), qPrintable(error));
    QVERIFY(after.live.first().quic);
    QCOMPARE(after.live.first().key(), before.live.first().key());
    QCOMPARE(after.live.first().bytesIn, before.live.first().bytesIn);

    ConversationsModel model;
    model.setRows(before.live);
    QPersistentModelIndex index(model.index(0));
    model.setRows(after.live);
    QVERIFY(index.isValid());
    QVERIFY(model.data(index, ConversationsModel::QuicRole).toBool());

    QVERIFY(execute(path, {QStringLiteral("UPDATE conversations SET quic=2 WHERE protocol=17")}));
    QVERIFY(!reader.read(path, options, 100, after, error));
    QVERIFY(after.live.first().quic); // Last good snapshot survives malformed metadata.
}

void DataTests::failedReadDoesNotModifySnapshot()
{
    QTemporaryDir dir;
    const QString path = createFixture(dir.filePath(QStringLiteral("fixture.db")));
    QVERIFY(!path.isEmpty());
    DatabaseReader reader;
    ReadOptions options;
    Snapshot s;
    QString error;
    QVERIFY(reader.read(path, options, 100, s, error));
    options.protocol = 42;
    QVERIFY(!reader.read(path, options, 101, s, error));
    QCOMPARE(s.loadedAt, qint64(100));
    QCOMPARE(s.live.size(), 3);
}

void DataTests::stableModelIndexes()
{
    ConversationsModel model;
    Conversation a, b;
    a.localAddress = QByteArray::fromHex("0a000001");
    a.remoteAddress = QByteArray::fromHex("01010101");
    b.localAddress = a.localAddress;
    b.remoteAddress = QByteArray::fromHex("02020202");
    model.setRows({a, b});
    QPersistentModelIndex original(model.index(0, 0));
    QVERIFY(original.isValid());
    model.setRows({b, a});
    QVERIFY(original.isValid());
    QCOMPARE(original.row(), 1);
    model.setRows({b});
    QVERIFY(!original.isValid());
}

QTEST_GUILESS_MAIN(DataTests)
#include "test_data.moc"
