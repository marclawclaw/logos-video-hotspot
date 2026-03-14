/**
 * Standalone launcher for the Video Hotspot miniapp.
 * Use this during development to run the UI without Basecamp.
 *
 * Platform notes (headless / Xvfb):
 *   DISPLAY=:99 QT_QPA_PLATFORM=xcb ./build/ui/app/video-hotspot-app
 *
 * The app builds with BUILD_UI_PLUGIN=ON so that createWidget() returns
 * a real QQuickWidget rather than nullptr.
 */

#include "../plugin/VideoHotspotPlugin.h"

#include <QApplication>
#include <QWidget>

int main(int argc, char* argv[])
{
    // xcb platform plugin works under Xvfb; offscreen has no visible output
    QApplication app(argc, argv);
    app.setApplicationName("VideoHotspot");
    app.setApplicationVersion("0.1.0");

    VideoHotspotPlugin plugin;

    // Create widget in standalone/mock mode (no Logos SDK context)
    QWidget* widget = plugin.createWidget(nullptr);
    if (!widget) {
        return 1;
    }

    widget->setWindowTitle("Video Hotspot — Standalone Demo");
    widget->resize(1280, 800);
    widget->show();

    int result = app.exec();

    plugin.destroyWidget(widget);
    return result;
}
