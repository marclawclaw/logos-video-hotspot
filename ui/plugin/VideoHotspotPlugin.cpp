#include "VideoHotspotPlugin.h"

#include <QUrl>

VideoHotspotPlugin::VideoHotspotPlugin(QObject* parent)
    : QObject(parent)
{
}

VideoHotspotPlugin::~VideoHotspotPlugin() = default;

void VideoHotspotPlugin::initialize(void* /*sdkContext*/)
{
    // TODO: cast sdkContext to the logos-cpp-sdk context type once
    // logos-app plugin interface headers are added as a dependency.
    // Initialise StorageClient, MessagingClient, IndexingService, UploadQueue here.
}

QUrl VideoHotspotPlugin::rootQmlUrl() const
{
    return QUrl("qrc:/qml/VideoHotspotApp.qml");
}

void VideoHotspotPlugin::shutdown()
{
    // TODO: stop IndexingService, flush MessagingClient pending queue
}
