# ADR-0003: Storage Module API Design

**Status:** Accepted  
**Date:** 2026-03-14  

---

## Context

Video Hotspot uses Logos Storage for decentralised, content-addressed video storage. We need to define how our `core/` module interacts with the storage system: upload, download, seeding, and local cache management.

---

## Decision

**Wrap the logos-cpp-sdk storage calls behind a `StorageClient` interface in `core/include/video_hotspot/storage_client.h`.**

Key design choices:

1. **Async-first**: All operations return `QFuture<T>` (Qt Concurrent) — never block the calling thread.
2. **CID-based addressing**: Content IDs are the canonical identity for every video. The `StorageClient` returns a `QString cid` on upload.
3. **Chunked upload with resumption**: `StorageClient::upload()` splits files into configurable chunks (default 1MB); progress is reported via `QFutureWatcher` signals.
4. **Local cache registry**: A SQLite table (`cache` schema) tracks downloaded CIDs, their local paths, sizes, and whether they are user-owned.

---

## Interface Summary

```cpp
class StorageClient : public QObject {
    Q_OBJECT
public:
    // Upload a local file; returns CID on success
    QFuture<QString> upload(const QString& filePath, const UploadOptions& opts = {});

    // Download a CID to a local path; returns local file path
    QFuture<QString> download(const QString& cid, const QString& destDir);

    // Check if a CID is already stored (dedup check)
    QFuture<bool> exists(const QString& cid);

    // Delete local cache entry (non-user-owned only; enforced internally)
    QFuture<void> evict(const QString& cid);

    // Total and per-category storage usage
    StorageStats stats() const;

signals:
    void uploadProgress(const QString& filePath, qint64 bytesUploaded, qint64 totalBytes);
    void downloadProgress(const QString& cid, qint64 bytesReceived, qint64 totalBytes);
    void uploadComplete(const QString& filePath, const QString& cid);
    void uploadFailed(const QString& filePath, const QString& error);
};
```

---

## Rationale

- `QFuture<T>` integrates cleanly with the Qt event loop; callers use `QFutureWatcher` to receive results on the main thread without manual thread management.
- Wrapping the SDK behind an interface makes `StorageClient` mockable for unit tests — the upload queue, dedup logic, and cache manager can be tested without a live Logos node.
- SQLite (via Qt's `QSqlDatabase`) is used for the cache registry to satisfy the FURPS Reliability requirement (upload queue persists across app restarts).

---

## Consequences

- `StorageClient` owns the chunk size, retry policy, and exponential backoff logic.
- The UI layer never calls logos-cpp-sdk storage APIs directly — always through `StorageClient`.
- Cache eviction policy (oldest-first, never user-owned) is enforced inside `StorageClient::evict()`.

---

## Alternatives Considered

| Option | Reason Rejected |
|---|---|
| Direct SDK calls from UI | Couples UI to SDK; not testable |
| HTTP REST proxy to storage | Adds latency and deployment complexity |
| Custom async runtime (coroutines) | Qt Concurrent already provides this cleanly |
