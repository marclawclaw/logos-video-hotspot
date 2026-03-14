#pragma once

/**
 * HeadlessCore — initialises all core modules for CLI/headless use.
 *
 * Created once in main() and passed to each command handler.
 * In headless mode there is no Qt UI event loop — commands run
 * synchronously and call app.quit() when done.
 *
 * When the real Logos SDK is available:
 *   - Pass logoscore connection details here
 *   - StorageClient/MessagingClient constructors accept a LogosAPI*
 */

#include <video_hotspot/deduplicator.h>
#include <video_hotspot/indexing_service.h>
#include <video_hotspot/messaging_client.h>
#include <video_hotspot/metadata_extractor.h>
#include <video_hotspot/storage_client.h>
#include <video_hotspot/upload_queue.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QMimeDatabase>
#include <memory>

namespace VideoHotspot {

struct HeadlessCore {
    std::unique_ptr<StorageClient>    storage;
    std::unique_ptr<Deduplicator>     deduplicator;
    std::unique_ptr<MessagingClient>  messaging;
    std::unique_ptr<IndexingService>  indexing;
    std::unique_ptr<MetadataExtractor> metadata;
    std::unique_ptr<UploadQueue>      queue;

    HeadlessCore()
    {
        storage      = std::make_unique<StorageClient>();
        deduplicator = std::make_unique<Deduplicator>();
        messaging    = std::make_unique<MessagingClient>();
        indexing     = std::make_unique<IndexingService>();
        metadata     = std::make_unique<MetadataExtractor>();
        queue        = std::make_unique<UploadQueue>(storage.get(), deduplicator.get());

        indexing->start();
        messaging->subscribe();

        // Wire: successful upload → local index entry.
        // The mock MetadataExtractor has no EXIF GPS, so we use (0,0) as the
        // geo-location placeholder. Replace with real EXIF extraction when
        // the Logos SDK and ffprobe are integrated.
        QObject::connect(queue.get(), &UploadQueue::itemChanged,
                         indexing.get(), [this](const UploadItem& item) {
                             if (item.status != UploadStatus::Complete) return;
                             const QFileInfo fi(item.filePath);
                             const QMimeDatabase mimeDb;
                             const QString mime = mimeDb.mimeTypeForFile(item.filePath).name();
                             indexing->onUploadComplete(
                                 item.cid,
                                 0.0, 0.0,   // placeholder: no EXIF geo in mock
                                 QDateTime::currentDateTimeUtc(),
                                 0,           // duration unknown without ffprobe
                                 fi.size(),
                                 mime
                             );
                         });
    }

    ~HeadlessCore()
    {
        indexing->stop();
        messaging->flushPending();
    }
};

}  // namespace VideoHotspot
