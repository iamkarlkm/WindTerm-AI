#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "Renderer/GPURenderer.h"

class OpenGLRenderer : public GPURenderer {
    Q_OBJECT
public:
    explicit OpenGLRenderer(QObject* parent = nullptr);
    
protected:
    void initShaders() override;
    void initBuffers() override;
    void renderGlyphs() override;
    
private:
    int m_posAttr;
    int m_texAttr;
    int m_colorAttr;
    int m_textureUniform;
    int m_sdfThresholdUniform;
};

#endif
