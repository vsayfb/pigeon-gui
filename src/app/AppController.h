#pragma once

#include "data/DatabaseReader.h"
#include "data/RateSampler.h"
#include "models/Models.h"

#include <QElapsedTimer>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>


namespace pigeon {

class ReaderWorker : public QObject {
    Q_OBJECT
public:
    ReaderWorker() = default;
public slots:
    void refresh(quint64 token, const QString &path, const ReadOptions &options);
    void finalize(quint64 token, bool accepted);

signals:
    void completed(quint64 token, const QString &path, bool ok,
                   const pigeon::Snapshot &snapshot, const QString &error);

private:
    DatabaseReader m_reader;
    RateSampler m_sampler;
    QElapsedTimer m_clock;
    QString m_samplePath;
    RateSampler m_candidateSampler;
    QString m_candidatePath;
    quint64 m_candidateToken = 0;
    bool m_candidateReady = false;
};

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString databasePath READ databasePath NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(bool hasSnapshot READ hasSnapshot NOTIFY stateChanged)
    Q_PROPERTY(QString conversationCount READ conversationCount NOTIFY stateChanged)
    Q_PROPERTY(QString remoteIpCount READ remoteIpCount NOTIFY stateChanged)
    Q_PROPERTY(QString downloaded READ downloaded NOTIFY stateChanged)
    Q_PROPERTY(QString uploaded READ uploaded NOTIFY stateChanged)
    Q_PROPERTY(QString downloadRate READ downloadRate NOTIFY stateChanged)
    Q_PROPERTY(QString uploadRate READ uploadRate NOTIFY stateChanged)
    Q_PROPERTY(bool ratesMeasured READ ratesMeasured NOTIFY stateChanged)
    Q_PROPERTY(double downloadRateValue READ downloadRateValue NOTIFY stateChanged)
    Q_PROPERTY(double uploadRateValue READ uploadRateValue NOTIFY stateChanged)
    Q_PROPERTY(QVariantList trafficHistory READ trafficHistory NOTIFY stateChanged)
    Q_PROPERTY(QString lastPacketAge READ lastPacketAge NOTIFY stateChanged)
    Q_PROPERTY(QString search READ search NOTIFY filtersChanged)
    Q_PROPERTY(int protocol READ protocol NOTIFY filtersChanged)
    Q_PROPERTY(int family READ family NOTIFY filtersChanged)
    Q_PROPERTY(int recentSeconds READ recentSeconds NOTIFY filtersChanged)
    Q_PROPERTY(int rankingSort READ rankingSort NOTIFY filtersChanged)
    Q_PROPERTY(int liveSort READ liveSort NOTIFY filtersChanged)
    Q_PROPERTY(bool applicationSelected READ applicationSelected NOTIFY filtersChanged)
    Q_PROPERTY(QString selectedApplication READ selectedApplication NOTIFY filtersChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY stateChanged)
    Q_PROPERTY(QString selectedIdentity READ selectedIdentity NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectedDetails READ selectedDetails NOTIFY selectionChanged)
    Q_PROPERTY(QAbstractItemModel *liveModel READ liveModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *topModel READ topModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *applicationsModel READ applicationsModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *endpointsModel READ endpointsModel CONSTANT)
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    QString databasePath() const { return m_path; }
    QString status() const { return m_status; }
    QString error() const { return m_error; }
    bool hasSnapshot() const { return m_hasSnapshot; }
    QString conversationCount() const { return QString::number(m_snapshot.conversations); }
    QString remoteIpCount() const { return QString::number(m_snapshot.remoteIps); }
    QString downloaded() const { return m_snapshot.exactDown.decimal(); }
    QString uploaded() const { return m_snapshot.exactUp.decimal(); }
    QString downloadRate() const;
    QString uploadRate() const;
    bool ratesMeasured() const { return m_snapshot.ratesValid; }
    double downloadRateValue() const { return m_snapshot.rateDown; }
    double uploadRateValue() const { return m_snapshot.rateUp; }
    QVariantList trafficHistory() const;
    QString lastPacketAge() const;
    QAbstractItemModel *liveModel() { return &m_live; }
    QAbstractItemModel *topModel() { return &m_top; }
    QAbstractItemModel *applicationsModel() { return &m_applications; }
    QAbstractItemModel *endpointsModel() { return &m_endpoints; }

    QString search() const { return m_options.search; }
    int protocol() const { return m_options.protocol; }
    int family() const { return m_options.family; }
    int recentSeconds() const { return m_options.recentSeconds; }
    int rankingSort() const { return m_options.sort; }
    int liveSort() const { return m_options.liveSort; }
    bool applicationSelected() const { return m_options.applicationSelected; }
    QString selectedApplication() const { return m_options.application; }
    bool paused() const { return m_paused; }
    QString selectedIdentity() const { return m_selectedIdentity; }
    QVariantMap selectedDetails() const { return m_selectedDetails; }

    Q_INVOKABLE void selectDatabase(const QString &path);
    Q_INVOKABLE void setSearch(const QString &search);
    Q_INVOKABLE void setProtocol(int protocol);
    Q_INVOKABLE void setFamily(int family);
    Q_INVOKABLE void setRecentSeconds(int seconds);
    Q_INVOKABLE void setRankingSort(int sort);
    Q_INVOKABLE void setLiveSort(int sort);
    Q_INVOKABLE void selectApplication(const QString &application);
    Q_INVOKABLE void clearApplication();
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE void setPaused(bool paused);
    Q_INVOKABLE void selectConversation(const QString &identity, bool live);
    Q_INVOKABLE void clearSelection();

signals:
    void stateChanged();
    void filtersChanged();
    void selectionChanged();
    void refreshRequested(quint64 token, const QString &path, const pigeon::ReadOptions &options);
    void finalizeRequested(quint64 token, bool accepted);

private slots:
    void requestRefresh();
    void acceptResult(quint64 token, const QString &path, bool ok,
                      const pigeon::Snapshot &snapshot, const QString &error);

private:
    void optionsChanged();
    void updateSelection();

    quint64 m_generation = 0;
    QThread m_thread;
    QTimer m_timer;
    QString m_path;
    QString m_status = QStringLiteral("Choose an existing schema-v4 Pigeon database");
    QString m_error;
    Snapshot m_snapshot;
    ReadOptions m_options;
    ConversationsModel m_live;
    ConversationsModel m_top;
    ApplicationsModel m_applications;
    EndpointsModel m_endpoints;
    quint64 m_nextToken = 0;
    quint64 m_activeToken = 0;
    bool m_inFlight = false;
    bool m_pending = false;
    bool m_hasSnapshot = false;
    bool m_paused = false;
    quint64 m_activeGeneration = 0;
    QString m_selectedIdentity;
    bool m_selectedLive = true;
    QVariantMap m_selectedDetails;
};

} // namespace pigeon
