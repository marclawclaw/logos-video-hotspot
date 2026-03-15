/**
 * VideoHotspotPlugin — logos-app IComponent entry point.
 *
 * Wiring:
 *   createWidget(logosAPI)
 *     ├── StorageClient::initLogos(logosAPI)
 *     │     ├── [LOGOS_CORE_AVAILABLE] → LogosAPI::getClient("codex")
 *     │     │     invokeRemoteMethod("CodexReplica", "storeFile"|"fetchFile"|"hasCid")
 *     │     └── [fallback] → local filesystem (SHA-256 CID, file copy)
 *     ├── MessagingClient::initLogos(logosAPI)
 *     │     ├── [LOGOS_CORE_AVAILABLE] → LogosAPI::getClient("waku")
 *     │     │     invokeRemoteMethod("WakuReplica", "subscribeToTopic"|"publishMessage")
 *     │     │     onEvent("WakuReplica", "messageReceived", ...)
 *     │     └── [fallback] → SQLite mock (local loopback)
 *     └── QQuickWidget with QML context properties:
 *           "storageClient"   → VideoHotspot::StorageClient*
 *           "messagingClient" → VideoHotspot::MessagingClient*
 *           "logosConnected"  → bool (true = real Codex+Waku active)
 */

#include "VideoHotspotPlugin.h"

#include <video_hotspot/storage_client.h>
#include <video_hotspot/messaging_client.h>

#include <QDebug>
#include <QQuickWidget>
#include <QQmlContext>
#include <QUrl>

#ifdef LOGOS_CORE_AVAILABLE
#include <logos_api.h>
#endif

VideoHotspotPlugin::VideoHotspotPlugin(QObject* parent)
    : QObject(parent)
{
    qDebug() << "VideoHotspotPlugin: loaded"
#ifdef LOGOS_CORE_AVAILABLE
             << "(LOGOS_CORE_AVAILABLE — real Codex+Waku integration)"
#else
             << "(local mock — no logos-cpp-sdk at build time)"
#endif
    ;
}

VideoHotspotPlugin::~VideoHotspotPlugin() = default;

QWidget* VideoHotspotPlugin::createWidget(LogosAPI* logosAPI)
{
    const bool logosAvailable = (logosAPI != nullptr);
    qDebug() << "VideoHotspotPlugin::createWidget — LogosAPI:"
             << (logosAvailable ? "connected" : "null (mock mode)");

    // ── Core services ──────────────────────────────────────────────────────
    // Parent to the widget so they are destroyed together.
    // (We create the widget first so we have a parent; swap below.)
    auto* quickWidget = new QQuickWidget();
    quickWidget->setMinimumSize(1024, 768);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    auto* storage   = new VideoHotspot::StorageClient(quickWidget);
    auto* messaging = new VideoHotspot::MessagingClient(quickWidget);

    // ── Wire LogosAPI to core services ────────────────────────────────────
    // initLogos() is a no-op when logosAPI is null.
    // When LOGOS_CORE_AVAILABLE is defined and logosAPI != null, each client
    // will use real Codex / Waku calls instead of the local mock.
    storage->initLogos(logosAPI);
    messaging->initLogos(logosAPI);

    // Auto-subscribe on startup (idempotent; no-op in mock mode without api)
    messaging->subscribe();

    // ── Expose to QML ─────────────────────────────────────────────────────
    QQmlContext* ctx = quickWidget->rootContext();

    // C++ backend objects — QML can call Q_INVOKABLE methods directly
    ctx->setContextProperty("storageClient",   storage);
    ctx->setContextProperty("messagingClient", messaging);

    // Convenience boolean so QML can show real vs mock status in the UI
    ctx->setContextProperty("logosConnected", logosAvailable);

    qDebug() << "VideoHotspotPlugin: context properties set:"
             << "storageClient, messagingClient, logosConnected=" << logosAvailable;

    // ── Load QML root ─────────────────────────────────────────────────────
    quickWidget->setSource(QUrl("qrc:/qml/qml/VideoHotspotApp.qml"));

    if (quickWidget->status() == QQuickWidget::Error) {
        qCritical() << "VideoHotspotPlugin: QML load errors:";
        for (const auto& err : quickWidget->errors()) {
            qCritical() << " " << err.toString();
        }
    }

    return quickWidget;
}

void VideoHotspotPlugin::destroyWidget(QWidget* widget)
{
    // Core services (StorageClient, MessagingClient) are parented to the widget
    // and will be destroyed automatically here.
    delete widget;
}
