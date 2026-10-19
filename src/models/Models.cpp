#include "models/Models.h"

#include <QVariant>

namespace pigeon {

QVariant ConversationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Conversation &r = m_rows.at(index.row());
    switch (role) {
    case IdentityRole: return QString::fromLatin1(r.key().toHex());
    case ApplicationRole: return r.application;
    case LocalIpRole: return r.localIp;
    case RemoteIpRole: return r.remoteIp;
    case RemoteNameRole: return r.remoteName;
    case RemotePortRole: return r.remotePort;
    case ProtocolRole: return r.protocol;
    case BytesDownRole: return QVariant::fromValue(r.bytesIn);
    case BytesUpRole: return QVariant::fromValue(r.bytesOut);
    case PacketsDownRole: return QVariant::fromValue(r.packetsIn);
    case PacketsUpRole: return QVariant::fromValue(r.packetsOut);
    case FirstSeenRole: return r.firstSeen;
    case LastSeenRole: return r.lastSeen;
    case RatesValidRole: return r.rateValid;
    case RateDownRole: return r.rateDown;
    case RateUpRole: return r.rateUp;
    case RateHistoryRole: {
        QVariantList points;
        points.reserve(r.rateHistory.size());
        for (double point : r.rateHistory)
            points.append(point);

        return points;
    }
    case BytesDownExactRole: return QString::number(r.bytesIn);
    case BytesUpExactRole: return QString::number(r.bytesOut);
    default: return {};
    }
}

QHash<int, QByteArray> ConversationsModel::roleNames() const
{
    return {{IdentityRole, "identity"}, {ApplicationRole, "application"}, {LocalIpRole, "localIp"},
            {RemoteIpRole, "remoteIp"}, {RemoteNameRole, "remoteName"}, {RemotePortRole, "remotePort"},
            {ProtocolRole, "protocol"}, {BytesDownRole, "bytesDown"}, {BytesUpRole, "bytesUp"},
            {PacketsDownRole, "packetsDown"}, {PacketsUpRole, "packetsUp"},
            {FirstSeenRole, "firstSeen"}, {LastSeenRole, "lastSeen"}, {RatesValidRole, "ratesValid"},
            {RateDownRole, "rateDown"}, {RateUpRole, "rateUp"},
            {BytesDownExactRole, "bytesDownExact"}, {BytesUpExactRole, "bytesUpExact"}, {RateHistoryRole, "rateHistory"}};
}

bool ConversationsModel::findIdentity(const QString &identity, Conversation &row) const
{
    // UI keys are encoded hex, so even embedded NULs remain unambiguous.
    const QByteArray key = QByteArray::fromHex(identity.toLatin1());
    if (key.isEmpty() || QString::fromLatin1(key.toHex()) != identity)
        return false;

    for (const Conversation &candidate : m_rows) {
        if (candidate.key() == key) {
            row = candidate;
            return true;
        }
    }
    return false;
}

QVariant ApplicationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Application &r = m_rows.at(index.row());
    switch (role) {
    case IdentityRole: return r.name;
    case NameRole: return r.name.isEmpty() ? QStringLiteral("Unresolved") : r.name;
    case BytesDownRole: return r.bytesDown;
    case BytesUpRole: return r.bytesUp;
    case ConversationsRole: return QVariant::fromValue(r.conversations);
    case RemoteIpsRole: return QVariant::fromValue(r.remoteIps);
    case LastSeenRole: return r.lastSeen;
    default: return {};
    }
}

QHash<int, QByteArray> ApplicationsModel::roleNames() const
{
    return {{IdentityRole, "identity"}, {NameRole, "name"}, {BytesDownRole, "bytesDown"},
            {BytesUpRole, "bytesUp"}, {ConversationsRole, "conversations"},
            {RemoteIpsRole, "remoteIps"}, {LastSeenRole, "lastSeen"}};
}

QVariant EndpointsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Endpoint &r = m_rows.at(index.row());
    switch (role) {
    case IdentityRole: return QString::fromLatin1(r.address.toHex());
    case IpRole: return r.ip;
    case NameRole: return r.name;
    case FirstSeenRole: return r.firstSeen;
    case LastSeenRole: return r.lastSeen;
    default: return {};
    }
}

QHash<int, QByteArray> EndpointsModel::roleNames() const
{
    return {{IdentityRole, "identity"}, {IpRole, "ip"}, {NameRole, "name"},
            {FirstSeenRole, "firstSeen"}, {LastSeenRole, "lastSeen"}};
}

} // namespace pigeon
