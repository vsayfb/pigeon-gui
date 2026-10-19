# Building the standalone GUI

## Requirements

- CMake 3.21+, a C++17 compiler, and Qt 6.4 or newer
- Qt modules: Quick, Quick Controls 2, SQL (including the QSQLITE driver), Network
- Qt Test (normally part of the Qt Base development package) when building tests

On Debian/Ubuntu, a typical development setup is `qt6-base-dev`,
`qt6-declarative-dev`, and `libqt6sql6-sqlite`. QML Quick Controls must also
be available at runtime. The GUI never links to or launches the collector.

## Build, test, run

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/pigeon-gui --database /absolute/path/to/existing-pigeon.db
```

Alternatively, launch without `--database` and enter a database path in the
window. Only an **existing** schema-v4 SQLite file is accepted. Failed reads
show an error while retaining the most recent good snapshot. The app may run
without a database; no database is created by starting it.

If Qt lives outside the normal CMake search path:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
```

Use `-DBUILD_TESTING=OFF` to skip the Qt Test dependency; this does not disable
runtime SQLite access. Qt SQL must have the `QSQLITE` driver installed.

On Windows, select a Qt kit matching your compiler (MSVC/MinGW); `windeployqt`
helps bundle the platform/SQL/QML plugins. On macOS use the matching Qt kit and
`macdeployqt`; the target is a `.app` bundle. Linux requires Qt's xcb or
Wayland platform plugin and Quick Controls runtime. Deployment validation and
packaging remain follow-up work.

## Slice-2 test coverage

The `pigeon-data` CTest target constructs disposable schema-v4 SQLite fixtures,
including IPv4/IPv6 addresses, unresolved applications, integers greater than
2^53, rate resets, and a stable-model-index check. Tests never touch a collector
or an actual capture database. Slice 3 displays these models across the dashboard pages, while interactive
search, sorting, drill-down and pause remain in the next reviewed slice.

## Slice-3 dashboard verification

Make a **new disposable** database (the generator refuses to overwrite a file):

```sh
python3 tests/fixtures/create_demo_db.py /tmp/pigeon-gui-screen-test.db
./build/pigeon-gui --database /tmp/pigeon-gui-screen-test.db
```

The fixture includes recent and old conversations, IPv4 and IPv6, an unresolved
application, long observed DNS names, mixed direction arrows, and an integer
payload counter greater than 2^53. It never uses the actual collector database.
On Overview, Live should show recent rows within 30 seconds of fixture creation;
Top should include old rows. A static fixture can give measured **zero** rate
after the second sample, which must not be confused with the waiting state.

Visually inspect the five pages at approximately 1240×780 and 700×600,
including horizontal table scrolling, long name tooltips, IPv6, the empty
SQLite database state, and a bad database path after a successful load. The
chart has no stored history; its samples begin when this viewer opens. Capture
screenshots using your desktop's screenshot tool for review. A headless test
run can use `QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software`, but this is
not a substitute for visual inspection on the real display server.

The optional `pigeon-gui-fixture` CTest checks the Python fixture itself.
The Qt data tests remain `pigeon-data`; there is not yet an automated QML
visual-regression suite.
