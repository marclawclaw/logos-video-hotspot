#include "commands.h"
#include "../headless_core.h"
#include "../output_formatter.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
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

static bool isTerminal(UploadStatus s) {
    return s == UploadStatus::Complete
        || s == UploadStatus::Failed
        || s == UploadStatus::Duplicate;
}

static UploadItem waitForCompletion(UploadQueue* queue, const QString& itemId)
{
    QEventLoop loop;
    UploadItem result;
    bool done = false;

    // Connect BEFORE checking current state to avoid TOCTOU: if the item
    // completes between checking state and connecting the signal, we'd wait
    // forever. By connecting first we're guaranteed to catch the transition.
    auto connection = QObject::connect(queue, &UploadQueue::itemChanged,
                     &loop, [&](const UploadItem& item) {
                         if (done || item.id != itemId) return;
                         if (isTerminal(item.status)) {
                             done = true;
                             result = item;
                             loop.quit();
                         }
                     });

    // Safety timeout: 60 s
    QTimer::singleShot(60000, &loop, [&loop, &result, &done, itemId]() {
        if (!done) {
            done = true;
            result.id = itemId;
            result.status = UploadStatus::Failed;
            result.errorMsg = "Upload timeout";
            loop.quit();
        }
    });

    // Fallback poll: every 50 ms re-check the DB in case the itemChanged
    // signal was emitted while another event loop level was active and the
    // slot was connected to a different loop context, causing the signal to
    // be processed but loop.quit() called on the wrong loop instance.
    QTimer pollTimer;
    pollTimer.setInterval(50);
    QObject::connect(&pollTimer, &QTimer::timeout, &loop, [&]() {
        if (done) { pollTimer.stop(); return; }
        for (const auto& item : queue->items()) {
            if (item.id == itemId && isTerminal(item.status)) {
                done = true;
                result = item;
                pollTimer.stop();
                loop.quit();
                return;
            }
        }
    });
    pollTimer.start();

    // Process any events already queued (e.g. finished signals from
    // concurrent uploads that completed before this call started).
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // Check current state AFTER processing pending events to catch
    // already-complete items.
    if (!done) {
        for (const auto& item : queue->items()) {
            if (item.id == itemId && isTerminal(item.status)) {
                done = true;
                result = item;
                break;
            }
        }
    }

    if (!done) {
        loop.exec();
    }

    pollTimer.stop();
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

// Enumerate video files in a directory (same filters as UploadQueue).
static QStringList findVideoFiles(const QString& dirPath)
{
    static const QStringList kVideoFilters = {
        "*.mp4", "*.mov", "*.avi", "*.mkv", "*.webm",
        "*.m4v", "*.flv", "*.wmv", "*.ts", "*.mpg", "*.mpeg"
    };
    QStringList paths;
    QDirIterator it(dirPath, kVideoFilters,
                    QDir::Files | QDir::Readable,
                    QDirIterator::NoIteratorFlags);
    while (it.hasNext())
        paths.append(it.next());
    return paths;
}

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

    const QStringList filePaths = findVideoFiles(dirPath);
    if (filePaths.isEmpty()) {
        fmt.successMessage("No video files found in " + dirPath);
        return 0;
    }

    HeadlessCore core;

    QTextStream out(stdout);
    if (humanMode) {
        out << "Queued " << filePaths.size() << " file(s)...\n";
    }

    int succeeded = 0;
    int failed    = 0;

    // Process files one at a time (sequential) to avoid concurrent event-loop
    // races where itemChanged signals fired during one file's waitForCompletion
    // loop are lost for subsequent files.
    for (const QString& fp : filePaths) {
        const QString id = core.queue->enqueue(fp);
        if (id.isEmpty()) {
            ++failed;
            continue;
        }
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
        summary["total"]     = filePaths.size();
        summary["succeeded"] = succeeded;
        summary["failed"]    = failed;
        QTextStream(stdout) << QJsonDocument(summary).toJson(QJsonDocument::Compact) << Qt::endl;
    } else {
        out << "\nSummary: " << succeeded << " uploaded, " << failed << " failed\n";
    }

    return failed == 0 ? 0 : 1;
}

}  // namespace VideoHotspot
