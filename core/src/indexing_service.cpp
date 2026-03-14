/**
 * IndexingService — manages the local video index and Logos Messaging pub/sub.
 *
 * Local DB: SQLite table `videos` at $XDG_DATA_HOME/VideoHotspot/index.db
 *
 * On upload complete: serialises VideoRecord as JSON, publishes via MessagingClient,
 * inserts into local `videos` table.
 *
 * On incoming message: parses JSON VideoRecord, inserts into local `videos` table,
 * emits recordAdded().
 *
 * Batch blockchain indexing is a stub (see ADR-0004 for design intent).
 */

#include <video_hotspot/indexing_service.h>
#include <video_hotspot/messaging_client.h>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTimeZone>

namespace VideoHotspot {

// ── DB ────────────────────────────────────────────────────────────────────────

static QString indexDbPath()
{
    const QString d = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(d);
    return d + "/index.db";
}

static QSqlDatabase openIndexDb()
{
    const QString conn = "index_db";
    if (QSqlDatabase::contains(conn)) return QSqlDatabase::database(conn);

    auto db = QSqlDatabase::addDatabase("QSQLITE", conn);
    db.setDatabaseName(indexDbPath());
    db.open();

    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS videos ("
           "  cid              TEXT PRIMARY KEY,"
           "  lat              REAL NOT NULL,"
           "  lon              REAL NOT NULL,"
           "  recorded_at      INTEGER NOT NULL,"
           "  indexed_at       INTEGER NOT NULL,"
           "  duration_seconds INTEGER NOT NULL DEFAULT 0,"
           "  size_bytes       INTEGER NOT NULL DEFAULT 0,"
           "  mime_type        TEXT NOT NULL DEFAULT '',"
           "  user_owned       INTEGER NOT NULL DEFAULT 0,"
           "  downloaded       INTEGER NOT NULL DEFAULT 0"
           ")");
    return db;
}

// ── Serialisation ─────────────────────────────────────────────────────────────

static QByteArray recordToJson(const VideoRecord& r)
{
    QJsonObject obj;
    obj["cid"]              = r.cid;
    obj["lat"]              = r.lat;
    obj["lon"]              = r.lon;
    obj["recorded_at"]      = r.recordedAt.toSecsSinceEpoch();
    obj["indexed_at"]       = r.indexedAt.toSecsSinceEpoch();
    obj["duration_seconds"] = r.durationSeconds;
    obj["size_bytes"]       = r.sizeBytes;
    obj["mime_type"]        = r.mimeType;
    obj["user_owned"]       = r.userOwned;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

static VideoRecord jsonToRecord(const QByteArray& data)
{
    const QJsonObject obj = QJsonDocument::fromJson(data).object();
    VideoRecord r;
    r.cid             = obj["cid"].toString();
    r.lat             = obj["lat"].toDouble();
    r.lon             = obj["lon"].toDouble();
    r.recordedAt      = QDateTime::fromSecsSinceEpoch(obj["recorded_at"].toInteger());
    r.indexedAt       = QDateTime::fromSecsSinceEpoch(obj["indexed_at"].toInteger());
    r.durationSeconds = obj["duration_seconds"].toInteger();
    r.sizeBytes       = obj["size_bytes"].toInteger();
    r.mimeType        = obj["mime_type"].toString();
    r.userOwned       = obj["user_owned"].toBool();
    return r;
}

static void insertRecord(const VideoRecord& r)
{
    auto db = QSqlDatabase::database("index_db");
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO videos "
              "(cid, lat, lon, recorded_at, indexed_at, duration_seconds, "
              " size_bytes, mime_type, user_owned, downloaded) "
              "VALUES (?,?,?,?,?,?,?,?,?,?)");
    q.addBindValue(r.cid);
    q.addBindValue(r.lat);
    q.addBindValue(r.lon);
    q.addBindValue(r.recordedAt.toSecsSinceEpoch());
    q.addBindValue(r.indexedAt.toSecsSinceEpoch());
    q.addBindValue(r.durationSeconds);
    q.addBindValue(r.sizeBytes);
    q.addBindValue(r.mimeType);
    q.addBindValue(r.userOwned ? 1 : 0);
    q.addBindValue(r.downloaded ? 1 : 0);
    if (!q.exec()) {
        qWarning() << "[IndexingService] insert failed:" << q.lastError().text();
    }
}

// ── IndexingService ───────────────────────────────────────────────────────────

struct IndexingServicePrivate {
    MessagingClient* messaging = nullptr;
};

IndexingService::IndexingService(QObject* parent)
    : QObject(parent)
    , d(new IndexingServicePrivate)
{
    openIndexDb();
    d->messaging = new MessagingClient(this);
}

IndexingService::~IndexingService() = default;

void IndexingService::start()
{
    d->messaging->subscribe();
    connect(d->messaging, &MessagingClient::messageReceived,
            this, [this](const QByteArray& payload) {
                VideoRecord r = jsonToRecord(payload);
                if (r.cid.isEmpty()) return;
                r.indexedAt = QDateTime::currentDateTimeUtc();
                insertRecord(r);
                emit recordAdded(r);
            });
    emit indexReady();
}

void IndexingService::stop()
{
    d->messaging->unsubscribe();
}

void IndexingService::onUploadComplete(const QString& cid,
                                       double lat, double lon,
                                       const QDateTime& recordedAt,
                                       qint64 durationSeconds,
                                       qint64 sizeBytes,
                                       const QString& mimeType)
{
    VideoRecord r;
    r.cid             = cid;
    r.lat             = lat;
    r.lon             = lon;
    r.recordedAt      = recordedAt;
    r.indexedAt       = QDateTime::currentDateTimeUtc();
    r.durationSeconds = durationSeconds;
    r.sizeBytes       = sizeBytes;
    r.mimeType        = mimeType;
    r.userOwned       = true;

    insertRecord(r);
    d->messaging->publish(recordToJson(r));
    emit recordAdded(r);
}

QList<VideoRecord> IndexingService::query(const VideoFilter& filter) const
{
    auto db = QSqlDatabase::database("index_db");
    QSqlQuery q(db);

    QString sql = "SELECT cid, lat, lon, recorded_at, indexed_at, "
                  "duration_seconds, size_bytes, mime_type, user_owned, downloaded "
                  "FROM videos WHERE "
                  "lat >= ? AND lat <= ? AND lon >= ? AND lon <= ?";

    QList<QVariant> binds = {filter.latMin, filter.latMax, filter.lonMin, filter.lonMax};

    if (filter.fromTime.isValid()) {
        sql += " AND recorded_at >= ?";
        binds.append(filter.fromTime.toSecsSinceEpoch());
    }
    if (filter.toTime.isValid()) {
        sql += " AND recorded_at <= ?";
        binds.append(filter.toTime.toSecsSinceEpoch());
    }

    sql += " ORDER BY recorded_at DESC";

    if (filter.limit > 0) {
        sql += " LIMIT ?";
        binds.append(filter.limit);
    }

    q.prepare(sql);
    for (const auto& b : binds) q.addBindValue(b);
    q.exec();

    QList<VideoRecord> result;
    while (q.next()) {
        VideoRecord r;
        r.cid             = q.value(0).toString();
        r.lat             = q.value(1).toDouble();
        r.lon             = q.value(2).toDouble();
        r.recordedAt      = QDateTime::fromSecsSinceEpoch(q.value(3).toLongLong(),
                                                          QTimeZone::UTC);
        r.indexedAt       = QDateTime::fromSecsSinceEpoch(q.value(4).toLongLong(),
                                                          QTimeZone::UTC);
        r.durationSeconds = q.value(5).toLongLong();
        r.sizeBytes       = q.value(6).toLongLong();
        r.mimeType        = q.value(7).toString();
        r.userOwned       = q.value(8).toBool();
        r.downloaded      = q.value(9).toBool();
        result.append(r);
    }
    return result;
}

VideoRecord IndexingService::findByCid(const QString& cid) const
{
    auto db = QSqlDatabase::database("index_db");
    QSqlQuery q(db);
    q.prepare("SELECT cid, lat, lon, recorded_at, indexed_at, "
              "duration_seconds, size_bytes, mime_type, user_owned, downloaded "
              "FROM videos WHERE cid = ?");
    q.addBindValue(cid);
    q.exec();

    if (!q.next()) return {};

    VideoRecord r;
    r.cid             = q.value(0).toString();
    r.lat             = q.value(1).toDouble();
    r.lon             = q.value(2).toDouble();
    r.recordedAt      = QDateTime::fromSecsSinceEpoch(q.value(3).toLongLong(), QTimeZone::UTC);
    r.indexedAt       = QDateTime::fromSecsSinceEpoch(q.value(4).toLongLong(), QTimeZone::UTC);
    r.durationSeconds = q.value(5).toLongLong();
    r.sizeBytes       = q.value(6).toLongLong();
    r.mimeType        = q.value(7).toString();
    r.userOwned       = q.value(8).toBool();
    r.downloaded      = q.value(9).toBool();
    return r;
}

int IndexingService::count() const
{
    auto db = QSqlDatabase::database("index_db");
    QSqlQuery q(db);
    q.exec("SELECT COUNT(*) FROM videos");
    if (q.next()) return q.value(0).toInt();
    return 0;
}

}  // namespace VideoHotspot
