#pragma once

#include <QHash>
#include <QImage>
#include <QQuickImageProvider>
#include <QString>
#include <QObject>
#include <QVariantMap>

namespace pigeon {

// Presentation only: application names and database identities stay unchanged.
class AppIconProvider final : public QQuickImageProvider {
public:
    AppIconProvider();
    QVariantMap resolve(const QString &application, const QString &domain);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    void indexDesktopEntries();
    QString applicationIcon(const QString &name) const;
    QHash<QString, QVariantMap> resolutions_;
    QHash<QString, QString> desktopIcons_;
    QHash<QString, QImage> cache_;
};

// Keep the QML facade separate from the engine-owned image provider so the
// provider works across supported Qt versions with different base classes.
class AppIconResolver final : public QObject {
    Q_OBJECT
public:
    explicit AppIconResolver(AppIconProvider *provider, QObject *parent = nullptr)
        : QObject(parent), provider_(provider) {}

    Q_INVOKABLE QVariantMap resolve(const QString &application, const QString &domain)
    {
        return provider_->resolve(application, domain);
    }

private:
    AppIconProvider *provider_;
};

} // namespace pigeon
