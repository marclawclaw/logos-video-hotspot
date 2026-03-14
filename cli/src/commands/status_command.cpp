#include "commands.h"
#include "../headless_core.h"
#include "../output_formatter.h"

namespace VideoHotspot {

int runStatus(const QStringList& args, bool humanMode)
{
    Q_UNUSED(args)
    OutputFormatter fmt(humanMode);
    HeadlessCore core;

    const bool connected      = core.messaging->isConnected();
    const StorageStats stats  = core.storage->stats();
    const int indexCount      = core.indexing->count();
    const int pendingMessages = core.messaging->pendingCount();

    fmt.statusOutput(connected, stats, indexCount, pendingMessages);
    return 0;
}

}  // namespace VideoHotspot
