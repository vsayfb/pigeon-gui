#pragma once

#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <memory>

namespace pigeon {

class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString databasePath READ databasePath NOTIFY changed)
    Q_PROPERTY(QString defaultDatabasePath READ defaultDatabasePath CONSTANT)
    Q_PROPERTY(bool autostart READ autostart NOTIFY changed)
    Q_PROPERTY(bool bootSupported READ bootSupported NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)

public:
    explicit SettingsController(QObject *parent = nullptr);
    QString databasePath() const { return m_database; }
    QString defaultDatabasePath() const { return m_root + QStringLiteral("/pigeon.db"); }
    bool autostart() const { return m_autostart; }
    bool bootSupported() const { return m_bootSupported; }
    bool available() const { return m_available; }
    bool busy() const { return m_retryAuthorization || m_waitingTerminal || m_process.state() != QProcess::NotRunning; }
    QString message() const { return m_message; }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setAutostart(bool enabled);
    Q_INVOKABLE void setDatabasePath(const QString &path);
    Q_INVOKABLE void clearHistory(const QString &database);
    Q_INVOKABLE void deleteConversation(const QString &identity, const QString &database);

signals:
    void changed();
    void databaseChanged(const QString &path);
    void historyDeleted(const QString &path);
    void historyResult(bool success, const QString &message);

private:
    void run(const QStringList &arguments, const QString &operation, bool terminalAuthorization = false);
    void finished(int code, QProcess::ExitStatus status);
    void readDatabase();
    void complete(int code, QProcess::ExitStatus status, const QString &output, const QString &error);
    void checkTerminal();
    QProcess m_process;
    QTimer m_terminalPoll;
    std::unique_ptr<QTemporaryDir> m_authFiles;
    bool m_waitingTerminal = false;
    bool m_terminalMode = false;
    bool m_retryAuthorization = false;
    QStringList m_arguments;
    QString m_historyDatabase;
    QString m_cli;
    QString m_root;
    QString m_database;
    QString m_operation;
    QString m_message;
    QString m_pendingDatabase;
    bool m_autostart = false;
    bool m_bootSupported = false;
    bool m_available = false;
};

} // namespace pigeon
