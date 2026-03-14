# Video Hotspot — FURPS Specification

> **Privacy-preserving video capture and geolocation-based event clustering on the Logos stack**

## Overview

Video Hotspot is a mobile-first application for capturing, uploading, and collectively mapping events through video footage. Users in the field can record and upload video clips that are automatically tagged with timestamp and geolocation. The system aggregates these clips to identify "hotspots" — geographic clusters of activity that indicate significant events (protests, accidents, natural disasters, cultural moments).

### Inspiration

This spec synthesizes concepts from [LoreLine.Live](https://github.com/logos-co/ideas/issues/7) (collaborative documentary creation, raw footage preservation, collective memory) with geolocation-based event detection. Like LoreLine, Video Hotspot believes:

- **Raw footage, your own opinion** — No overlays, no editorial framing. Present what was captured.
- **Everyone is a documentarian** — Ordinary people capture extraordinary moments; those fragments deserve to become part of something bigger.
- **Individuals can compete with institutions** — Collectively, thousands of phones have more angles than any news crew.

### Core Differentiator

Video Hotspot adds **spatial intelligence** to collective documentation: automatically detecting where events are clustering in real-time through geolocation data, creating a "heat map" of unfolding situations.

---

## FURPS+ Specification

### F — Functionality

#### Video Capture & Upload
- [ ] Native camera integration (iOS/Android) for video recording
- [ ] Automatic EXIF stripping (preserve only timestamp + user-approved geolocation)
- [ ] Configurable quality presets (low/medium/high) for bandwidth-constrained environments
- [ ] Background upload queue with retry logic
- [ ] Optional audio-only mode (smaller files, less identifiable)

#### Timestamp & Geolocation Tagging
- [ ] Capture precise timestamp (Unix epoch, timezone-aware)
- [ ] GPS coordinates at capture time (device location)
- [ ] Optional: user-corrected location (drag pin on map) for uploads from other devices
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
- [ ] **Waku** — Peer-to-peer transport for upload metadata, hotspot announcements, and real-time notifications
- [ ] **Codex** — Decentralized storage for video files (content-addressed, erasure-coded)
- [ ] **Nomos** (optional) — Timestamping proofs for clip authenticity (if blockchain anchoring desired)

---

### U — Usability

#### Mobile-First Design
- [ ] Single-tap capture: launch → record → upload in <3 taps
- [ ] Offline-capable: capture and queue uploads without connectivity
- [ ] Minimal UI: focus on map + camera, reduce cognitive load
- [ ] Dark mode default (field use, nighttime events)

#### Low-Connectivity Environments
- [ ] Progressive upload: lower resolution first, full quality when bandwidth available
- [ ] Store-and-forward: local storage until connection restored
- [ ] Mesh relay (optional): Waku nodes on local network can relay uploads

#### Accessibility
- [ ] VoiceOver/TalkBack support for core flows
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
- [ ] Redundant storage via Codex erasure coding
- [ ] Optional: blockchain anchoring for timestamping proofs

#### Graceful Degradation
- [ ] If Codex unreachable: local-only mode with periodic retry
- [ ] If Waku unreachable: metadata batched and sent when available
- [ ] Clear status indicators: user knows when offline/syncing/synced

---

### P — Performance

#### Upload Time
- [ ] Target: <10s upload start-to-confirmation for 30s clip on 4G
- [ ] Chunk size tuned for typical mobile bandwidth (512KB–2MB chunks)
- [ ] Parallel chunk upload where beneficial

#### Map Rendering
- [ ] Target: <2s initial load of map with 100 hotspots visible
- [ ] Lazy loading: fetch clip details on demand (tap hotspot)
- [ ] Tile caching for offline map access in previously viewed areas

#### Hotspot Aggregation
- [ ] Real-time updates: new clips reflected in hotspot intensity within <30s
- [ ] Background aggregation: client receives push updates via Waku
- [ ] Efficient spatial indexing (R-tree or geohash-based)

#### Battery & Resource Usage
- [ ] Background location polling: low-power mode (significant location changes only)
- [ ] Video compression before upload (H.265/HEVC where supported)
- [ ] Configurable upload schedule (immediate vs. Wi-Fi-only vs. scheduled)

---

### S — Supportability

#### Configurable Retention
- [ ] User-configurable local storage limits (auto-delete oldest after X GB)
- [ ] Network-level retention policies (Codex node operators define)
- [ ] User can request deletion of their uploads (propagates to Codex nodes honoring policy)

#### Open Formats
- [ ] Video: MP4 (H.264/H.265), WebM (VP9) supported
- [ ] Metadata: JSON-LD or CBOR, schema-documented
- [ ] Export: download original files + metadata bundle

#### No Lock-In
- [ ] Self-hostable backend (Codex + Waku nodes)
- [ ] API-first: documented REST/GraphQL endpoints for third-party clients
- [ ] Open-source client (MIT/Apache 2.0)

#### Observability
- [ ] Structured logging (OpenTelemetry-compatible)
- [ ] Metrics: upload success rate, hotspot detection latency, storage utilization
- [ ] User-facing: upload history, storage used, sync status

---

### + — Hardware & Deployment

#### Mobile App
- [ ] iOS 15+ (Swift/SwiftUI)
- [ ] Android 10+ (Kotlin/Jetpack Compose)
- [ ] Cross-platform option: React Native or Flutter for faster iteration

#### Logos Stack Dependencies
- [ ] **Waku (nwaku or js-waku)** — Messaging layer
  - Content topics for: upload announcements, hotspot updates, notifications
  - Filter/LightPush for mobile (not full relay node)
- [ ] **Codex** — Storage layer
  - Client library for upload/download
  - Discovery via Waku or DHT
- [ ] **Nomos** (optional) — Blockchain layer
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
- [ ] Plausible deniability: Waku relay doesn't reveal origin

---

## MVP Scope

For initial release:

1. **Capture & Upload** — Single-clip recording, upload to Codex via Waku
2. **Map View** — View own uploads and public uploads on map
3. **Basic Hotspot** — Grid-based clustering with simple heat map
4. **Offline Queue** — Capture offline, sync when connected

Out of scope for MVP:
- Collections/lorelines
- Community contributions
- Nomos timestamping
- Advanced search/filter

---

## References

- [LoreLine.Live (logos-co/ideas #7)](https://github.com/logos-co/ideas/issues/7) — Collaborative documentary platform concept
- [Privacy Preserving Location Tracker (logos-co/ideas #3)](https://github.com/logos-co/ideas/issues/3) — Private location sharing
- [Waku Documentation](https://waku.org/about/architect)
- [Codex Storage](https://codex.storage/)
- [Logos Network](https://logos.co)

---

## Status

**Draft** — Awaiting review

*Created: 2026-03-14*
