#ifndef RENDERER_FACTORY_H
#define RENDERER_FACTORY_H

#include "Renderer/GPURenderer.h"
#include "Renderer/PlatformDetector.h"

class RendererFactory {
public:
    static GPURenderer* createRenderer(RendererBackend backend = RendererBackend::Auto, QObject* parent = nullptr);
    static RendererBackend detectAndCreate(GPURenderer** renderer, QObject* parent = nullptr);
    
private:
    static GPURenderer* createMetalRenderer(QObject* parent);
    static GPURenderer* createOpenGLRenderer(QObject* parent);
    static GPURenderer* createDirectX12Renderer(QObject* parent);
    static GPURenderer* createVulkanRenderer(QObject* parent);
    static GPURenderer* createSoftwareRenderer(QObject* parent);
};

#endif
