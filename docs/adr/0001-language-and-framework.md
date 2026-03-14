# ADR-0001: Language and Framework Choice

**Status:** Accepted  
**Date:** 2026-03-14  

---

## Context

Video Hotspot must run as a miniapp inside the Logos desktop application ("Basecamp"). Basecamp is a Qt6 application written in C++, loaded via a plugin system where each miniapp is a shared library (`.so`/`.dylib`) implementing a Qt plugin interface. The build system is CMake + Nix.

We need to decide the implementation language and UI framework.

---

## Decision

**Use C++17 with Qt6** — matching every other Logos miniapp.

- **Language:** C++17
- **UI framework:** Qt6 (Widgets + QML for the map/timeline screen)
- **Build system:** CMake (with Nix flake for reproducible builds)
- **Plugin format:** Qt plugin shared library loaded by logos-app

---

## Rationale

1. **Ecosystem fit** — All existing Logos miniapps (logos-storage-ui, logos-chatsdk-ui, logos-blockchain-ui) use C++/Qt6. Using Python Qt bindings (PyQt6/PySide6) would require a separate runtime, complicating the plugin loading mechanism and adding a substantial dependency.

2. **logos-cpp-sdk** — The official Logos SDK provides C++ headers. Python bindings do not exist and wrapping them would require significant extra engineering.

3. **Qt Remote Objects** — The logos-app IPC layer (used for core↔UI communication) is Qt-native and easiest to use from C++.

4. **Performance** — Video thumbnail generation, hash computation, and map rendering benefit from native code. C++ lets us call into ffmpeg/libavcodec directly.

5. **Plugin interface** — Qt plugin machinery (`Q_PLUGIN_METADATA`, `Q_INTERFACES`) is a first-class C++ feature.

---

## Consequences

- Developers must be proficient in C++17 and Qt6.
- QML is used for the Map Screen (complex interactive canvas); other screens may use Qt Widgets or QML.
- Python is only used for CLI tooling and test harness scripts (not the main codebase).
- Cross-compilation for Windows/macOS is handled via Nix.

---

## Alternatives Considered

| Option | Reason Rejected |
|---|---|
| Python + PySide6 | No logos-cpp-sdk bindings; plugin loading complexity; runtime overhead |
| Rust + egui | No Qt plugin integration; logos-cpp-sdk not available in Rust |
| Electron / Web | Not a Qt miniapp; incompatible with Basecamp plugin system |
