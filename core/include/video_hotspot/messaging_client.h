#pragma once

#include <QByteArray>
#include <QFuture>
#include <QObject>
#include <QString>
#include <memory>

class LogosAPI;  // forward declaration — real header in logos-cpp-sdk

namespace VideoHotspot {

struct MessagingClientPrivate;

/**
 * @brief Thin async wrapper around Logos Messaging for the video-hotspot topic.
 *
 * Handles publish with local retry queue (persisted to SQLite `pending_messages`)
 * and subscribe with message fan-out via the messageReceived signal.
 *
 * @see ADR-0004
 */
class MessagingClient : public QObject {
    Q_OBJECT

public:
    static constexpr const char* kTopic = "video-hotspot/v1/index";

    explicit MessagingClient(QObject* parent = nullptr);
    ~MessagingClient() override;

    /**
     * Wire this client to the Logos SDK.
     *
     * When called with a non-null logosAPI (and built with LOGOS_CORE_AVAILABLE),
     * publish/subscribe use real Waku via LogosAPI::getClient("waku").
     * Falls back to SQLite mock otherwise.
     *
     * Expected module/object/method names:
     *   module:     "waku"
     *   subscribe:  invokeRemoteMethod("WakuReplica", "subscribeToTopic", {topic})
     *   publish:    invokeRemoteMethod("WakuReplica", "publishMessage",   {topic, payload})
     *   onMessage:  onEvent(replica, this, "messageReceived", callback)
     *
     * @see https://github.com/logos-co/logos-cpp-sdk
     */
    void initLogos(LogosAPI* logosAPI);

    /// Returns true when connected to real Logos Waku messaging.
    bool isLogosConnected() const;

    /// Subscribe to the video-hotspot topic. Idempotent.
    void subscribe();

    /// Unsubscribe.
    void unsubscribe();

    /**
     * Publish a message payload to the topic.
     * If the network is unavailable, the message is queued locally and
     * retried with exponential backoff when connectivity is restored.
     *
     * @return Future resolving to true on successful delivery.
     */
    QFuture<bool> publish(const QByteArray& payload);

    /// Number of messages currently queued for retry.
    int pendingCount() const;

    /// Flush pending messages immediately (e.g., called on reconnect event).
    void flushPending();

    bool isConnected() const;

signals:
    void logosConnectionChanged(bool connected);
    void messageReceived(const QByteArray& payload);
    void connectionStateChanged(bool connected);
    void publishFailed(const QByteArray& payload, const QString& error);

private:
    std::unique_ptr<MessagingClientPrivate> d;
    LogosAPI* m_logosAPI = nullptr;
};

}  // namespace VideoHotspot
