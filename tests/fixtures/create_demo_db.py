#!/usr/bin/env python3
"""Create an isolated Pigeon schema-v4 sample for UI inspection, never a live DB."""

import argparse
import os
from pathlib import Path
import socket
import sqlite3
import time


def address(value):
    family = socket.AF_INET6 if ":" in value else socket.AF_INET
    return socket.inet_pton(family, value)


def create(path, now=None):
    path = Path(path)
    if not path.parent.is_dir():
        raise ValueError("Parent directory must exist")

    # Refuse to overwrite an existing database, even between existence checks.
    fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    os.close(fd)
    now = int(time.time()) if now is None else int(now)

    rows = [
        ("10.0.0.2", "93.184.215.14", 443, 6, 85, 74,
         8_500_000, 41_500_000, -700, -2,
         "a-very-long-observed-domain-association.for-visual-layout-tests.example", "firefox"),
        ("10.0.0.2", "1.1.1.1", 53, 17, 8, 12,
         1080, 3800, -18, -5, "resolver.example", "firefox"),
        ("10.0.0.2", "93.184.215.14", 443, 6, 1, 2,
         20, 84, -720, -10, "", ""),
        ("2001:db8::10", "2001:db8::42", 443, 6, 45, 80,
         400_000, 3_900_000, -120, -1, "ipv6.example", "chromium"),
        ("10.0.0.2", "203.0.113.10", 22, 6, 2, 6,
         128, 2560, -8 * 86400, -7 * 86400, "", "ssh"),
        ("10.0.0.2", "198.51.100.20", 443, 6, 1023, 213,
         9007199254740997, 9007199254741009, -3600, -4,
         "large-counter.example", "large-counter-test"),
    ]

    try:
        with sqlite3.connect(str(path)) as db:
            db.executescript("""
                CREATE TABLE conversations (
                    us_ip BLOB NOT NULL, remote_ip BLOB NOT NULL,
                    remote_port INTEGER NOT NULL, protocol INTEGER NOT NULL,
                    packets_out INTEGER NOT NULL, packets_in INTEGER NOT NULL,
                    bytes_out INTEGER NOT NULL, bytes_in INTEGER NOT NULL,
                    first_seen INTEGER NOT NULL, last_seen INTEGER NOT NULL,
                    remote_name TEXT, application TEXT NOT NULL,
                    PRIMARY KEY (us_ip, remote_ip, remote_port, protocol, application)
                ) WITHOUT ROWID;
                CREATE TABLE dns_cache (
                    ip BLOB PRIMARY KEY, name TEXT, observed_at INTEGER, expires_at INTEGER
                );
                PRAGMA user_version=4;
            """)
            db.executemany(
                "INSERT INTO conversations VALUES (?,?,?,?,?,?,?,?,?,?,?,?)",
                [(address(local), address(remote), port, proto, p_out, p_in,
                  b_out, b_in, now + first, now + last, name, app)
                 for local, remote, port, proto, p_out, p_in,
                     b_out, b_in, first, last, name, app in rows]
            )
    except Exception:
        path.unlink(missing_ok=True)
        raise

    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path, help="NEW disposable output .db path")
    args = parser.parse_args()
    print(create(args.path).resolve())


if __name__ == "__main__":
    main()
