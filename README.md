# Video Hotspot — FURPS Specification

> **Privacy-preserving video capture and geolocation-based event clustering on the Logos stack**

## Overview

Video Hotspot is a desktop application for capturing, uploading, and collectively mapping events through video footage. Users can record and upload video clips that are automatically tagged with timestamp and geolocation. The system aggregates these clips to identify "hotspots" — geographic clusters of activity that indicate significant events (protests, accidents, natural disasters, cultural moments).

### Inspiration

This spec synthesizes concepts from [LoreLine.Live](https://github.com/logos-co/ideas/issues/7) (collaborative documentary creation, raw footage preservation, collective memory) with geolocation-based event detection. Like LoreLine, Video Hotspot believes:

- **Raw footage, your own opinion** — No overlays, no editorial framing. Present what was captured.
- **Everyone is a documentarian** — Ordinary people capture extraordinary moments; those fragments deserve to become part of something bigger.
- **Individuals can compete with institutions** — Collectively, distributed cameras have more angles than any news crew.

### Core Differentiator

Video Hotspot adds **spatial intelligence** to collective documentation: automatically detecting where events are clustering in real-time through geolocation data, creating a "heat map" of unfolding situations.

---

## FURPS+ Specification

### F — Functionality

#### Video Capture & Upload
- [ ] Webcam/external camera integration for video recording
- [ ] Import existing video files (drag-and-drop or file picker)
- [ ] Automatic EXIF stripping (preserve only timestamp + user-approved geolocation)
- [ ] Configurable quality presets (low/medium/high) for bandwidth-constrained environments
- [ ] Background upload queue with retry logic
- [ ] Optional audio-only mode (smaller files, less identifiable)

#### Timestamp & Geolocation Tagging
- [ ] Capture precise timestamp (Unix epoch, timezone-aware)
- [ ] Manual geolocation input (map pin selection or coordinates)
- [ ] IP-based approximate location as optional default
- [ ] Privacy mode: fuzzy location (city-level or neighborhood-level only)

#### Hotspot Detection & Visualization
- [ ] Real-time aggregation of uploads by geolocation + time window
- [ ] Configurable clustering algorithm (e.g., DBSCAN or grid-based)
- [ ] Heat map visualization on map interface
- [ ] Hotspot alerts: notify users when activity spike detected in their region
- [ ] Temporal scrubbing: view hotspot evolution over time (slider to "rewind")

#### Search & Filter
- [ ] Filter by date range, location radius, keywords (user-provided tags)
- [ ] Sort by recency, proximity, or activity density
- [ ] Bookmark/save hotspots of interest
- [ ] Follow specific locations (persistent notifications)

#### Content Organization
- [ ] User-created collections ("lorelines") grouping related clips
- [ ] Community contributions: suggest clips to someone else's collection
- [ ] Moderation: collection owner approves/rejects contributions

#### Logos Stack Integration
- [ ] **Logos Messaging** — Peer-to-peer transport for upload metadata, hotspot announcements, and real-time notifications
- [ ] **Logos Storage** — Decentralized storage for video files (content-addressed, erasure-coded)
- [ ] **Logos Blockchain** (optional) — Timestamping proofs for clip authenticity (if blockchain anchoring desired)

---

### U — Usability

#### Desktop-First Design
- [ ] Clean, focused interface: map view + capture/import panel
- [ ] Keyboard shortcuts for power users (record, upload, navigate map)
- [ ] System tray integration for background operation
- [ ] Dark mode default (field use, nighttime events)

#### Low-Connectivity Environments
- [ ] Progressive upload: lower resolution first, full quality when bandwidth available
- [ ] Store-and-forward: local storage until connection restored
- [ ] Mesh relay (optional): Logos Messaging nodes on local network can relay uploads

#### Accessibility
- [ ] Screen reader support for core flows
- [ ] High-contrast mode
- [ ] Localization: i18n-ready (initial: English, Spanish, Arabic, Mandarin)

#### Onboarding
- [ ] Zero-KYC: no account creation required for basic use
- [ ] Optional pseudonymous identity (keypair-based, no PII)
- [ ] Progressive trust: unlock features (collections, following) after first upload

---

### R — Reliability

#### Offline Operation
- [ ] Full capture functionality without internet
- [ ] Local database for pending uploads (SQLite or equivalent)
- [ ] Sync on reconnect: background service handles upload queue

#### Upload Resilience
- [ ] Chunked upload with resumption (no full-file restart on failure)
- [ ] Automatic retry with exponential backoff
- [ ] Corruption detection (checksum verification pre-upload)

#### Data Integrity
- [ ] Content-addressed storage (CID-based) ensures immutability
- [ ] Redundant storage via Logos Storage erasure coding
- [ ] Optional: blockchain anchoring for timestamping proofs

#### Graceful Degradation
- [ ] If Logos Storage unreachable: local-only mode with periodic retry
- [ ] If Logos Messaging unreachable: metadata batched and sent when available
- [ ] Clear status indicators: user knows when offline/syncing/synced

---

### P — Performance

#### Upload Time
- [ ] Target: <10s upload start-to-confirmation for 30s clip on broadband
- [ ] Chunk size tuned for typical bandwidth (512KB–2MB chunks)
- [ ] Parallel chunk upload where beneficial

#### Map Rendering
- [ ] Target: <2s initial load of map with 100 hotspots visible
- [ ] Lazy loading: fetch clip details on demand (click hotspot)
- [ ] Tile caching for offline map access in previously viewed areas

#### Hotspot Aggregation
- [ ] Real-time updates: new clips reflected in hotspot intensity within <30s
- [ ] Background aggregation: client receives push updates via Logos Messaging
- [ ] Efficient spatial indexing (R-tree or geohash-based)

#### Resource Usage
- [ ] Video compression before upload (H.265/HEVC where supported)
- [ ] Configurable upload schedule (immediate vs. metered connection vs. scheduled)
- [ ] Background process: minimal CPU/memory footprint when idle

---

### S — Supportability

#### Configurable Retention
- [ ] User-configurable local storage limits (auto-delete oldest after X GB)
- [ ] Network-level retention policies (Logos Storage node operators define)
- [ ] User can request deletion of their uploads (propagates to Logos Storage nodes honoring policy)

#### Open Formats
- [ ] Video: MP4 (H.264/H.265), WebM (VP9) supported
- [ ] Metadata: JSON-LD or CBOR, schema-documented
- [ ] Export: download original files + metadata bundle

#### No Lock-In
- [ ] Self-hostable backend (Logos Storage + Logos Messaging nodes)
- [ ] API-first: documented REST/GraphQL endpoints for third-party clients
- [ ] Open-source client (MIT/Apache 2.0)

#### Observability
- [ ] Structured logging (OpenTelemetry-compatible)
- [ ] Metrics: upload success rate, hotspot detection latency, storage utilization
- [ ] User-facing: upload history, storage used, sync status

---

### + — Hardware & Deployment

#### Desktop Application
- [ ] **Windows** — Windows 10+ (Electron or native)
- [ ] **macOS** — macOS 11+ (Electron or native)
- [ ] **Linux** — Ubuntu 20.04+, Fedora 34+ (Electron or native AppImage/Flatpak)
- [ ] Cross-platform framework: Electron (Chromium + Node.js) or Tauri (Rust + WebView)

#### Out of Scope
- [ ] Mobile platforms (iOS, Android) — explicitly excluded from this spec

#### Logos Stack Dependencies
- [ ] **Logos Messaging** — Messaging layer
  - Content topics for: upload announcements, hotspot updates, notifications
  - Filter/LightPush for desktop client
- [ ] **Logos Storage** — Storage layer
  - Client library for upload/download
  - Discovery via Logos Messaging or DHT
- [ ] **Logos Blockchain** (optional) — Blockchain layer
  - Timestamping service (anchor CID + timestamp)
  - Not required for MVP

#### Infrastructure
- [ ] No centralized server required (Logos stack is peer-to-peer)
- [ ] Optional: aggregation node for hotspot computation (can run on any peer)
- [ ] Bootstrap nodes: community-run or Logos-provided for initial discovery

#### Security & Privacy
- [ ] End-to-end encryption for private collections (NaCl/libsodium)
- [ ] Metadata minimization: only timestamp + fuzzy location required
- [ ] No PII storage; pseudonymous identities only
- [ ] Plausible deniability: Logos Messaging relay doesn't reveal origin

---

## MVP Scope

For initial release:

1. **Capture & Upload** — Webcam recording or file import, upload to Logos Storage via Logos Messaging
2. **Map View** — View own uploads and public uploads on map
3. **Basic Hotspot** — Grid-based clustering with simple heat map
4. **Offline Queue** — Capture offline, sync when connected

Out of scope for MVP:
- Collections/lorelines
- Community contributions
- Logos Blockchain timestamping
- Advanced search/filter
- Mobile platforms

---

## References

- [LoreLine.Live (logos-co/ideas #7)](https://github.com/logos-co/ideas/issues/7) — Collaborative documentary platform concept
- [Privacy Preserving Location Tracker (logos-co/ideas #3)](https://github.com/logos-co/ideas/issues/3) — Private location sharing
- [Logos Network](https://logos.co)

---

## Status

**Draft** — Awaiting review

*Created: 2026-03-14*
