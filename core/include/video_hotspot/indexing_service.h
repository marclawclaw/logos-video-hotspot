#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

namespace VideoHotspot {

/// A single indexed video entry (stored locally + announced via Messaging).
struct VideoRecord {
    QString   cid;
    double    lat          = 0.0;
    double    lon          = 0.0;
    QDateTime recordedAt;         ///< Extracted from EXIF or user-provided
    QDateTime indexedAt;          ///< When we first saw this record
    qint64    durationSeconds = 0;
    qint64    sizeBytes       = 0;
    QString   mimeType;
    bool      userOwned   = false; ///< true = uploaded by this user
    bool      downloaded  = false; ///< true = locally cached
};

/// Filter for map queries.
struct VideoFilter {
    double    latMin    =  -90.0;
    double    latMax    =   90.0;
    double    lonMin    = -180.0;
    double    lonMax    =  180.0;
    QDateTime fromTime;
    QDateTime toTime;
    int       limit     = 500;    ///< 0 = unlimited
};

/**
 * @brief Manages the local video index and live pub/sub via Logos Messaging.
 *
 * On start: subscribes to the `video-hotspot/v1/index` messaging topic.
 * On upload complete: publishes a VideoRecord as a JSON message.
 * On incoming message: parses and inserts into the local SQLite `videos` table.
 *
 * Batch blockchain indexing (24h aggregates) is a separate periodic job
 * triggered internally by this service.
 *
 * @see ADR-0004
 */
class IndexingService : public QObject {
    Q_OBJECT

public:
    explicit IndexingService(QObject* parent = nullptr);
    ~IndexingService() override;

    /// Start subscribing to the messaging topic. Call on app launch.
    void start();
    void stop();

    /// Query the local index for map display.
    QList<VideoRecord> query(const VideoFilter& filter = {}) const;

    /// Lookup a single record by CID.
    VideoRecord findByCid(const QString& cid) const;

    /// Total number of records in the local index.
    int count() const;

public slots:
    /// Called by UploadQueue when an upload succeeds and geolocation is available.
    void onUploadComplete(const QString& cid, double lat, double lon,
                          const QDateTime& recordedAt, qint64 durationSeconds,
                          qint64 sizeBytes, const QString& mimeType);

signals:
    /// Emitted when a new record arrives (from messaging or local upload).
    void recordAdded(const VideoRecord& record);

    /// Emitted when the local index has finished bootstrapping from history.
    void indexReady();
};

}  // namespace VideoHotspot
