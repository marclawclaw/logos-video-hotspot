# Video Hotspot — FURPS Specification

> **Privacy-preserving video upload and geolocation-based event mapping on the Logos stack**

## CLI Demo

![CLI demo](demo/cli-demo.gif)

---

## Loading in Logos Basecamp (logos-app)

Video Hotspot is a real [`logos-co/logos-app`](https://github.com/logos-co/logos-app)
plugin implementing the `IComponent` interface (`com.logos.component.IComponent`).
logos-app discovers plugins via `manifest.json` (included in `ui/plugin/`) and
loads them via `QPluginLoader` from the plugin directory.

### Integration architecture

```
logos-app
  └── Window::setupUi()
        └── QPluginLoader("…/video_hotspot/video_hotspot.so")
              └── VideoHotspotPlugin::createWidget(logosAPI)
                    ├── StorageClient::initLogos(logosAPI)
                    │     └── LogosAPI::getClient("codex") → Codex storage
                    ├── MessagingClient::initLogos(logosAPI)
                    │     └── LogosAPI::getClient("waku") → Waku messaging
                    └── QQuickWidget (VideoHotspotApp.qml)
                          context: storageClient, messagingClient, logosConnected
```

### 1 — Clone with submodules (logos-cpp-sdk for real integration)

```bash
git clone --recurse-submodules https://github.com/marclawclaw/logos-video-hotspot
# or after cloning:
git submodule update --init --recursive
```

The submodules provide `logos-co/logos-cpp-sdk` and `logos-co/logos-liblogos` headers.
Without them the plugin still builds and runs in **local mock mode** (no real Codex/Waku).

### 2 — Build the plugin

```bash
cmake -B build -DBUILD_UI_PLUGIN=ON
cmake --build build --target video_hotspot
# Output: build/ui/plugin/video_hotspot.so  (Linux)
#         build/ui/plugin/video_hotspot.dylib  (macOS)
```

To build with real Logos SDK integration (requires submodules or manual paths):

```bash
cmake -B build -DBUILD_UI_PLUGIN=ON \
  -DLOGOS_CPP_SDK_ROOT=vendor/logos-cpp-sdk \
  -DLOGOS_LIBLOGOS_ROOT=vendor/logos-liblogos
cmake --build build --target video_hotspot
# CMake will print: LOGOS_CORE_AVAILABLE=1 — real Codex+Waku integration enabled
```

### 3 — Install into the logos-app plugin directory

Both `video_hotspot.so` and `manifest.json` must be present in the plugin directory —
logos-app uses `manifest.json` for plugin discovery and dependency resolution.

```bash
# Non-portable build (default logos-app):
PLUGIN_DIR="$HOME/.local/share/Logos/LogosAppNix/plugins/video_hotspot"
mkdir -p "$PLUGIN_DIR"
cp build/ui/plugin/video_hotspot.so "$PLUGIN_DIR/"
cp ui/plugin/manifest.json "$PLUGIN_DIR/"

# Portable build (logos-app with LOGOS_PORTABLE_BUILD):
PLUGIN_DIR="$HOME/.local/share/LogosApp/plugins/video_hotspot"
mkdir -p "$PLUGIN_DIR"
cp build/ui/plugin/video_hotspot.so "$PLUGIN_DIR/"
cp ui/plugin/manifest.json "$PLUGIN_DIR/"
```

Or use the CMake install target (installs both `.so` and `manifest.json`):

```bash
cmake --install build --prefix "$HOME/.local"
# Installs to ~/.local/share/Logos/LogosAppNix/plugins/video_hotspot/
```

### 4 — Launch logos-app

> **Building logos-app:** See [logos-co/logos-app](https://github.com/logos-co/logos-app) for build instructions.
>
> **Standalone development:** You can also use the standalone demo app (`video-hotspot-app`) built with `BUILD_UI_APP=ON` for development without needing logos-app:
> ```bash
> cmake -B build -DBUILD_UI_APP=ON -DBUILD_UI_PLUGIN=ON
> cmake --build build --target video-hotspot-app
> ./build/ui/app/video-hotspot-app
> ```

```bash
logos-app
```

logos-app calls `VideoHotspotPlugin::createWidget(logosAPI)` at startup,
which wires the `LogosAPI*` to `StorageClient` and `MessagingClient`.
When both Codex and Waku modules are running, real storage and messaging
activate automatically — no configuration needed.

### 5 — Verify it loaded

```bash
logos-app 2>&1 | grep -i "VideoHotspot"
```

Expected output:
```
VideoHotspotPlugin: loaded (LOGOS_CORE_AVAILABLE — real Codex+Waku integration)
VideoHotspotPlugin::createWidget — LogosAPI: connected
StorageClient: LogosAPI connected — real Codex storage enabled
MessagingClient: LogosAPI connected — real Waku messaging enabled
```

In mock mode (no logos-app / no LogosAPI):
```
VideoHotspotPlugin: loaded (local mock — no logos-cpp-sdk at build time)
VideoHotspotPlugin::createWidget — LogosAPI: null (mock mode)
```

### Local mock mode (development without logos-app)

When logos-app calls `createWidget(nullptr)` (no `LogosAPI` context), Video Hotspot
falls back to local SQLite storage and filesystem-only uploads — useful for UI
development without a running Logos node. See `VideoHotspotPlugin.cpp` for details.

`LogosAPI` wiring is fully implemented: `StorageClient::initLogos()` and
`MessagingClient::initLogos()` are called in `createWidget()`. Real Codex+Waku calls
activate when `LOGOS_CORE_AVAILABLE` is defined and logos-app provides a non-null
`LogosAPI*`. See ADR-0002 and ADR-0003 for the design, and the comments in
`storage_client.cpp` / `messaging_client.cpp` for the exact method signatures.

---

## GUI Demo

<!-- GitHub renders MP4 when uploaded via drag-drop to an issue/PR; local path for repo reference -->
**[▶ gui-demo.mp4](demo/gui-demo.mp4)** — real 20 s recording on a headless Pi via Xvfb

> Captured with `ffmpeg -f x11grab` on a virtual display (no monitor required).
> The app is built with `BUILD_UI_APP=ON` and `BUILD_UI_PLUGIN=ON`.
> See [`demo/README.md § Virtual Display (Xvfb)`](demo/README.md#virtual-display-xvfb) for the exact setup.
>
> To run the app locally:
> ```bash
> cmake -B build -DBUILD_UI_APP=ON -DBUILD_UI_PLUGIN=ON
> cmake --build build --target video-hotspot-app
> ./build/ui/app/video-hotspot-app
> ```

### Module-Embedded Demo

**[▶ demo-module.mp4](demo/demo-module.mp4)** — Video Hotspot shown as a loaded module inside the Logos Basecamp shell

Shows the plugin embedded in a mock Basecamp frame with sidebar navigation,
network status (Waku/Codex/Nomos), and plugin footer (`video_hotspot.so`).

Snapshots: [launch](demo/snapshot_01_launch.png) · [map](demo/snapshot_02_map.png) · [timeline](demo/snapshot_03_timeline.png) · [upload](demo/snapshot_04_upload.png) · [plugin loaded](demo/snapshot_05_plugin_loaded.png)

See [`demo/FURPS_VERIFICATION.md`](demo/FURPS_VERIFICATION.md) for full FURPS+ spec verification against the demo.

---

## Quick Start

### Prerequisites

- Qt 6 (Core, Concurrent, Network, Sql)
- Qt6 RemoteObjects — only needed when building with `BUILD_UI_PLUGIN=ON`
  - Fedora: `qt6-qtremoteobjects-devel`
  - Debian/Ubuntu: `libqt6remoteobjects6-dev`
- CMake ≥ 3.22, C++17 compiler

### Build

```bash
git clone <repo>
cd logos-video-hotspot
cmake -B build
cmake --build build
```

The CLI binary lands at `build/cli/video-hotspot`.

### Building the UI Plugin Against Logos-App Qt (Nix Qt 6.9.2)

logos-app ships its own Qt 6.9.2 via Nix. When building `BUILD_UI_PLUGIN=ON`,
you **must** build against that exact Qt — system Qt causes ABI mismatches and
link errors at runtime.

Use the helper script (edit it first to replace `<hash>` placeholders):

```bash
# Find the correct Nix store hash:
#   cat $(which logos-app) | grep qtbase
# Then edit scripts/build-with-nix-qt.sh and update the hash
bash scripts/build-with-nix-qt.sh
```

Or configure manually:

```bash
export NIX_QT_PREFIX="/nix/store/<hash>-qtbase-6.9.2"
cmake -B build -DBUILD_UI_PLUGIN=ON -DBUILD_WITH_NIX_QT=ON
cmake --build build --target video_hotspot
```

### CLI Usage

Human-readable output is the default. Add `--json` (`-J`) for machine-readable JSON.

#### Check node status

```bash
./build/cli/video-hotspot status
# Logos connection: connected (mock)
# Videos indexed:   0
# Pending messages: 0
# User-owned bytes: 0
# Cached bytes:     0
# Total used:       0
```

#### Upload a single video

```bash
./build/cli/video-hotspot upload path/to/video.mp4
# Uploaded: path/to/video.mp4
# CID:      44da7506d4de4d647af7ebffad3893e8ff7c0cefee50c573fc1660b17f2bc78a
```

#### Deduplication — upload the same file again

```bash
./build/cli/video-hotspot upload path/to/video.mp4
# Duplicate: path/to/video.mp4
# CID:       44da7506d4de4d647af7ebffad3893e8ff7c0cefee50c573fc1660b17f2bc78a (already uploaded)
```

#### Upload all videos in a folder

```bash
./build/cli/video-hotspot upload-folder path/to/folder
# Uploaded: path/to/folder/clip-a.mp4
# CID:      dc325b95ab25d5e15f41fc5253860bf8986a068875438c1cfca1f5ccc231ec36
# Uploaded: path/to/folder/clip-b.mp4
# CID:      8ec7e4b5670438b9e2044735dad688319ccd71480e1eae81710b12a94835614e
# Queued 2 file(s)...
#
# Summary: 2 uploaded, 0 failed
```

#### List indexed videos

```bash
./build/cli/video-hotspot list
# Total: 3 video(s)
#   CID:  44da7506d4de4d647af7ebffad3893e8ff7c0cefee50c573fc1660b17f2bc78a
#   Geo:  0, 0
#   Time: 2026-03-14T11:07:00Z
#   Size: 1730 bytes
#   Type: video/mp4
#   Owned: yes
#   ...
```

#### Download a video by CID

```bash
./build/cli/video-hotspot download 44da7506d4de4d647af7ebffad3893e8ff7c0cefee50c573fc1660b17f2bc78a ./downloads
# cid: 44da7506d4de4d647af7ebffad3893e8ff7c0cefee50c573fc1660b17f2bc78a
# local_path: ./downloads/44da7506d4de4d647af7ebffad3893e8ff7c0cefee50c573fc1660b17f2bc78a
# status: ok
```

#### Clear cached (non-user-owned) videos

```bash
./build/cli/video-hotspot cache clear
# cleared_bytes: 0
# cleared_count: 0
# status: ok
```

#### JSON output (machine-readable, with --json flag)

```bash
./build/cli/video-hotspot status --json
# {"cached_bytes":0,"connected":true,"index_count":0,"mode":"mock","pending_messages":0,"status":"ok","total_used_bytes":0,"user_owned_bytes":0}
```

#### Monitor a folder for new videos (foreground process)

```bash
./build/cli/video-hotspot monitor path/to/folder
```

---

## Overview

Video Hotspot is a Qt miniapp running inside the Logos app ("Basecamp") for uploading, storing, and collectively mapping events through video footage. Users import video files that are tagged with timestamp and geolocation. The system indexes these clips by location and time, allowing anyone to browse a live map of documented events.

### Inspiration

This spec synthesizes concepts from [LoreLine.Live](https://github.com/logos-co/ideas/issues/7) (collaborative documentary creation, raw footage preservation, collective memory) with geolocation-based event discovery. Like LoreLine, Video Hotspot believes:

- **Raw footage, your own opinion** — No overlays, no editorial framing. Present what was captured.
- **Everyone is a documentarian** — Ordinary people capture extraordinary moments; those fragments deserve to become part of something bigger.
- **Individuals can compete with institutions** — Collectively, distributed cameras have more angles than any news crew.

### Core Differentiator

Video Hotspot adds **spatial intelligence** to collective documentation: indexing videos by geolocation and timestamp so users can browse a map and scrub through time to see what happened, where, when.

---

## FURPS+ Specification

### F — Functionality

#### Video Upload
- [x] Import individual video files via file picker (manual select)
- [x] Import all videos from a folder (folder picker)
- [x] Folder monitoring: watch a folder, auto-upload new files added to it
- [x] Deduplication: compute content hash before upload — never upload the same file twice
- [x] Upload queue with progress indicators and retry logic
- [ ] Background uploading (continues while browsing map)

#### Timestamp & Geolocation Tagging
- [ ] Extract timestamp from video file metadata (EXIF/creation date)
- [ ] If video has EXIF geolocation: use it automatically
- [ ] If no EXIF geolocation: prompt user to pinpoint location on interactive map
- [x] Store precise coordinates + timestamp — no fuzzy zones or clustering for now

#### Map Browsing
- [ ] Interactive map showing video pins at their geolocations
- [ ] Click pin to preview/play video
- [ ] Zoom and pan across regions
- [ ] Basic search by location (center map on searched area)

> **Note:** Map UI is a vector mockup (Canvas-rendered, no pixel artefacts); pins respond to the timeline slider with animated opacity fades. Real interactive map, pin data from index, and granularity buttons are pending SDK integration.

#### Timeline Component (Video Scrubber)
A proper video-scrubber-style timeline replaces the basic range slider. This component shows **where data exists** in time and provides a playback-style UX for temporal navigation.

##### Visual Design
- [ ] **Track bar** — Horizontal bar at bottom of map screen (~48–64px tall)
- [ ] **Data heatmap overlay** — On the track itself, render density indicators showing where video data exists in time (similar to YouTube's "most replayed" heatmap)
  - Computed per-bucket: divide visible time range into N buckets (e.g., 100), count videos per bucket
  - Render as translucent bars or gradient overlay on track (higher density = taller bar / warmer color)
  - Empty time ranges show flat/dim track — user sees at a glance where content exists
- [ ] **Playhead** — Vertical line/marker indicating current time position
  - Draggable handle on top of playhead for scrubbing
  - Current timestamp displayed above or beside playhead (e.g., "2026-03-14 14:30")
- [ ] **Time axis labels** — Start and end timestamps of visible range displayed at track edges
- [ ] **Granularity button** — Single cycle button to toggle time granularity
  - Cycles through: **hour → day → week → month → year → hour...**
  - Displays current granularity as icon+label (e.g., 📅 Day)
  - One tap advances to next granularity level
- [ ] **Play/Pause button** — Animate playhead forward through time automatically

##### Interaction Model
- [ ] **Drag playhead** — Click and drag the playhead handle to scrub through time; map pins animate (appear/fade) in real-time as playhead moves
- [ ] **Click to jump** — Click anywhere on the track to jump playhead to that position
- [ ] **Scrub velocity** — Fast drag = fast scrub; slow drag = precise scrub
- [ ] **Granularity cycle** — Tap the granularity button to cycle to the next level; the track re-renders with a new time scale
  - Hour granularity: 1h increments, ~24h visible range
  - Day granularity: 1d increments, ~30d visible range
  - Week granularity: 1w increments, ~3 months visible range
  - Month granularity: 1mo increments, ~1 year visible range
  - Year granularity: 1y increments, ~10 years visible range
- [ ] **Play mode** — Press play to auto-advance playhead; playhead moves forward one increment per second (configurable)
- [ ] **Keyboard shortcuts** — Left/Right arrows to step playhead; Space to play/pause

##### Data Density Computation
- [ ] Query IndexingService for video timestamps within visible time range
- [ ] Bucket timestamps into N bins (N ≈ track width / 4px for ~4px per bucket)
- [ ] Normalize bucket counts to [0, 1] range (max bucket = 1.0)
- [ ] Render each bucket as a vertical bar on the track, height proportional to normalized density
- [ ] Update heatmap when: (a) granularity changes, (b) new videos added, (c) visible map region changes (if filtering by geo)

##### State Management
- [ ] **Owns:** playhead position (timestamp), playing state (bool), granularity level (enum)
- [ ] **Receives:** time range bounds (min/max timestamp from IndexingService), video timestamp list (for heatmap), geo filter (optional, from map viewport)
- [ ] **Emits:** `currentTimeChanged(timestamp)` — map listens to show/hide pins; `granularityChanged(level)` — for any UI sync

##### Qt Implementation Notes
- Recommend custom `QWidget` subclass (not `QSlider`) for full rendering control
- Use `QPainter` to draw: track background, heatmap bars, playhead, labels
- Handle `mousePressEvent`, `mouseMoveEvent` for drag/click interactions
- Expose Qt properties: `currentTime`, `minTime`, `maxTime`, `granularity`, `playing`
- For QML: wrap as `Q_OBJECT` with `Q_PROPERTY` for reactive binding, or implement as pure QML `Item` with `Canvas` rendering

#### Video Playback & Download
- [ ] Play videos directly from Logos Storage
- [x] Download videos for offline viewing
- [ ] Downloaded videos become seedable (user becomes uploader for that content)
- [x] Track which videos are user-owned vs. cached downloads

#### Logos Stack Integration
- [ ] **Logos Messaging** — Live/real-time indexing as videos are uploaded
  - Publishes metadata (CID, geolocation, timestamp) on upload
  - Subscribers receive new video announcements in real-time
- [ ] **Logos Storage** — Decentralized storage for video files (content-addressed)
- [ ] **Logos Blockchain** — Batch/historical indexing
  - Periodic batches (e.g., 24-hour aggregates) of video metadata committed to blockchain
  - Indexing only — not for proofing or authentication

> **Note:** All three Logos stack components are mock implementations (local SQLite / filesystem). The API surfaces are wired and the wrappers are production-ready; the SDK calls are stubbed pending logos-cpp-sdk integration.

#### CLI (Headless Mode)
Command-line interface for scripting, automation, and end-to-end testing. Runs against Logos Core in headless mode (no Qt UI required).

**Commands:**
- [x] `upload <file>` — Upload a single video file
- [x] `upload-folder <path>` — Upload all videos in a folder
- [x] `monitor <path>` — Start monitoring a folder for new videos (foreground process)
- [x] `list` — List all indexed videos (timestamp, geolocation, CID)
- [x] `download <id>` — Download a video by ID/CID
- [x] `status` — Show node status, storage usage, connection state
- [x] `cache clear` — Clear cached (non-user-owned) videos

**Output:**
- [x] Human-readable output by default (terminal-friendly)
- [x] `--json` / `-J` flag for machine-readable JSON output
- [x] Exit codes follow standard conventions (0 = success, non-zero = error)

---

### U — Usability

#### Screens (Detailed)

##### 1. Upload Screen
The upload screen is the primary entry point for adding content:

- **File Picker Button** — Opens system file dialog to select one or more video files
- **Folder Picker Button** — Select a folder; all video files within are queued for upload
- **Folder Monitor Toggle** — Enable/disable watching a configured folder for new files
  - When enabled: any new video dropped into the monitored folder auto-queues
  - Visual indicator shows monitoring status (active/inactive)
- **Upload Queue** — List of pending/in-progress uploads showing:
  - Filename and thumbnail preview
  - Progress bar (percentage complete)
  - Status: pending, uploading, processing, complete, failed
  - Retry button for failed uploads
- **Dedup Status** — Indicator when a file is skipped (already uploaded, hash match)
  - "Already uploaded" badge with link to existing entry
- **Geolocation Prompt** — For files without EXIF location:
  - Inline map widget to pinpoint location
  - Required before upload proceeds

##### 2. Map Screen
The map screen is the core browsing experience:

- **Interactive Map** — Full-screen map (OpenStreetMap or similar)
  - Video pins displayed as markers at their geolocations
  - Pin density visualization: areas with many videos show clustered indicators
  - Pins color-coded or sized by recency (brighter/larger = more recent)
- **Timeline Component** — Video-scrubber-style horizontal bar at bottom of screen (see [Timeline Component](#timeline-component-video-scrubber) for full spec)
  - **Data heatmap track** — Density overlay showing where videos exist in time (YouTube "most replayed" style)
  - **Playhead** — Vertical marker showing current time, draggable to scrub
  - **Granularity cycle button** — Single button cycles: hour → day → week → month → year
  - **Click-to-jump** — Click anywhere on track to jump to that time
  - **Play button** — Auto-advance playhead through time
  - As playhead moves, pins appear/disappear/fade based on timestamp
- **Pin Interaction**
  - Hover: show timestamp and thumbnail preview
  - Click: expand to video player overlay
  - Download button on expanded view
- **Zoom Controls** — Standard +/- or scroll-to-zoom
- **Search Bar** — Type location name to center map on that area

##### 3. Settings Screen
Configuration and preferences:

- **Storage Allocation**
  - Slider or input to set maximum local storage for cached/downloaded videos
  - Current usage display (e.g., "Using 2.3 GB of 10 GB allocated")
- **Folder Monitor Path**
  - Display currently monitored folder path
  - Button to change monitored folder
  - Toggle to enable/disable monitoring
- **Network Settings**
  - Bandwidth limits (optional)
  - Connection status indicator

##### 4. Downloaded / Cache Management Screen
Manage local video storage:

- **Space Usage Bar** — Visual bar showing total allocated vs. used space
  - Breakdown: user-owned vs. cached (downloaded from others)
- **Video List** — Two sections:
  - **Your Videos** (user-uploaded) — Never auto-deleted
    - Each entry shows: thumbnail, title/filename, size, upload date
    - Manual delete option (removes from local + stops seeding)
  - **Cached Videos** (downloaded from others) — Deletable
    - Each entry shows: thumbnail, title/filename, size, download date
    - Checkbox selection for batch delete
    - "Clear All Cached" button
- **Auto-Clean Settings**
  - When storage limit reached: auto-delete oldest cached videos (not user-owned)
  - Display estimated days of cache remaining at current usage rate
- **Manual Delete Controls**
  - Select multiple cached videos
  - Delete selected button
  - Confirmation dialog

#### Interface Guidelines
- [x] Clean, minimal interface — map-centric design
- [x] Dark mode default (field use, nighttime events)
- [ ] Integrates within Basecamp app navigation

---

### R — Reliability

#### Offline Operation
- [x] Upload queue persists across app restarts
- [ ] Downloaded videos playable offline
- [x] Local database for pending uploads (SQLite or equivalent)
- [ ] Sync on reconnect: background service handles upload queue

#### Upload Resilience
- [ ] Chunked upload with resumption (no full-file restart on failure)
- [ ] Automatic retry with exponential backoff
- [x] Corruption detection (checksum verification pre-upload)
- [x] Dedup check prevents wasted bandwidth on re-uploads

#### Data Integrity
- [x] Content-addressed storage (CID-based) ensures immutability
- [ ] Hash verification on download

#### Graceful Degradation
- [ ] If Logos Storage unreachable: local-only mode with periodic retry
- [ ] If Logos Messaging unreachable: metadata batched and sent when available
- [ ] Clear status indicators: user knows when offline/syncing/synced

> **Note:** The mock always operates in local-only mode. `flushPending()` in MessagingClient is a stub; the `pending` DB table is scaffolded but the retry loop is not wired.

---

### P — Performance

#### Upload Time
- [ ] Target: <10s upload start-to-confirmation for 30s clip on broadband
- [ ] Chunk size tuned for typical bandwidth (512KB–2MB chunks)
- [ ] Parallel chunk upload where beneficial
- [x] Dedup check (hash comparison) completes before upload starts

#### Map Rendering
- [ ] Target: <2s initial load of map with 100 pins visible
- [ ] Lazy loading: fetch video details on demand (click pin)
- [ ] Tile caching for offline map access in previously viewed areas
- [x] Smooth timeline scrubbing (no UI freeze)

#### Timeline Component Performance
- [ ] Heatmap recomputation: <50ms for 10,000 videos (bucket aggregation must be fast)
- [ ] Playhead drag: 60fps rendering, no dropped frames during scrub
- [ ] Granularity switch: <100ms to re-render track with new time scale
- [ ] Play mode: smooth 1-increment-per-second animation (configurable playback speed)
- [ ] Memory: cache heatmap bucket data; only recompute on granularity change or data change

#### Resource Usage
- [ ] Video compression before upload (H.265/HEVC where supported)
- [ ] Background process: minimal CPU/memory footprint when idle
- [x] Configurable storage limits to prevent disk bloat

---

### S — Supportability

#### Platform
- [ ] **Qt miniapp** running inside Basecamp (Logos desktop app)
- [x] Cross-platform via Qt: Windows, macOS, Linux

#### Storage Management
- [x] User-configurable local storage limits
- [x] Auto-clean: oldest cached videos deleted when limit reached
- [x] User-owned videos never auto-deleted
- [ ] Manual deletion controls for both owned and cached content

#### Open Formats
- [x] Video: MP4 (H.264/H.265), WebM (VP9) supported
- [x] Metadata: JSON, schema-documented
- [x] Export: download original files

#### Observability
- [ ] Upload history visible to user
- [ ] Storage used / allocated display
- [ ] Sync status indicators
- [ ] Folder monitor status

#### CLI & Headless Mode
- [x] CLI and Qt UI share the same core module APIs (no duplicate logic)
- [x] Logos Core runs in headless mode when CLI is invoked (no window spawned)
- [x] CLI commands are scriptable and deterministic (designed for e2e testing)
- [x] CLI exit codes and JSON output enable automated test harnesses

---

### + — Hardware & Deployment

#### Basecamp Integration
- [ ] Runs as Qt miniapp inside Basecamp (Logos desktop app)
- [ ] Inherits Basecamp's platform support (Windows 10+, macOS 11+, Linux)
- [ ] Uses Basecamp's Logos stack connections (Messaging, Storage, Blockchain)

> **Note:** Plugin interface (`IComponent`) is implemented, `manifest.json` is shipped alongside the `.so` (enabling logos-app discovery), and the plugin builds cleanly. Actual Basecamp wiring (`createWidget(logosAPI)` → real SDK) is marked TODO in `VideoHotspotPlugin.cpp` pending a live Basecamp binary.

#### Logos Stack Dependencies
- [ ] **Logos Messaging** — Real-time indexing
  - Publish video metadata on upload
  - Subscribe to new video announcements
- [ ] **Logos Storage** — Decentralized video storage
  - Upload/download via client library
  - Content-addressed (CID-based)
- [ ] **Logos Blockchain** — Batch indexing
  - 24-hour batches of video metadata
  - Indexing only (not proofing/authentication)

#### Security & Privacy
- [ ] Metadata minimization: only timestamp + explicit user-provided geolocation
- [x] No PII storage
- [ ] Downloaded videos contribute to network (seeding)

---

## MVP Scope

For initial release:

1. **Upload** — File picker, folder picker, folder monitor, dedup, upload queue
2. **Geolocation** — EXIF extraction or manual pin placement
3. **Map View** — Browse pins by location, timeline component with data heatmap and scrubber UX for time filtering
4. **Download & Cache** — Download videos, storage management, auto-clean
5. **Offline Queue** — Capture offline, sync when connected

---

## References

- [LoreLine.Live (logos-co/ideas #7)](https://github.com/logos-co/ideas/issues/7) — Collaborative documentary platform concept
- [Logos Network](https://logos.co)

---

## Status

**Draft** — Awaiting review

*Created: 2026-03-14*
