#!/usr/bin/env python3
"""Launch every GUI view offscreen against an isolated read-only SQLite fixture."""

import os
from pathlib import Path
import subprocess
import sqlite3
import sys
import tempfile

from create_demo_db import create


def check(command, env):
    result = subprocess.run(command, capture_output=True, text=True,
                            env=env, timeout=20, check=False)
    output = result.stdout + result.stderr
    if result.returncode != 0:
        raise AssertionError(
            f"GUI smoke test returned {result.returncode}: {command!r}\n{output}"
        )

    # Qt's engine catches most QML errors via QQmlEngine::warnings; verify
    # stderr as well for JS ReferenceError diagnostics outside that signal.
    for marker in ("ReferenceError:", "Type ", "Invalid property assignment:",
                   "Cannot assign to non-existent property", "Binding loop detected"):
        if marker in output:
            raise AssertionError(f"QML startup output contains {marker!r}:\n{output}")


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: test_gui_startup.py /path/to/pigeon-gui")

    executable = str(Path(sys.argv[1]).resolve())
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software")
    with tempfile.TemporaryDirectory(prefix="pigeon-qml-test-") as tmp:
        database = create(Path(tmp) / "fixture.db")
        check([executable, "--check-qml", "--database", str(database)], env)
        with sqlite3.connect(database) as db:
            db.execute("PRAGMA user_version=5")
            db.execute("INSERT INTO conversations VALUES(x'0a000002',x'08080808',0,1,1,1,4,4,strftime('%s','now'),strftime('%s','now'),NULL,'')")
            db.execute("INSERT INTO conversations VALUES(x'20010db8000000000000000000000010',x'20010db8000000000000000000000042',0,58,1,1,4,4,strftime('%s','now'),strftime('%s','now'),NULL,'')")
        check([executable, "--check-qml", "--database", str(database)], env)
        with sqlite3.connect(database) as db:
            db.execute("ALTER TABLE conversations ADD COLUMN quic INTEGER NOT NULL DEFAULT 0")
            db.execute("UPDATE conversations SET quic=1 WHERE protocol=17")
            db.execute("PRAGMA user_version=6")
        check([executable, "--check-qml", "--database", str(database)], env)
        with sqlite3.connect(database) as db:
            db.execute("PRAGMA user_version=7")
            db.execute("INSERT INTO conversations VALUES(x'0a000002',x'08080808',0,50,1,1,8,8,strftime('%s','now'),strftime('%s','now'),NULL,'',0)")
            db.execute("INSERT INTO conversations VALUES(x'20010db8000000000000000000000010',x'20010db8000000000000000000000042',0,50,1,1,8,8,strftime('%s','now'),strftime('%s','now'),NULL,'',0)")
        check([executable, "--check-qml", "--database", str(database)], env)
        check([executable, "--check-qml"], env)


if __name__ == "__main__":
    main()
