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
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &SettingsController::finished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
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
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PIGEON_GUI_AUTH"), QStringLiteral("1"));
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(m_cli);
    m_process.setArguments(arguments);
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
    const QString output = QString::fromUtf8(m_process.readAllStandardOutput()).trimmed();
    const QString error = QString::fromUtf8(m_process.readAllStandardError()).trimmed();
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
