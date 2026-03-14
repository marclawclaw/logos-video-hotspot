#pragma once

#include <QFuture>
#include <QObject>
#include <QString>
#include <QStringList>

namespace VideoHotspot {

/// Stats about local storage usage.
struct StorageStats {
    qint64 allocatedBytes  = 0;  ///< User-configured limit
    qint64 userOwnedBytes  = 0;  ///< Bytes from user-uploaded videos (never auto-deleted)
    qint64 cachedBytes     = 0;  ///< Bytes from downloaded-from-others videos
    qint64 totalUsedBytes() const { return userOwnedBytes + cachedBytes; }
};

/// Options for upload operations.
struct UploadOptions {
    qint64  chunkSizeBytes = 1 * 1024 * 1024;  ///< Default: 1 MB chunks
    int     maxRetries     = 5;
    bool    compressFirst  = true;              ///< Attempt H.265 transcoding before upload
};

/// Record of a local cached or user-owned video.
struct CacheEntry {
    QString cid;
    QString localPath;
    qint64  sizeBytes  = 0;
    bool    userOwned  = false;   ///< true = uploaded by this user; never auto-evicted
};

/**
 * @brief Async interface to Logos Storage for video upload, download, and cache management.
 *
 * All async methods return QFuture<T>; use QFutureWatcher to receive results on the
 * main thread. The implementation wraps logos-cpp-sdk storage calls and manages
 * chunked transfers, retry logic, and the local SQLite cache registry.
 *
 * @see ADR-0003
 */
class StorageClient : public QObject {
    Q_OBJECT

public:
    explicit StorageClient(QObject* parent = nullptr);
    ~StorageClient() override;

    // -------------------------------------------------------------------------
    // Upload
    // -------------------------------------------------------------------------

    /**
     * Upload a local video file to Logos Storage.
     * @param filePath  Absolute path to the video file.
     * @param opts      Upload options (chunk size, retries, compression).
     * @return          Future resolving to the Content ID (CID) on success,
     *                  or an empty string on failure (check uploadFailed signal).
     */
    QFuture<QString> upload(const QString& filePath, const UploadOptions& opts = {});

    // -------------------------------------------------------------------------
    // Download
    // -------------------------------------------------------------------------

    /**
     * Download a video by CID to a local directory.
     * @param cid       Content ID of the video.
     * @param destDir   Directory to write the downloaded file into.
     * @return          Future resolving to the absolute local file path on success.
     */
    QFuture<QString> download(const QString& cid, const QString& destDir);

    // -------------------------------------------------------------------------
    // Existence / dedup
    // -------------------------------------------------------------------------

    /**
     * Check whether a CID is already stored on the Logos Storage network.
     * Used as the second layer of deduplication (after local hash check).
     */
    QFuture<bool> exists(const QString& cid);

    // -------------------------------------------------------------------------
    // Cache management
    // -------------------------------------------------------------------------

    /**
     * Evict a cached (non-user-owned) video from local storage.
     * Calling this on a user-owned entry is a no-op (enforced internally).
     */
    QFuture<void> evict(const QString& cid);

    /**
     * Evict oldest cached videos until total usage is below @p targetBytes.
     * User-owned videos are never touched.
     */
    QFuture<void> autoEvict(qint64 targetBytes);

    /**
     * Return the list of all locally cached entries.
     */
    QList<CacheEntry> cachedEntries() const;

    /**
     * Return current storage statistics.
     */
    StorageStats stats() const;

    /**
     * Set the user-configured storage allocation limit (bytes).
     * Triggers autoEvict if current usage exceeds the new limit.
     */
    void setStorageLimit(qint64 bytes);

signals:
    void uploadProgress(const QString& filePath, qint64 bytesUploaded, qint64 totalBytes);
    void downloadProgress(const QString& cid, qint64 bytesReceived, qint64 totalBytes);
    void uploadComplete(const QString& filePath, const QString& cid);
    void uploadFailed(const QString& filePath, const QString& error);
    void downloadComplete(const QString& cid, const QString& localPath);
    void downloadFailed(const QString& cid, const QString& error);
    void storageStatsChanged(const StorageStats& stats);
};

}  // namespace VideoHotspot
