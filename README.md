# Video Hotspot — FURPS Specification

> **Privacy-preserving video upload and geolocation-based event mapping on the Logos stack**

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
- [ ] Import individual video files via file picker (manual select)
- [ ] Import all videos from a folder (folder picker)
- [ ] Folder monitoring: watch a folder, auto-upload new files added to it
- [ ] Deduplication: compute content hash before upload — never upload the same file twice
- [ ] Upload queue with progress indicators and retry logic
- [ ] Background uploading (continues while browsing map)
- [ ] **No webcam integration** — import existing files only

#### Timestamp & Geolocation Tagging
- [ ] Extract timestamp from video file metadata (EXIF/creation date)
- [ ] If video has EXIF geolocation: use it automatically
- [ ] If no EXIF geolocation: prompt user to pinpoint location on interactive map
- [ ] Store precise coordinates + timestamp — no fuzzy zones or clustering for now
- [ ] **No IP-based geolocation** — never infer location from network data

#### Map Browsing
- [ ] Interactive map showing video pins at their geolocations
- [ ] Timeline slider to filter by time range (scrub to see events over time)
- [ ] Click pin to preview/play video
- [ ] Zoom and pan across regions
- [ ] Basic search by location (center map on searched area)

#### Video Playback & Download
- [ ] Stream videos directly from Logos Storage
- [ ] Download videos for offline viewing
- [ ] Downloaded videos become seedable (user becomes uploader for that content)
- [ ] Track which videos are user-owned vs. cached downloads

#### Logos Stack Integration
- [ ] **Logos Messaging** — Live/real-time indexing as videos are uploaded
  - Publishes metadata (CID, geolocation, timestamp) on upload
  - Subscribers receive new video announcements in real-time
- [ ] **Logos Storage** — Decentralized storage for video files (content-addressed)
- [ ] **Logos Blockchain** — Batch/historical indexing
  - Periodic batches (e.g., 24-hour aggregates) of video metadata committed to blockchain
  - Indexing only — not for proofing or authentication

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
- **Timeline Slider** — Horizontal slider at bottom of screen
  - Drag to filter videos by time range
  - Shows date/time labels at slider position
  - As slider moves, pins appear/disappear based on timestamp
  - "Play" button to animate through time automatically
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
  - Bootstrap nodes configuration
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
- [ ] Clean, minimal interface — map-centric design
- [ ] Dark mode default (field use, nighttime events)
- [ ] Integrates within Basecamp app navigation

#### Language
- [ ] English only (no localization for now)

#### What's NOT Included
- [ ] No notifications (no alerts, no push messages)
- [ ] No content organization (no tags, no collections, no editorial features)
- [ ] No mesh relay (direct p2p only)

---

### R — Reliability

#### Offline Operation
- [ ] Upload queue persists across app restarts
- [ ] Downloaded videos playable offline
- [ ] Local database for pending uploads (SQLite or equivalent)
- [ ] Sync on reconnect: background service handles upload queue

#### Upload Resilience
- [ ] Chunked upload with resumption (no full-file restart on failure)
- [ ] Automatic retry with exponential backoff
- [ ] Corruption detection (checksum verification pre-upload)
- [ ] Dedup check prevents wasted bandwidth on re-uploads

#### Data Integrity
- [ ] Content-addressed storage (CID-based) ensures immutability
- [ ] Hash verification on download

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
- [ ] Dedup check (hash comparison) completes before upload starts

#### Map Rendering
- [ ] Target: <2s initial load of map with 100 pins visible
- [ ] Lazy loading: fetch video details on demand (click pin)
- [ ] Tile caching for offline map access in previously viewed areas
- [ ] Smooth timeline scrubbing (no UI freeze)

#### Resource Usage
- [ ] Video compression before upload (H.265/HEVC where supported)
- [ ] Background process: minimal CPU/memory footprint when idle
- [ ] Configurable storage limits to prevent disk bloat

---

### S — Supportability

#### Platform
- [ ] **Qt miniapp** running inside Basecamp (Logos desktop app)
- [ ] Cross-platform via Qt: Windows, macOS, Linux
- [ ] **Not** a standalone Electron/Tauri application

#### Storage Management
- [ ] User-configurable local storage limits
- [ ] Auto-clean: oldest cached videos deleted when limit reached
- [ ] User-owned videos never auto-deleted
- [ ] Manual deletion controls for both owned and cached content

#### Open Formats
- [ ] Video: MP4 (H.264/H.265), WebM (VP9) supported
- [ ] Metadata: JSON, schema-documented
- [ ] Export: download original files

#### Observability
- [ ] Upload history visible to user
- [ ] Storage used / allocated display
- [ ] Sync status indicators
- [ ] Folder monitor status

---

### + — Hardware & Deployment

#### Basecamp Integration
- [ ] Runs as Qt miniapp inside Basecamp (Logos desktop app)
- [ ] Inherits Basecamp's platform support (Windows 10+, macOS 11+, Linux)
- [ ] Uses Basecamp's Logos stack connections (Messaging, Storage, Blockchain)

#### Out of Scope
- [ ] Mobile platforms (iOS, Android) — explicitly excluded
- [ ] Standalone desktop app — must run inside Basecamp
- [ ] Webcam/camera capture — import existing files only

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
- [ ] No IP-based geolocation (never)
- [ ] Metadata minimization: only timestamp + explicit user-provided geolocation
- [ ] No PII storage
- [ ] Downloaded videos contribute to network (seeding)

---

## MVP Scope

For initial release:

1. **Upload** — File picker, folder picker, folder monitor, dedup, upload queue
2. **Geolocation** — EXIF extraction or manual pin placement
3. **Map View** — Browse pins by location, timeline slider for time filtering
4. **Download & Cache** — Download videos, storage management, auto-clean
5. **Offline Queue** — Capture offline, sync when connected

Out of scope for MVP:
- Collections/lorelines
- Tags or content organization
- Notifications
- Mobile platforms
- Webcam capture
- Multiple languages

---

## References

- [LoreLine.Live (logos-co/ideas #7)](https://github.com/logos-co/ideas/issues/7) — Collaborative documentary platform concept
- [Logos Network](https://logos.co)

---

## Status

**Draft** — Awaiting review

*Created: 2026-03-14*
