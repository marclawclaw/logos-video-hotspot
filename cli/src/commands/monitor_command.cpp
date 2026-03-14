#include "commands.h"
#include "../headless_core.h"
#include "../output_formatter.h"

#include <QDir>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace VideoHotspot {

int runMonitor(const QStringList& args, bool humanMode)
{
    OutputFormatter fmt(humanMode);

    if (args.isEmpty()) {
        return fmt.error("Usage: video-hotspot monitor <path>");
    }

    const QString dirPath = args.first();
    if (!QDir(dirPath).exists()) {
        return fmt.error("Directory not found: " + dirPath);
    }

    HeadlessCore core;
    core.queue->startMonitoring(dirPath);

    QJsonObject startMsg;
    startMsg["status"]     = "ok";
    startMsg["monitoring"] = dirPath;
    startMsg["mode"]       = "headless";
    fmt.success(startMsg);

    // Connect signals to print events
    QObject::connect(core.queue.get(), &UploadQueue::itemAdded,
                     [humanMode](const UploadItem& item) {
                         if (!humanMode) {
                             QJsonObject obj;
                             obj["event"]  = "item_added";
                             obj["id"]     = item.id;
                             obj["file"]   = item.filePath;
                             obj["status"] = "pending";
                             QTextStream(stdout)
                                 << QJsonDocument(obj).toJson(QJsonDocument::Compact)
                                 << Qt::endl;
                         } else {
                             QTextStream(stdout) << "New file: " << item.filePath << Qt::endl;
                         }
                     });

    QObject::connect(core.queue.get(), &UploadQueue::itemChanged,
                     [&fmt](const UploadItem& item) {
                         if (item.status != UploadStatus::Complete &&
                             item.status != UploadStatus::Duplicate &&
                             item.status != UploadStatus::Failed) return;
                         fmt.uploadResult(item);
                     });

    // Run forever (Ctrl+C to stop)
    return QCoreApplication::exec();
}

}  // namespace VideoHotspot
