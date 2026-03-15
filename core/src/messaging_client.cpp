/**
 * MessagingClient — Logos Waku messaging integration.
 *
 * When built with LOGOS_CORE_AVAILABLE (default when SDK is present):
 *   - initLogos(LogosAPI*) MUST be called before publish/subscribe.
 *   - subscribe() calls Waku via LogosAPI::getClient("waku")
 *   - publish() sends real Waku messages
 *   - Incoming messages arrive via onEvent("WakuReplica","messageReceived")
 *   - If m_logosAPI is null (initLogos not called), publish() logs an error
 *     and persists to pending queue — no silent discard.
 *
 * When built WITHOUT LOGOS_CORE_AVAILABLE (SDK absent):
 *   - Uses SQLite for local message persistence (development / CI only).
 *   - This path is intentionally disabled when the SDK is present.
 *
 * See ADR-0004.
 */

#include <video_hotspot/messaging_client.h>

#ifdef LOGOS_CORE_AVAILABLE
#include <logos_api.h>
#include <logos_api_client.h>
#endif

#include <QDebug>

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
    bool connected  = false;
};

MessagingClient::MessagingClient(QObject* parent)
    : QObject(parent)
    , d(new MessagingClientPrivate)
{
    openMsgDb();
}

MessagingClient::~MessagingClient() = default;

void MessagingClient::initLogos(LogosAPI* logosAPI)
{
    m_logosAPI = logosAPI;
#ifdef LOGOS_CORE_AVAILABLE
    if (logosAPI) {
        qDebug() << "MessagingClient: LogosAPI connected — real Waku messaging enabled";
        qDebug() << "MessagingClient: topic=" << kTopic;
        d->connected = true;
        emit logosConnectionChanged(true);

        // Register incoming message handler from Waku
        auto* client = logosAPI->getClient("waku");
        auto* replica = client->requestObject("WakuReplica");
        if (replica) {
            client->onEvent(replica, this, "messageReceived",
                [this](const QString& /*eventName*/, const QVariantList& data) {
                    if (!data.isEmpty()) {
                        const QByteArray payload = data.first().toByteArray();
                        emit messageReceived(payload);
                        qDebug() << "MessagingClient: Waku message received, size="
                                 << payload.size();
                    }
                });
            qDebug() << "MessagingClient: registered Waku messageReceived handler";
        } else {
            qWarning() << "MessagingClient: WakuReplica not available";
        }
    } else {
        qWarning() << "MessagingClient: initLogos(nullptr) — Logos not connected;"
                   << "publish() calls will be queued in pending table";
        d->connected = false;
        emit logosConnectionChanged(false);
    }
#else
    Q_UNUSED(logosAPI)
    qDebug() << "MessagingClient: built without LOGOS_CORE_AVAILABLE — local-only mode";
#endif
}

bool MessagingClient::isLogosConnected() const
{
#ifdef LOGOS_CORE_AVAILABLE
    return m_logosAPI != nullptr;
#else
    return false;
#endif
}

void MessagingClient::subscribe()
{
    if (!d->subscribed) {
        d->subscribed = true;
#ifdef LOGOS_CORE_AVAILABLE
        if (m_logosAPI) {
            auto* client = m_logosAPI->getClient("waku");
            client->invokeRemoteMethod("WakuReplica", "subscribeToTopic",
                                       QVariant(QString(kTopic)));
            qDebug() << "MessagingClient: subscribed to Waku topic" << kTopic;
        } else {
            qWarning() << "MessagingClient: subscribe() called but Logos not connected;"
                       << "will subscribe when initLogos() is called";
        }
#endif
    }
}

void MessagingClient::unsubscribe()
{
    if (d->subscribed) {
        d->subscribed = false;
#ifdef LOGOS_CORE_AVAILABLE
        if (m_logosAPI) {
            auto* client = m_logosAPI->getClient("waku");
            client->invokeRemoteMethod("WakuReplica", "unsubscribeFromTopic",
                                       QVariant(QString(kTopic)));
        }
#endif
    }
}

QFuture<bool> MessagingClient::publish(const QByteArray& payload)
{
    QPromise<bool> promise;
    QFuture<bool> future = promise.future();
    promise.start();

#ifdef LOGOS_CORE_AVAILABLE
    if (m_logosAPI) {
        // ── Real Waku publish ─────────────────────────────────────────────
        auto* client = m_logosAPI->getClient("waku");
        QVariant result = client->invokeRemoteMethod(
            "WakuReplica", "publishMessage",
            QVariant(QString(kTopic)), QVariant(payload));

        const bool ok = result.isValid() && result.toBool();
        promise.addResult(ok);
        promise.finish();

        if (!ok) {
            qWarning() << "MessagingClient: Waku publishMessage failed — queuing for retry";
            auto db = QSqlDatabase::database("messaging_db");
            QSqlQuery q(db);
            q.prepare("INSERT INTO pending (topic, payload, created_at) VALUES (?, ?, ?)");
            q.addBindValue(QString(kTopic));
            q.addBindValue(payload);
            q.addBindValue(QDateTime::currentSecsSinceEpoch());
            q.exec();
            emit publishFailed(payload, "Waku publishMessage returned false");
        } else {
            qDebug() << "MessagingClient: Waku publish ok, size=" << payload.size();
        }
        return future;
    }

    // m_logosAPI is null — queue message for when connection is established
    qWarning() << "MessagingClient: publish() called before initLogos() — queuing message";
    {
        auto db = QSqlDatabase::database("messaging_db");
        QSqlQuery q(db);
        q.prepare("INSERT INTO pending (topic, payload, created_at) VALUES (?, ?, ?)");
        q.addBindValue(QString(kTopic));
        q.addBindValue(payload);
        q.addBindValue(QDateTime::currentSecsSinceEpoch());
        q.exec();
    }
    emit publishFailed(payload, "Logos not connected: message queued for retry");
    promise.addResult(false);
    promise.finish();
    return future;

#else
    // ── Local-only build (no SDK) ─────────────────────────────────────────
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
#endif
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
#ifdef LOGOS_CORE_AVAILABLE
    if (!m_logosAPI) return;

    auto db = QSqlDatabase::database("messaging_db");
    QSqlQuery q(db);
    q.exec("SELECT id, payload FROM pending ORDER BY created_at ASC");

    while (q.next()) {
        const int id = q.value(0).toInt();
        const QByteArray payload = q.value(1).toByteArray();

        auto* client = m_logosAPI->getClient("waku");
        QVariant result = client->invokeRemoteMethod(
            "WakuReplica", "publishMessage",
            QVariant(QString(kTopic)), QVariant(payload));

        if (result.isValid() && result.toBool()) {
            QSqlQuery del(db);
            del.prepare("DELETE FROM pending WHERE id = ?");
            del.addBindValue(id);
            del.exec();
            qDebug() << "MessagingClient: flushed pending message id=" << id;
        }
    }
#endif
}

bool MessagingClient::isConnected() const { return d->connected; }

}  // namespace VideoHotspot
