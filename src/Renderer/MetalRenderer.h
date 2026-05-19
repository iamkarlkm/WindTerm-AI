#ifndef METAL_RENDERER_H
#define METAL_RENDERER_H

#include "Renderer/GPURenderer.h"

#ifdef Q_OS_MACOS

#include <QVector>

struct MetalRendererPrivate;

class MetalRenderer : public GPURenderer {
    Q_OBJECT
public:
    explicit MetalRenderer(QObject* parent = nullptr);
    ~MetalRenderer() override;
    
    bool initialize() override;
    void resize(int width, int height) override;
    void render() override;
    void finalize() override;
    
protected:
    void initShaders() override;
    void initBuffers() override;
    void renderGlyphs() override;
    
private:
    MetalRendererPrivate* d;
};

#else

class MetalRenderer : public GPURenderer {
    Q_OBJECT
public:
    explicit MetalRenderer(QObject* parent = nullptr) : GPURenderer(parent) {
        qWarning() << "[MetalRenderer] Metal is only available on macOS";
    }
};

#endif

#endif
