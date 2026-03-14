#pragma once

#include <QFuture>
#include <QObject>
#include <QString>

namespace VideoHotspot {

/// Result of a deduplication check.
struct DedupResult {
    bool     isDuplicate  = false;
    QString  existingCid;   ///< Non-empty if isDuplicate == true
    QString  blake3Hash;    ///< Always populated after check completes
};

/**
 * @brief Two-layer file deduplication: local BLAKE3 hash DB + Logos Storage CID check.
 *
 * Call checkFile() before enqueuing any upload. If the result is a duplicate,
 * the upload should be skipped and the UI should show the "Already uploaded" badge.
 *
 * Layer 1: BLAKE3 hash computed locally, compared against local SQLite DB.
 * Layer 2: If hash not found locally, check CID existence on Logos Storage network.
 *
 * @see ADR-0005
 */
class Deduplicator : public QObject {
    Q_OBJECT

public:
    explicit Deduplicator(QObject* parent = nullptr);
    ~Deduplicator() override;

    /**
     * Run the full dedup check for a local file.
     * This is the primary entry point — call before any upload.
     *
     * @param filePath  Absolute path to the video file.
     * @return          Future resolving to a DedupResult.
     */
    QFuture<DedupResult> checkFile(const QString& filePath);

    /**
     * Compute the BLAKE3 hash of a file without a full dedup check.
     * Runs in a background thread.
     */
    QFuture<QString> computeHash(const QString& filePath);

    /**
     * Record a completed upload so future dedup checks find it locally.
     * Called by UploadQueue after upload success.
     */
    void recordUpload(const QString& filePath, const QString& blake3Hash, const QString& cid);

signals:
    /// Emitted when a duplicate is detected, before the future resolves.
    void duplicateDetected(const QString& filePath, const QString& existingCid);

    /// Emitted during hash computation to show spinner progress.
    void hashProgress(const QString& filePath, qint64 bytesHashed, qint64 totalBytes);
};

}  // namespace VideoHotspot
