/**
 * video-hotspot CLI — headless frontend for the Video Hotspot core.
 *
 * Uses QCoreApplication (no GUI) to satisfy Qt's event loop requirements
 * while keeping the process windowless. See ADR-0002.
 *
 * Usage:
 *   video-hotspot upload <file>
 *   video-hotspot upload-folder <path>
 *   video-hotspot monitor <path>
 *   video-hotspot list
 *   video-hotspot download <cid> [dest-dir]
 *   video-hotspot status
 *   video-hotspot cache clear
 *
 * Flags:
 *   -H, --human   Human-readable output (default: JSON)
 */

#include "commands/commands.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <cstdlib>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("video-hotspot");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("VideoHotspot");
    app.setOrganizationDomain("logos.co");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Video Hotspot — privacy-preserving video upload and geo-mapping on the Logos stack\n"
        "\nCommands:\n"
        "  upload <file>          Upload a single video file\n"
        "  upload-folder <path>   Upload all videos in a folder\n"
        "  monitor <path>         Watch a folder and auto-upload new videos\n"
        "  list                   List all indexed videos\n"
        "  download <cid> [dir]   Download a video by CID\n"
        "  status                 Show node status and storage stats\n"
        "  cache clear            Clear all cached (non-user-owned) videos"
    );
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("command",
        "upload | upload-folder | monitor | list | download | status | cache");
    parser.addPositionalArgument("args", "Command-specific arguments", "[args...]");

    QCommandLineOption humanOutput({"H", "human"},
        "Human-readable output (default: JSON)");
    parser.addOption(humanOutput);

    parser.parse(app.arguments());

    const QStringList positional = parser.positionalArguments();
    if (positional.isEmpty()) {
        parser.showHelp(EXIT_FAILURE);
    }

    const QString command = positional.first();
    const QStringList args = positional.mid(1);
    const bool human = parser.isSet(humanOutput);

    if (command == "upload") {
        return VideoHotspot::runUpload(args, human);
    } else if (command == "upload-folder") {
        return VideoHotspot::runUploadFolder(args, human);
    } else if (command == "monitor") {
        return VideoHotspot::runMonitor(args, human);
    } else if (command == "list") {
        return VideoHotspot::runList(args, human);
    } else if (command == "download") {
        return VideoHotspot::runDownload(args, human);
    } else if (command == "status") {
        return VideoHotspot::runStatus(args, human);
    } else if (command == "cache") {
        if (args.isEmpty() || args.first() == "clear") {
            return VideoHotspot::runCacheClear(args, human);
        }
        QTextStream(stderr) << "Unknown cache subcommand: " << args.first() << "\n";
        return EXIT_FAILURE;
    } else {
        QTextStream(stderr) << "Unknown command: " << command
                            << "\nRun with --help for usage.\n";
        return EXIT_FAILURE;
    }
}
