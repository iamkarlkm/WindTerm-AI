#include <QApplication>
#include <QSurfaceFormat>
#include <QCommandLineParser>
#include "Widget/TerminalMainWindow.h"
#include "Widget/TerminalWidget.h"
#include "Widget/TerminalPane.h"
#include "Renderer/PlatformDetector.h"
#include "Utility/Logger.h"
#include "Utility/TerminalOutputFilter.h"
#include "Utility/TerminalOutputServer.h"

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
    QCommandLineOption filterConfigOption(
        QStringList() << "filter-config",
        "Terminal output filter rules (JSON file)",
        "file");
    QCommandLineOption filterServerPortOption(
        QStringList() << "filter-server-port",
        "Start WebSocket server for dynamic filter subscriptions on <port>",
        "port",
        "0");

    parser.addOption(logFileOption);
    parser.addOption(logLevelOption);
    parser.addOption(filterConfigOption);
    parser.addOption(filterServerPortOption);
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

    QString filterConfig = parser.value(filterConfigOption);
    TerminalOutputFilter::instance().init(filterConfig);

    quint16 filterServerPort = parser.value(filterServerPortOption).toUShort();
    if (filterServerPort > 0) {
        if (TerminalOutputServer::instance().start(filterServerPort)) {
            QObject::connect(
                &TerminalOutputFilter::instance(),
                &TerminalOutputFilter::lineProcessed,
                &TerminalOutputServer::instance(),
                &TerminalOutputServer::feedLine);
        }
    }

    RendererBackend backend = PlatformDetector::detectBestBackend();
    LOG_DEBUG("Main") << "Detected backend:"
                      << PlatformDetector::backendToString(backend);

    TerminalMainWindow mainWindow;

    if (filterServerPort > 0 && TerminalOutputServer::instance().isRunning()) {
        QObject::connect(
            &TerminalOutputServer::instance(),
            &TerminalOutputServer::commandRequested,
            &mainWindow, [&mainWindow](const QString &command) {
                TerminalWidget *tw = mainWindow.activeTerminal();
                if (!tw) return;
                TerminalPane *pane = tw->activePane();
                if (!pane) return;
                TerminalSession *session = pane->session();
                if (session && session->isRunning()) {
                    session->write(command.toUtf8());
                }
            });

        QObject::connect(
            &TerminalOutputServer::instance(),
            &TerminalOutputServer::rawBytesRequested,
            &mainWindow, [&mainWindow](const QByteArray &data) {
                TerminalWidget *tw = mainWindow.activeTerminal();
                if (!tw) return;
                TerminalPane *pane = tw->activePane();
                if (!pane) return;
                TerminalSession *session = pane->session();
                if (session && session->isRunning()) {
                    session->write(data);
                }
            });

        QObject::connect(
            &TerminalOutputServer::instance(),
            &TerminalOutputServer::signalRequested,
            &mainWindow, [&mainWindow](int sig) {
                TerminalWidget *tw = mainWindow.activeTerminal();
                if (!tw) return;
                TerminalPane *pane = tw->activePane();
                if (!pane) return;
                TerminalSession *session = pane->session();
                if (session && session->isRunning()) {
                    session->sendSignal(sig);
                }
            });
    }
    mainWindow.show();

    return app.exec();
}
