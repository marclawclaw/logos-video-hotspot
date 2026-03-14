/**
 * MetadataExtractor — mock implementation using file stats.
 *
 * Mock strategy (no libavformat/ffprobe):
 *   - recordedAt: from file's last-modified time
 *   - location: always nullopt (no EXIF parser in mock)
 *   - sizeBytes: from QFileInfo
 *   - mimeType: guessed from file extension
 *   - durationSeconds: 0 (no media parser in mock)
 *   - width/height/codec: 0/0/"" (no media parser in mock)
 *   - thumbnail: generates a 1x1 dummy PNG
 *
 * When ffprobe or Qt Multimedia is available:
 *   - Replace mock with QProcess("ffprobe -v quiet -print_format json -show_streams -show_format")
 *   - Parse JSON output for all metadata fields
 *   - Read EXIF GPS data for location
 */

#include <video_hotspot/metadata_extractor.h>

#include <QFileInfo>
#include <QMimeDatabase>
#include <QtConcurrent/QtConcurrent>

namespace VideoHotspot {

MetadataExtractor::MetadataExtractor(QObject* parent)
    : QObject(parent)
{
}

MetadataExtractor::~MetadataExtractor() = default;

QFuture<VideoMetadata> MetadataExtractor::extract(const QString& filePath)
{
    return QtConcurrent::run([filePath]() -> VideoMetadata {
        VideoMetadata meta;

        QFileInfo fi(filePath);
        if (!fi.exists()) return meta;

        meta.sizeBytes  = fi.size();
        meta.recordedAt = fi.lastModified().toUTC();

        // MIME type from extension
        QMimeDatabase mimeDb;
        const QMimeType mime = mimeDb.mimeTypeForFile(filePath);
        meta.mimeType = mime.name();

        // Mock values for media properties
        meta.durationSeconds = 0;  // TODO: ffprobe
        meta.width           = 0;  // TODO: ffprobe
        meta.height          = 0;  // TODO: ffprobe
        meta.codec           = {};  // TODO: ffprobe

        // location is nullopt — no EXIF parser in mock
        // In real implementation: parse MP4/EXIF GPS atoms

        return meta;
    });
}

QFuture<QString> MetadataExtractor::thumbnail(const QString& filePath,
                                               int offsetSeconds,
                                               int maxWidth)
{
    return QtConcurrent::run([filePath, offsetSeconds, maxWidth]() -> QString {
        Q_UNUSED(offsetSeconds)
        Q_UNUSED(maxWidth)

        // Mock: generate a tiny valid PNG file as placeholder
        // 1x1 black pixel PNG
        static const QByteArray kMinimalPng = QByteArray::fromBase64(
            "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9Q"
            "DwADhgGAWjR9awAAAABJRU5ErkJggg==");

        const QString dataDir = QStandardPaths::writableLocation(
            QStandardPaths::AppLocalDataLocation);
        QDir().mkpath(dataDir + "/thumbs");

        QFileInfo fi(filePath);
        const QString thumbPath = dataDir + "/thumbs/" + fi.completeBaseName() + "_thumb.png";

        QFile f(thumbPath);
        if (!f.exists()) {
            f.open(QIODevice::WriteOnly);
            f.write(kMinimalPng);
        }
        return thumbPath;
    });
}

}  // namespace VideoHotspot
