# ADR-0004: Messaging Module for Live Indexing

**Status:** Accepted  
**Date:** 2026-03-14  

---

## Context

The FURPS requires real-time video discovery: when a user uploads a video, all connected peers should be able to see it on the map immediately. Logos Messaging (the decentralised transport layer) is the designated mechanism for this.

We need to define the message schema, topic structure, and how our app subscribes/publishes.

---

## Decision

**Use a single Logos Messaging topic per application (`video-hotspot/v1/index`) for all video metadata announcements. Publish on upload complete; subscribe on app start.**

### Message Schema (JSON, published as message payload)

```json
{
  "schema": "video-hotspot/v1",
  "cid": "bafybeig...",
  "timestamp_utc": "2026-03-14T05:00:00Z",
  "lat": -33.8688,
  "lon": 151.2093,
  "duration_s": 42,
  "size_bytes": 18432000,
  "mime": "video/mp4"
}
```

Fields:
- `cid` — Logos Storage content ID (canonical identity, used for dedup)
- `timestamp_utc` — Recording time extracted from EXIF or user-provided
- `lat`, `lon` — Geolocation (explicit user-approved; not device GPS)
- `duration_s`, `size_bytes`, `mime` — Content metadata
- No PII; no uploader identity

### Publish flow

1. Upload completes → `StorageClient` emits `uploadComplete(filePath, cid)`.
2. `IndexingService::onUploadComplete()` constructs the JSON message.
3. `MessagingClient::publish(topic, payload)` sends to the network.
4. On failure: message queued locally (SQLite `pending_messages` table); retried on reconnect.

### Subscribe flow

1. App starts → `MessagingClient::subscribe(topic)`.
2. Incoming messages → `IndexingService::onMessage()` → parsed and inserted into local video index (SQLite `videos` table).
3. Map UI refreshed via Qt signal.

---

## Rationale

- A single topic simplifies peer discovery — any node running Video Hotspot automatically subscribes to the same channel.
- Schema versioning (`video-hotspot/v1`) allows future breaking changes without disrupting existing clients.
- Separating `MessagingClient` (SDK wrapper) from `IndexingService` (business logic) keeps each testable independently.
- The FURPS also requires batch indexing via Logos Blockchain for historical data — that is a separate periodic job and does not affect this real-time path.

---

## Consequences

- All video metadata is public on the messaging layer (privacy-by-design: only explicit user-provided geolocation is shared, no PII).
- `MessagingClient` must implement a local send queue for offline resilience (FURPS Reliability).
- The local SQLite `videos` table is the source of truth for the map — rebuilt from messaging history on first run.

---

## Alternatives Considered

| Option | Reason Rejected |
|---|---|
| One topic per geographic region | Complexity; partition boundaries are arbitrary |
| Blockchain-only indexing | Too slow for real-time map updates |
| Custom DHT/gossip | Logos Messaging already provides this; reinventing it wastes effort |
