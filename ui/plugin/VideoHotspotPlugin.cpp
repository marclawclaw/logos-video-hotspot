#include "VideoHotspotPlugin.h"

#ifdef BUILD_UI_PLUGIN
#include <QQuickWidget>
#include <QQmlContext>
#include <QUrl>
#endif

VideoHotspotPlugin::VideoHotspotPlugin(QObject* parent)
    : QObject(parent)
{
}

VideoHotspotPlugin::~VideoHotspotPlugin() = default;

QWidget* VideoHotspotPlugin::createWidget(LogosAPI* logosAPI)
{
#ifdef BUILD_UI_PLUGIN
    // Full UI implementation (requires Qt Quick)
    auto* quickWidget = new QQuickWidget();
    quickWidget->setMinimumSize(1024, 768);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // TODO: Create core modules, pass logosAPI to StorageClient/MessagingClient
    // auto* storage   = new VideoHotspot::StorageClient(logosAPI);
    // auto* messaging = new VideoHotspot::MessagingClient(logosAPI);
    // quickWidget->rootContext()->setContextProperty("videoHotspot", backend);
    Q_UNUSED(logosAPI)

    // Load the embedded QML root (Rectangle — embeds cleanly in QQuickWidget)
    quickWidget->setSource(QUrl("qrc:/qml/qml/VideoHotspotApp.qml"));

    return quickWidget;
#else
    Q_UNUSED(logosAPI)
    // UI plugin built without Qt Quick — return nullptr
    // This should not happen in a properly configured build
    return nullptr;
#endif
}

void VideoHotspotPlugin::destroyWidget(QWidget* widget)
{
    delete widget;
}
