# Building and validating Pigeon GUI

The collector and GUI are separate applications. Pigeon GUI only opens an
**existing schema-v4 SQLite database**; neither tests nor startup require a
running collector or capture privileges.

## Requirements

- CMake 3.21+, C++17 compiler
- Qt 6.4+ with Quick, Quick Controls 2, SQL/SQLite plugin and Network
- Qt Test for C++ tests; Python 3 for fixture and offscreen startup tests

On Debian/Ubuntu, a typical setup is:

```sh
sudo apt install cmake g++ qt6-base-dev qt6-declarative-dev libqt6sql6-sqlite \
  qml6-module-qtquick qml6-module-qtquick-controls \
  qml6-module-qtquick-layouts qml6-module-qtquick-window \
  qml6-module-qtqml-workerscript
```

If Qt is installed outside system paths, add
`-DCMAKE_PREFIX_PATH=/path/to/Qt/<version>/<kit>` to configuration.

## Build and test

```sh
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/pigeon-gui --database /absolute/path/to/existing-pigeon.db
```

Launch without `--database` to select a path in the application. When a read
fails, the last good snapshot remains displayed with an error banner. Use
**Retry** after fixing the problem, even when the database path has not changed.
Database reads are read-only. Unresolved application names are displayed as
"Unresolved" and retain their empty-string identity during drill-down.

The tests are:

- `pigeon-data`: SQL, IPv4/IPv6, rates and stable-model regression checks.
- `pigeon-controller`: filter/selection, pause, stale generations and same-path retry.
- `pigeon-gui-fixture`: isolated Python fixture checks.
- `pigeon-qml-startup`: offscreen launch without a database and with a **temporary**
  fixture. Exercises each of the five views at 1240×780 and 700×600 and fails on
  engine-reported QML warnings. This tests QML creation, not visual quality.

For manual smoke checks on a headless Linux runner:

```sh
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
    ./build/pigeon-gui --check-qml
```

On a desktop, create a **new disposable** database for screenshots:

```sh
python3 tests/fixtures/create_demo_db.py /tmp/new-pigeon-gui-fixture.db
./build/pigeon-gui --database /tmp/new-pigeon-gui-fixture.db
```

The generator refuses to overwrite an existing database. Never use a live
collector database in a test. For older capture timestamps, select the
**All recorded** recency filter in Live; old cumulative totals are not
historical traffic rates. Rates and charts begin with this viewer session.

## Desktop review checklist

Inspect all five views at both 1240×780 and 700×600 (or similar sizes):

1. Resize and use the horizontal compact navigation; all five tabs should
   remain available, including via keyboard Tab.
2. Use Tab, Up/Down, Left/Right and Enter in conversation and application
   tables; check the visible selection/focus state. Escape closes details.
3. Inspect long observed names and IPv6 addresses with tooltips/details.
4. Check waiting-vs-zero traffic rates and the viewer-session graph.
5. Open an invalid database, then create/correct the **same** path and use
   Retry. Confirm an earlier good snapshot remains shown after a read error.
6. Verify focus, contrast and wrapping with OS text scaling and a HiDPI display.

## Platform status

The GitHub Actions workflow validates the Linux build, CTest suite and
headless QML component startup. It does not produce visual screenshots or
constitute a full accessibility audit. Windows and macOS builds and packaging
have **not** been validated by this workflow; use matching Qt kits, CMake and
CTest on each platform, and perform the desktop checks above. On Windows,
`windeployqt` can deploy plugins; on macOS use `macdeployqt` for `.app` bundles.
Do not treat a successful Linux CI run as proof of Windows/macOS support.
