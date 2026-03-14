/**
 * Deduplicator — mock implementation using SHA-256 via Qt6.
 *
 * Production note: Replace QCryptographicHash with BLAKE3 for real deployment.
 * See ADR-0005. The API contract is unchanged; only the hash algorithm changes.
 *
 * Storage: SQLite table `hashes` at $XDG_DATA_HOME/VideoHotspot/dedup.db
 *   CREATE TABLE hashes (hash TEXT PRIMARY KEY, cid TEXT, file_path TEXT);
 */

#include <video_hotspot/deduplicator.h>

#include <QCryptographicHash>
#include <QFile>
#include <QFuture>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

namespace VideoHotspot {

// ── DB helpers ───────────────────────────────────────────────────────────────

static QString dbPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    return dir + "/dedup.db";
}

static QSqlDatabase openDb(const QString& connectionName = "dedup")
{
    if (QSqlDatabase::contains(connectionName)) {
        return QSqlDatabase::database(connectionName);
    }
    auto db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbPath());
    if (!db.open()) {
        qWarning() << "[Deduplicator] DB open failed:" << db.lastError().text();
    }
    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS hashes ("
           "  hash      TEXT PRIMARY KEY,"
           "  cid       TEXT NOT NULL,"
           "  file_path TEXT NOT NULL"
           ")");
    return db;
}

// ── Implementation ────────────────────────────────────────────────────────────

struct DeduplicatorPrivate {
    DeduplicatorPrivate() { openDb(); }
};

Deduplicator::Deduplicator(QObject* parent)
    : QObject(parent)
{
    openDb();
}

Deduplicator::~Deduplicator() = default;

// Compute SHA-256 hash of a file (mock for BLAKE3).
// NOTE: Replace with BLAKE3 for production.
static QString sha256File(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};

    QCryptographicHash hasher(QCryptographicHash::Sha256);
    while (!f.atEnd()) {
        const QByteArray chunk = f.read(64 * 1024);
        hasher.addData(chunk);
    }
    return hasher.result().toHex();
}

QFuture<QString> Deduplicator::computeHash(const QString& filePath)
{
    return QtConcurrent::run([this, filePath]() -> QString {
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) return {};  // File not found or not readable

        const qint64 total = f.size();
        qint64 done = 0;

        QCryptographicHash hasher(QCryptographicHash::Sha256);
        while (!f.atEnd()) {
            const QByteArray chunk = f.read(64 * 1024);
            hasher.addData(chunk);
            done += chunk.size();
            emit hashProgress(filePath, done, total);
        }
        return hasher.result().toHex();
    });
}

QFuture<DedupResult> Deduplicator::checkFile(const QString& filePath)
{
    return QtConcurrent::run([this, filePath]() -> DedupResult {
        const QString hash = sha256File(filePath);
        if (hash.isEmpty()) {
            return DedupResult{false, {}, {}};
        }

        auto db = QSqlDatabase::database("dedup");

        // Use a thread-local connection to avoid cross-thread SQLite issues
        const QString connName = QString("dedup_%1").arg(
            QString::number(reinterpret_cast<quintptr>(QThread::currentThread())));
        QSqlDatabase tdb;
        if (QSqlDatabase::contains(connName)) {
            tdb = QSqlDatabase::database(connName);
        } else {
            tdb = QSqlDatabase::addDatabase("QSQLITE", connName);
            tdb.setDatabaseName(dbPath());
            tdb.open();
        }

        QSqlQuery q(tdb);
        q.prepare("SELECT cid FROM hashes WHERE hash = ?");
        q.addBindValue(hash);
        q.exec();

        if (q.next()) {
            const QString cid = q.value(0).toString();
            emit duplicateDetected(filePath, cid);
            return DedupResult{true, cid, hash};
        }
        return DedupResult{false, {}, hash};
    });
}

void Deduplicator::recordUpload(const QString& filePath,
                                const QString& blake3Hash,
                                const QString& cid)
{
    auto db = QSqlDatabase::database("dedup");
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO hashes (hash, cid, file_path) VALUES (?, ?, ?)");
    q.addBindValue(blake3Hash);
    q.addBindValue(cid);
    q.addBindValue(filePath);
    if (!q.exec()) {
        qWarning() << "[Deduplicator] recordUpload failed:" << q.lastError().text();
    }
}

}  // namespace VideoHotspot
