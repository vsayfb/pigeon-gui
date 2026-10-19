#pragma once

#include <QLocalSocket>
#include <QObject>
#include <QTimer>
#include <QVariantList>

namespace pigeon {

class CaptureController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString databasePath READ databasePath WRITE setDatabasePath NOTIFY changed)
    Q_PROPERTY(QString currentInterface READ currentInterface NOTIFY changed)
    Q_PROPERTY(bool automatic READ automatic NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantList interfaces READ interfaces NOTIFY changed)

public:
    explicit CaptureController(QObject *parent = nullptr);
    QString databasePath() const { return m_database; }
    QString currentInterface() const { return m_current; }
    bool automatic() const { return m_automatic; }
    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    QVariantList interfaces() const { return m_interfaces; }
    void setDatabasePath(const QString &path);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void selectInterface(const QString &name);

signals:
    void changed();

private:
    void request(const QByteArray &command);
    void finish();
    void fail(const QString &message);

    QLocalSocket m_socket;
    QTimer m_deadline;
    QString m_database;
    QString m_current;
    QString m_status = QStringLiteral("Collector not connected");
    QVariantList m_interfaces;
    QByteArray m_command;
    QByteArray m_reply;
    bool m_busy = false;
    bool m_automatic = false;
};

} // namespace pigeon
