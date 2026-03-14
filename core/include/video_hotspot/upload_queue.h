#pragma once

#include "deduplicator.h"
#include "storage_client.h"

#include <QList>
#include <QObject>
#include <QString>

namespace VideoHotspot {

/// Status of a single item in the upload queue.
enum class UploadStatus {
    Pending,     ///< Waiting for dedup check or a free upload slot
    Hashing,     ///< Computing BLAKE3 hash
    Uploading,   ///< Transfer in progress
    Processing,  ///< Server-side processing (post-upload)
    Complete,    ///< Successfully uploaded
    Failed,      ///< Upload failed (see error message)
    Duplicate,   ///< Skipped — file already uploaded (existingCid is set)
    AwaitingGeo, ///< No EXIF location; waiting for user to pin on map
};

/// A single entry in the upload queue.
struct UploadItem {
    QString      id;           ///< Unique item ID (UUID)
    QString      filePath;
    UploadStatus status       = UploadStatus::Pending;
    int          progressPct  = 0;
    QString      cid;          ///< Set on Complete or Duplicate
    QString      existingCid;  ///< Set on Duplicate
    QString      errorMsg;     ///< Set on Failed
};

/**
 * @brief Manages the ordered queue of video uploads.
 *
 * Coordinates dedup checks, geolocation prompts, and actual uploads.
 * The queue is persisted to SQLite so it survives app restarts.
 *
 * Upload slots are limited to avoid saturating the network; parallel
 * chunk uploading within a single file is handled by StorageClient.
 *
 * @see ADR-0002 (headless mode), ADR-0003 (storage), ADR-0005 (dedup)
 */
class UploadQueue : public QObject {
    Q_OBJECT

public:
    explicit UploadQueue(StorageClient* storage,
                         Deduplicator*  deduplicator,
                         QObject*       parent = nullptr);
    ~UploadQueue() override;

    /// Add a single file to the queue. Returns the new item ID.
    QString enqueue(const QString& filePath);

    /// Add all video files found in a directory (non-recursive by default).
    QStringList enqueueFolder(const QString& dirPath, bool recursive = false);

    /// Start folder monitoring: auto-enqueue new files dropped into dirPath.
    void startMonitoring(const QString& dirPath);
    void stopMonitoring();
    bool isMonitoring() const;
    QString monitoredPath() const;

    /// Retry a failed item.
    void retry(const QString& itemId);

    /// Cancel and remove a pending item.
    void cancel(const QString& itemId);

    /// Current snapshot of the queue.
    QList<UploadItem> items() const;

    /// Provide geolocation for an AwaitingGeo item; this unblocks its upload.
    void provideGeolocation(const QString& itemId, double lat, double lon);

signals:
    void itemAdded(const UploadItem& item);
    void itemChanged(const UploadItem& item);
    void itemRemoved(const QString& itemId);
    void allComplete();
};

}  // namespace VideoHotspot
