#include "output_formatter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>

namespace VideoHotspot {

OutputFormatter::OutputFormatter(bool humanMode)
    : m_human(humanMode)
{}

// ── Helpers ───────────────────────────────────────────────────────────────────

static void printJson(const QJsonObject& obj)
{
    QTextStream out(stdout);
    out << QJsonDocument(obj).toJson(QJsonDocument::Compact) << Qt::endl;
}

static void printJsonArray(const QJsonArray& arr)
{
    QTextStream out(stdout);
    out << QJsonDocument(arr).toJson(QJsonDocument::Compact) << Qt::endl;
}

// ── Generic ───────────────────────────────────────────────────────────────────

void OutputFormatter::success(const QJsonObject& obj) const
{
    if (m_human) {
        QTextStream out(stdout);
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            out << it.key() << ": " << it.value().toVariant().toString() << Qt::endl;
        }
    } else {
        printJson(obj);
    }
}

void OutputFormatter::successMessage(const QString& message) const
{
    if (m_human) {
        QTextStream out(stdout);
        out << message << Qt::endl;
    } else {
        QJsonObject obj;
        obj["status"] = "ok";
        obj["message"] = message;
        printJson(obj);
    }
}

int OutputFormatter::error(const QString& message, int exitCode) const
{
    QTextStream err(stderr);
    if (m_human) {
        err << "Error: " << message << Qt::endl;
    } else {
        QJsonObject obj;
        obj["status"] = "error";
        obj["message"] = message;
        err << QJsonDocument(obj).toJson(QJsonDocument::Compact) << Qt::endl;
    }
    return exitCode;
}

// ── Domain ────────────────────────────────────────────────────────────────────

QJsonObject OutputFormatter::videoToJson(const VideoRecord& r)
{
    QJsonObject obj;
    obj["cid"]              = r.cid;
    obj["lat"]              = r.lat;
    obj["lon"]              = r.lon;
    obj["recorded_at"]      = r.recordedAt.toString(Qt::ISODate);
    obj["indexed_at"]       = r.indexedAt.toString(Qt::ISODate);
    obj["duration_seconds"] = r.durationSeconds;
    obj["size_bytes"]       = r.sizeBytes;
    obj["mime_type"]        = r.mimeType;
    obj["user_owned"]       = r.userOwned;
    obj["downloaded"]       = r.downloaded;
    return obj;
}

void OutputFormatter::videoList(const QList<VideoRecord>& records) const
{
    if (m_human) {
        QTextStream out(stdout);
        out << "Total: " << records.size() << " video(s)\n";
        for (const auto& r : records) {
            out << "  CID:  " << r.cid << "\n"
                << "  Geo:  " << r.lat << ", " << r.lon << "\n"
                << "  Time: " << r.recordedAt.toString(Qt::ISODate) << "\n"
                << "  Size: " << r.sizeBytes << " bytes\n"
                << "  Type: " << r.mimeType << "\n"
                << "  Owned:" << (r.userOwned ? " yes" : " no") << "\n\n";
        }
    } else {
        QJsonObject wrapper;
        QJsonArray arr;
        for (const auto& r : records) arr.append(videoToJson(r));
        wrapper["status"] = "ok";
        wrapper["count"] = records.size();
        wrapper["videos"] = arr;
        printJson(wrapper);
    }
}

void OutputFormatter::video(const VideoRecord& r) const
{
    if (m_human) {
        QTextStream out(stdout);
        out << "CID:      " << r.cid << "\n"
            << "Lat/Lon:  " << r.lat << ", " << r.lon << "\n"
            << "Recorded: " << r.recordedAt.toString(Qt::ISODate) << "\n"
            << "Duration: " << r.durationSeconds << "s\n"
            << "Size:     " << r.sizeBytes << " bytes\n"
            << "Type:     " << r.mimeType << "\n"
            << "Owned:    " << (r.userOwned ? "yes" : "no") << "\n";
    } else {
        QJsonObject obj;
        obj["status"] = "ok";
        obj["video"] = videoToJson(r);
        printJson(obj);
    }
}

QJsonObject OutputFormatter::uploadItemToJson(const UploadItem& item)
{
    QJsonObject obj;
    obj["id"]       = item.id;
    obj["file"]     = item.filePath;
    obj["status"]   = static_cast<int>(item.status);
    obj["progress"] = item.progressPct;
    obj["cid"]      = item.cid;
    if (!item.existingCid.isEmpty())
        obj["existing_cid"] = item.existingCid;
    if (!item.errorMsg.isEmpty())
        obj["error"] = item.errorMsg;
    return obj;
}

void OutputFormatter::uploadResult(const UploadItem& item) const
{
    if (m_human) {
        QTextStream out(stdout);
        switch (item.status) {
        case UploadStatus::Complete:
            out << "Uploaded: " << item.filePath << "\n"
                << "CID:      " << item.cid << "\n";
            break;
        case UploadStatus::Duplicate:
            out << "Duplicate: " << item.filePath << "\n"
                << "CID:       " << item.existingCid << " (already uploaded)\n";
            break;
        case UploadStatus::Failed:
            out << "Failed:    " << item.filePath << "\n"
                << "Error:     " << item.errorMsg << "\n";
            break;
        default:
            out << "Status: " << static_cast<int>(item.status) << "\n";
        }
    } else {
        QJsonObject obj;
        obj["status"] = item.status == UploadStatus::Complete ? "ok"
                      : item.status == UploadStatus::Duplicate ? "duplicate"
                      : "error";
        obj["upload"] = uploadItemToJson(item);
        printJson(obj);
    }
}

void OutputFormatter::storageStats(const StorageStats& stats) const
{
    if (m_human) {
        QTextStream out(stdout);
        out << "User-owned: " << stats.userOwnedBytes << " bytes\n"
            << "Cached:     " << stats.cachedBytes << " bytes\n"
            << "Total:      " << stats.totalUsedBytes() << " bytes\n";
    } else {
        QJsonObject obj;
        obj["status"]           = "ok";
        obj["user_owned_bytes"] = stats.userOwnedBytes;
        obj["cached_bytes"]     = stats.cachedBytes;
        obj["total_used_bytes"] = stats.totalUsedBytes();
        printJson(obj);
    }
}

void OutputFormatter::cacheEntries(const QList<CacheEntry>& entries) const
{
    if (m_human) {
        QTextStream out(stdout);
        out << "Cache entries: " << entries.size() << "\n";
        for (const auto& e : entries) {
            out << "  CID:   " << e.cid << "\n"
                << "  Path:  " << e.localPath << "\n"
                << "  Size:  " << e.sizeBytes << " bytes\n"
                << "  Owned: " << (e.userOwned ? "yes" : "no") << "\n\n";
        }
    } else {
        QJsonArray arr;
        for (const auto& e : entries) {
            QJsonObject obj;
            obj["cid"]        = e.cid;
            obj["local_path"] = e.localPath;
            obj["size_bytes"] = e.sizeBytes;
            obj["user_owned"] = e.userOwned;
            arr.append(obj);
        }
        QJsonObject wrapper;
        wrapper["status"]  = "ok";
        wrapper["count"]   = entries.size();
        wrapper["entries"] = arr;
        printJson(wrapper);
    }
}

void OutputFormatter::statusOutput(bool connected, const StorageStats& stats,
                                    int indexCount, int pendingMessages) const
{
    if (m_human) {
        QTextStream out(stdout);
        out << "Logos connection: " << (connected ? "connected" : "disconnected") << "\n"
            << "Videos indexed:   " << indexCount << "\n"
            << "Pending messages: " << pendingMessages << "\n"
            << "User-owned bytes: " << stats.userOwnedBytes << "\n"
            << "Cached bytes:     " << stats.cachedBytes << "\n"
            << "Total used:       " << stats.totalUsedBytes() << "\n";
    } else {
        QJsonObject obj;
        obj["status"]           = "ok";
        obj["connected"]        = connected;
#ifdef LOGOS_CORE_AVAILABLE
        obj["mode"]             = connected ? "sdk" : "offline";
#else
        // Built without SDK — local-only mode (development/CI)
        obj["mode"]             = "local-only";
#endif
        obj["index_count"]      = indexCount;
        obj["pending_messages"] = pendingMessages;
        obj["user_owned_bytes"] = stats.userOwnedBytes;
        obj["cached_bytes"]     = stats.cachedBytes;
        obj["total_used_bytes"] = stats.totalUsedBytes();
        printJson(obj);
    }
}

}  // namespace VideoHotspot
