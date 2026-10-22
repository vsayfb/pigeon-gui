# GUI installation and releases

```sh
curl -fsSL https://piigeon.pages.dev/gui/install.sh | sh
```

This installs only `pigeon-gui`. Install the collector/TUI separately:

```sh
curl -fsSL https://piigeon.pages.dev/cli/install.sh | sh
```

Files go under `~/.local/share/pigeon/versions` and command symlinks into
`~/.local/bin`. Add that bin directory to PATH if needed. The GUI opens the
collector's existing database automatically. The application menu entry and window
are named **Pigeon**, using the supplied pigeon logo (including Wayland desktop
identity). The terminal command remains `pigeon-gui`; `pigeon` opens the TUI:


```sh
pigeon-gui
```

## GitHub Actions configuration

Add these repository secrets, using the same Cloudflare account as the CLI:

- `CLOUDFLARE_API_TOKEN`: account-scoped token with Cloudflare Pages / Edit.
- `CLOUDFLARE_ACCOUNT_ID`: your account ID.

Run **Release and publish Pigeon GUI** manually, or push a `vX.Y.Z` tag matching
this repository's CMake version, currently `v0.0.3`.

This repository owns its installer, packaging and release workflow. It builds
only GUI archives and stages its files under `/gui/`. Before deploying, it
downloads and verifies the currently published `/cli/` directory, then uploads
the combined static directory to the shared `piigeon` Pages project. The CLI
workflow does the reverse. Each catalog (`files.json`) records paths and SHA-256s
so preservation does not need directory listings or extra storage.

Either workflow can create the Direct Upload Pages project on the first run.
Both use **static Pages hosting only**: no R2, GitHub Releases or custom Worker.
Invalid/missing published files abort preservation instead of removing downloads.
A genuinely unpublished component can be absent on the first deployment.

**Publish one repository at a time.** GitHub concurrency is per repository;
simultaneous deployments could overwrite an update using an older snapshot.
CLI and GUI versions are independent. Each component publishes its latest release;
older local installations and databases remain untouched.

## Compatibility and local packaging

CI builds x86-64 and ARM64 glibc GUI archives on Ubuntu 24.04 (glibc 2.39).
Qt QML modules, plugins and non-libc dependencies are bundled. A display session
and host graphics drivers are required. Older libc versions may fail the
compatibility check. Musl GUI releases are not provided yet.

Each archive is split into 24 MiB parts for Pages' per-file limit. The installer
verifies part/archive SHA-256s before extraction and execution. These are
integrity checks over HTTPS, not signatures.

With CMake, Ninja, a C++17 compiler, Python 3.10+, patchelf, and the Qt 6
Quick/Controls/SQL/Network development/runtime modules installed:

```sh
python3 packaging/release.py build --version 0.0.3 --output dist
python3 packaging/release.py site --version 0.0.3 --artifacts dist --output dist/upload
PIGEON_INSTALL_ROOT=/tmp/pigeon-gui-check/data PIGEON_BIN_DIR=/tmp/pigeon-gui-check/bin \
  sh packaging/install.sh --from "$PWD/dist/upload"
```

Local builds inherit the host's libc baseline. The workflow lists the required
Qt QML, SQLite and Wayland/XCB packages.
`PIGEON_DOWNLOAD_URL` overrides the GUI origin (default
`https://piigeon.pages.dev/gui`). `--version X.Y.Z` selects that version if still
published. Reinstallation updates only the GUI command; unrelated commands are
refused. To uninstall, remove its symlink and GUI version directories while
keeping the collector and database.

The installer shows installation stages and an interactive download progress bar.
To manage the installed collector, use `pigeon start`, `pigeon stop`,
`pigeon restart`, `pigeon status`, or `pigeon logs`. Running `pigeon` opens the TUI;
closing a viewer does not stop capture.

Desktop registration uses `${XDG_DATA_HOME:-$HOME/.local/share}/applications/pigeon-gui.desktop`
and the icon at `$PIGEON_INSTALL_ROOT/desktop/pigeon.png` (the default root is
`~/.local/share/pigeon`). The launcher uses absolute paths, so it works without
adding `~/.local/bin` to your desktop session PATH. Remove this desktop file and
icon when uninstalling the GUI. Existing unrelated desktop entries are preserved.

Uninstall the GUI with `pigeon-gui --uninstall`. This removes its command,
desktop entry, logo and GUI bundles, while preserving the collector and database.
Run `pigeon uninstall` separately to remove CLI/TUI and stop capture.
Wayland libraries are supplied by the host alongside its graphics drivers.

The GUI Settings dialog controls collector boot startup and database storage.
The default database file is `~/.local/share/pigeon/pigeon.db`. Saving a new
location preserves history through SQLite backup and keeps the original file.
Active capture restarts at the new location; boot startup stays on or off as
configured. Desktop authorization uses PolicyKit (`pkexec`), with a terminal sudo prompt
as a fallback when PolicyKit is unavailable. These settings require an updated
Pigeon CLI with `autostart` and `database` support. Opening an existing database
in the viewer remains separate from changing collector storage.
