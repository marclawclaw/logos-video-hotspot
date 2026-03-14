# TODO — Video Hotspot Progress Notes

_Last updated: 2026-03-14 by Marclaw subagent_

---

## ✅ What Has Been Implemented

### Project Structure & Tooling
- CMake build system (root + per-module: core, cli, ui/plugin, ui/app, tests)
- ADRs (Architecture Decision Records) covering:
  - ADR-0001: Language & framework (C++17 + Qt 6)
  - ADR-0002: Logos Core headless mode for CLI
  - ADR-0003: Storage module API design
  - ADR-0004: Messaging module + live indexing
  - ADR-0005: BLAKE3 deduplication strategy

### Core Module (`core/`)
All six core module **headers** fully designed + **mock implementations** written:

| Module | Header | Implementation |
|---|---|---|
| `StorageClient` | ✅ | ✅ mock (local filesystem + SQLite, SHA-256 as CID) |
| `Deduplicator` | ✅ | ✅ mock (BLAKE3 hash + local SQLite DB) |
| `MessagingClient` | ✅ | ✅ mock (in-memory pub/sub, pending queue persisted to SQLite) |
| `IndexingService` | ✅ | ✅ mock (SQLite `videos` table, JSON parse/publish) |
| `MetadataExtractor` | ✅ | ✅ mock (returns placeholder data — no real ffprobe/EXIF) |
| `UploadQueue` | ✅ | ✅ mock (QFutureWatcher pipeline: hash → dedup → upload → index) |

### CLI (`cli/`)
- `main.cpp` — full command dispatch for all 7 commands
- `headless_core.h` — `HeadlessCore` struct wiring all modules together (no Qt GUI)
- `output_formatter.{h,cpp}` — JSON (default) and `--human` formatted output
- `commands/commands.h` — command function declarations
- Command implementations:
  - `upload_command.cpp` — single file upload
  - `upload_command.cpp` (folder) — `upload-folder` enqueuing all videos in dir
  - `monitor_command.cpp` — foreground folder watcher loop
  - `list_command.cpp` — query IndexingService, output as JSON array
  - `download_command.cpp` — download by CID to dest dir
  - `status_command.cpp` — node status + storage stats
  - `cache_command.cpp` — `cache clear` evicts all non-user-owned cached videos

### UI Plugin (`ui/plugin/`)
- `IComponent.h` — Logos app plugin interface (from jimmy-claw/scala)
- `VideoHotspotPlugin.{h,cpp}` — Q_PLUGIN_METADATA wired to IComponent; `createWidget()` / `destroyWidget()` skeleton
- `VideoHotspotApp.qml` — Root QML with 4-tab navigation shell (Upload / Map / Downloads / Settings) — **tabs are all placeholder "TODO" labels**
- `video_hotspot_plugin.json` — plugin metadata

### Tests
- Unit tests (`tests/unit/`):
  - `test_deduplicator.cpp` — dedup detection, hash persistence
  - `test_indexing_service.cpp` — record insert/query
  - `test_metadata_extractor.cpp` — placeholder extraction
  - `test_upload_queue.cpp` — queue lifecycle, status transitions
- E2E shell tests (`tests/e2e/`):
  - `e2e_helpers.sh` — shared setup/teardown helpers
  - `test_upload.sh` — single file upload, verify CID returned
  - `test_upload_folder.sh` — folder upload, verify all files
  - `test_deduplication.sh` — re-upload returns duplicate
  - `test_download.sh` — download by CID
  - `test_list.sh` — list after upload
  - `test_status.sh` — status output shape
  - `test_cache_clear.sh` — cache clear only removes non-user-owned
  - `test_folder_monitor.sh` — drop file into monitored folder, verify auto-upload
  - `test_upload_download.sh` — full round-trip

### FURPS+ Spec
- `README.md` contains the full FURPS+ specification with all screens designed in detail.

---

## 🔲 What Still Needs To Be Done

### Core — Replace Mocks with Real Logos SDK

- [ ] **StorageClient**: Replace SHA-256/filesystem mock with real `logos::storage::Client` calls
  - Chunked upload with resumption (FURPS: `upload` + retry)
  - Hash verification on download
  - Real CID (content-addressed) from Logos Storage
- [ ] **MessagingClient**: Replace in-memory mock with real `logos::messaging::Client`
  - Connect to Logos Messaging node (pass LogosAPI* from plugin)
  - Exponential backoff retry for publish queue
- [ ] **MetadataExtractor**: Implement real metadata extraction
  - ffprobe/libavformat for EXIF timestamp and GPS coordinates
  - Thumbnail generation (seek to 1s, extract frame as PNG)
  - Without this: all uploads go to `AwaitingGeo` status (no EXIF geo)
- [ ] **IndexingService**: Batch blockchain indexing job
  - 24-hour aggregate batches committed to Logos Blockchain
  - Periodic timer or cron-style trigger inside the service
- [ ] **Deduplicator**: Switch from SHA-256 to BLAKE3
  - Current mock uses QCryptographicHash (no BLAKE3 in Qt)
  - Add BLAKE3 C library (e.g., `b3sum` or embed `blake3.h`)

### UI — QML Screens (All TODO)

- [ ] **Upload Screen**: Replace placeholder with real UI
  - File picker button (QML FileDialog)
  - Folder picker button
  - Folder monitor toggle with status indicator
  - Upload queue list (filename, progress bar, status, retry button)
  - Dedup badge ("Already uploaded" with link)
  - Inline map widget for `AwaitingGeo` items
- [ ] **Map Screen**: Core browsing UI
  - Embed map tile renderer (MapLibre GL / QtLocation)
  - Video pin markers at geolocations
  - Timeline slider with granularity selector (hour/day/week/month/year)
  - Animate through time (Play button)
  - Click pin → video player overlay
  - Download button on expanded pin
  - Search bar to center map on location
- [ ] **Downloads/Cache Screen**
  - Space usage bar (user-owned vs cached)
  - Two-section list: Your Videos / Cached Videos
  - Batch delete checkboxes for cached
  - "Clear All Cached" button
  - Auto-clean settings display
- [ ] **Settings Screen**
  - Storage allocation slider + current usage display
  - Folder monitor path + toggle
  - Network settings (bandwidth limits, connection status)
- [ ] **VideoHotspotPlugin**: Wire `createWidget()` to actual QML widget
  - Currently just a skeleton — needs to instantiate the QML engine + root component
  - Pass LogosAPI to StorageClient/MessagingClient when non-null

### Build & Integration

- [ ] Verify CMake builds cleanly on target platform (Linux/macOS/Windows)
- [ ] Add `.gitignore` entries for `build/` directory (currently tracked — should be excluded)
- [ ] Test CLI binary runs: `./build/cli/video-hotspot --help`
- [ ] Run unit tests: `ctest` from build dir
- [ ] Run e2e tests: requires a built binary at expected path
- [ ] Add `build/` to `.gitignore` — build artifacts shouldn't be in the repo

### Testing Gaps

- [ ] Unit tests for `StorageClient` (currently no test file)
- [ ] Unit tests for `MessagingClient` (currently no test file)
- [ ] Integration tests with a real (or mock-server) Logos stack
- [ ] E2E tests likely need the binary built first — validate test scripts work

### Logos Stack Integration (Blocked on SDK)

- [ ] Confirm Logos SDK (`logos-cpp-sdk`) availability and API surface
- [ ] Confirm jimmy-claw/scala IComponent interface matches `interfaces/IComponent.h`
- [ ] Check if `logos::identity::Manager` is needed for signed video metadata
- [ ] Logos Blockchain batch indexing API (what does commit look like?)

---

## 🚧 Blockers & Notes for Next Session

1. **No real Logos SDK yet** — all core modules are mock implementations. The architecture is ready to plug in the real SDK, but nothing works end-to-end against a real Logos node.

2. **MetadataExtractor is a placeholder** — without real EXIF/ffprobe extraction, every upload will hit the `AwaitingGeo` flow (user must manually pin location). This is fine for now but blocks automated testing of the happy path.

3. **`build/` is committed** — should be added to `.gitignore` and removed from tracking. This is polluting the repo.

4. **QML screens are all empty** — the tab shell exists but every screen is just a label saying "TODO". No UI is functional yet.

5. **BLAKE3 dependency** — Qt doesn't include BLAKE3. Need to decide: embed `blake3.h` directly (single-file), use `b3sum` via QProcess, or find a Qt-compatible library.

6. **Plugin loading path** — the deploy path `~/.local/share/Logos/LogosAppNix/plugins/video_hotspot/` in `VideoHotspotPlugin.h` needs to be confirmed against the actual Logos app (jimmy-claw/scala) plugin discovery mechanism.

7. **CMake hasn't been validated** — the build files were written but not tested in this session. First thing next session: run `cmake .. && make` and fix any compile errors.

---

## Suggested Next Session Order

1. Fix `.gitignore` to exclude `build/`
2. Build the project: `cd build && cmake .. && make -j$(nproc)`
3. Fix any compile errors
4. Run `./build/cli/video-hotspot --help` and `./build/cli/video-hotspot status`
5. Run unit tests with `ctest`
6. Start on UI: implement UploadScreen QML or wire `createWidget()` in the plugin
7. Investigate real Logos SDK availability
