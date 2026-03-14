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
 *   video-hotspot list [--json] [--human]
 *   video-hotspot download <cid>
 *   video-hotspot status
 *   video-hotspot cache clear
 */

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>
#include <cstdlib>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("video-hotspot");
    app.setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Video Hotspot — privacy-preserving video upload and geo-mapping on the Logos stack");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("command",
        "upload | upload-folder | monitor | list | download | status | cache");

    // Global flags
    QCommandLineOption humanOutput({"H", "human"}, "Human-readable output (default: JSON)");
    parser.addOption(humanOutput);

    parser.parse(app.arguments());

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        parser.showHelp(EXIT_FAILURE);
    }

    const QString command = args.first();

    // TODO: dispatch to command handlers
    // Each command handler initialises Logos Core in headless mode,
    // runs its operation, then calls app.quit().

    QTextStream err(stderr);
    err << "Command '" << command << "' not yet implemented.\n";
    return EXIT_FAILURE;
}
