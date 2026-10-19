# Existing SQLite database contract

This document records the **collector-owned** persistence contract used by the
GUI reader. Its authority is the collector reference's
`docs/SCHEMA.md` and `src/store/schema.h`. The reader supports schema v4, v5 and v6.

## File ownership

The collector writes the SQLite database; the GUI is a reader. The GUI must open an **existing** file read-only, verify `PRAGMA user_version` is 4, 5 or 6,
and reject other versions or missing tables without creating, migrating, or
modifying any data. The GUI must not require collector source or capture
privileges. The collector may use SQLite WAL mode, so readers must tolerate
normal concurrent writes using short snapshot transactions.

## `conversations`

One row describes an aggregate, **not** an individual socket or process. The
composite primary key is:

```text
(us_ip, remote_ip, remote_port, protocol, application)
```

| Column | Type | Meaning |
| --- | --- | --- |
| `us_ip` | BLOB | Local network-order IPv4 (4 bytes) or IPv6 (16 bytes) |
| `remote_ip` | BLOB | Remote address, same family/byte length as `us_ip` |
| `remote_port` | INTEGER | Remote port, 0–65535; zero sentinel for ICMP |
| `protocol` | INTEGER | ICMP = 1, TCP = 6, UDP = 17, ICMPv6 = 58 |
| `quic` (v6) | INTEGER | 0/1; QUIC observed in this UDP aggregate |
| `application` | TEXT | Executable basename, max 255 bytes; empty means unresolved |
| `packets_out`, `packets_in` | INTEGER | Cumulative directional packet counts |
| `bytes_out`, `bytes_in` | INTEGER | Cumulative directional **protocol payload** bytes |
| `first_seen`, `last_seen` | INTEGER | First/last observations, Unix seconds |
| `remote_name` | TEXT or NULL | Optional historical DNS association |

Both 4-byte and 16-byte addresses are preserved as separate identities,
including IPv4-mapped IPv6. Multiple connections/processes with the same
basename are aggregated; distinct applications remain distinct. Do not infer
process IDs, socket ports, initiation direction or application paths from rows.

Packet counters include packets without payload. Byte totals exclude network
headers and count retransmitted payload again. Direction icons must use packet
counts, **not** byte totals. The recorded name is an observed DNS association,
not evidence that an endpoint belongs to a particular service.

## `dns_cache`

| Column | Type | Meaning |
| --- | --- | --- |
| `ip` | BLOB | 4-byte or 16-byte address; primary key |
| `name` | TEXT | Last observed DNS name |
| `observed_at` | INTEGER | Observation time, Unix seconds |
| `expires_at` | INTEGER | Exclusive DNS TTL expiry, Unix seconds |

`dns_cache` is collector-maintained. `remote_name` on a conversation is a
historical attribution; it may remain after cache expiry.

## Information not stored

The database contains no traffic time series, capture heartbeat, connection
initiation flag, LAN classification, interface metadata, process IDs or local
ports. Cumulative totals must not be presented as historical-window usage.
The viewer must not infer whether capture is running from packet timestamps.

Current rates are viewer-session estimates derived from
cumulative counter differences over monotonic elapsed time, not persisted
historical metrics. First observations do not have a valid rate yet.

## Reader implementation (slice 2)

`DatabaseReader` opens only existing schema-v4 files with Qt's SQLite driver
in read-only and query-only modes. Each snapshot runs inside `BEGIN`/`COMMIT`
with rollback on failure; the worker owns the connection and never runs SQL
from the GUI thread. New snapshots replace models only after a successful
transaction. Failed reads keep the previous snapshot and rate baselines.

The overview adds unsigned exact integer counters in a portable 128-bit pair;
byte rates subtract this exact sum before conversion to floating point.
Display totals and application rankings use floating-point approximations, not
as inputs for rate sampling. Long-running counters above 2^53 still produce
correct small positive deltas. Per-row rates also subtract integer counters.

The four bounded model lists use at most 200 rows each. Live samples the 200
newest matching rows (30-second default); Top ranks cumulative totals, while
Applications groups ALL records before selecting the top 200. New Endpoints
ranks grouped remote IPs by their earliest recorded appearance, and never
applies the protocol/application filter. Name/IP searching is post-filtered
on ordered SQL candidates when a search is active, so IPv6 text stays portable
and only the first 200 matching rows are returned; exceptionally large search
queries may scan many candidate rows.

Sampling uses monotonic viewer-session time, not packet timestamps. New visible
rows show waiting until two successful reads at least one second apart.
Failed reads leave baselines unchanged; subsecond reads retain previous
estimates; counter decreases, clock regression and lifetime changes reset
baselines. The ring buffer contains at most 60 actual rate samples.

## Schema v5 control traffic

ICMP (1) rows use IPv4 addresses and ICMPv6 (58) rows use IPv6 addresses. Both
use remote_port=0 and application=''. Port zero is a sentinel: the UI shows a
dash. These are peer observations, not established connections. The collector
uses outer source/destination addresses for errors; quoted packets do not imply
additional connections or process ownership. Byte counts exclude the decoded
ICMP header and can include the quoted packet. The reader validates control-row
family, empty application and zero port. Local discovery/control exclusions are
owned by the collector. Existing v4 files remain supported without migration.

## Schema v6 QUIC metadata

The reader requires `quic` in v6 and supplies zero when reading v4/v5 files.
It validates 0/1 values and rejects a true flag on non-UDP rows. Classification
is metadata, outside identity and transient rate baselines. Tables/details show
`QUIC (UDP)` for a true flag; protocol filtering continues to use UDP=17.
The flag is evidence observed somewhere in an aggregate, not a promise that all
its lifetime bytes are QUIC. No HTTP/3 classification or decryption is implied.
The collector owns detection, migration and sticky persistence. Restart the
viewer after deploying a collector schema upgrade.
