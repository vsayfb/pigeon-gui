#pragma once

#include "data/Snapshot.h"

#include <QHash>

namespace pigeon {

// Worker-thread-only sampler. A failed database read MUST NOT call sample().
class RateSampler {
public:
    void reset();
    void sample(Snapshot &snapshot, double monotonicSeconds, int liveSort = 0);

private:
    struct RowBaseline {
        Conversation observed;
        quint64 baseDown = 0;
        quint64 baseUp = 0;
        double sampledAt = 0;
        double rateDown = 0;
        double rateUp = 0;
        bool valid = false;
        QVector<double> history;
    };

    QHash<QByteArray, RowBaseline> m_rows;
    double m_liveReadAt = -1;
    Counter128 m_previousDown;
    Counter128 m_previousUp;
    double m_previousAt = 0;
    bool m_sampled = false;
    bool m_ratesValid = false;
    double m_rateDown = 0;
    double m_rateUp = 0;
    QVector<RatePoint> m_history;
};

} // namespace pigeon
