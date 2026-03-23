/**
 * StorageClient — Logos Codex storage integration.
 *
 * When built with LOGOS_CORE_AVAILABLE (default when SDK is present):
 *   - initLogos(LogosAPI*) MUST be called before any upload/download.
 *   - upload() delegates to Codex via LogosAPI::getClient("codex")
 *   - download() fetches from Codex
 *   - exists() queries Codex
 *   - If m_logosAPI is null (initLogos not called), operations fail with a
 *     clear error. There is no silent local fallback.
 *
 * When built WITHOUT LOGOS_CORE_AVAILABLE (SDK absent):
 *   - All operations use local filesystem + SQLite (development / CI only).
 *   - This path is intentionally disabled when the SDK is present.
 *
 * See ADR-0003.
 */

#include <video_hotspot/storage_client.h>

#ifdef LOGOS_CORE_AVAILABLE
#include <logos_api.h>
#include <logos_api_client.h>
#endif

#include <QDebug>

#include <QVariantMap>
#include <QVariantList>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

namespace VideoHotspot {

// ── Paths ─────────────────────────────────────────────────────────────────────

static QString dataDir()
{
    const QString d = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(d + "/storage");
    return d;
}

static QString storeDir() { return dataDir() + "/storage"; }
static QString cacheDbPath() { return dataDir() + "/cache.db"; }

// ── DB ────────────────────────────────────────────────────────────────────────

static QSqlDatabase openCacheDb()
{
    const QString conn = "cache_db";
    if (QSqlDatabase::contains(conn)) return QSqlDatabase::database(conn);

    auto db = QSqlDatabase::addDatabase("QSQLITE", conn);
    db.setDatabaseName(cacheDbPath());
    db.open();

    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS cache ("
           "  cid        TEXT PRIMARY KEY,"
           "  local_path TEXT NOT NULL,"
           "  size_bytes INTEGER NOT NULL,"
           "  user_owned INTEGER NOT NULL DEFAULT 0"
           ")");
    return db;
}

static QSqlDatabase threadDb()
{
    const QString name = QString("cache_%1").arg(
        QString::number(reinterpret_cast<quintptr>(QThread::currentThread())));
    if (QSqlDatabase::contains(name)) return QSqlDatabase::database(name);
    auto db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName(cacheDbPath());
    db.open();
    return db;
}

#ifndef LOGOS_CORE_AVAILABLE
// SHA-256 based CID for local-only builds (development / CI without SDK)
static QString computeLocalCid(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash h(QCryptographicHash::Sha256);
    while (!f.atEnd()) h.addData(f.read(64 * 1024));
    return h.result().toHex();
}
#endif

// ── StorageClient ─────────────────────────────────────────────────────────────

StorageClient::StorageClient(QObject* parent)
    : QObject(parent)
{
    openCacheDb();
}

StorageClient::~StorageClient() = default;

void StorageClient::initLogos(LogosAPI* logosAPI)
{
    m_logosAPI = logosAPI;
#ifdef LOGOS_CORE_AVAILABLE
    if (logosAPI) {
        qDebug() << "StorageClient: LogosAPI connected — real Codex storage enabled";
    } else {
        qWarning() << "StorageClient: initLogos(nullptr) — Logos not connected;"
                   << "upload/download will fail until a LogosAPI is provided";
    }
#else
    Q_UNUSED(logosAPI)
    qDebug() << "StorageClient: built without LOGOS_CORE_AVAILABLE — local-only mode";
#endif
}

bool StorageClient::isLogosConnected() const
{
#ifdef LOGOS_CORE_AVAILABLE
    return m_logosAPI != nullptr;
#else
    return false;
#endif
}

QFuture<QString> StorageClient::upload(const QString& filePath,
                                       const UploadOptions& opts)
{
    return QtConcurrent::run([this, filePath, opts]() -> QString {
        QFileInfo fi(filePath);
        if (!fi.exists()) {
            emit uploadFailed(filePath, "File not found: " + filePath);
            return {};
        }

        const qint64 total = fi.size();
        emit uploadProgress(filePath, 0, total);

#ifdef LOGOS_CORE_AVAILABLE
        // ── Real Codex storage via LogosAPI ───────────────────────────────
        if (!m_logosAPI) {
            const QString err = QStringLiteral(
                "Logos not connected: initLogos() was not called. "
                "Start the Logos daemon and ensure logos_core_start() succeeded.");
            qCritical() << "StorageClient::upload:" << err;
            emit uploadFailed(filePath, err);
            return {};
        }

        auto* client = m_logosAPI->getClient("codex");
        QVariant result = client->invokeRemoteMethod(
            "CodexReplica", "storeFile", QVariant(filePath));

        if (!result.isNull() && !result.toString().isEmpty()) {
            const QString cid = result.toString();
            emit uploadProgress(filePath, total, total);

            // Cache CID → path for fast local lookups (list, status)
            auto db = threadDb();
            QSqlQuery q(db);
            q.prepare("INSERT OR REPLACE INTO cache "
                      "(cid, local_path, size_bytes, user_owned) "
                      "VALUES (?, ?, ?, 1)");
            q.addBindValue(cid);
            q.addBindValue(filePath);
            q.addBindValue(total);
            q.exec();

            emit uploadComplete(filePath, cid);
            emit storageStatsChanged(stats());
            qDebug() << "StorageClient: Codex upload complete, CID=" << cid;
            return cid;
        }

        const QString err = QStringLiteral(
            "Codex storeFile RPC failed: daemon reachable but returned empty CID. "
            "Check Codex module is loaded in the Logos daemon.");
        qCritical() << "StorageClient::upload:" << err;
        emit uploadFailed(filePath, err);
        return {};

#else
        // ── Local-only build (no SDK) ────────────────────────────────────
        Q_UNUSED(opts)
        const QString cid = computeLocalCid(filePath);
        if (cid.isEmpty()) {
            emit uploadFailed(filePath, "Cannot read file");
            return {};
        }

        const QString destPath = storeDir() + "/" + cid;
        if (!QFile::exists(destPath)) {
            if (!QFile::copy(filePath, destPath)) {
                emit uploadFailed(filePath, "Failed to store file locally");
                return {};
            }
        }

        emit uploadProgress(filePath, total, total);

        auto db = threadDb();
        QSqlQuery q(db);
        q.prepare("INSERT OR REPLACE INTO cache (cid, local_path, size_bytes, user_owned) "
                  "VALUES (?, ?, ?, 1)");
        q.addBindValue(cid);
        q.addBindValue(destPath);
        q.addBindValue(total);
        q.exec();

        emit uploadComplete(filePath, cid);
        emit storageStatsChanged(stats());
        return cid;
#endif
    });
}

QFuture<QString> StorageClient::download(const QString& cid, const QString& destDir)
{
    return QtConcurrent::run([this, cid, destDir]() -> QString {
        QDir().mkpath(destDir);

#ifdef LOGOS_CORE_AVAILABLE
        if (!m_logosAPI) {
            const QString err = QStringLiteral(
                "Logos not connected: initLogos() was not called. "
                "Start the Logos daemon and ensure logos_core_start() succeeded.");
            qCritical() << "StorageClient::download:" << err;
            emit downloadFailed(cid, err);
            return {};
        }

        auto* client = m_logosAPI->getClient("codex");
        QVariant result = client->invokeRemoteMethod(
            "CodexReplica", "fetchFile", QVariant(cid), QVariant(destDir));

        if (!result.isNull() && !result.toString().isEmpty()) {
            const QString localPath = result.toString();
            const qint64 size = QFileInfo(localPath).size();
            emit downloadProgress(cid, size, size);

            auto db = threadDb();
            QSqlQuery q(db);
            q.prepare("INSERT OR IGNORE INTO cache "
                      "(cid, local_path, size_bytes, user_owned) "
                      "VALUES (?, ?, ?, 0)");
            q.addBindValue(cid);
            q.addBindValue(localPath);
            q.addBindValue(size);
            q.exec();

            emit downloadComplete(cid, localPath);
            qDebug() << "StorageClient: Codex fetch complete, path=" << localPath;
            return localPath;
        }

        const QString err = QStringLiteral(
            "Codex fetchFile RPC failed for CID: ") + cid;
        qCritical() << "StorageClient::download:" << err;
        emit downloadFailed(cid, err);
        return {};

#else
        // ── Local-only build (no SDK) ────────────────────────────────────
        const QString srcPath = storeDir() + "/" + cid;
        if (!QFile::exists(srcPath)) {
            emit downloadFailed(cid, "CID not found in local store: " + cid);
            return {};
        }

        const QString destPath = destDir + "/" + cid;
        if (!QFile::exists(destPath)) {
            if (!QFile::copy(srcPath, destPath)) {
                emit downloadFailed(cid, "Failed to write to destination");
                return {};
            }
        }

        const qint64 size = QFileInfo(destPath).size();
        emit downloadProgress(cid, size, size);

        auto db = threadDb();
        QSqlQuery q(db);
        q.prepare("INSERT OR IGNORE INTO cache (cid, local_path, size_bytes, user_owned) "
                  "VALUES (?, ?, ?, 0)");
        q.addBindValue(cid);
        q.addBindValue(destPath);
        q.addBindValue(size);
        q.exec();

        emit downloadComplete(cid, destPath);
        return destPath;
#endif
    });
}

QFuture<bool> StorageClient::exists(const QString& cid)
{
    return QtConcurrent::run([this, cid]() -> bool {
#ifdef LOGOS_CORE_AVAILABLE
        if (!m_logosAPI) return false;
        auto* client = m_logosAPI->getClient("codex");
        QVariant result = client->invokeRemoteMethod(
            "CodexReplica", "hasCid", QVariant(cid));
        return result.isValid() && result.toBool();
#else
        return QFile::exists(storeDir() + "/" + cid);
#endif
    });
}

QFuture<void> StorageClient::evict(const QString& cid)
{
    return QtConcurrent::run([this, cid]() {
        auto db = threadDb();
        QSqlQuery check(db);
        check.prepare("SELECT user_owned, local_path FROM cache WHERE cid = ?");
        check.addBindValue(cid);
        check.exec();

        if (!check.next()) return;
        if (check.value(0).toInt() == 1) return;  // user-owned, never evict

        const QString path = check.value(1).toString();
        QFile::remove(path);

        QSqlQuery del(db);
        del.prepare("DELETE FROM cache WHERE cid = ?");
        del.addBindValue(cid);
        del.exec();

        emit storageStatsChanged(stats());
    });
}

QFuture<void> StorageClient::autoEvict(qint64 targetBytes)
{
    return QtConcurrent::run([this, targetBytes]() {
        auto db = threadDb();
        QSqlQuery q(db);
        q.exec("SELECT cid, local_path, size_bytes FROM cache "
               "WHERE user_owned = 0 ORDER BY rowid ASC");

        StorageStats s = stats();
        while (s.totalUsedBytes() > targetBytes && q.next()) {
            const QString cid = q.value(0).toString();
            const QString path = q.value(1).toString();
            const qint64 sz = q.value(2).toLongLong();

            QFile::remove(path);

            QSqlQuery del(db);
            del.prepare("DELETE FROM cache WHERE cid = ?");
            del.addBindValue(cid);
            del.exec();

            s.cachedBytes -= sz;
        }
        emit storageStatsChanged(stats());
    });
}

QList<CacheEntry> StorageClient::cachedEntries() const
{
    auto db = QSqlDatabase::database("cache_db");
    QSqlQuery q(db);
    q.exec("SELECT cid, local_path, size_bytes, user_owned FROM cache");

    QList<CacheEntry> result;
    while (q.next()) {
        CacheEntry e;
        e.cid       = q.value(0).toString();
        e.localPath = q.value(1).toString();
        e.sizeBytes = q.value(2).toLongLong();
        e.userOwned = q.value(3).toBool();
        result.append(e);
    }
    return result;
}

StorageStats StorageClient::stats() const
{
    auto db = QSqlDatabase::database("cache_db");
    QSqlQuery q(db);

    StorageStats s;
    q.exec("SELECT SUM(size_bytes) FROM cache WHERE user_owned = 1");
    if (q.next()) s.userOwnedBytes = q.value(0).toLongLong();

    q.exec("SELECT SUM(size_bytes) FROM cache WHERE user_owned = 0");
    if (q.next()) s.cachedBytes = q.value(0).toLongLong();

    return s;
}

void StorageClient::setStorageLimit(qint64 bytes)
{
    const StorageStats s = stats();
    if (s.totalUsedBytes() > bytes) {
        autoEvict(bytes);
    }
}

qint64 StorageClient::statsAllocatedBytes() const { return stats().allocatedBytes; }
qint64 StorageClient::statsUserOwnedBytes() const { return stats().userOwnedBytes; }
qint64 StorageClient::statsCachedBytes() const { return stats().cachedBytes; }
qint64 StorageClient::statsTotalUsedBytes() const { return stats().totalUsedBytes(); }

int StorageClient::cachedEntriesCount() const
{
    return cachedEntries().size();
}

QVariantMap StorageClient::cachedEntryAt(int index) const
{
    const auto entries = cachedEntries();
    if (index < 0 || index >= entries.size()) return {};
    const CacheEntry& e = entries.at(index);
    QVariantMap m;
    m["cid"]       = e.cid;
    m["localPath"] = e.localPath;
    m["sizeBytes"] = e.sizeBytes;
    m["userOwned"] = e.userOwned;
    return m;
}

QVariantList StorageClient::cachedEntriesVariantList() const
{
    QVariantList result;
    for (const CacheEntry& e : cachedEntries()) {
        QVariantMap m;
        m["cid"]       = e.cid;
        m["localPath"] = e.localPath;
        m["sizeBytes"] = e.sizeBytes;
        m["userOwned"] = e.userOwned;
        result.append(m);
    }
    return result;
}

}  // namespace VideoHotspot
