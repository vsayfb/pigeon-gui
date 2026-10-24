#include "app/SettingsController.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace pigeon {

SettingsController::SettingsController(QObject *parent) : QObject(parent)
{
    m_cli = QStandardPaths::findExecutable(QStringLiteral("pigeon"));
    if (m_cli.isEmpty()) {
        const QString candidate = QDir::homePath() + QStringLiteral("/.local/bin/pigeon");
        if (QFileInfo(candidate).isExecutable())
            m_cli = candidate;
    }

    m_root = qEnvironmentVariable("PIGEON_INSTALL_ROOT",
        QDir::homePath() + QStringLiteral("/.local/share/pigeon"));
    if (!m_cli.isEmpty()) {
        QDir bundle(QFileInfo(QFileInfo(m_cli).canonicalFilePath()).absolutePath());
        bundle.cdUp();
        if (QFileInfo(bundle.absolutePath()).dir().dirName() == QStringLiteral("versions")) {
            bundle.cdUp();
            bundle.cdUp();
            m_root = bundle.absolutePath();
        }
    }
    readDatabase();
    m_terminalPoll.setInterval(200);
    connect(&m_terminalPoll, &QTimer::timeout, this, &SettingsController::checkTerminal);
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &SettingsController::finished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_waitingTerminal = false;
            m_terminalPoll.stop();
            m_available = false;
            m_message = tr("Could not start the Pigeon CLI. Install or update the collector first.");
            if (m_operation == QStringLiteral("history"))
                historyFailed(m_message);
            emit changed();
        }
    });
}

void SettingsController::readDatabase()
{
    m_database = qEnvironmentVariable("PIGEON_DATABASE", defaultDatabasePath());
    QFile config(m_root + QStringLiteral("/database.conf"));
    if (!qEnvironmentVariableIsSet("PIGEON_DATABASE") && config.open(QIODevice::ReadOnly)) {
        const QString saved = QString::fromUtf8(config.readAll()).trimmed();
        if (QFileInfo(saved).isAbsolute())
            m_database = saved;
    }
}

void SettingsController::run(const QStringList &arguments, const QString &operation, bool terminalAuthorization)
{
    if (busy())
        return;
    if (m_cli.isEmpty()) {
        m_message = tr("Install the Pigeon CLI to configure the collector.");
        m_available = false;
        if (operation == QStringLiteral("history"))
            historyFailed(m_message);
        emit changed();
        return;
    }
    m_operation = operation;
    m_arguments = arguments;
    m_terminalMode = false;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PIGEON_GUI_AUTH"), QStringLiteral("1"));
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(m_cli);
    m_process.setArguments(arguments);
    if (terminalAuthorization || (operation != QStringLiteral("status") && operation != QStringLiteral("history") &&
        QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty())) {
        QString terminal = QStandardPaths::findExecutable(QStringLiteral("x-terminal-emulator"));
        if (terminal.isEmpty())
            terminal = QStandardPaths::findExecutable(QStringLiteral("konsole"));
        if (terminal.isEmpty())
            terminal = QStandardPaths::findExecutable(QStringLiteral("xterm"));
        if (terminal.isEmpty()) {
            m_message = tr("Administrator authorization is unavailable. Configure Pigeon from a terminal.");
            if (operation == QStringLiteral("history"))
                historyFailed(m_message);
            emit changed();
            return;
        }
        m_authFiles = std::make_unique<QTemporaryDir>();
        QFile script(m_authFiles->path() + QStringLiteral("/authorize.sh"));
        if (!m_authFiles->isValid() || !script.open(QIODevice::WriteOnly)) {
            m_message = tr("Could not prepare the authorization prompt.");
            if (operation == QStringLiteral("history"))
                historyFailed(m_message);
            emit changed();
            return;
        }
        script.write("#!/bin/sh\n"
                     "directory=$1; shift\n"
                     "trap 'code=$?; printf \"%s\\n\" \"$code\" > \"$directory/result\"' EXIT\n"
                     "trap 'exit 130' HUP INT TERM\n"
                     "export PIGEON_GUI_AUTH=0\n"
                     "\"$@\" > \"$directory/output\" 2> \"$directory/error\"\n"
                     "exit $?\n");
        script.close();
        QStringList terminalArguments{QStringLiteral("-e"), QStringLiteral("/bin/sh"), script.fileName(),
                                      m_authFiles->path(), m_cli};
        terminalArguments.append(arguments);
        m_process.setProgram(terminal);
        m_process.setArguments(terminalArguments);
        m_waitingTerminal = true;
        m_terminalMode = true;
        m_terminalPoll.start();
    }
    m_message = operation == QStringLiteral("status") ? tr("Checking collector settings…")
        : operation == QStringLiteral("history") && m_deleteTotal > 0
            ? tr("Deleting conversation %1 of %2…").arg(m_deleted + 1).arg(m_deleteTotal)
            : tr("Applying settings… Complete the administrator prompt if shown.");
    m_process.start();
    emit changed();
}

void SettingsController::refresh()
{
    readDatabase();
    run({QStringLiteral("autostart"), QStringLiteral("status")}, QStringLiteral("status"));
}

void SettingsController::setAutostart(bool enabled)
{
    run({QStringLiteral("autostart"), enabled ? QStringLiteral("on") : QStringLiteral("off")},
        QStringLiteral("autostart"));
}

void SettingsController::setDatabasePath(const QString &path)
{
    if (!QFileInfo(path).isAbsolute() || path.contains('\n') || path.contains('\r')) {
        m_message = tr("Choose an absolute database file path.");
        emit changed();
        return;
    }
    m_pendingDatabase = QDir::cleanPath(path);
    run({QStringLiteral("database"), m_pendingDatabase}, QStringLiteral("database"));
}

void SettingsController::clearHistory(const QString &database)
{
    if (busy())
        return;
    m_historyDatabase = QFileInfo(database).canonicalFilePath();
    if (m_historyDatabase.isEmpty()) {
        emit historyResult(false, tr("Open an existing traffic database first."));
        return;
    }
    run({QStringLiteral("history"), QStringLiteral("clear"), m_historyDatabase}, QStringLiteral("history"));
}

void SettingsController::deleteConversation(const QString &identity, const QString &database)
{
    deleteConversations({identity}, database);
}

void SettingsController::deleteConversations(const QStringList &identities, const QString &database)
{
    if (busy())
        return;
    m_historyDatabase = QFileInfo(database).canonicalFilePath();
    QStringList unique = identities;
    unique.removeDuplicates();
    if (m_historyDatabase.isEmpty() || unique.isEmpty() || unique.size() > 200) {
        emit historyResult(false, tr("Select between 1 and 200 recorded conversations first."));
        return;
    }
    for (const auto &identity : unique) {
        if (identity.isEmpty() || identity.size() > 584 || identity.size() % 2
            || std::any_of(identity.cbegin(), identity.cend(), [](QChar c) {
                return !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
            })) {
            emit historyResult(false, tr("The selected conversation identity is invalid."));
            return;
        }
    }
    m_deletions = unique;
    m_deleted = 0;
    m_deleteTotal = unique.size();
    run({QStringLiteral("history"), QStringLiteral("delete"), m_deletions.takeFirst(), m_historyDatabase},
        QStringLiteral("history"));
}

void SettingsController::historyFailed(const QString &message)
{
    m_message = m_deleteTotal > 1
        ? tr("Completed %1 of %2 deletions. Remaining conversations were not deleted. %3")
            .arg(m_deleted).arg(m_deleteTotal).arg(message) : message;
    m_deletions.clear();
    m_deleteTotal = 0;
    if (m_deleted > 0)
        emit historyDeleted(m_historyDatabase);
    m_deleted = 0;
    emit historyResult(false, m_message);
}

void SettingsController::finished(int code, QProcess::ExitStatus status)
{
    if (m_terminalMode) {
        // Some terminal emulators return before their command window closes.
        // The command's result file is authoritative, not the terminal PID.
        if (m_waitingTerminal)
            checkTerminal();
        else
            emit changed();
        return;
    }
    complete(code, status, QString::fromUtf8(m_process.readAllStandardOutput()).trimmed(),
             QString::fromUtf8(m_process.readAllStandardError()).trimmed());
}

void SettingsController::checkTerminal()
{
    if (!m_waitingTerminal || !m_authFiles)
        return;
    QFile result(m_authFiles->path() + QStringLiteral("/result"));
    if (!result.open(QIODevice::ReadOnly))
        return;
    bool valid = false;
    const int code = result.readAll().trimmed().toInt(&valid);
    if (!valid || m_process.state() != QProcess::NotRunning)
        return;
    QFile output(m_authFiles->path() + QStringLiteral("/output"));
    QFile error(m_authFiles->path() + QStringLiteral("/error"));
    output.open(QIODevice::ReadOnly);
    error.open(QIODevice::ReadOnly);
    m_waitingTerminal = false;
    m_terminalPoll.stop();
    complete(code, QProcess::NormalExit, QString::fromUtf8(output.readAll()).trimmed(),
             QString::fromUtf8(error.readAll()).trimmed());
}

void SettingsController::complete(int code, QProcess::ExitStatus status, const QString &output,
                                  const QString &error)
{
    if (m_operation == QStringLiteral("history") && code == 3 && !m_terminalMode) {
        m_retryAuthorization = true;
        QTimer::singleShot(0, this, [this] {
            m_retryAuthorization = false;
            run(m_arguments, QStringLiteral("history"), true);
        });
        emit changed();
        return;
    }
    if (code != 0 || status != QProcess::NormalExit) {
        m_message = error.isEmpty() ? tr("Settings were not changed. Authorization may have been cancelled.") : error;
        if (m_operation == QStringLiteral("status")) {
            m_available = false;
            m_message = tr("Install or update the Pigeon CLI to configure collector settings.");
        }
        if (m_operation == QStringLiteral("history"))
            historyFailed(m_message);
        emit changed();
        return;
    }
    if (m_operation == QStringLiteral("history")) {
        if (m_deleteTotal > 0)
            ++m_deleted;
        if (!m_deletions.isEmpty()) {
            m_retryAuthorization = true;
            QTimer::singleShot(0, this, [this] {
                m_retryAuthorization = false;
                run({QStringLiteral("history"), QStringLiteral("delete"), m_deletions.takeFirst(), m_historyDatabase},
                    QStringLiteral("history"));
            });
            emit changed();
            return;
        }
        m_message = m_deleteTotal > 1
            ? tr("Completed %1 deletions. New traffic can appear again.").arg(m_deleted)
            : output.isEmpty() ? tr("Recorded traffic deleted. New traffic can appear again.") : output;
        m_deleteTotal = 0;
        m_deleted = 0;
        emit historyDeleted(m_historyDatabase);
        emit historyResult(true, m_message);
    } else if (m_operation == QStringLiteral("database")) {
        readDatabase();
        emit databaseChanged(m_database);
        m_message = output;
    } else if (output.contains(QStringLiteral("Start at boot: on")) ||
               output.contains(QStringLiteral("Start at boot: off"))) {
        m_autostart = output.contains(QStringLiteral("Start at boot: on"));
        m_available = true;
        m_bootSupported = true;
        m_message = m_autostart ? tr("Pigeon starts automatically at boot.")
                               : tr("Start at boot is off. Current capture is left running.");
    } else if (output.contains(QStringLiteral("Start at boot: unknown"))) {
        m_available = true;
        m_bootSupported = false;
        m_message = tr("Boot startup status cannot be read. Update the Pigeon CLI; the service may still be enabled.");
    } else if (output.contains(QStringLiteral("Start at boot: unavailable"))) {
        m_available = true;
        m_bootSupported = false;
        m_message = tr("Boot startup is unavailable on this system. Database settings are available.");
    } else {
        m_available = false;
        m_bootSupported = false;
        m_message = output.isEmpty() ? tr("Update the Pigeon CLI to use these settings.") : output;
    }
    emit changed();
}

} // namespace pigeon
