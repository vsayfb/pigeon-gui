#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace pigeon {

constexpr int MaxRows = 200;
constexpr int HistorySamples = 60;

// Exact, portable unsigned sum of nonnegative SQLite INT64 counters.
// Neither lifetime totals nor the delta calculation round at 2^53.
struct Counter128 {
    quint64 high = 0;
    quint64 low = 0;

    void add(quint64 value)
    {
        const quint64 old = low;
        low += value;
        high += low < old ? 1 : 0;
    }
    bool operator<(const Counter128 &other) const
    {
        return high < other.high || (high == other.high && low < other.low);
    }
    QString decimal() const
    {
        quint32 words[4] = {quint32(high >> 32), quint32(high),
                            quint32(low >> 32), quint32(low)};
        QString result;
        do {
            quint64 remainder = 0;
            bool nonzero = false;
            for (quint32 &word : words) {
                const quint64 current = (remainder << 32) | word;
                word = quint32(current / 10);
                remainder = current % 10;
                nonzero |= word != 0;
            }
            result.prepend(QChar(ushort('0' + remainder)));
            if (!nonzero)
                break;
        } while (true);
        return result;
    }
    double approximate() const
    {
        return static_cast<double>(high) * 18446744073709551616.0
             + static_cast<double>(low);
    }
    double difference(const Counter128 &previous) const
    {
        const quint64 lowDelta = low - previous.low;
        const quint64 borrowed = low < previous.low ? 1 : 0;
        const quint64 highDelta = high - previous.high - borrowed;
        return static_cast<double>(highDelta) * 18446744073709551616.0
             + static_cast<double>(lowDelta);
    }
};

struct ReadOptions {
    QString search;
    int protocol = 0;       // 0 = all, 6 = TCP, 17 = UDP
    int family = 0;         // 0 = all, 4 = IPv4, 6 = IPv6
    int recentSeconds = 30; // 0 = all recorded
    int sort = 0;           // 0 = total, 1 = download, 2 = upload
    int liveSort = 0;       // 0 = recency, 1 = total, 2 = download, 3 = upload
    bool applicationSelected = false; // permits selecting the empty application
    QString application;
};

struct Conversation {
    QByteArray localAddress;
    QByteArray remoteAddress;
    QString localIp;
    QString remoteIp;
    QString application;
    QString remoteName;
    int remotePort = 0;
    int protocol = 0;
    quint64 packetsOut = 0;
    quint64 packetsIn = 0;
    quint64 bytesOut = 0;
    quint64 bytesIn = 0;
    qint64 firstSeen = 0;
    qint64 lastSeen = 0;
    bool rateValid = false;
    double rateDown = 0;
    double rateUp = 0;
    QVector<double> rateHistory; // combined rates, viewer session only

    QByteArray key() const
    {
        QByteArray id;
        id.reserve(2 + localAddress.size() + remoteAddress.size() + 3 + application.size() * 4);
        id.append(char(localAddress.size()));
        id.append(localAddress);
        id.append(char(remoteAddress.size()));
        id.append(remoteAddress);
        id.append(char((remotePort >> 8) & 0xff));
        id.append(char(remotePort & 0xff));
        id.append(char(protocol));
        id.append(application.toUtf8());
        return id;
    }
};

struct Application {
    QString name; // empty means unresolved, not all applications
    double bytesDown = 0;
    double bytesUp = 0;
    quint64 conversations = 0;
    quint64 remoteIps = 0;
    qint64 lastSeen = 0;
    QByteArray key() const { return name.toUtf8(); }
};

struct Endpoint {
    QByteArray address;
    QString ip;
    QString name;
    qint64 firstSeen = 0;
    qint64 lastSeen = 0;
    QByteArray key() const { return address; }
};

struct RatePoint {
    double down = 0;
    double up = 0;
    double elapsedSeconds = 0;
};

struct Snapshot {
    QVector<Conversation> live;
    QVector<Conversation> top;
    QVector<Application> applications;
    QVector<Endpoint> endpoints;
    quint64 conversations = 0;
    quint64 remoteIps = 0;
    Counter128 exactDown;
    Counter128 exactUp;
    double bytesDown = 0; // approximate display value; NOT used for rate deltas
    double bytesUp = 0;
    bool hasPackets = false;
    qint64 latestPacket = 0;
    qint64 loadedAt = 0;
    bool ratesValid = false;
    double rateDown = 0;
    double rateUp = 0;
    QVector<RatePoint> history; // viewer session only
};

} // namespace pigeon

Q_DECLARE_METATYPE(pigeon::ReadOptions)
Q_DECLARE_METATYPE(pigeon::Snapshot)
