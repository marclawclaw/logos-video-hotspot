#pragma once

#include <QStringList>

namespace VideoHotspot {

/// All command entry points. Each returns an exit code (0 = success).
/// args: positional arguments after the command name.
/// humanMode: true if --human flag was set.

int runUpload(const QStringList& args, bool humanMode);
int runUploadFolder(const QStringList& args, bool humanMode);
int runMonitor(const QStringList& args, bool humanMode);
int runList(const QStringList& args, bool humanMode);
int runDownload(const QStringList& args, bool humanMode);
int runStatus(const QStringList& args, bool humanMode);
int runCacheClear(const QStringList& args, bool humanMode);

}  // namespace VideoHotspot
