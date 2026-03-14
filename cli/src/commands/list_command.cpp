#include "commands.h"
#include "../headless_core.h"
#include "../output_formatter.h"

namespace VideoHotspot {

int runList(const QStringList& args, bool humanMode)
{
    Q_UNUSED(args)
    OutputFormatter fmt(humanMode);
    HeadlessCore core;

    const QList<VideoRecord> records = core.indexing->query();
    fmt.videoList(records);
    return 0;
}

}  // namespace VideoHotspot
