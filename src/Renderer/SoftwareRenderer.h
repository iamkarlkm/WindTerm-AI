#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include "Renderer/GPURenderer.h"
#include <QImage>
#include <QPainter>

class SoftwareRenderer : public GPURenderer {
    Q_OBJECT
public:
    explicit SoftwareRenderer(QObject* parent = nullptr);
    ~SoftwareRenderer() override;
    
    bool initialize() override;
    void resize(int width, int height) override;
    void render() override;
    void finalize() override;

protected:
    void initShaders() override {}
    void initBuffers() override {}
    void renderGlyphs() override;

private:
    QImage m_frameBuffer;
    QPainter m_painter;
    QFont m_font;
    bool m_initialized;
};

#endif
