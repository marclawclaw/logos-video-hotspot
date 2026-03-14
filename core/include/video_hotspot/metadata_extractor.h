#pragma once

#include <QDateTime>
#include <QFuture>
#include <QObject>
#include <QString>
#include <optional>

namespace VideoHotspot {

/// Geolocation data extracted from or provided for a video file.
struct GeoLocation {
    double lat = 0.0;
    double lon = 0.0;
};

/// All metadata we care about for a video file.
struct VideoMetadata {
    QDateTime               recordedAt;    ///< Extracted from EXIF; null if unavailable
    std::optional<GeoLocation> location;  ///< nullopt if no EXIF geolocation
    qint64                  durationSeconds = 0;
    qint64                  sizeBytes       = 0;
    QString                 mimeType;      ///< e.g. "video/mp4"
    int                     width          = 0;
    int                     height         = 0;
    QString                 codec;         ///< e.g. "h264"
};

/**
 * @brief Extracts timestamp, geolocation, and video properties from a file.
 *
 * Uses Qt Multimedia and/or libavformat (ffprobe) to read EXIF/MP4 container
 * metadata. All extraction runs in a background thread.
 *
 * If location is nullopt after extraction, the upload queue will transition
 * the item to AwaitingGeo status so the user can pin the location on the map.
 */
class MetadataExtractor : public QObject {
    Q_OBJECT

public:
    explicit MetadataExtractor(QObject* parent = nullptr);
    ~MetadataExtractor() override;

    /**
     * Extract metadata from a local video file.
     * Runs in QThreadPool; safe to call from main thread.
     */
    QFuture<VideoMetadata> extract(const QString& filePath);

    /**
     * Generate a thumbnail image for a video at the given timestamp offset.
     * @param filePath      Source video.
     * @param offsetSeconds Seek to this position (default: 1s in).
     * @param maxWidth      Maximum thumbnail width in pixels.
     * @return              Future resolving to absolute path of the PNG thumbnail.
     */
    QFuture<QString> thumbnail(const QString& filePath,
                               int offsetSeconds = 1,
                               int maxWidth      = 320);
};

}  // namespace VideoHotspot
