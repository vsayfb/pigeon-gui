#include "app/AppController.h"

#include <QDateTime>
#include <QFileInfo>
#include <QHostAddress>
#include <QUrl>

#include <utility>

namespace pigeon {

void ReaderWorker::refresh(quint64 token, const QString &path, const ReadOptions &options)
{
    if (!m_clock.isValid())
        m_clock.start();

    Snapshot candidate;
    QString error;
    const bool ok = m_reader.read(path, options, QDateTime::currentSecsSinceEpoch(), candidate, error);
    m_candidateReady = false;
    if (ok) {
        // A rate calculation is provisional until the controller accepts the
        // matching generation. A superseded result cannot advance baselines.
        m_candidateSampler = m_sampler;
        m_candidatePath = m_samplePath;
        if (m_candidatePath != path) {
            m_candidateSampler.reset();
            m_candidatePath = path;
        }
        m_candidateSampler.sample(candidate, m_clock.nsecsElapsed() / 1000000000.0, options.liveSort);
        m_candidateToken = token;
        m_candidateReady = true;
    }
    emit completed(token, path, ok, candidate, error);
}

void ReaderWorker::finalize(quint64 token, bool accepted)
{
    if (m_candidateReady && m_candidateToken == token && accepted) {
        m_sampler = std::move(m_candidateSampler);
        m_samplePath = m_candidatePath;
    }
    m_candidateReady = false;
}

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_live(this)
    , m_top(this)
    , m_applications(this)
    , m_endpoints(this)
{
    qRegisterMetaType<ReadOptions>();
    qRegisterMetaType<Snapshot>();
    auto *worker = new ReaderWorker;
    worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &AppController::refreshRequested, worker, &ReaderWorker::refresh,
            Qt::QueuedConnection);
    connect(this, &AppController::finalizeRequested, worker, &ReaderWorker::finalize,
            Qt::QueuedConnection);
    connect(worker, &ReaderWorker::completed, this, &AppController::acceptResult,
            Qt::QueuedConnection);
    m_thread.start();
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (!m_paused)
            requestRefresh();
    });
    m_timer.start();
}

AppController::~AppController()
{
    m_timer.stop();
    m_thread.quit();
    m_thread.wait(); // worker finishes its current short read before destruction
}

QString AppController::downloadRate() const
{
    return m_snapshot.ratesValid ? QString::number(m_snapshot.rateDown, 'f', 1)
                                 : QStringLiteral("Waiting for second sample");
}

QString AppController::uploadRate() const
{
    return m_snapshot.ratesValid ? QString::number(m_snapshot.rateUp, 'f', 1)
                                 : QStringLiteral("Waiting for second sample");
}

QVariantList AppController::trafficHistory() const
{
    QVariantList history;
    history.reserve(m_snapshot.history.size());
    for (const RatePoint &point : m_snapshot.history) {
        history.append(QVariantMap{{QStringLiteral("down"), point.down},
                                   {QStringLiteral("up"), point.up},
                                   {QStringLiteral("seconds"), point.elapsedSeconds}});
    }
    return history;
}

QString AppController::lastPacketAge() const
{
    if (!m_hasSnapshot || !m_snapshot.hasPackets)
        return QStringLiteral("—");

    const qint64 seconds = QDateTime::currentSecsSinceEpoch() - m_snapshot.latestPacket;
    if (seconds < 0)
        return QStringLiteral("Future packet timestamp");
    if (seconds < 60)
        return QStringLiteral("%1s ago").arg(seconds);
    if (seconds < 3600)
        return QStringLiteral("%1m ago").arg(seconds / 60);
    if (seconds < 86400)
        return QStringLiteral("%1h ago").arg(seconds / 3600);
    return QStringLiteral("%1d ago").arg(seconds / 86400);
}

QString AppController::endpointName(const QString &name, const QString &ip) const
{
    if (!name.isEmpty())
        return name;
    if (addressScope(ip).isEmpty())
        return QStringLiteral("-");

    const QHostAddress address(ip);
    bool converted = false;
    const quint32 v4 = address.toIPv4Address(&converted);
    return address.isLoopback() || (converted && (v4 >> 24) == 127)
        ? tr("This device") : tr("Local network");
}

QString AppController::addressScope(const QString &ip) const
{
    const QHostAddress address(ip);
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        const quint32 value = address.toIPv4Address();
        const unsigned first = value >> 24;
        const unsigned second = (value >> 16) & 255;
        if (first == 10 || (first == 172 && (second & 0xf0) == 16) || (first == 192 && second == 168))
            return tr("Private address range (may include VPN networks)");
        if (first == 127)
            return tr("Loopback address (this device)");
        if (first == 169 && second == 254)
            return tr("Link-local address range");
        if ((value & 0xffffff00U) == 0xe0000000U || first == 239)
            return tr("Locally scoped multicast address");
        if (value == 0xffffffffU)
            return tr("Local broadcast address");
        return {};
    }
    if (address.protocol() != QAbstractSocket::IPv6Protocol)
        return {};

    const Q_IPV6ADDR bytes = address.toIPv6Address();
    bool mapped = bytes[10] == 255 && bytes[11] == 255;
    for (int i = 0; i < 10; ++i)
        mapped = mapped && bytes[i] == 0;
    if (mapped) {
        const quint32 value = (quint32(bytes[12]) << 24) | (quint32(bytes[13]) << 16)
                            | (quint32(bytes[14]) << 8) | bytes[15];
        return addressScope(QHostAddress(value).toString());
    }
    if (address.isLoopback())
        return tr("Loopback address (this device)");
    if ((bytes[0] & 0xfe) == 0xfc)
        return tr("Private address range (may include VPN networks)");
    if (bytes[0] == 0xfe && (bytes[1] & 0xc0) == 0x80)
        return tr("Link-local address range");
    const unsigned scope = bytes[1] & 15;
    if (bytes[0] == 0xff && (scope == 1 || scope == 2 || scope == 3 || scope == 4 || scope == 5 || scope == 8))
        return tr("Locally scoped multicast address");
    return {};
}

void AppController::selectDatabase(const QString &requestedPath)
{
    QString cleaned = requestedPath.trimmed();
    const QUrl url(cleaned);
    if (url.isValid() && url.isLocalFile())
        cleaned = url.toLocalFile();
    if (!cleaned.isEmpty())
        cleaned = QFileInfo(cleaned).absoluteFilePath();
    if (cleaned == m_path)
        return;

    ++m_generation;
    m_path = cleaned;
    m_paused = false; // explicitly opening a different database resumes sampling
    m_hasSnapshot = false;
    m_snapshot = {};
    m_live.setRows({});
    m_top.setRows({});
    m_applications.setRows({});
    m_endpoints.setRows({});
    clearSelection();
    m_error.clear();
    m_status = m_path.isEmpty() ? QStringLiteral("Choose an existing schema-v4 Pigeon database")
                                : QStringLiteral("Opening selected database read-only…");
    m_pending = true;
    emit stateChanged();
    if (!m_inFlight)
        requestRefresh();
}

void AppController::retryRead()
{
    if (m_path.isEmpty())
        return;

    // A retry is explicit: it also resumes a paused reader. If a query is
    // already running, queue a fresh query rather than overlapping SQLite work.
    m_paused = false;
    m_pending = true;
    m_status = QStringLiteral("Retrying database read…");
    emit stateChanged();
    if (!m_inFlight)
        requestRefresh();
}

void AppController::requestRefresh()
{
    if (m_path.isEmpty() || m_inFlight || m_paused)
        return;
    m_pending = false;
    m_inFlight = true;
    m_activeToken = ++m_nextToken;
    m_activeGeneration = m_generation;
    emit refreshRequested(m_activeToken, m_path, m_options);
}

void AppController::acceptResult(quint64 token, const QString &path, bool ok,
                                 const Snapshot &snapshot, const QString &error)
{
    if (token != m_activeToken)
        return;
    m_inFlight = false;
    const bool accepted = path == m_path && m_activeGeneration == m_generation && !m_paused;
    // Queued to the worker before any next refresh; it commits only the accepted
    // snapshot's rate baselines, in the same order as queued refresh requests.
    emit finalizeRequested(token, accepted && ok);
    if (!accepted) {
        // Never publish a stale read or an obsolete set of filters.
        if (!m_paused)
            requestRefresh();
        return;
    }
    if (ok) {
        m_snapshot = snapshot;
        m_hasSnapshot = true;
        m_error.clear();
        m_status = QStringLiteral("Read-only snapshot · ")
                 + QDateTime::fromSecsSinceEpoch(snapshot.loadedAt).toString(Qt::ISODate);
        m_live.setRows(snapshot.live);
        m_top.setRows(snapshot.top);
        m_applications.setRows(snapshot.applications);
        m_endpoints.setRows(snapshot.endpoints);
        updateSelection();
    } else {
        // Keep the last good snapshot AND model rows on error.
        m_error = error;
        m_status = m_hasSnapshot ? QStringLiteral("Read failed · showing previous snapshot")
                                 : QStringLiteral("Unable to read database");
    }
    emit stateChanged();
    if (m_pending && !m_paused)
        requestRefresh();
}

} // namespace pigeon

namespace pigeon {

void AppController::optionsChanged()
{
    ++m_generation;
    m_pending = true;
    emit filtersChanged();
    if (!m_inFlight && !m_paused)
        requestRefresh();
}

void AppController::setSearch(const QString &search)
{
    const QString value = search.left(127);
    if (m_options.search == value)
        return;
    m_options.search = value;
    optionsChanged();
}

void AppController::setProtocol(int protocol)
{
    if ((protocol != 0 && protocol != 6 && protocol != 17 && protocol != 1 && protocol != 58 && protocol != 50) || m_options.protocol == protocol)
        return;
    m_options.protocol = protocol;
    optionsChanged();
}

void AppController::setFamily(int family)
{
    if ((family != 0 && family != 4 && family != 6) || m_options.family == family)
        return;
    m_options.family = family;
    optionsChanged();
}

void AppController::setRecentSeconds(int seconds)
{
    if ((seconds != 0 && seconds != 10 && seconds != 30 && seconds != 120)
        || m_options.recentSeconds == seconds)
        return;
    m_options.recentSeconds = seconds;
    optionsChanged();
}

void AppController::setRankingSort(int sort)
{
    if (sort < 0 || sort > 2 || m_options.sort == sort)
        return;
    m_options.sort = sort;
    optionsChanged();
}

void AppController::setLiveSort(int sort)
{
    if (sort < 0 || sort > 3 || m_options.liveSort == sort)
        return;
    m_options.liveSort = sort;
    optionsChanged();
}

void AppController::selectApplication(const QString &application)
{
    if (application.toUtf8().size() > 255)
        return;

    // Clear text search even when the same application is selected again.
    if (m_options.applicationSelected && m_options.application == application
        && m_options.search.isEmpty())
        return;
    m_options.application = application.isNull() ? QStringLiteral("") : application;
    m_options.applicationSelected = true;
    m_options.search.clear();
    optionsChanged();
}

void AppController::clearApplication()
{
    if (!m_options.applicationSelected)
        return;
    m_options.applicationSelected = false;
    m_options.application.clear();
    optionsChanged();
}

void AppController::clearFilters()
{
    const bool changed = !m_options.search.isEmpty() || m_options.protocol != 0
        || m_options.family != 0 || m_options.recentSeconds != 30
        || m_options.sort != 0 || m_options.liveSort != 0 || m_options.applicationSelected;
    if (!changed)
        return;
    m_options = {};
    optionsChanged();
}

void AppController::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    if (paused) {
        // Invalidate a read already in flight, freezing both UI and rate baseline.
        ++m_generation;
    } else {
        m_pending = true;
        if (!m_inFlight)
            requestRefresh();
    }
    emit stateChanged();
}

void AppController::selectConversation(const QString &identity, bool live)
{
    if (m_selectedIdentity == identity && m_selectedLive == live)
        return;
    m_selectedIdentity = identity;
    m_selectedLive = live;
    m_selectedDetails.clear();
    updateSelection();
}

void AppController::clearSelection()
{
    if (m_selectedIdentity.isEmpty() && m_selectedDetails.isEmpty())
        return;
    m_selectedIdentity.clear();
    m_selectedDetails.clear();
    emit selectionChanged();
}

void AppController::updateSelection()
{
    if (m_selectedIdentity.isEmpty())
        return;

    Conversation row;
    const bool found = (m_selectedLive ? m_live : m_top).findIdentity(m_selectedIdentity, row);
    if (!found) {
        // Keep the last known detail visible, marked as not in the current view.
        m_selectedDetails.insert(QStringLiteral("inCurrentView"), false);
    } else {
        m_selectedDetails = {
            {QStringLiteral("inCurrentView"), true},
            {QStringLiteral("application"), row.application.isEmpty() ? QStringLiteral("Process not identified") : row.application},
            {QStringLiteral("localIp"), row.localIp},
            {QStringLiteral("remoteIp"), row.remoteIp},
            {QStringLiteral("remoteName"), row.remoteName},
            {QStringLiteral("remotePort"), row.remotePort},
            {QStringLiteral("protocol"), row.protocol},
            {QStringLiteral("quic"), row.quic},
            {QStringLiteral("bytesDown"), QString::number(row.bytesIn)},
            {QStringLiteral("bytesUp"), QString::number(row.bytesOut)},
            {QStringLiteral("packetsDown"), QString::number(row.packetsIn)},
            {QStringLiteral("packetsUp"), QString::number(row.packetsOut)},
            {QStringLiteral("firstSeen"), row.firstSeen},
            {QStringLiteral("lastSeen"), row.lastSeen},
            {QStringLiteral("ratesValid"), row.rateValid},
            {QStringLiteral("rateDown"), row.rateDown},
            {QStringLiteral("rateUp"), row.rateUp}
        };
    }
    emit selectionChanged();
}

} // namespace pigeon
