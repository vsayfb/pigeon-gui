"""Verify that the screen-inspection fixture exercises important display cases."""

import sqlite3
import tempfile
import unittest
from pathlib import Path

from create_demo_db import create


class DemoFixtureTests(unittest.TestCase):
    def test_schema_and_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "ui.db"
            create(path, now=1_800_000_000)
            with sqlite3.connect(str(path)) as db:
                self.assertEqual(db.execute("PRAGMA user_version").fetchone()[0], 4)
                self.assertEqual(db.execute("SELECT count(*) FROM conversations").fetchone()[0], 6)
                self.assertEqual(db.execute("SELECT count(DISTINCT remote_ip) FROM conversations").fetchone()[0], 5)
                self.assertEqual(db.execute("SELECT count(*) FROM conversations WHERE application='' ").fetchone()[0], 1)
                self.assertEqual(db.execute("SELECT count(*) FROM conversations WHERE length(remote_ip)=16").fetchone()[0], 1)
                self.assertEqual(db.execute("SELECT max(bytes_in) FROM conversations").fetchone()[0], 9007199254741009)
                self.assertEqual(db.execute("SELECT count(*) FROM conversations WHERE remote_name LIKE 'a-very-long%'").fetchone()[0], 1)

            with self.assertRaises(FileExistsError):
                create(path)
            with sqlite3.connect(str(path)) as db:
                self.assertEqual(db.execute("SELECT count(*) FROM conversations").fetchone()[0], 6)


if __name__ == "__main__":
    unittest.main()
