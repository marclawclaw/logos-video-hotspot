#pragma once

/**
 * VideoHotspotPlugin — logos-app IComponent UI plugin entry point.
 *
 * Implements the IComponent interface discovered from jimmy-claw/scala.
 * Plugin IID: "com.logos.component.IComponent"
 *
 * logos-app loads this plugin from:
 *   ~/.local/share/Logos/LogosAppNix/plugins/video_hotspot/libvideo_hotspot_plugin.so
 *
 * Lifecycle:
 *   1. logos-app dlopen()s the .so
 *   2. Qt resolves Q_PLUGIN_METADATA and instantiates VideoHotspotPlugin
 *   3. logos-app calls createWidget(logosAPI) to get the embedded widget
 *   4. On teardown: logos-app calls destroyWidget(widget)
 *
 * LogosAPI provides access to:
 *   - logos::messaging::Client (real-time pub/sub)
 *   - logos::storage::Client (content-addressed storage)
 *   - logos::identity::Manager (identity/keys)
 *
 * @see ADR-0001, interfaces/IComponent.h
 * @see https://github.com/jimmy-claw/scala/blob/main/src/scala_ui_component.h
 */

#include "interfaces/IComponent.h"

#include <QObject>

class VideoHotspotPlugin : public QObject, public IComponent {
    Q_OBJECT
    Q_INTERFACES(IComponent)
    Q_PLUGIN_METADATA(IID IComponent_iid FILE "video_hotspot_plugin.json")

public:
    explicit VideoHotspotPlugin(QObject* parent = nullptr);
    ~VideoHotspotPlugin() override;

    /**
     * Create the Video Hotspot embedded widget.
     * @param logosAPI  Logos SDK context (nullptr in mock/standalone mode).
     *
     * When logosAPI is non-null:
     *   - Passes it to StorageClient and MessagingClient constructors
     *   - Enables real Logos stack connectivity
     *
     * When logosAPI is null (mock mode):
     *   - Falls back to local filesystem storage (SQLite + file copy)
     *   - Suitable for development without a running Logos node
     */
    QWidget* createWidget(LogosAPI* logosAPI = nullptr) override;

    /**
     * Destroy the widget and clean up core services.
     */
    void destroyWidget(QWidget* widget) override;
};
