# ADR-0005: Deduplication Strategy

**Status:** Accepted  
**Date:** 2026-03-14  

---

## Context

The FURPS requires that the same video file is never uploaded twice. This is both a bandwidth optimisation and a data quality requirement (no duplicate pins on the map). We need to define when and how deduplication checks occur.

---

## Decision

**Two-layer deduplication: local hash check before upload, then CID existence check against Logos Storage.**

### Layer 1 — Local content hash (pre-upload)

- Hash algorithm: **BLAKE3** (fast, parallelisable, suitable for large files)
- Computed in a background thread (`QThreadPool`) before any upload begins
- Hash stored in SQLite `uploaded_files` table: `(local_path, blake3_hash, cid, upload_date)`
- If the hash already exists in the local DB → skip upload immediately, show "Already uploaded" badge with link to existing CID

### Layer 2 — CID existence check (network)

- Logos Storage is content-addressed; the CID is derived from the file content
- After local hash lookup passes (hash not seen before), call `StorageClient::exists(cid)`
- If CID exists on the network (uploaded by another user) → still skip upload; index the CID in the local video DB
- This catches cross-user deduplication without the local DB having seen the file

### Dedup flow

```
file selected
    ↓
compute BLAKE3 hash (background thread)
    ↓
query local DB for hash → found? → show "Already uploaded" badge, link to CID → STOP
    ↓
derive expected CID from hash (or upload chunk-0 to get CID)
    ↓
StorageClient::exists(cid) → true? → index it, show badge → STOP
    ↓
proceed with upload
```

---

## Rationale

- **BLAKE3** over SHA-256: ~3× faster on large files; single-pass parallelism; native support in C++ via the official BLAKE3 C implementation.
- Local DB check first: avoids a network round-trip for the common case (user re-selects an already-uploaded file).
- CID-based network check: leverages the storage layer's content-addressing for cross-user dedup at no extra cost.
- Hash stored separately from CID: allows us to do the local check without knowing the CID scheme ahead of time.

---

## Consequences

- `core/` depends on BLAKE3 (added as CMake FetchContent or vendored as a submodule).
- `Deduplicator` class owns the hash computation, DB lookup, and network CID check — called by `UploadQueue` before each upload begins.
- UI receives `DuplicateDetected(filePath, existingCid)` signal from `Deduplicator` to show the badge.
- Hash computation adds latency before upload starts; progress is shown in the upload queue entry (spinner during hashing).

---

## Alternatives Considered

| Option | Reason Rejected |
|---|---|
| SHA-256 | Slower on large files; no meaningful security advantage for dedup |
| Filename-based dedup | Unreliable (renames, re-exports) |
| No local DB, network-only check | Too slow for large queues; requires network connectivity |
| MD5 | Collision-prone; unacceptable for content integrity |
