/**
 * StorageClient — Logos Codex storage integration.
 *
 * Integration strategy:
 *   When built with LOGOS_CORE_AVAILABLE and initLogos() has been called:
 *     - upload() delegates to Codex via LogosAPI::getClient("codex")
 *       invokeRemoteMethod("CodexReplica", "storeFile", filePath) → CID
 *     - download() delegates to Codex:
 *       invokeRemoteMethod("CodexReplica", "fetchFile", cid, destDir) → localPath
 *     - exists() delegates to Codex:
 *       invokeRemoteMethod("CodexReplica", "hasCid", cid) → bool
 *     CIDs returned from Codex are real content-addressed identifiers on the
 *     Logos Storage network (not SHA-256 hex).
 *
 *   When initLogos() has not been called (standalone / no logos-app):
 *     - Falls back to local filesystem mock (SHA-256 CID, file copy)
 *     - Useful for development and testing without a running Logos node
 *
 * See ADR-0003.
 */

#include <video_hotspot/storage_client.h>

#ifdef LOGOS_CORE_AVAILABLE
#include <logos_api.h>
#include <logos_api_client.h>
#endif

#include <QDebug>

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

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString computeCid(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash h(QCryptographicHash::Sha256);
    while (!f.atEnd()) h.addData(f.read(64 * 1024));
    return h.result().toHex();
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
        qDebug() << "StorageClient: using module='codex', replica='CodexReplica'";
    } else {
        qDebug() << "StorageClient: initLogos called with null — using local mock";
    }
#else
    Q_UNUSED(logosAPI)
    qDebug() << "StorageClient: built without LOGOS_CORE_AVAILABLE — local mock only";
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
        if (m_logosAPI) {
            auto* client = m_logosAPI->getClient("codex");
            QVariant result = client->invokeRemoteMethod(
                "CodexReplica", "storeFile", QVariant(filePath));

            if (!result.isNull() && !result.toString().isEmpty()) {
                const QString cid = result.toString();
                emit uploadProgress(filePath, total, total);

                // Record in local cache as user-owned for fast lookups
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
            qWarning() << "StorageClient: Codex storeFile failed, falling back to local mock";
        }
#else
        Q_UNUSED(opts)
#endif

        // ── Local filesystem mock ─────────────────────────────────────────
        // Compute CID (SHA-256 of file content — simulates content addressing)
        const QString cid = computeCid(filePath);
        if (cid.isEmpty()) {
            emit uploadFailed(filePath, "Cannot read file");
            return {};
        }

        const QString destPath = storeDir() + "/" + cid;

        // Already stored?
        if (!QFile::exists(destPath)) {
            if (!QFile::copy(filePath, destPath)) {
                emit uploadFailed(filePath, "Failed to store file");
                return {};
            }
        }

        emit uploadProgress(filePath, total, total);

        // Record in cache DB as user-owned
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
    });
}

QFuture<QString> StorageClient::download(const QString& cid, const QString& destDir)
{
    return QtConcurrent::run([this, cid, destDir]() -> QString {
        QDir().mkpath(destDir);

#ifdef LOGOS_CORE_AVAILABLE
        // ── Real Codex fetch via LogosAPI ─────────────────────────────────
        if (m_logosAPI) {
            auto* client = m_logosAPI->getClient("codex");
            QVariant result = client->invokeRemoteMethod(
                "CodexReplica", "fetchFile",
                QVariant(cid), QVariant(destDir));

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
            qWarning() << "StorageClient: Codex fetchFile failed, falling back to local mock";
        }
#endif

        // ── Local filesystem mock ─────────────────────────────────────────
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
    });
}

QFuture<bool> StorageClient::exists(const QString& cid)
{
    return QtConcurrent::run([this, cid]() -> bool {
#ifdef LOGOS_CORE_AVAILABLE
        if (m_logosAPI) {
            auto* client = m_logosAPI->getClient("codex");
            QVariant result = client->invokeRemoteMethod(
                "CodexReplica", "hasCid", QVariant(cid));
            return result.isValid() && result.toBool();
        }
#endif
        return QFile::exists(storeDir() + "/" + cid);
    });
}

QFuture<void> StorageClient::evict(const QString& cid)
{
    return QtConcurrent::run([this, cid]() {
        // Never evict user-owned entries
        auto db = threadDb();
        QSqlQuery check(db);
        check.prepare("SELECT user_owned, local_path FROM cache WHERE cid = ?");
        check.addBindValue(cid);
        check.exec();

        if (!check.next()) return;
        if (check.value(0).toInt() == 1) return;  // user-owned, skip

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
        // Order by oldest first (ROWID is insertion order)
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

}  // namespace VideoHotspot
