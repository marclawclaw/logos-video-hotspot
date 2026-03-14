#include "commands.h"
#include "../headless_core.h"
#include "../output_formatter.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QTimer>

namespace VideoHotspot {

// ── Shared helper: wait for an item to reach a terminal status ───────────────

static UploadItem waitForCompletion(UploadQueue* queue, const QString& itemId)
{
    QEventLoop loop;
    UploadItem result;

    // Connect BEFORE checking current state to avoid TOCTOU: if the item
    // completes between checking state and connecting the signal, we'd wait
    // forever. By connecting first we're guaranteed to catch the transition.
    auto connection = QObject::connect(queue, &UploadQueue::itemChanged,
                     &loop, [&](const UploadItem& item) {
                         if (item.id != itemId) return;
                         switch (item.status) {
                         case UploadStatus::Complete:
                         case UploadStatus::Failed:
                         case UploadStatus::Duplicate:
                             result = item;
                             loop.quit();
                             break;
                         default:
                             break;
                         }
                     });

    // Safety timeout: 60 s
    QTimer::singleShot(60000, &loop, [&loop, &result, itemId]() {
        result.id = itemId;
        result.status = UploadStatus::Failed;
        result.errorMsg = "Upload timeout";
        loop.quit();
    });

    // Check current state AFTER connecting to catch already-complete items
    // (the signal may have fired while we were setting up the event loop).
    for (const auto& item : queue->items()) {
        if (item.id == itemId) {
            switch (item.status) {
            case UploadStatus::Complete:
            case UploadStatus::Failed:
            case UploadStatus::Duplicate:
                QObject::disconnect(connection);
                return item;
            default:
                break;
            }
            break;
        }
    }

    loop.exec();
    QObject::disconnect(connection);
    return result;
}

// ── upload command ────────────────────────────────────────────────────────────

int runUpload(const QStringList& args, bool humanMode)
{
    OutputFormatter fmt(humanMode);

    if (args.isEmpty()) {
        return fmt.error("Usage: video-hotspot upload <file>");
    }

    const QString filePath = args.first();
    if (!QFile::exists(filePath)) {
        return fmt.error("File not found: " + filePath);
    }

    HeadlessCore core;

    const QString itemId = core.queue->enqueue(filePath);
    if (itemId.isEmpty()) {
        return fmt.error("Failed to enqueue: " + filePath);
    }

    const UploadItem item = waitForCompletion(core.queue.get(), itemId);
    fmt.uploadResult(item);

    return (item.status == UploadStatus::Complete ||
            item.status == UploadStatus::Duplicate) ? 0 : 1;
}

// ── upload-folder command ─────────────────────────────────────────────────────

int runUploadFolder(const QStringList& args, bool humanMode)
{
    OutputFormatter fmt(humanMode);

    if (args.isEmpty()) {
        return fmt.error("Usage: video-hotspot upload-folder <path>");
    }

    const QString dirPath = args.first();
    if (!QDir(dirPath).exists()) {
        return fmt.error("Directory not found: " + dirPath);
    }

    HeadlessCore core;

    const QStringList ids = core.queue->enqueueFolder(dirPath);
    if (ids.isEmpty()) {
        fmt.successMessage("No video files found in " + dirPath);
        return 0;
    }

    QTextStream out(stdout);
    if (humanMode) {
        out << "Queued " << ids.size() << " file(s)...\n";
    }

    int succeeded = 0;
    int failed    = 0;

    for (const QString& id : ids) {
        const UploadItem item = waitForCompletion(core.queue.get(), id);
        fmt.uploadResult(item);
        if (item.status == UploadStatus::Complete ||
            item.status == UploadStatus::Duplicate) {
            ++succeeded;
        } else {
            ++failed;
        }
    }

    if (!humanMode) {
        // Print summary
        QJsonObject summary;
        summary["status"]    = failed == 0 ? "ok" : "partial";
        summary["total"]     = ids.size();
        summary["succeeded"] = succeeded;
        summary["failed"]    = failed;
        QTextStream(stdout) << QJsonDocument(summary).toJson(QJsonDocument::Compact) << Qt::endl;
    } else {
        out << "\nSummary: " << succeeded << " uploaded, " << failed << " failed\n";
    }

    return failed == 0 ? 0 : 1;
}

}  // namespace VideoHotspot
