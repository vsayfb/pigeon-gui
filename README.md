# Pigeon GUI

A standalone, cross-platform desktop viewer for the existing Pigeon collector's
SQLite database. The collector and GUI are separate programs and repositories:

```text
Pigeon collector --> existing SQLite file <-- Pigeon GUI
```

## Status

**Slice 1 of 5: application shell only.** This checkout opens a Qt Quick window
with the first theme and responsive shell. It does **not** open a database,
show real traffic, calculate rates, or start capture. Those features begin in
subsequent, separately reviewed slices. No sample traffic is fabricated.

## Build

You need CMake 3.21 or later, a C++17 compiler and **Qt 6.4 or later** with the
Quick and Quick Controls 2 development components. Qt 6.4 is the initial
compatibility target; it has not yet been tested in the authoring environment.

```sh
cmake -S . -B build
cmake --build build
```

Run the `pigeon-gui` executable in the build directory (or `pigeon-gui.app` on
macOS). Qt must be able to find its runtime QML modules and platform plugin.
See [docs/BUILDING.md](docs/BUILDING.md) for platform notes and Qt discovery.

No collector executable, libpcap, ncurses, admin rights, or collector source is
required to build or launch this viewer.

## Data boundary

The planned reader will accept **only an existing schema-v4** SQLite database,
open it read-only and never create or migrate it. See
[docs/DATABASE.md](docs/DATABASE.md) for the stored contract and its limits.
There is no database access in slice 1.

## License

A license has not been supplied for this new repository. Add one only after the
repository owner chooses its terms.
