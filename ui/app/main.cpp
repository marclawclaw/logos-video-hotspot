/**
 * Standalone launcher for the Video Hotspot miniapp.
 * Use this during development to run the UI without Basecamp.
 */

#include "../plugin/VideoHotspotPlugin.h"

#include <QApplication>
#include <QWidget>

int main(int argc, char* argv[])
{
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
