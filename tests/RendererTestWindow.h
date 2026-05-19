#ifndef RENDERER_TEST_WINDOW_H
#define RENDERER_TEST_WINDOW_H

#include <QOpenGLWidget>
#include <QTimer>
#include <QKeyEvent>

#include "Renderer/GPURenderer.h"
#include "Renderer/RendererFactory.h"
#include "Buffer/CircularTextBuffer.h"

class RendererTestWindow : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit RendererTestWindow(QWidget* parent = nullptr);
    ~RendererTestWindow() override;
    
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent* event) override;
    
private slots:
    void onRenderTimer();
    
private:
    void loadTestText();
    
    GPURenderer* m_renderer;
    CircularTextBuffer* m_textBuffer;
    RendererBackend m_backend;
    QTimer* m_renderTimer;
    
    int m_currentLine;
    int m_fps;
    int m_frameCount;
    qint64 m_lastFpsUpdate;
};

#endif
