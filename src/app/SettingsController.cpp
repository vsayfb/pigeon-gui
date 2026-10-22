#include "app/SettingsController.h"

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

void SettingsController::run(const QStringList &arguments, const QString &operation)
{
    if (busy())
        return;
    if (m_cli.isEmpty()) {
        m_message = tr("Install the Pigeon CLI to configure the collector.");
        m_available = false;
        emit changed();
        return;
    }
    m_operation = operation;
    m_terminalMode = false;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PIGEON_GUI_AUTH"), QStringLiteral("1"));
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(m_cli);
    m_process.setArguments(arguments);
    if (operation != QStringLiteral("status") &&
        QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty()) {
        QString terminal = QStandardPaths::findExecutable(QStringLiteral("x-terminal-emulator"));
        if (terminal.isEmpty())
            terminal = QStandardPaths::findExecutable(QStringLiteral("konsole"));
        if (terminal.isEmpty())
            terminal = QStandardPaths::findExecutable(QStringLiteral("xterm"));
        if (terminal.isEmpty()) {
            m_message = tr("Administrator authorization is unavailable. Configure Pigeon from a terminal.");
            emit changed();
            return;
        }
        m_authFiles = std::make_unique<QTemporaryDir>();
        QFile script(m_authFiles->path() + QStringLiteral("/authorize.sh"));
        if (!m_authFiles->isValid() || !script.open(QIODevice::WriteOnly)) {
            m_message = tr("Could not prepare the authorization prompt.");
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
    if (!valid)
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
    if (code != 0 || status != QProcess::NormalExit) {
        m_message = error.isEmpty() ? tr("Settings were not changed. Authorization may have been cancelled.") : error;
        if (m_operation == QStringLiteral("status")) {
            m_available = false;
            m_message = tr("Install or update the Pigeon CLI to configure collector settings.");
        }
        emit changed();
        return;
    }
    if (m_operation == QStringLiteral("database")) {
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
