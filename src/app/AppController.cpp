#include "app/AppController.h"

#include <QDateTime>
#include <QFileInfo>
#include <QUrl>

namespace pigeon {

void ReaderWorker::refresh(quint64 token, const QString &path, const ReadOptions &options)
{
    if (!m_clock.isValid())
        m_clock.start();
    Snapshot candidate;
    QString error;
    const bool ok = m_reader.read(path, options, QDateTime::currentSecsSinceEpoch(), candidate, error);
    if (ok) {
        // Only committed, successful reads advance rate baselines.
        if (m_samplePath != path) {
            m_sampler.reset();
            m_samplePath = path;
        }
        m_sampler.sample(candidate, m_clock.nsecsElapsed() / 1000000000.0, options.liveSort);
    }
    emit completed(token, path, ok, candidate, error);
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
    connect(worker, &ReaderWorker::completed, this, &AppController::acceptResult,
            Qt::QueuedConnection);
    m_thread.start();
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &AppController::requestRefresh);
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
    m_path = cleaned;
    m_error.clear();
    m_status = m_path.isEmpty() ? QStringLiteral("Choose an existing schema-v4 Pigeon database")
                                : QStringLiteral("Opening selected database read-only…");
    m_pending = true;
    emit stateChanged();
    if (!m_inFlight)
        requestRefresh();
}

void AppController::requestRefresh()
{
    if (m_path.isEmpty() || m_inFlight)
        return;
    m_pending = false;
    m_inFlight = true;
    m_activeToken = ++m_nextToken;
    emit refreshRequested(m_activeToken, m_path, m_options);
}

void AppController::acceptResult(quint64 token, const QString &path, bool ok,
                                 const Snapshot &snapshot, const QString &error)
{
    if (token != m_activeToken)
        return;
    m_inFlight = false;
    if (path != m_path) {
        // A previous database's worker response may NEVER replace a new choice.
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
    } else {
        // Keep the last good snapshot AND model rows on error.
        m_error = error;
        m_status = m_hasSnapshot ? QStringLiteral("Read failed · showing previous snapshot")
                                 : QStringLiteral("Unable to read database");
    }
    emit stateChanged();
    if (m_pending)
        requestRefresh();
}

} // namespace pigeon
