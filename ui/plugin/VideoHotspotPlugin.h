#pragma once

/**
 * VideoHotspotPlugin — logos-app IComponent UI plugin entry point.
 *
 * Implements the IComponent interface from logos-co/logos-app.
 * Plugin IID: "com.logos.component.IComponent"
 *
 * logos-app loads this plugin from:
 *   Non-portable: ~/.local/share/LogosAppNix/plugins/video_hotspot/video_hotspot.so
 *   Portable:     ~/.local/share/LogosApp/plugins/video_hotspot/video_hotspot.so
 *
 * Lifecycle:
 *   1. logos-app (or QPluginLoader) dlopen()s the .so
 *   2. Qt resolves Q_PLUGIN_METADATA and instantiates VideoHotspotPlugin
 *   3. logos-app calls createWidget(logosAPI) to get the embedded widget
 *   4. On teardown: logos-app calls destroyWidget(widget)
 *
 * LogosAPI integration (logos-cpp-sdk):
 *   - getClient("codex")  → Codex storage (upload/download/exists)
 *   - getClient("waku")   → Waku messaging (subscribe/publish/receive)
 *   Enabled at build time with -DLOGOS_CORE_AVAILABLE=1 and linked against
 *   logos-cpp-sdk. Falls back to local filesystem mock when unavailable.
 *
 * @see ADR-0001, interfaces/IComponent.h
 * @see https://github.com/logos-co/logos-app/blob/master/app/interfaces/IComponent.h
 * @see https://github.com/logos-co/logos-cpp-sdk
 */

#include "interfaces/IComponent.h"

#include <QObject>

namespace VideoHotspot {
class StorageClient;
class MessagingClient;
}

class VideoHotspotPlugin : public QObject, public IComponent {
    Q_OBJECT
    Q_INTERFACES(IComponent)
    Q_PLUGIN_METADATA(IID IComponent_iid FILE "video_hotspot.json")

public:
    explicit VideoHotspotPlugin(QObject* parent = nullptr);
    ~VideoHotspotPlugin() override;

    /**
     * Create the Video Hotspot embedded widget.
     *
     * @param logosAPI  Logos SDK context from logos-app. May be null when
     *                  loaded outside logos-app (e.g. during development).
     *
     * When logosAPI is non-null (built with LOGOS_CORE_AVAILABLE):
     *   - Calls StorageClient::initLogos(logosAPI)  → real Codex storage
     *   - Calls MessagingClient::initLogos(logosAPI) → real Waku messaging
     *   - Both clients expose isLogosConnected() for QML status display
     *
     * When logosAPI is null or LOGOS_CORE_AVAILABLE is not defined:
     *   - StorageClient uses local filesystem (SHA-256 CID, file copy)
     *   - MessagingClient uses SQLite mock (local loopback)
     *   - Suitable for development without a running Logos node
     */
    QWidget* createWidget(LogosAPI* logosAPI = nullptr) override;

    /**
     * Destroy the widget and clean up core services.
     */
    void destroyWidget(QWidget* widget) override;
};
