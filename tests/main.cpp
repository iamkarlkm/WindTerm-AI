#include <QApplication>
#include "RendererTestWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("WindTerm GPU Renderer Test");
    app.setApplicationVersion("0.2.0");
    
    RendererTestWindow window;
    window.show();
    
    return app.exec();
}
