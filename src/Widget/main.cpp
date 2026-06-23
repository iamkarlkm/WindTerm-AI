#include <QApplication>
#include <QSurfaceFormat>
#include <QCommandLineParser>
#include "Widget/TerminalMainWindow.h"
#include "Renderer/PlatformDetector.h"
#include "Utility/Logger.h"

int main(int argc, char* argv[]) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setApplicationName("WindTerm Extensions");
    app.setApplicationVersion("0.2.0");
    app.setOrganizationName("WindTerm");

    QCommandLineParser parser;
    parser.setApplicationDescription("WindTerm - GPU Accelerated Terminal Emulator");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption logFileOption(
        QStringList() << "log-file",
        "Write log output to <file>",
        "file");
    QCommandLineOption logLevelOption(
        QStringList() << "log-level",
        "Minimum log level: debug, info, warning, error, critical, fatal, off",
        "level",
        "debug");

    parser.addOption(logFileOption);
    parser.addOption(logLevelOption);
    parser.process(app);

    QString logFile = parser.value(logFileOption);
    QString logLevelStr = parser.value(logLevelOption).toLower();

    LogLevel minLevel = LogLevel::Debug;
    if (logLevelStr == "info")        minLevel = LogLevel::Info;
    else if (logLevelStr == "warning") minLevel = LogLevel::Warning;
    else if (logLevelStr == "error")   minLevel = LogLevel::Error;
    else if (logLevelStr == "critical") minLevel = LogLevel::Critical;
    else if (logLevelStr == "fatal")   minLevel = LogLevel::Fatal;
    else if (logLevelStr == "off")     minLevel = LogLevel::Off;

    Logger::instance().init(logFile, minLevel, true);

    RendererBackend backend = PlatformDetector::detectBestBackend();
    LOG_DEBUG("Main") << "Detected backend:"
                      << PlatformDetector::backendToString(backend);

    TerminalMainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
