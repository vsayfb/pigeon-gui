# Building the standalone GUI

## Requirements

- CMake 3.21+
- A compiler with C++17 support (GCC, Clang or MSVC)
- Qt 6.4+ with the `Quick` and `QuickControls2` CMake packages and QML runtime

Qt 6.4 is an initial compatibility target, **not yet build-verified**. The GUI
build has no dependency on the Linux collector, libpcap or ncurses.

## Configure and build

```sh
cmake -S . -B build
cmake --build build
```

If Qt is installed outside the default CMake search paths, select the Qt
installation explicitly (adapt the path to your machine):

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
cmake --build build
```

On Windows, use a Qt build matching the compiler/architecture of your CMake
kit (for example, MSVC or MinGW). Run the executable with a correctly
configured Qt environment or use `windeployqt` for a portable bundle.

On macOS, use a matching Qt kit; the executable is in a `.app` bundle. Use
`macdeployqt` for a standalone deployed application bundle.

On Linux, install or deploy the matching Qt QML modules and a platform plugin
(for example `xcb` or `wayland`). Launch the generated `pigeon-gui` executable.

Deployment, signing and distribution are intentionally outside this first
slice. The Qt SDK and its QML modules must be available at runtime.

## Validation status

No GUI test suite exists in slice 1. The minimum acceptance smoke test on a
machine with Qt installed is to configure, build, open the window, resize it
across 820px, and close it. The shell must exit without database or collector
access. No Linux, macOS or Windows launch has been verified yet.

Database reader tests and `ctest` integration are planned for slice 2.
