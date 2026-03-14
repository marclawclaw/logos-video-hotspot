/**
 * StorageClient — mock implementation using local filesystem.
 *
 * Mock strategy (no Logos SDK):
 *   - Files are stored in $XDG_DATA_HOME/VideoHotspot/storage/<cid>
 *   - CID = SHA-256 hex of file content (simulates content-addressed storage)
 *   - Cache registry: SQLite table `cache` at the same data dir
 *
 * When logos-cpp-sdk is available:
 *   - Replace MockStorage methods with real logos::storage::Client calls
 *   - Keep the QFuture-based async API unchanged
 *
 * See ADR-0003.
 */

#include <video_hotspot/storage_client.h>

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

        // Compute CID (content-addressed)
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
        const QString srcPath = storeDir() + "/" + cid;
        if (!QFile::exists(srcPath)) {
            emit downloadFailed(cid, "CID not found in local store: " + cid);
            return {};
        }

        QDir().mkpath(destDir);
        const QString destPath = destDir + "/" + cid;

        if (!QFile::exists(destPath)) {
            if (!QFile::copy(srcPath, destPath)) {
                emit downloadFailed(cid, "Failed to write to destination");
                return {};
            }
        }

        const qint64 size = QFileInfo(destPath).size();
        emit downloadProgress(cid, size, size);

        // Record as cached (not user-owned)
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
    return QtConcurrent::run([cid]() -> bool {
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
