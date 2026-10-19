# Existing SQLite database contract

This document records the **collector-owned** persistence contract used by the
future GUI reader. Its authority is the collector reference's
`docs/SCHEMA.md` and `src/store/schema.h` at commit
`b4cbca6`. **Slice 1 contains no SQL or database connection.**

## File ownership

The collector writes the SQLite database; the GUI is a reader. Subsequent GUI
slices must open an **existing** file read-only, verify `PRAGMA user_version = 4`,
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
| `remote_port` | INTEGER | Remote port, 0–65535 |
| `protocol` | INTEGER | TCP = 6, UDP = 17 |
| `application` | TEXT | Executable basename, max 255 bytes; empty means unresolved |
| `packets_out`, `packets_in` | INTEGER | Cumulative directional packet counts |
| `bytes_out`, `bytes_in` | INTEGER | Cumulative directional **transport payload** bytes |
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

Current rates in a later slice will be viewer-session estimates derived from
cumulative counter differences over monotonic elapsed time, not persisted
historical metrics. First observations do not have a valid rate yet.
