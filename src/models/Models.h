#pragma once

#include "data/Snapshot.h"

#include <QAbstractListModel>

namespace pigeon {

// Reconcile keyed rows using insert/move/remove, rather than resetting every
// second. Persistent indexes for unchanged identities remain valid.
template <typename Row>
class KeyedModel : public QAbstractListModel {
public:
    explicit KeyedModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_rows.size();
    }

protected:
    void update(QVector<Row> incoming)
    {
        for (int i = 0; i < incoming.size(); ++i) {
            int found = -1;
            for (int j = i; j < m_rows.size(); ++j) {
                if (m_rows[j].key() == incoming[i].key()) {
                    found = j;
                    break;
                }
            }
            if (found < 0) {
                beginInsertRows(QModelIndex(), i, i);
                m_rows.insert(i, std::move(incoming[i]));
                endInsertRows();
            } else {
                if (found > i) {
                    beginMoveRows(QModelIndex(), found, found, QModelIndex(), i);
                    m_rows.move(found, i);
                    endMoveRows();
                }
                m_rows[i] = std::move(incoming[i]);
                emit dataChanged(index(i), index(i));
            }
        }
        if (m_rows.size() > incoming.size()) {
            beginRemoveRows(QModelIndex(), incoming.size(), m_rows.size() - 1);
            m_rows.resize(incoming.size());
            endRemoveRows();
        }
    }

    QVector<Row> m_rows;
};

class ConversationsModel : public KeyedModel<Conversation> {
    Q_OBJECT
public:
    enum Roles {
        IdentityRole = Qt::UserRole + 1, ApplicationRole, LocalIpRole, RemoteIpRole,
        RemoteNameRole, RemotePortRole, ProtocolRole, BytesDownRole, BytesUpRole,
        PacketsDownRole, PacketsUpRole, FirstSeenRole, LastSeenRole, RatesValidRole,
        RateDownRole, RateUpRole, BytesDownExactRole, BytesUpExactRole
    };
    using KeyedModel::KeyedModel;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setRows(QVector<Conversation> rows) { update(std::move(rows)); }
};

class ApplicationsModel : public KeyedModel<Application> {
    Q_OBJECT
public:
    enum Roles {
        IdentityRole = Qt::UserRole + 1, NameRole, BytesDownRole, BytesUpRole,
        ConversationsRole, RemoteIpsRole, LastSeenRole
    };
    using KeyedModel::KeyedModel;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setRows(QVector<Application> rows) { update(std::move(rows)); }
};

class EndpointsModel : public KeyedModel<Endpoint> {
    Q_OBJECT
public:
    enum Roles { IdentityRole = Qt::UserRole + 1, IpRole, NameRole, FirstSeenRole, LastSeenRole };
    using KeyedModel::KeyedModel;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setRows(QVector<Endpoint> rows) { update(std::move(rows)); }
};

} // namespace pigeon
