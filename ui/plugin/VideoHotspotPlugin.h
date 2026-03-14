#pragma once

#include <QObject>
#include <QtPlugin>

/**
 * @brief Qt plugin entry point for the Video Hotspot miniapp.
 *
 * This class is loaded by Basecamp (logos-app) via Qt's plugin system.
 * It is responsible for bootstrapping the core services and returning
 * the root QML component or widget for embedding in Basecamp's navigation.
 *
 * Plugin lifecycle:
 *   1. Basecamp dlopen()s the .so / .dylib
 *   2. Qt resolves Q_PLUGIN_METADATA and instantiates VideoHotspotPlugin
 *   3. Basecamp calls initialize() with a reference to the Logos SDK context
 *   4. Basecamp calls rootComponent() to embed the UI
 *   5. On teardown: Basecamp calls shutdown()
 */
class VideoHotspotPlugin : public QObject {
    Q_OBJECT
    // IID and metadata will be filled in once the logos-app plugin interface
    // headers are available as a dependency.
    // Q_PLUGIN_METADATA(IID "co.logos.MiniAppInterface/1.0"
    //                   FILE "video_hotspot_plugin.json")

public:
    explicit VideoHotspotPlugin(QObject* parent = nullptr);
    ~VideoHotspotPlugin() override;

    /// Called by Basecamp after loading; receive SDK context.
    /// @param sdkContext  Opaque pointer to the logos-cpp-sdk context object.
    void initialize(void* sdkContext);

    /// Return the root QML URL for embedding.
    QUrl rootQmlUrl() const;

    /// Clean up before unload.
    void shutdown();
};
