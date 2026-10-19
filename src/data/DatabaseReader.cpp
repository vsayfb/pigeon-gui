#include "data/DatabaseReader.h"

#include <QFileInfo>
#include <QHostAddress>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <limits>

namespace pigeon {
namespace {

QString ipString(const QByteArray &bytes)
{
    if (bytes.size() == 4) {
        const auto *b = reinterpret_cast<const unsigned char *>(bytes.constData());
        const quint32 ip = quint32(b[0]) << 24 | quint32(b[1]) << 16
                         | quint32(b[2]) << 8 | quint32(b[3]);
        return QHostAddress(ip).toString();
    }
    if (bytes.size() == 16) {
        Q_IPV6ADDR ip {};
        for (int i = 0; i < 16; ++i)
            ip.c[i] = static_cast<quint8>(bytes.at(i));
        return QHostAddress(ip).toString();
    }
    return {};
}

bool addressAt(const QSqlQuery &q, int column, QByteArray &bytes, QString &text)
{
    const QVariant v = q.value(column);
    if (v.metaType().id() != QMetaType::QByteArray)
        return false;
    bytes = v.toByteArray();
    if (bytes.size() != 4 && bytes.size() != 16)
        return false;
    text = ipString(bytes);
    return !text.isEmpty();
}

bool nonnegative(const QSqlQuery &q, int index, quint64 &out)
{
    bool ok = false;
    const qint64 n = q.value(index).toLongLong(&ok);
    if (!ok || n < 0)
        return false;
    out = static_cast<quint64>(n);
    return true;
}

QString queryError(const QSqlQuery &q, const QString &operation)
{
    return operation + QStringLiteral(": ") + q.lastError().text();
}

bool matches(const QString &needle, const Conversation &row)
{
    return row.remoteName.contains(needle, Qt::CaseInsensitive)
        || row.remoteIp.contains(needle, Qt::CaseInsensitive)
        || row.application.contains(needle, Qt::CaseInsensitive);
}

bool validOptions(const ReadOptions &o)
{
    return (o.protocol == 0 || o.protocol == 6 || o.protocol == 17 || o.protocol == 1 || o.protocol == 58)
        && (o.family == 0 || o.family == 4 || o.family == 6)
        && o.recentSeconds >= 0 && o.sort >= 0 && o.sort <= 2
        && o.liveSort >= 0 && o.liveSort <= 3
        && o.search.size() <= 127 && o.application.toUtf8().size() <= 255;
}

} // namespace

DatabaseReader::~DatabaseReader()
{
    close();
}

void DatabaseReader::close()
{
    if (m_connectionName.isEmpty())
        return;
    m_db.close();
    m_db = QSqlDatabase(); // release every database handle before removeDatabase
    QSqlDatabase::removeDatabase(m_connectionName);
    m_connectionName.clear();
    m_path.clear();
}

bool DatabaseReader::checkSchema(QString &error)
{
    QSqlQuery version(m_db);
    if (!version.exec(QStringLiteral("PRAGMA user_version")) || !version.next()) {
        error = queryError(version, QStringLiteral("Read schema version"));
        return false;
    }
    if (version.value(0).toInt() != 4 && version.value(0).toInt() != 5) {
        error = QStringLiteral("Schema version 4 or 5 required; use the collector to migrate the database");
        return false;
    }
    version.finish();

    const auto validateTable = [this, &error](const QString &table, const QSet<QString> &expected) {
        QSqlQuery q(m_db);
        if (!q.exec(QStringLiteral("PRAGMA table_info('") + table + QStringLiteral("')"))) {
            error = queryError(q, QStringLiteral("Read ") + table + QStringLiteral(" schema"));
            return false;
        }
        QSet<QString> seen;
        while (q.next())
            seen.insert(q.value(1).toString());
        QSet<QString> missing = expected;
        missing.subtract(seen);
        if (!missing.isEmpty()) {
            error = QStringLiteral("Schema v4 is missing required columns in '") + table + QLatin1Char('\'');
            return false;
        }
        return true;
    };

    return validateTable(QStringLiteral("conversations"),
                         {QStringLiteral("us_ip"), QStringLiteral("remote_ip"), QStringLiteral("remote_port"),
                          QStringLiteral("protocol"), QStringLiteral("packets_out"), QStringLiteral("packets_in"),
                          QStringLiteral("bytes_out"), QStringLiteral("bytes_in"), QStringLiteral("first_seen"),
                          QStringLiteral("last_seen"), QStringLiteral("remote_name"), QStringLiteral("application")})
        && validateTable(QStringLiteral("dns_cache"),
                         {QStringLiteral("ip"), QStringLiteral("name"), QStringLiteral("observed_at"),
                          QStringLiteral("expires_at")});
}

bool DatabaseReader::open(const QString &path, QString &error)
{
    // QFileInfo checks keep a typo from creating a new SQLite file, even if a
    // future Qt SQLite driver changes its default open behavior.
    const QFileInfo file(path);
    if (!file.exists() || !file.isFile() || !file.isReadable()) {
        error = QStringLiteral("Database must be an existing, readable file: ") + path;
        return false;
    }

    m_connectionName = QStringLiteral("pigeon-reader-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    if (!m_db.isValid()) {
        error = QStringLiteral("Qt SQLite driver (QSQLITE) is unavailable");
        close();
        return false;
    }
    m_db.setDatabaseName(file.absoluteFilePath());
    m_db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=150"));
    if (!m_db.open()) {
        error = QStringLiteral("Open read-only database: ") + m_db.lastError().text();
        close();
        return false;
    }
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("PRAGMA query_only=ON"))) {
        error = queryError(query, QStringLiteral("Protect read-only connection"));
        query.finish();
        // query is destroyed after this scope; do not remove the connection here
    } else {
        query.finish();
        if (checkSchema(error)) {
            m_path = file.absoluteFilePath();
            return true;
        }
    }
    // 'query' must be destroyed before removeDatabase(), including on failure.
    query = QSqlQuery();
    close();
    return false;
}

bool DatabaseReader::read(const QString &path, const ReadOptions &options, qint64 unixSeconds,
                          Snapshot &output, QString &error)
{
    if (!validOptions(options)) {
        error = QStringLiteral("Invalid dashboard filter or sort options");
        return false;
    }
    if (path.isEmpty()) {
        error = QStringLiteral("Select an existing Pigeon SQLite database");
        return false;
    }
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    if (absolutePath != m_path) {
        close();
        if (!open(absolutePath, error))
            return false;
    }

    QSqlQuery tx(m_db);
    if (!tx.exec(QStringLiteral("BEGIN"))) {
        error = queryError(tx, QStringLiteral("Begin read snapshot"));
        return false;
    }
    tx.finish();
    Snapshot candidate;
    const bool success = load(candidate, options, unixSeconds, error);
    if (success && tx.exec(QStringLiteral("COMMIT"))) {
        output = std::move(candidate);
        error.clear();
        return true;
    }
    if (success)
        error = queryError(tx, QStringLiteral("Commit read snapshot"));
    tx.finish();
    tx.exec(QStringLiteral("ROLLBACK"));
    return false;
}

bool DatabaseReader::load(Snapshot &snapshot, const ReadOptions &options, qint64 now, QString &error)
{
    snapshot.loadedAt = now;
    return readOverview(snapshot, error)
        && readApplications(snapshot.applications, options.sort, error)
        && readConversations(snapshot.live, options, now, false, error)
        && readConversations(snapshot.top, options, now, true, error)
        && readEndpoints(snapshot.endpoints, options, error);
}

bool DatabaseReader::readOverview(Snapshot &snapshot, QString &error)
{
    {
        QSqlQuery q(m_db);
        if (!q.exec(QStringLiteral("SELECT count(*),count(DISTINCT remote_ip),max(last_seen) FROM conversations"))
            || !q.next()) {
            error = queryError(q, QStringLiteral("Read overview"));
            return false;
        }
        if (!nonnegative(q, 0, snapshot.conversations) || !nonnegative(q, 1, snapshot.remoteIps)) {
            error = QStringLiteral("Invalid overview counts");
            return false;
        }
        snapshot.hasPackets = !q.value(2).isNull();
        if (snapshot.hasPackets)
            snapshot.latestPacket = q.value(2).toLongLong();
    }
    // SQLite SUM(INT64) can overflow, while TOTAL() rounds beyond 2^53.
    // Scan exact integer payload counters to calculate unrounded rate deltas.
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT bytes_in,bytes_out FROM conversations"))) {
        error = queryError(q, QStringLiteral("Read exact counters"));
        return false;
    }
    while (q.next()) {
        quint64 down = 0, up = 0;
        if (!nonnegative(q, 0, down) || !nonnegative(q, 1, up)) {
            error = QStringLiteral("Invalid cumulative byte counters");
            return false;
        }
        snapshot.exactDown.add(down);
        snapshot.exactUp.add(up);
    }
    if (q.lastError().isValid()) {
        error = queryError(q, QStringLiteral("Scan exact counters"));
        return false;
    }
    snapshot.bytesDown = snapshot.exactDown.approximate();
    snapshot.bytesUp = snapshot.exactUp.approximate();
    return true;
}

bool DatabaseReader::readConversations(QVector<Conversation> &rows, const ReadOptions &options,
                                       qint64 now, bool top, QString &error)
{
    QString sql = QStringLiteral(
        "SELECT us_ip,remote_ip,remote_port,protocol,packets_out,packets_in,bytes_out,bytes_in,"
        "first_seen,last_seen,coalesce(remote_name,''),application FROM conversations "
        "WHERE (?=0 OR protocol=?) AND (?=0 OR length(remote_ip)=?) "
        "AND (?=0 OR application=?) ");
    if (!top)
        sql += QStringLiteral("AND (?=0 OR last_seen>=?) AND last_seen<=? ");
    if (top) {
        sql += QStringLiteral(
            "ORDER BY CASE ? WHEN 1 THEN bytes_in WHEN 2 THEN bytes_out "
            "ELSE cast(bytes_in AS REAL)+bytes_out END DESC,"
            "last_seen DESC,remote_ip,remote_port,protocol,us_ip,application");
    } else {
        sql += QStringLiteral("ORDER BY last_seen DESC,remote_ip,remote_port,protocol,us_ip,application");
    }
    // On text search, traverse ordered candidates until we have 200 matches.
    // IP formatting is performed on the owned BLOB so IPv6 searches work too.
    if (options.search.isEmpty())
        sql += QStringLiteral(" LIMIT 200");

    QSqlQuery q(m_db);
    if (!q.prepare(sql)) {
        error = queryError(q, QStringLiteral("Prepare conversations"));
        return false;
    }
    q.addBindValue(options.protocol);
    q.addBindValue(options.protocol);
    const int familyLength = options.family == 4 ? 4 : options.family == 6 ? 16 : 0;
    q.addBindValue(familyLength);
    q.addBindValue(familyLength);
    q.addBindValue(options.applicationSelected ? 1 : 0);
    // A null QString binds as SQL NULL on QSQLITE, but unresolved ownership is
    // persisted as an empty TEXT value. Preserve that distinction when selected.
    q.addBindValue(options.application.isNull() ? QStringLiteral("") : options.application);
    if (!top) {
        const qint64 cutoff = now < std::numeric_limits<qint64>::min() + options.recentSeconds
            ? std::numeric_limits<qint64>::min() : now - options.recentSeconds;
        q.addBindValue(options.recentSeconds);
        q.addBindValue(cutoff);
        q.addBindValue(now);
    } else {
        q.addBindValue(options.sort);
    }
    if (!q.exec()) {
        error = queryError(q, QStringLiteral("Query conversations"));
        return false;
    }
    while (q.next()) {
        Conversation row;
        if (!addressAt(q, 0, row.localAddress, row.localIp)
            || !addressAt(q, 1, row.remoteAddress, row.remoteIp)
            || row.localAddress.size() != row.remoteAddress.size()) {
            error = QStringLiteral("Invalid conversation address BLOB");
            return false;
        }
        bool portOk = false, protoOk = false;
        row.remotePort = q.value(2).toInt(&portOk);
        row.protocol = q.value(3).toInt(&protoOk);
        if (!portOk || row.remotePort < 0 || row.remotePort > 65535
            || !protoOk || (row.protocol != 6 && row.protocol != 17 && row.protocol != 1 && row.protocol != 58)
            || !nonnegative(q, 4, row.packetsOut) || !nonnegative(q, 5, row.packetsIn)
            || !nonnegative(q, 6, row.bytesOut) || !nonnegative(q, 7, row.bytesIn)) {
            error = QStringLiteral("Invalid conversation counters, protocol or port");
            return false;
        }
        row.firstSeen = q.value(8).toLongLong();
        row.lastSeen = q.value(9).toLongLong();
        row.remoteName = q.value(10).toString();
        row.application = q.value(11).toString();
        const bool invalidControl = (row.protocol == 1 || row.protocol == 58)
            && (row.remotePort != 0 || !row.application.isEmpty()
                || row.localAddress.size() != (row.protocol == 1 ? 4 : 16));

        if (invalidControl || row.firstSeen > row.lastSeen || row.application.toUtf8().size() > 255) {
            error = QStringLiteral("Invalid conversation identity or timestamps");
            return false;
        }
        if (!options.search.isEmpty() && !matches(options.search, row))
            continue;
        rows.append(std::move(row));
        if (rows.size() == MaxRows)
            break;
    }
    if (q.lastError().isValid()) {
        error = queryError(q, QStringLiteral("Read conversations"));
        return false;
    }
    return true;
}

bool DatabaseReader::readApplications(QVector<Application> &rows, int sort, QString &error)
{
    QSqlQuery q(m_db);
    const QString sql = QStringLiteral(
        "SELECT application,total(bytes_in),total(bytes_out),count(*),count(DISTINCT remote_ip),"
        "max(last_seen) FROM conversations GROUP BY application "
        "ORDER BY CASE ? WHEN 1 THEN total(bytes_in) WHEN 2 THEN total(bytes_out) "
        "ELSE total(bytes_in)+total(bytes_out) END DESC,application LIMIT 200");
    if (!q.prepare(sql)) {
        error = queryError(q, QStringLiteral("Prepare application totals"));
        return false;
    }
    q.addBindValue(sort);
    if (!q.exec()) {
        error = queryError(q, QStringLiteral("Read application totals"));
        return false;
    }
    while (q.next()) {
        Application row;
        row.name = q.value(0).toString();
        row.bytesDown = q.value(1).toDouble();
        row.bytesUp = q.value(2).toDouble();
        if (!nonnegative(q, 3, row.conversations) || !nonnegative(q, 4, row.remoteIps)) {
            error = QStringLiteral("Invalid application counts");
            return false;
        }
        row.lastSeen = q.value(5).toLongLong();
        rows.append(std::move(row));
    }
    if (q.lastError().isValid()) {
        error = queryError(q, QStringLiteral("Read applications"));
        return false;
    }
    return true;
}

bool DatabaseReader::readEndpoints(QVector<Endpoint> &rows, const ReadOptions &options, QString &error)
{
    QString sql = QStringLiteral(
        "SELECT remote_ip,coalesce(max(remote_name),''),min(first_seen),max(last_seen) "
        "FROM conversations WHERE (?=0 OR length(remote_ip)=?) GROUP BY remote_ip "
        "ORDER BY min(first_seen) DESC,remote_ip");
    if (options.search.isEmpty())
        sql += QStringLiteral(" LIMIT 200");
    QSqlQuery q(m_db);
    if (!q.prepare(sql)) {
        error = queryError(q, QStringLiteral("Prepare new endpoints"));
        return false;
    }
    const int familyLength = options.family == 4 ? 4 : options.family == 6 ? 16 : 0;
    q.addBindValue(familyLength);
    q.addBindValue(familyLength);
    if (!q.exec()) {
        error = queryError(q, QStringLiteral("Query new endpoints"));
        return false;
    }
    while (q.next()) {
        Endpoint row;
        if (!addressAt(q, 0, row.address, row.ip)) {
            error = QStringLiteral("Invalid remote endpoint address BLOB");
            return false;
        }
        row.name = q.value(1).toString();
        row.firstSeen = q.value(2).toLongLong();
        row.lastSeen = q.value(3).toLongLong();
        if (!options.search.isEmpty()
            && !row.name.contains(options.search, Qt::CaseInsensitive)
            && !row.ip.contains(options.search, Qt::CaseInsensitive))
            continue;
        rows.append(std::move(row));
        if (rows.size() == MaxRows)
            break;
    }
    if (q.lastError().isValid()) {
        error = queryError(q, QStringLiteral("Read new endpoints"));
        return false;
    }
    return true;
}

} // namespace pigeon
