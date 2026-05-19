#ifndef DIRECTX12_RENDERER_H
#define DIRECTX12_RENDERER_H

#include "Renderer/GPURenderer.h"

#ifdef Q_OS_WIN

struct DirectX12RendererPrivate;

class DirectX12Renderer : public GPURenderer {
    Q_OBJECT
public:
    explicit DirectX12Renderer(QObject* parent = nullptr);
    ~DirectX12Renderer() override;
    
    bool initialize() override;
    void resize(int width, int height) override;
    void render() override;
    void finalize() override;
    
protected:
    void initShaders() override;
    void initBuffers() override;
    void renderGlyphs() override;
    
private:
    DirectX12RendererPrivate* d;
};

#else

class DirectX12Renderer : public GPURenderer {
    Q_OBJECT
public:
    explicit DirectX12Renderer(QObject* parent = nullptr) : GPURenderer(parent) {
        qWarning() << "[DirectX12Renderer] DirectX12 is only available on Windows";
    }
};

#endif

#endif
