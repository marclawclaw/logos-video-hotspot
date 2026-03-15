/**
 * MessagingClient — Logos Waku messaging integration.
 *
 * Integration strategy:
 *   When built with LOGOS_CORE_AVAILABLE and initLogos() has been called:
 *     - subscribe() calls Waku via LogosAPI::getClient("waku"):
 *       invokeRemoteMethod("WakuReplica", "subscribeToTopic", kTopic)
 *     - publish() sends real Waku messages:
 *       invokeRemoteMethod("WakuReplica", "publishMessage", {kTopic, payload})
 *     - Incoming messages arrive via onEvent("WakuReplica", "messageReceived")
 *       and are fanned out via the messageReceived() signal.
 *
 *   When initLogos() has not been called (standalone / no logos-app):
 *     - Falls back to SQLite mock (messages stored locally, looped back)
 *     - Useful for development without a running Logos node
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
    bool connected  = true;  // always "connected" in mock
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

        // Register incoming message handler
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
            qDebug() << "MessagingClient: subscribed to Waku 'messageReceived' events";
        } else {
            qWarning() << "MessagingClient: WakuReplica not available";
        }
    } else {
        qDebug() << "MessagingClient: initLogos called with null — using local mock";
        emit logosConnectionChanged(false);
    }
#else
    Q_UNUSED(logosAPI)
    qDebug() << "MessagingClient: built without LOGOS_CORE_AVAILABLE — local mock only";
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
            return;
        }
#endif
        // Mock: no-op (subscribe is implicit in mock)
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
        // ── Real Waku publish via LogosAPI ────────────────────────────────
        auto* client = m_logosAPI->getClient("waku");
        QVariant result = client->invokeRemoteMethod(
            "WakuReplica", "publishMessage",
            QVariant(QString(kTopic)), QVariant(payload));

        const bool ok = result.isValid() && result.toBool();
        promise.addResult(ok);
        promise.finish();

        if (!ok) {
            qWarning() << "MessagingClient: Waku publishMessage failed — queueing for retry";
            // Persist to pending table for retry on reconnect
            auto db = QSqlDatabase::database("messaging_db");
            QSqlQuery q(db);
            q.prepare("INSERT INTO pending (topic, payload, created_at) "
                      "VALUES (?, ?, ?)");
            q.addBindValue(QString(kTopic));
            q.addBindValue(payload);
            q.addBindValue(QDateTime::currentSecsSinceEpoch());
            q.exec();
            emit publishFailed(payload, "Waku publishMessage returned false");
        } else {
            qDebug() << "MessagingClient: Waku publish success, size=" << payload.size();
        }
        return future;
    }
#endif

    // ── Local mock: synchronous DB write + signal fan-out ─────────────────
    // No real network I/O; keep synchronous to avoid races in tests.
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
#ifdef LOGOS_CORE_AVAILABLE
    if (m_logosAPI) {
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
        return;
    }
#endif
    // Mock: nothing is truly pending
}

bool MessagingClient::isConnected() const { return d->connected; }

}  // namespace VideoHotspot
