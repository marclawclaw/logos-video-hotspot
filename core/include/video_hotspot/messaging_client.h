#pragma once

#include <QByteArray>
#include <QFuture>
#include <QObject>
#include <QString>

namespace VideoHotspot {

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
    void messageReceived(const QByteArray& payload);
    void connectionStateChanged(bool connected);
    void publishFailed(const QByteArray& payload, const QString& error);
};

}  // namespace VideoHotspot
