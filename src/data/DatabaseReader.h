#pragma once

#include "data/Snapshot.h"

#include <QSqlDatabase>

namespace pigeon {

// Use only on its owning worker thread. The collector's file is NEVER created,
// migrated, or opened for writing. Every read uses a short SQLite transaction.
class DatabaseReader {
public:
    DatabaseReader() = default;
    ~DatabaseReader();
    DatabaseReader(const DatabaseReader &) = delete;
    DatabaseReader &operator=(const DatabaseReader &) = delete;

    bool read(const QString &path, const ReadOptions &options, qint64 unixSeconds,
              Snapshot &output, QString &error);
    void close();
    QString path() const { return m_path; }

private:
    bool open(const QString &path, QString &error);
    bool checkSchema(QString &error);
    bool load(Snapshot &snapshot, const ReadOptions &options, qint64 now, QString &error);
    bool readOverview(Snapshot &snapshot, QString &error);
    bool readConversations(QVector<Conversation> &rows, const ReadOptions &options,
                           qint64 now, bool top, QString &error);
    bool readApplications(QVector<Application> &rows, int sort, QString &error);
    bool readEndpoints(QVector<Endpoint> &rows, const ReadOptions &options, QString &error);

    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_path;
};

} // namespace pigeon
