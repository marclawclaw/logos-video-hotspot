#pragma once

#include <video_hotspot/indexing_service.h>
#include <video_hotspot/storage_client.h>
#include <video_hotspot/upload_queue.h>
#include <video_hotspot/messaging_client.h>

#include <QJsonObject>
#include <QTextStream>

namespace VideoHotspot {

/**
 * @brief Formats CLI output as JSON (default) or human-readable text.
 *
 * JSON mode: writes one JSON object per line to stdout.
 * Human mode (--human): writes formatted text to stdout.
 *
 * Error output always goes to stderr in both modes.
 */
class OutputFormatter {
public:
    explicit OutputFormatter(bool humanMode = false);

    // ── Generic output ───────────────────────────────────────────────────────

    /// Print a success result with an arbitrary JSON object.
    void success(const QJsonObject& obj) const;

    /// Print a plain success message (human mode: text, JSON mode: {status:"ok", message:...})
    void successMessage(const QString& message) const;

    /// Print an error and return the exit code (always 1).
    int error(const QString& message, int exitCode = 1) const;

    // ── Domain-specific output ───────────────────────────────────────────────

    /// Print a list of VideoRecords.
    void videoList(const QList<VideoRecord>& records) const;

    /// Print a single VideoRecord.
    void video(const VideoRecord& record) const;

    /// Print upload result.
    void uploadResult(const UploadItem& item) const;

    /// Print storage stats.
    void storageStats(const StorageStats& stats) const;

    /// Print cache entries.
    void cacheEntries(const QList<CacheEntry>& entries) const;

    /// Print status output.
    void statusOutput(bool connected, const StorageStats& stats,
                      int indexCount, int pendingMessages) const;

private:
    bool m_human;

    static QJsonObject videoToJson(const VideoRecord& r);
    static QJsonObject uploadItemToJson(const UploadItem& item);
};

}  // namespace VideoHotspot
