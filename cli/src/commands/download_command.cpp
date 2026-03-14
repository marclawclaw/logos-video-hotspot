#include "commands.h"
#include "../headless_core.h"
#include "../output_formatter.h"

#include <QDir>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QTimer>

namespace VideoHotspot {

int runDownload(const QStringList& args, bool humanMode)
{
    OutputFormatter fmt(humanMode);

    if (args.isEmpty()) {
        return fmt.error("Usage: video-hotspot download <cid>");
    }

    const QString cid = args.first();
    const QString destDir = args.size() > 1 ? args[1]
        : QDir::currentPath() + "/downloads";

    HeadlessCore core;

    // Check it's in the index
    const VideoRecord record = core.indexing->findByCid(cid);
    if (record.cid.isEmpty()) {
        return fmt.error("CID not found in index: " + cid);
    }

    QEventLoop loop;
    QString resultPath;
    QString resultError;

    auto* watcher = new QFutureWatcher<QString>();
    QObject::connect(watcher, &QFutureWatcher<QString>::finished,
                     &loop, [&]() {
                         resultPath = watcher->result();
                         watcher->deleteLater();
                         loop.quit();
                     });

    QTimer::singleShot(60000, &loop, [&]() {
        resultError = "Download timeout";
        loop.quit();
    });

    watcher->setFuture(core.storage->download(cid, destDir));
    loop.exec();

    if (!resultError.isEmpty()) {
        return fmt.error(resultError);
    }
    if (resultPath.isEmpty()) {
        return fmt.error("Download failed for CID: " + cid);
    }

    QJsonObject obj;
    obj["status"]     = "ok";
    obj["cid"]        = cid;
    obj["local_path"] = resultPath;
    fmt.success(obj);
    return 0;
}

}  // namespace VideoHotspot
