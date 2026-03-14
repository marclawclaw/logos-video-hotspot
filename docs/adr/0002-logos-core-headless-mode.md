# ADR-0002: Logos Core Headless Mode Integration

**Status:** Accepted  
**Date:** 2026-03-14  

---

## Context

The FURPS specifies a CLI frontend that shares the same core business logic as the Qt UI — no duplicated logic. The CLI must work without spawning a Qt window ("headless mode"). Logos Core (`logos-liblogos`) may require initialization that normally happens inside the Qt application event loop.

---

## Decision

**The CLI frontend links against the same `core/` library as the Qt plugin and initialises Logos Core via the headless API in `logos-liblogos`.**

Concretely:

1. `core/` is compiled as a static library (`libvideo_hotspot_core`).
2. The Qt plugin (`ui/plugin/`) links `libvideo_hotspot_core` + Qt6.
3. The CLI (`cli/`) links `libvideo_hotspot_core` and starts a minimal `QCoreApplication` (no window, no widgets) to satisfy Qt event loop requirements.
4. Logos Core is initialised with the headless flag from `logos-cpp-sdk` before any SDK calls.

---

## Rationale

- `QCoreApplication` is the non-GUI subset of Qt's application class; it provides the event loop needed by Qt networking (used internally by logos-liblogos) without opening any window.
- Using a shared static `core/` library guarantees CLI and UI are always in sync — there is literally one copy of every business-logic function.
- Headless mode is explicitly called out in the FURPS Supportability section and is a design requirement for e2e test harnesses.

---

## Consequences

- CLI binary depends on Qt6Core (no Qt6Widgets or Qt6Quick).
- Integration tests can run the CLI against a local Logos testnet without a display server.
- `core/` must never include any `QWidget` or `QML` headers — enforced at the CMake target level via `target_link_libraries` scoping.

---

## Alternatives Considered

| Option | Reason Rejected |
|---|---|
| Duplicate logic in CLI | Violates FURPS + engineer modular design principles |
| Custom event loop (libuv/asio) | Incompatible with Qt's internal object model used by logos-cpp-sdk |
| Run Qt app hidden (`-platform offscreen`) | Fragile; not available on all platforms |
