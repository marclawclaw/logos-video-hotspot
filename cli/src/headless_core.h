#pragma once

/**
 * HeadlessCore — initialises all core modules for CLI/headless use.
 *
 * Created once in main() and passed to each command handler.
 * In headless mode there is no Qt UI event loop — commands run
 * synchronously and call app.quit() when done.
 *
 * When built with LOGOS_CORE_AVAILABLE (ADR-0002):
 *   - Initialises Logos Core via logos_core_init/set_mode/start from logos-liblogos
 *   - Creates a LogosAPI instance and calls initLogos() on StorageClient and
 *     MessagingClient so they use real Codex storage and Waku messaging
 *
 * When LOGOS_CORE_AVAILABLE is not defined:
 *   - All services fall back to local filesystem/SQLite mocks
 *
 * Real SDK API (logos-liblogos logos_core.h):
 *   void logos_core_init(int argc, char* argv[])
 *   void logos_core_set_mode(int mode)   // LOGOS_MODE_REMOTE=0, LOGOS_MODE_LOCAL=1
 *   void logos_core_start()
 *   void logos_core_cleanup()
 *
 * Real SDK API (logos-cpp-sdk logos_api.h):
 *   LogosAPI(const QString& module_name, QObject* parent)
 */

#include <video_hotspot/deduplicator.h>
#include <video_hotspot/indexing_service.h>
#include <video_hotspot/messaging_client.h>
#include <video_hotspot/metadata_extractor.h>
#include <video_hotspot/storage_client.h>
#include <video_hotspot/upload_queue.h>

#ifdef LOGOS_CORE_AVAILABLE
#include <logos_api.h>      // LogosAPI class — logos-cpp-sdk
#include <logos_core.h>     // logos_core_init/start/cleanup C API — logos-liblogos
#endif

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
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

#ifdef LOGOS_CORE_AVAILABLE
    LogosAPI* m_logosAPI = nullptr;
#endif

    HeadlessCore()
    {
        storage      = std::make_unique<StorageClient>();
        deduplicator = std::make_unique<Deduplicator>();
        messaging    = std::make_unique<MessagingClient>();
        indexing     = std::make_unique<IndexingService>();
        metadata     = std::make_unique<MetadataExtractor>();
        queue        = std::make_unique<UploadQueue>(storage.get(), deduplicator.get());

#ifdef LOGOS_CORE_AVAILABLE
        // Initialise Logos Core in headless/remote mode per ADR-0002.
        //
        // LOGOS_MODE_REMOTE (0): connects to a running Logos daemon process
        // via Qt Remote Objects. The daemon must already be running on the
        // local machine (started via `logoscore` or system service).
        //
        // logos_core_init requires argc/argv — pass dummy values for CLI headless use.
        int dummyArgc = 0;
        logos_core_init(dummyArgc, nullptr);
        logos_core_set_mode(LOGOS_MODE_REMOTE);
        logos_core_start();

        m_logosAPI = new LogosAPI(QStringLiteral("video-hotspot"));
        if (m_logosAPI) {
            qDebug() << "HeadlessCore: Logos SDK initialised in headless/remote mode";
            storage->initLogos(m_logosAPI);
            messaging->initLogos(m_logosAPI);
        } else {
            qWarning() << "HeadlessCore: LogosAPI construction failed"
                        << "— falling back to local mock";
        }
#else
        qDebug() << "HeadlessCore: built without LOGOS_CORE_AVAILABLE — using local mock";
#endif

        indexing->start();
        messaging->subscribe();

        // Wire: successful upload → local index entry.
        QObject::connect(queue.get(), &UploadQueue::itemChanged,
                         indexing.get(), [this](const UploadItem& item) {
                             if (item.status != UploadStatus::Complete) return;
                             const QFileInfo fi(item.filePath);
                             const QMimeDatabase mimeDb;
                             const QString mime = mimeDb.mimeTypeForFile(item.filePath).name();
                             indexing->onUploadComplete(
                                 item.cid,
                                 0.0, 0.0,   // placeholder: no EXIF geo yet
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
#ifdef LOGOS_CORE_AVAILABLE
        delete m_logosAPI;
        m_logosAPI = nullptr;
        logos_core_cleanup();
#endif
    }
};

}  // namespace VideoHotspot
