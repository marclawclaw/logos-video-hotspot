/**
 * UploadQueue — orchestrates dedup → geolocation prompt → upload.
 *
 * State is persisted in SQLite table `upload_queue` at
 * $XDG_DATA_HOME/VideoHotspot/queue.db so it survives app restarts.
 *
 * Folder monitoring uses QFileSystemWatcher to detect new files.
 *
 * See ADR-0002 (headless mode), ADR-0003 (storage), ADR-0005 (dedup).
 */

#include <video_hotspot/upload_queue.h>
#include <video_hotspot/indexing_service.h>
#include <video_hotspot/metadata_extractor.h>

#include <QDir>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QFutureWatcher>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace VideoHotspot {

// ── DB ────────────────────────────────────────────────────────────────────────

static QString queueDbPath()
{
    const QString d = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(d);
    return d + "/queue.db";
}

static QSqlDatabase openQueueDb()
{
    const QString conn = "queue_db";
    if (QSqlDatabase::contains(conn)) return QSqlDatabase::database(conn);

    auto db = QSqlDatabase::addDatabase("QSQLITE", conn);
    db.setDatabaseName(queueDbPath());
    db.open();

    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS upload_queue ("
           "  id          TEXT PRIMARY KEY,"
           "  file_path   TEXT NOT NULL,"
           "  status      INTEGER NOT NULL DEFAULT 0,"
           "  progress    INTEGER NOT NULL DEFAULT 0,"
           "  cid         TEXT NOT NULL DEFAULT '',"
           "  existing_cid TEXT NOT NULL DEFAULT '',"
           "  error_msg   TEXT NOT NULL DEFAULT '',"
           "  lat         REAL,"
           "  lon         REAL"
           ")");
    return db;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

static int statusToInt(UploadStatus s) { return static_cast<int>(s); }
static UploadStatus intToStatus(int i) { return static_cast<UploadStatus>(i); }

static UploadItem rowToItem(QSqlQuery& q)
{
    UploadItem item;
    item.id          = q.value(0).toString();
    item.filePath    = q.value(1).toString();
    item.status      = intToStatus(q.value(2).toInt());
    item.progressPct = q.value(3).toInt();
    item.cid         = q.value(4).toString();
    item.existingCid = q.value(5).toString();
    item.errorMsg    = q.value(6).toString();
    return item;
}

// ── UploadQueuePrivate ────────────────────────────────────────────────────────

struct UploadQueuePrivate {
    StorageClient*    storage       = nullptr;
    Deduplicator*     deduplicator  = nullptr;
    QFileSystemWatcher* watcher     = nullptr;
    QString           monitoredPath;
};

// ── UploadQueue ───────────────────────────────────────────────────────────────

UploadQueue::UploadQueue(StorageClient* storage,
                         Deduplicator*  deduplicator,
                         QObject*       parent)
    : QObject(parent)
    , d(new UploadQueuePrivate)
{
    d->storage      = storage;
    d->deduplicator = deduplicator;
    openQueueDb();
}

UploadQueue::~UploadQueue() = default;

static void updateStatus(const QString& id, UploadStatus status,
                         int progress = -1,
                         const QString& cid = {},
                         const QString& existingCid = {},
                         const QString& errorMsg = {})
{
    auto db = QSqlDatabase::database("queue_db");
    QSqlQuery q(db);
    q.prepare("UPDATE upload_queue SET status=?, progress=?, cid=?, "
              "existing_cid=?, error_msg=? WHERE id=?");
    q.addBindValue(statusToInt(status));
    q.addBindValue(progress >= 0 ? progress : 0);
    q.addBindValue(cid);
    q.addBindValue(existingCid);
    q.addBindValue(errorMsg);
    q.addBindValue(id);
    q.exec();
}

QString UploadQueue::enqueue(const QString& filePath)
{
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        qWarning() << "[UploadQueue] enqueue: file not found:" << filePath;
        return {};
    }

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Persist to queue DB
    auto db = QSqlDatabase::database("queue_db");
    QSqlQuery q(db);
    q.prepare("INSERT INTO upload_queue (id, file_path, status) VALUES (?, ?, ?)");
    q.addBindValue(id);
    q.addBindValue(filePath);
    q.addBindValue(statusToInt(UploadStatus::Pending));
    q.exec();

    UploadItem item;
    item.id       = id;
    item.filePath = filePath;
    item.status   = UploadStatus::Pending;
    emit itemAdded(item);

    // Start processing asynchronously
    QMetaObject::invokeMethod(this, [this, id, filePath]() {
        processItem(id, filePath);
    }, Qt::QueuedConnection);

    return id;
}

void UploadQueue::processItem(const QString& id, const QString& filePath)
{
    // Step 1: dedup check
    updateStatus(id, UploadStatus::Hashing, 0);
    {
        UploadItem item;
        item.id = id; item.filePath = filePath;
        item.status = UploadStatus::Hashing;
        emit itemChanged(item);
    }

    auto* dedupWatcher = new QFutureWatcher<DedupResult>(this);
    connect(dedupWatcher, &QFutureWatcher<DedupResult>::finished,
            this, [this, id, filePath, dedupWatcher]() {
                dedupWatcher->deleteLater();
                const DedupResult result = dedupWatcher->result();

                if (result.isDuplicate) {
                    updateStatus(id, UploadStatus::Duplicate, 100,
                                 result.existingCid, result.existingCid);
                    UploadItem item;
                    item.id = id; item.filePath = filePath;
                    item.status = UploadStatus::Duplicate;
                    item.existingCid = result.existingCid;
                    item.cid = result.existingCid;
                    emit itemChanged(item);
                    return;
                }

                // Step 2: upload
                startUpload(id, filePath, result.blake3Hash);
            });
    dedupWatcher->setFuture(d->deduplicator->checkFile(filePath));
}

void UploadQueue::startUpload(const QString& id, const QString& filePath,
                               const QString& blake3Hash)
{
    updateStatus(id, UploadStatus::Uploading, 0);
    {
        UploadItem item;
        item.id = id; item.filePath = filePath;
        item.status = UploadStatus::Uploading;
        emit itemChanged(item);
    }

    // Connect progress
    connect(d->storage, &StorageClient::uploadProgress,
            this, [this, id, filePath](const QString& fp, qint64 done, qint64 total) {
                if (fp != filePath) return;
                const int pct = total > 0 ? int(done * 100 / total) : 0;
                auto db = QSqlDatabase::database("queue_db");
                QSqlQuery q(db);
                q.prepare("UPDATE upload_queue SET progress=? WHERE id=?");
                q.addBindValue(pct);
                q.addBindValue(id);
                q.exec();
                UploadItem item;
                item.id = id; item.filePath = filePath;
                item.status = UploadStatus::Uploading;
                item.progressPct = pct;
                emit itemChanged(item);
            });

    auto* uploadWatcher = new QFutureWatcher<QString>(this);
    connect(uploadWatcher, &QFutureWatcher<QString>::finished,
            this, [this, id, filePath, blake3Hash, uploadWatcher]() {
                uploadWatcher->deleteLater();
                const QString cid = uploadWatcher->result();

                if (cid.isEmpty()) {
                    updateStatus(id, UploadStatus::Failed, 0, {}, {},
                                 "Upload failed");
                    UploadItem item;
                    item.id = id; item.filePath = filePath;
                    item.status = UploadStatus::Failed;
                    item.errorMsg = "Upload failed";
                    emit itemChanged(item);
                    return;
                }

                // Record hash → cid for future dedup
                d->deduplicator->recordUpload(filePath, blake3Hash, cid);

                updateStatus(id, UploadStatus::Complete, 100, cid);
                UploadItem item;
                item.id = id; item.filePath = filePath;
                item.status = UploadStatus::Complete;
                item.progressPct = 100;
                item.cid = cid;
                emit itemChanged(item);

                // Check if all complete
                auto db = QSqlDatabase::database("queue_db");
                QSqlQuery q(db);
                q.exec("SELECT COUNT(*) FROM upload_queue WHERE status NOT IN (3,4,6)");
                if (q.next() && q.value(0).toInt() == 0) emit allComplete();
            });
    uploadWatcher->setFuture(d->storage->upload(filePath));
}

QStringList UploadQueue::enqueueFolder(const QString& dirPath, bool recursive)
{
    QStringList ids;
    QDir dir(dirPath);
    if (!dir.exists()) return ids;

    static const QStringList kVideoFilters = {
        "*.mp4", "*.mov", "*.avi", "*.mkv", "*.webm",
        "*.m4v", "*.flv", "*.wmv", "*.ts", "*.mpg", "*.mpeg"
    };

    const QDir::Filters flags = QDir::Files | QDir::Readable;
    const QDirIterator::IteratorFlags iterFlags = recursive
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;

    QDirIterator it(dirPath, kVideoFilters, flags, iterFlags);
    while (it.hasNext()) {
        ids.append(enqueue(it.next()));
    }
    return ids;
}

void UploadQueue::startMonitoring(const QString& dirPath)
{
    if (d->watcher) {
        d->watcher->removePaths(d->watcher->directories());
        delete d->watcher;
    }
    d->watcher = new QFileSystemWatcher(this);
    d->watcher->addPath(dirPath);
    d->monitoredPath = dirPath;

    connect(d->watcher, &QFileSystemWatcher::directoryChanged,
            this, [this, dirPath](const QString& /*path*/) {
                // Enumerate new video files and enqueue them
                static const QStringList kExts = {
                    "*.mp4","*.mov","*.avi","*.mkv","*.webm",
                    "*.m4v","*.flv","*.wmv","*.ts","*.mpg","*.mpeg"
                };
                QDirIterator it(dirPath, kExts, QDir::Files | QDir::Readable);
                while (it.hasNext()) {
                    const QString fp = it.next();
                    // Skip if already in queue
                    auto db = QSqlDatabase::database("queue_db");
                    QSqlQuery q(db);
                    q.prepare("SELECT COUNT(*) FROM upload_queue WHERE file_path=?");
                    q.addBindValue(fp);
                    q.exec();
                    if (q.next() && q.value(0).toInt() == 0) {
                        enqueue(fp);
                    }
                }
            });
}

void UploadQueue::stopMonitoring()
{
    if (d->watcher) {
        delete d->watcher;
        d->watcher = nullptr;
    }
    d->monitoredPath.clear();
}

bool UploadQueue::isMonitoring() const { return d->watcher != nullptr; }
QString UploadQueue::monitoredPath() const { return d->monitoredPath; }

void UploadQueue::retry(const QString& itemId)
{
    auto db = QSqlDatabase::database("queue_db");
    QSqlQuery q(db);
    q.prepare("SELECT file_path FROM upload_queue WHERE id=?");
    q.addBindValue(itemId);
    q.exec();
    if (!q.next()) return;

    const QString filePath = q.value(0).toString();
    updateStatus(itemId, UploadStatus::Pending);
    QMetaObject::invokeMethod(this, [this, itemId, filePath]() {
        processItem(itemId, filePath);
    }, Qt::QueuedConnection);
}

void UploadQueue::cancel(const QString& itemId)
{
    auto db = QSqlDatabase::database("queue_db");
    QSqlQuery q(db);
    q.prepare("DELETE FROM upload_queue WHERE id=? AND status IN (0,5)");  // Pending or Failed
    q.addBindValue(itemId);
    q.exec();
    emit itemRemoved(itemId);
}

QList<UploadItem> UploadQueue::items() const
{
    auto db = QSqlDatabase::database("queue_db");
    QSqlQuery q(db);
    q.exec("SELECT id, file_path, status, progress, cid, existing_cid, error_msg "
           "FROM upload_queue ORDER BY rowid ASC");

    QList<UploadItem> result;
    while (q.next()) result.append(rowToItem(q));
    return result;
}

void UploadQueue::provideGeolocation(const QString& itemId, double lat, double lon)
{
    auto db = QSqlDatabase::database("queue_db");
    QSqlQuery q(db);
    q.prepare("UPDATE upload_queue SET lat=?, lon=?, status=? WHERE id=?");
    q.addBindValue(lat);
    q.addBindValue(lon);
    q.addBindValue(statusToInt(UploadStatus::Pending));
    q.addBindValue(itemId);
    q.exec();

    QSqlQuery sel(db);
    sel.prepare("SELECT file_path FROM upload_queue WHERE id=?");
    sel.addBindValue(itemId);
    sel.exec();
    if (sel.next()) {
        QMetaObject::invokeMethod(this, [this, itemId, fp = sel.value(0).toString()]() {
            processItem(itemId, fp);
        }, Qt::QueuedConnection);
    }
}

}  // namespace VideoHotspot
