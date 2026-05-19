#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>
#include "Widget/TerminalMainWindow.h"
#include "Renderer/PlatformDetector.h"

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
    
    RendererBackend backend = PlatformDetector::detectBestBackend();
    qDebug() << "[Main] Detected backend:" << PlatformDetector::backendToString(backend);
    
    TerminalMainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}
