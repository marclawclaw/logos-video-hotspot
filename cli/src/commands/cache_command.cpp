#include "commands.h"
#include "../headless_core.h"
#include "../output_formatter.h"

#include <QEventLoop>
#include <QFutureWatcher>
#include <QTimer>

namespace VideoHotspot {

int runCacheClear(const QStringList& args, bool humanMode)
{
    Q_UNUSED(args)
    OutputFormatter fmt(humanMode);
    HeadlessCore core;

    const QList<CacheEntry> before = core.storage->cachedEntries();
    qint64 cachedBytes = 0;
    int cachedCount = 0;
    for (const auto& e : before) {
        if (!e.userOwned) {
            cachedBytes += e.sizeBytes;
            ++cachedCount;
        }
    }

    if (cachedCount == 0) {
        // Return structured response even when empty so callers can inspect fields
        QJsonObject obj;
        obj["status"]        = "ok";
        obj["cleared_count"] = 0;
        obj["cleared_bytes"] = 0;
        fmt.success(obj);
        return 0;
    }

    // Evict all cached (non-user-owned) entries
    QEventLoop loop;
    int evicted = 0;
    for (const auto& e : before) {
        if (e.userOwned) continue;
        auto* w = new QFutureWatcher<void>(&loop);
        QObject::connect(w, &QFutureWatcher<void>::finished, &loop, [&evicted, w]() {
            ++evicted;
            w->deleteLater();
        });
        w->setFuture(core.storage->evict(e.cid));
        // Process events to allow futures to complete
        loop.processEvents();
    }

    // Wait briefly for async evictions to complete
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();

    QJsonObject obj;
    obj["status"]        = "ok";
    obj["cleared_count"] = cachedCount;
    obj["cleared_bytes"] = cachedBytes;
    fmt.success(obj);
    return 0;
}

}  // namespace VideoHotspot
