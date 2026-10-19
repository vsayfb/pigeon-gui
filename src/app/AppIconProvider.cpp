#include "AppIconProvider.h"

#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QPainter>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QRegularExpression>

namespace pigeon {

AppIconProvider::AppIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    // Headless and minimal platform plugins may provide only resource paths.
    // Include standard OS icon directories without replacing configured themes.
    QStringList searchPaths = QIcon::themeSearchPaths();
    const auto dataDirectories = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

    for (const QString &directory : dataDirectories) {
        const QString icons = directory + QStringLiteral("/icons");
        if (QDir(icons).exists() && !searchPaths.contains(icons))
            searchPaths.append(icons);
    }

    QIcon::setThemeSearchPaths(searchPaths);

    if (QIcon::fallbackThemeName().isEmpty())
        QIcon::setFallbackThemeName(QStringLiteral("hicolor"));

    indexDesktopEntries();
}

void AppIconProvider::indexDesktopEntries()
{
    const auto directories = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);

    for (const QString &directory : directories) {
        QDirIterator entries(directory, {QStringLiteral("*.desktop")}, QDir::Files,
                             QDirIterator::Subdirectories);

        while (entries.hasNext()) {
            QFile file(entries.next());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;

            QString icon;
            QStringList names{QFileInfo(file).completeBaseName().toCaseFolded()};
            bool desktopEntry = false;
            QTextStream stream(&file);

            while (!stream.atEnd()) {
                const QString line = stream.readLine().trimmed();

                if (line.startsWith('[')) {
                    desktopEntry = line == QStringLiteral("[Desktop Entry]");
                    continue;
                }

                if (!desktopEntry)
                    continue;

                const int separator = line.indexOf('=');
                const QString key = line.left(separator);
                const QString value = line.mid(separator + 1);

                if (key == QStringLiteral("Icon"))
                    icon = value;
                else if (key == QStringLiteral("Name") || key == QStringLiteral("StartupWMClass"))
                    names.append(value.toCaseFolded());
                else if (key == QStringLiteral("Exec")) {
                    const auto arguments = QProcess::splitCommand(value);
                    bool webApp = false;
                    for (const QString &argument : arguments) {
                        webApp |= argument.startsWith(QStringLiteral("--app-id="))
                            || argument.startsWith(QStringLiteral("--app="));
                    }

                    // A browser-hosted site's launcher must not claim the
                    // browser executable's icon for all its conversations.
                    if (!arguments.isEmpty() && !webApp)
                        names.append(QFileInfo(arguments.first()).fileName().toCaseFolded());
                }
            }

            if (icon.isEmpty())
                continue;

            for (const QString &name : names) {
                if (!name.isEmpty() && !desktopIcons_.contains(name))
                    desktopIcons_.insert(name, icon);
            }
        }
    }
}

QString AppIconProvider::applicationIcon(const QString &requestedName) const
{
    QString name = requestedName.toCaseFolded();
    static const QHash<QString, QStringList> aliases{
        {QStringLiteral("chrome"), {QStringLiteral("google-chrome"), QStringLiteral("google-chrome-stable")}},
        {QStringLiteral("firefox"), {QStringLiteral("firefox-esr")}},
        {QStringLiteral("brave"), {QStringLiteral("brave-browser"), QStringLiteral("brave-browser-stable")}},
        {QStringLiteral("chromium"), {QStringLiteral("chromium-browser")}}
    };

    if (!desktopIcons_.contains(name)) {
        for (const QString &candidate : aliases.value(name)) {
            if (desktopIcons_.contains(candidate)) {
                name = candidate;
                break;
            }
        }
    }

    const QString iconName = desktopIcons_.value(name, name);
    const QIcon icon = QFileInfo(iconName).isAbsolute() ? QIcon(iconName) : QIcon::fromTheme(iconName);

    return icon.isNull() ? QString() : iconName;
}

QVariantMap AppIconProvider::resolve(const QString &application, const QString &domain)
{
    const QString key = QString::fromUtf8(application.toUtf8().toHex()) + ':' + domain.toCaseFolded();
    if (resolutions_.contains(key))
        return resolutions_.value(key);

    QString iconName = applicationIcon(application);
    bool site = false;
    static const QRegularExpression hostname(QStringLiteral("^[a-z0-9.-]+$"));
    static const QRegularExpression punctuation(QStringLiteral("[^a-z0-9]"));
    const QString host = domain.toCaseFolded();

    // Domain tokens are only a visual fallback, never process attribution.
    // Match installed desktop names exactly after normalization; avoid fuzzy
    // substring matches that would assign unrelated applications' icons.
    if (iconName.isEmpty() && hostname.match(host).hasMatch()) {
        const QStringList labels = host.split('.');

        for (int index = labels.size() - 2; index >= 0 && iconName.isEmpty(); --index) {
            QString token = labels[index];
            token.remove(punctuation);
            if (token.size() < 4 || token == QStringLiteral("www"))
                continue;

            QString candidate;
            bool ambiguous = false;

            for (auto entry = desktopIcons_.cbegin(); entry != desktopIcons_.cend(); ++entry) {
                QString name = entry.key();
                name.remove(punctuation);
                if (name != token)
                    continue;

                const QString found = applicationIcon(entry.key());
                if (found.isEmpty())
                    continue;

                if (!candidate.isEmpty() && candidate != found) {
                    ambiguous = true;
                    break;
                }

                candidate = found;
            }

            if (!ambiguous && !candidate.isEmpty()) {
                iconName = candidate;
                site = true;
            }
        }
    }

    const QVariantMap result{
        {QStringLiteral("url"), QStringLiteral("image://appicons/") +
            QString::fromLatin1(QUrl::toPercentEncoding(iconName))},
        {QStringLiteral("site"), site}
    };

    if (resolutions_.size() >= 256)
        resolutions_.clear();

    resolutions_.insert(key, result);

    return result;
}

QImage AppIconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString name = QUrl::fromPercentEncoding(id.toUtf8());
    const int pixels = qBound(16, qMax(requestedSize.width(), requestedSize.height()), 128);
    const QString key = name + ':' + QString::number(pixels);

    if (!cache_.contains(key)) {
        const QString iconName = QFileInfo(name).isAbsolute() ? name : applicationIcon(name);
        QIcon icon = QFileInfo(iconName).isAbsolute() ? QIcon(iconName) : QIcon::fromTheme(iconName);
        QImage image = icon.pixmap(pixels, pixels).toImage();

        if (image.isNull()) {
            image = QImage(pixels, pixels, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.scale(pixels / 32.0, pixels / 32.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(QStringLiteral("#E7EEFA")));
            painter.drawRoundedRect(QRectF(1, 1, 30, 30), 8, 8);
            painter.setBrush(QColor(QStringLiteral("#738BB1")));

            for (int y : {9, 18}) {
                for (int x : {9, 18})
                    painter.drawRoundedRect(QRectF(x, y, 6, 6), 1.5, 1.5);
            }
        }

        if (cache_.size() >= 256)
            cache_.clear();

        cache_.insert(key, image);
    }

    const QImage image = cache_.value(key);
    if (size)
        *size = image.size();

    return image;
}

} // namespace pigeon
