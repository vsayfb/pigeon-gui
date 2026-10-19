#include "app/CaptureController.h"

#include <QFileInfo>
#include <QVariantMap>

namespace pigeon {

CaptureController::CaptureController(QObject *parent) : QObject(parent)
{
    m_deadline.setSingleShot(true);
    m_deadline.setInterval(10000);
    connect(&m_deadline, &QTimer::timeout, this, [this] {
        fail(QStringLiteral("Collector request timed out. Refresh to check the current source."));
    });
    connect(&m_socket, &QLocalSocket::connected, this, [this] {
        m_socket.write(m_command + '\n');
    });
    connect(&m_socket, &QLocalSocket::readyRead, this, [this] {
        m_reply += m_socket.readAll();
        if (m_reply.size() > 16384)
            fail(QStringLiteral("Invalid collector response"));
    });
    connect(&m_socket, &QLocalSocket::disconnected, this, &CaptureController::finish);
    connect(&m_socket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError error) {
        // EOF follows the complete reply; process it in disconnected().
        if (error != QLocalSocket::PeerClosedError && m_busy)
            fail(QStringLiteral("Collector unavailable: %1").arg(m_socket.errorString()));
    });
}

void CaptureController::setDatabasePath(const QString &path)
{
    if (path == m_database)
        return;

    m_busy = false; // discard results belonging to the previous database
    m_deadline.stop();
    m_socket.abort();
    m_database = path;
    m_current.clear();
    m_interfaces.clear();
    m_reply.clear();
    m_automatic = false;
    m_status = QStringLiteral("Collector not connected");
    emit changed();
    refresh();
}

void CaptureController::refresh()
{
    request("LIST");
}

void CaptureController::selectInterface(const QString &name)
{
    if (name.contains('\n') || name.contains('\r') || name.contains('\t') || name.contains(' ') ||
        name.contains('/') || name.toUtf8().size() >= 16) {
        fail(QStringLiteral("Invalid interface name"));
        return;
    }
    request(name.isEmpty() ? QByteArray("AUTO") : QByteArray("SELECT ") + name.toUtf8());
}

void CaptureController::request(const QByteArray &command)
{
    if (m_busy)
        return;

    const QString canonical = QFileInfo(m_database).canonicalFilePath();
    if (canonical.isEmpty()) {
        fail(QStringLiteral("Open an existing collector database first"));
        return;
    }

    const QString socketPath = canonical + QStringLiteral(".control");
    if (socketPath.toUtf8().size() >= 108) {
        fail(QStringLiteral("Database path is too long for collector control"));
        return;
    }

    m_command = command;
    m_reply.clear();
    m_busy = true;
    m_status = command == "LIST" ? QStringLiteral("Discovering capture interfaces…")
                                  : QStringLiteral("Applying capture source…");
    emit changed();
    m_deadline.start();
    m_socket.connectToServer(socketPath);
}

void CaptureController::finish()
{
    if (!m_busy)
        return;

    m_reply += m_socket.readAll();
    m_busy = false;
    m_deadline.stop();
    if (m_reply.size() > 16384 || !m_reply.endsWith('\n')) {
        fail(QStringLiteral("Incomplete collector response. Refresh to check the current source."));
        return;
    }

    const auto lines = m_reply.split('\n');
    const auto header = lines.first().split('\t');
    if (header.size() == 2 && header.first() == "ERROR") {
        fail(QString::fromUtf8(header.at(1)));
        return;
    }
    if (header.size() != 3 || header.first() != "OK" ||
        (header.at(2) != "auto" && header.at(2) != "manual")) {
        fail(QStringLiteral("Invalid collector response"));
        return;
    }

    m_current = QString::fromUtf8(header.at(1));
    m_automatic = header.at(2) == "auto";
    m_status = QStringLiteral("Capturing on %1 (%2)").arg(m_current, m_automatic ? "automatic" : "manual");
    if (m_command == "LIST") {
        QVariantList rows;
        for (qsizetype i = 1; i < lines.size(); ++i) {
            const auto fields = lines.at(i).split('\t');
            if (fields.size() != 3)
                continue;
            rows.append(QVariantMap{{"name", QString::fromUtf8(fields.at(0))},
                                    {"active", fields.at(1) == "active"},
                                    {"supported", fields.at(2) == "supported"},
                                    {"state", QString::fromUtf8(fields.at(1)) + " · " + QString::fromUtf8(fields.at(2))}});
        }
        m_interfaces = rows;
    }
    emit changed();
    if (m_command != "LIST") {
        // Let QLocalSocket complete its disconnect before opening the next
        // connection. Keep busy until the inventory refresh starts.
        m_busy = true;
        const QString database = m_database;
        QTimer::singleShot(0, this, [this, database] {
            if (database != m_database)
                return;
            m_busy = false;
            refresh();
        });
    }
}

void CaptureController::fail(const QString &message)
{
    m_busy = false;
    m_deadline.stop();
    m_socket.abort();
    m_status = message;
    emit changed();
}

} // namespace pigeon
