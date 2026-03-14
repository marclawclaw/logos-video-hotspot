/**
 * MessagingClient — mock implementation using SQLite.
 *
 * Mock strategy (no Logos Messaging SDK):
 *   - Messages are stored in SQLite table `messages` at
 *     $XDG_DATA_HOME/VideoHotspot/messaging.db
 *   - publish() stores the message and emits messageReceived() locally
 *   - subscribe/unsubscribe are no-ops in mock mode
 *
 * When Logos Messaging SDK is available:
 *   - Replace mock publish/subscribe with real logos::messaging::Client calls
 *   - Keep the QFuture-based async API and signal names unchanged
 *
 * See ADR-0004.
 */

#include <video_hotspot/messaging_client.h>

#include <QDir>
#include <QPromise>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace VideoHotspot {

constexpr const char* MessagingClient::kTopic;

// ── DB ────────────────────────────────────────────────────────────────────────

static QString msgDbPath()
{
    const QString d = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(d);
    return d + "/messaging.db";
}

static QSqlDatabase openMsgDb()
{
    const QString conn = "messaging_db";
    if (QSqlDatabase::contains(conn)) return QSqlDatabase::database(conn);

    auto db = QSqlDatabase::addDatabase("QSQLITE", conn);
    db.setDatabaseName(msgDbPath());
    db.open();

    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS messages ("
           "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  topic      TEXT NOT NULL,"
           "  payload    BLOB NOT NULL,"
           "  created_at INTEGER NOT NULL,"
           "  delivered  INTEGER NOT NULL DEFAULT 0"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS pending ("
           "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  topic      TEXT NOT NULL,"
           "  payload    BLOB NOT NULL,"
           "  created_at INTEGER NOT NULL,"
           "  retries    INTEGER NOT NULL DEFAULT 0"
           ")");
    return db;
}

// ── MessagingClient ───────────────────────────────────────────────────────────

struct MessagingClientPrivate {
    bool subscribed = false;
    bool connected  = true;  // always "connected" in mock
};

MessagingClient::MessagingClient(QObject* parent)
    : QObject(parent)
    , d(new MessagingClientPrivate)
{
    openMsgDb();
}

MessagingClient::~MessagingClient() = default;

void MessagingClient::subscribe()
{
    if (!d->subscribed) {
        d->subscribed = true;
        // In real SDK: subscribe to kTopic
    }
}

void MessagingClient::unsubscribe()
{
    d->subscribed = false;
}

QFuture<bool> MessagingClient::publish(const QByteArray& payload)
{
    // Mock implementation: synchronous DB write + local signal fan-out.
    // No real network I/O, so async threading adds no value and risks
    // use-after-free races in tests. Keep it synchronous.
    // Production implementation would replace this with real SDK calls.
    QPromise<bool> promise;
    QFuture<bool> future = promise.future();
    promise.start();

    auto db = QSqlDatabase::database("messaging_db");
    QSqlQuery q(db);
    q.prepare("INSERT INTO messages (topic, payload, created_at, delivered) "
              "VALUES (?, ?, ?, 1)");
    q.addBindValue(QString(kTopic));
    q.addBindValue(payload);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    const bool ok = q.exec();

    promise.addResult(ok);
    promise.finish();

    if (ok) {
        if (d->subscribed) emit messageReceived(payload);
    } else {
        emit publishFailed(payload, db.lastError().text());
    }
    return future;
}

int MessagingClient::pendingCount() const
{
    auto db = QSqlDatabase::database("messaging_db");
    QSqlQuery q(db);
    q.exec("SELECT COUNT(*) FROM pending");
    if (q.next()) return q.value(0).toInt();
    return 0;
}

void MessagingClient::flushPending()
{
    // In mock: nothing is truly pending since we're always "connected"
    // In real SDK: re-publish all rows in pending table then clear them
}

bool MessagingClient::isConnected() const { return d->connected; }

}  // namespace VideoHotspot
