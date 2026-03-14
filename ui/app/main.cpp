/**
 * Standalone launcher for the Video Hotspot miniapp.
 * Use this during development to run the UI without Basecamp.
 */

#include "../plugin/VideoHotspotPlugin.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("VideoHotspot");
    app.setApplicationVersion("0.1.0");

    VideoHotspotPlugin plugin;
    plugin.initialize(nullptr); // no SDK context in standalone mode

    QQmlApplicationEngine engine;
    engine.load(plugin.rootQmlUrl());

    return app.exec();
}
