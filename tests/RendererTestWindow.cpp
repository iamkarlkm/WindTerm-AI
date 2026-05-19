#include "RendererTestWindow.h"
#include <QDebug>
#include <QDateTime>
#include <QVBoxLayout>
#include <QLabel>

RendererTestWindow::RendererTestWindow(QWidget* parent)
    : QOpenGLWidget(parent), m_renderer(nullptr), m_textBuffer(nullptr),
      m_currentLine(0), m_fps(0), m_frameCount(0), m_lastFpsUpdate(0) {
    
    setWindowTitle("WindTerm GPU Renderer Test");
    resize(800, 600);
    
    m_textBuffer = new CircularTextBuffer(5000);
    
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(16);
    connect(m_renderTimer, &QTimer::timeout, this, &RendererTestWindow::onRenderTimer);
    
    loadTestText();
}

RendererTestWindow::~RendererTestWindow() {
    if (m_renderer) {
        makeCurrent();
        m_renderer->finalize();
        delete m_renderer;
    }
    delete m_textBuffer;
}

void RendererTestWindow::initializeGL() {
    m_backend = RendererFactory::detectAndCreate(&m_renderer, this);
    
    if (!m_renderer) {
        qCritical() << "Failed to create renderer";
        return;
    }
    
    if (!m_renderer->initialize()) {
        qCritical() << "Failed to initialize renderer";
        return;
    }
    
    m_renderer->setFontFamily("Monaco");
    m_renderer->setFontSize(14);
    m_renderer->setBackgroundColor(QColor(30, 30, 30));
    m_renderer->setForegroundColor(QColor(200, 200, 200));
    
    qDebug() << "[Test] Renderer backend:" << PlatformDetector::backendToString(m_backend);
    
    m_renderTimer->start();
    m_lastFpsUpdate = QDateTime::currentMSecsSinceEpoch();
}

void RendererTestWindow::resizeGL(int w, int h) {
    if (m_renderer) {
        m_renderer->resize(w, h);
    }
}

void RendererTestWindow::paintGL() {
    if (!m_renderer) return;
    
    m_renderer->clear();
    
    int linesVisible = height() / 20;
    for (int i = 0; i < linesVisible && (m_currentLine + i) < m_textBuffer->size(); i++) {
        const LineData& line = m_textBuffer->lineAt(m_currentLine + i);
        m_renderer->appendText(line.text, 10, 20 + i * 20);
    }
    
    m_renderer->render();
    
    m_frameCount++;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastFpsUpdate >= 1000) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_lastFpsUpdate = now;
        setWindowTitle(QString("WindTerm GPU Renderer Test - FPS: %1 - Backend: %2")
            .arg(m_fps)
            .arg(PlatformDetector::backendToString(m_backend)));
    }
}

void RendererTestWindow::keyPressEvent(QKeyEvent* event) {
    int linesVisible = height() / 20;
    
    switch (event->key()) {
        case Qt::Key_Up:
            m_currentLine = qMax(0, m_currentLine - 1);
            update();
            break;
        case Qt::Key_Down:
            m_currentLine = qMin(m_currentLine + 1, qMax(0, m_textBuffer->size() - linesVisible));
            update();
            break;
        case Qt::Key_PageUp:
            m_currentLine = qMax(0, m_currentLine - linesVisible);
            update();
            break;
        case Qt::Key_PageDown:
            m_currentLine = qMin(m_currentLine + linesVisible, qMax(0, m_textBuffer->size() - linesVisible));
            update();
            break;
        case Qt::Key_Escape:
            close();
            break;
        default:
            QOpenGLWidget::keyPressEvent(event);
    }
}

void RendererTestWindow::onRenderTimer() {
    update();
}

void RendererTestWindow::loadTestText() {
    QStringList testLines;
    
    testLines.append("=== WindTerm GPU Renderer Test ===");
    testLines.append("");
    testLines.append("This is a test application for the GPU-accelerated rendering engine.");
    testLines.append("Use arrow keys to scroll, Page Up/Down for faster navigation.");
    testLines.append("");
    testLines.append("Features under test:");
    testLines.append("  - GPU-accelerated text rendering");
    testLines.append("  - Glyph atlas texture caching");
    testLines.append("  - Circular text buffer for history management");
    testLines.append("  - Platform-specific backend detection (Metal/OpenGL/DirectX)");
    testLines.append("");
    testLines.append("Performance targets:");
    testLines.append("  - Scroll at 60 FPS");
    testLines.append("  - Startup time < 0.3s");
    testLines.append("  - CPU usage reduced by 60%");
    testLines.append("");
    
    for (int i = 0; i < 200; i++) {
        testLines.append(QString("Line %1: Lorem ipsum dolor sit amet, consectetur adipiscing elit.").arg(i + 1));
    }
    
    m_textBuffer->appendLines(testLines);
}
