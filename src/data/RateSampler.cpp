#include "data/RateSampler.h"

#include <algorithm>
#include <cmath>

namespace pigeon {

void RateSampler::reset()
{
    m_rows.clear();
    m_liveReadAt = -1;
    m_previousDown = {};
    m_previousUp = {};
    m_previousAt = 0;
    m_sampled = false;
    m_ratesValid = false;
    m_rateDown = 0;
    m_rateUp = 0;
    m_history.clear();
}

void RateSampler::sample(Snapshot &snapshot, double now, int liveSort)
{
    if (!std::isfinite(now) || now < 0)
        return;

    if (!m_sampled || now < m_previousAt || snapshot.exactDown < m_previousDown
        || snapshot.exactUp < m_previousUp) {
        m_previousDown = snapshot.exactDown;
        m_previousUp = snapshot.exactUp;
        m_previousAt = now;
        m_sampled = true;
        m_ratesValid = false;
        m_rateDown = m_rateUp = 0;
        m_history.clear();
    } else if (now - m_previousAt >= 1.0) {
        const double seconds = now - m_previousAt;
        m_rateDown = snapshot.exactDown.difference(m_previousDown) / seconds;
        m_rateUp = snapshot.exactUp.difference(m_previousUp) / seconds;
        m_ratesValid = true;
        m_previousDown = snapshot.exactDown;
        m_previousUp = snapshot.exactUp;
        m_previousAt = now;
        if (m_history.size() == HistorySamples)
            m_history.removeFirst();
        m_history.append({m_rateDown, m_rateUp, seconds});
    }

    QHash<QByteArray, RowBaseline> next;
    next.reserve(snapshot.live.size());
    for (Conversation &row : snapshot.live) {
        const QByteArray key = row.key();
        const auto it = m_rows.constFind(key);
        RowBaseline state;
        const bool resetRow = it == m_rows.cend() || now < m_liveReadAt
            || row.bytesIn < it->observed.bytesIn || row.bytesOut < it->observed.bytesOut
            || row.packetsIn < it->observed.packetsIn || row.packetsOut < it->observed.packetsOut
            || row.firstSeen != it->observed.firstSeen;
        if (resetRow) {
            state.baseDown = row.bytesIn;
            state.baseUp = row.bytesOut;
            state.sampledAt = now;
        } else {
            state = *it;
            const double seconds = now - state.sampledAt;
            if (seconds >= 1.0) {
                // Integer subtraction precedes floating-point conversion.
                state.rateDown = static_cast<double>(row.bytesIn - state.baseDown) / seconds;
                state.rateUp = static_cast<double>(row.bytesOut - state.baseUp) / seconds;
                state.baseDown = row.bytesIn;
                state.baseUp = row.bytesOut;
                state.sampledAt = now;
                state.valid = true;
            }
        }
        state.observed = row;
        row.rateValid = state.valid;
        row.rateDown = state.rateDown;
        row.rateUp = state.rateUp;
        next.insert(key, std::move(state));
    }
    m_rows = std::move(next); // rows leaving Live lose their baselines
    m_liveReadAt = now;

    if (liveSort != 0) {
        const auto rate = [liveSort](const Conversation &row) {
            return liveSort == 2 ? row.rateDown
                 : liveSort == 3 ? row.rateUp : row.rateDown + row.rateUp;
        };
        std::stable_sort(snapshot.live.begin(), snapshot.live.end(), [&](const Conversation &a, const Conversation &b) {
            if (a.rateValid != b.rateValid)
                return a.rateValid;
            return a.rateValid && rate(a) > rate(b);
        });
    }

    snapshot.rateDown = m_rateDown;
    snapshot.rateUp = m_rateUp;
    snapshot.ratesValid = m_ratesValid;
    snapshot.history = m_history;
}

} // namespace pigeon
