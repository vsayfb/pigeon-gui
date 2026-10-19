# Pigeon GUI

A standalone, cross-platform desktop viewer for the existing Pigeon collector's SQLite database. The collector and GUI are separate programs and repositories:

```
Pigeon collector --> existing SQLite file <-- Pigeon GUI
```

The desktop interface includes Overview, Live Traffic, Top Traffic, Applications,
and New Endpoints, with light and dark themes, searchable tables, conversation
details, and traffic charts built from actual database samples. Live rows include a bounded,
individually scaled sparkline of their combined payload rate (up to 30 samples). Incoming traffic
is green; outgoing traffic is blue. Pausing the viewer does not pause capture.

Application icons are resolved locally from the OS icon theme. On Linux, installed
`.desktop` entries associate executable names with their declared icons. Icons
are cached in GUI memory; missing icons (including applications installed on a
different computer) can use an installed site's icon by matching a domain token
(e.g. `chatgpt.com` → `ChatGPT`). This fallback is identified as a site icon in its
tooltip; it does not change process attribution. If neither lookup succeeds, the
GUI uses a neutral app symbol. Browser web-app launchers cannot override the
browser executable's icon. No icon data is written to SQLite,
and no network requests or extra fonts are required. Other platforms use available
Qt icon-theme integration with the same fallback.

The GUI does not perform IP geolocation. There is no traffic map or location column.
Metric cards show real recorded totals and sampled payload rates, without synthetic
trends or a claim that the collector is running.

For a reproducible offscreen visual check after building:

```sh
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ./build/pigeon-gui \
  --database /path/to/pigeon.db --render-preview /tmp/pigeon-preview.png
```

Use `--preview-page 0` through `4` and `--preview-width 700` for other views and
narrow layouts. `--preview-dark` switches themes; `--preview-delay 6500` allows
more live samples before rendering. Normal startup does not write a screenshot.

ICMP and ICMPv6 conversations are supported in collector schema v5. The GUI
continues to read schema v4 too. Protocol filters include both control protocols;
port cells show a dash because ICMP has no ports. These rows have no attributed
application. The collector performs migration; the GUI remains read-only.
