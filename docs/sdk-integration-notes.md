# SDK Integration Notes

**Branch:** `feat/real-logos-sdk-integration`  
**Date:** 2026-03-16  
**Status:** Build passing, tests passing (unit), awaiting runtime validation against live daemon

---

## Summary

This branch completes the real Logos SDK integration for logos-video-hotspot.
The scaffolding that was previously in place assumed non-existent API surface;
this document records what was found, what was changed, and what remains.

---

## API Surface Mismatches (Fixed)

### 1. `createHeadlessLogosAPI()` / `destroyHeadlessLogosAPI()` — Does Not Exist

**Previous scaffolding assumed:**
```cpp
#include <interface.h>
m_logosAPI = createHeadlessLogosAPI();
destroyHeadlessLogosAPI(m_logosAPI);
```

**Reality (logos-liblogos `src/logos_core/logos_core.h`):**
```c
void logos_core_init(int argc, char *argv[]);
void logos_core_set_mode(int mode);  // LOGOS_MODE_REMOTE=0, LOGOS_MODE_LOCAL=1
void logos_core_start();
void logos_core_cleanup();
```

**Reality (logos-cpp-sdk `cpp/logos_api.h`):**
```cpp
LogosAPI(const QString& module_name, QObject* parent = nullptr);
LogosAPIClient* getClient(const QString& target_module) const;
```

**Fix:** `cli/src/headless_core.h` updated to call the real C API.

---

### 2. `interface.h` Location Mismatch

**Previous scaffolding assumed:** `<interface.h>` at vendor root  
**Reality:** `vendor/logos-liblogos/src/common/interface.h` — a `PluginInterface` base class (unrelated to headless init)

**Fix:** CLI uses `<logos_core.h>` (from `src/logos_core/`) for the C API. CMakeLists.txt updated with correct include paths.

---

### 3. `ui/plugin/CMakeLists.txt` Wrong Include Path

Same `interface.h` path issue in the UI plugin CMakeLists. Fixed to point to `src/logos_core/` and `src/common/`.

---

## Build Dependencies

The SDK is split across three repos, all of which must be built:

| Repo | Purpose | Build output |
|------|---------|--------------|
| `logos-co/logos-cpp-sdk` | `LogosAPI`, `LogosAPIClient` C++ class | compiled as part of logos-liblogos |
| `logos-co/logos-module` | `module_lib.h` — required by logos-liblogos | `liblogos_module.a` |
| `logos-co/logos-liblogos` | `logos_core_init/start/cleanup` C API | `liblogos_core.so` + `liblogos_sdk.a` |

### Build Blocker: `-fPIC` Required

logos-liblogos builds `logos_core` as a **shared library** (`liblogos_core.so`).
logos-cpp-sdk sources are compiled inline; without `-fPIC`, linking fails with:

```
relocation R_AARCH64_ADR_PREL_PG_HI21 ... can not be used when making a shared object; recompile with -fPIC
```

**Fix:** Pass `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` when building logos-liblogos.

### Build Blocker: `logos-module` Required

logos-liblogos's `plugin_manager.cpp` includes `<module_lib/module_lib.h>` which comes from logos-module. This repo is not a submodule of logos-liblogos by default and must be built and installed separately.

**Fix:** `scripts/build-sdk.sh` builds logos-module first, then passes `-DLOGOS_MODULE_ROOT=...` to logos-liblogos.

---

## What "Zero Mock" Means in This Build

Per Franck's requirements, there is no silent mock fallback in the production code paths:

- **`StorageClient::upload()`** — when `m_logosAPI` is null (daemon not started), emits `uploadFailed()` with message `"Logos not connected: initLogos() was not called."` Not a silent local copy.
- **`StorageClient::download()`** — same: fails with clear error when not connected.
- **`MessagingClient::publish()`** — when not connected, persists to pending queue and emits `publishFailed()`. No silent discard.
- **CLI status output** — `"mode": "sdk"` when connected, `"mode": "offline"` when daemon unreachable.
- **`#else` (no-SDK builds)** — reports `"mode": "local-only"`, not `"mock"`.

The `#else` local-only path (compiled only when SDK is absent) is preserved for CI environments without a Logos daemon. It is never compiled when the SDK is present.

---

## Runtime Requirements

To run the CLI against a real Logos network:

1. Download and run the [Logos app](https://github.com/logos-co/logos-app/releases/latest) — the daemon starts automatically.
2. Ensure the Codex and Waku modules are loaded (check via `lm` CLI from logos-module).
3. Run: `./build/cli/video-hotspot status`

Expected output when connected:
```json
{"status":"ok","connected":true,"mode":"sdk","index_count":0,...}
```

Expected output when daemon not running:
```json
{"status":"ok","connected":false,"mode":"offline","index_count":0,...}
```

---

## Unit Test Notes

Unit tests (DeduplicatorTests, IndexingServiceTests, MetadataExtractorTests) pass without a daemon — they don't touch SDK paths.

`UploadQueueTests`:
- `test_pipelineAlwaysTerminates` — passes without daemon (terminal state = `Failed` with clear error)
- `test_duplicateFileIsDetected` — skipped automatically when no daemon is reachable
- All other tests pass without daemon

---

## What Remains (Future Work)

- [ ] **Runtime validation** against live Logos daemon (Codex + Waku module names may need tuning)
- [ ] **UI plugin build** — `ui/plugin/CMakeLists.txt` updated but not built in this PR (requires Qt6::Quick + running logos-app for plugin loading)
- [ ] **Real demos** — CLI demo and miniapp demo per Franck's requirements (separate task)
- [ ] **Codex module name** — `"codex"` / `"CodexReplica"` assumed; validate against logos-app plugin registry
- [ ] **Waku module name** — `"waku"` / `"WakuReplica"` assumed; validate similarly
- [ ] **`logos_core_init(0, nullptr)`** — passing null argv is valid per C standard but should be tested
