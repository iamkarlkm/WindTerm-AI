#include "RendererFactory.h"
#include "Renderer/OpenGLRenderer.h"
#include "Renderer/PlatformDetector.h"
#include <QDebug>

#ifdef Q_OS_MACOS
#include "Renderer/MetalRenderer.h"
#endif

GPURenderer* RendererFactory::createRenderer(RendererBackend backend, QObject* parent) {
    if (backend == RendererBackend::Auto) {
        backend = PlatformDetector::detectBestBackend();
    }
    
    qDebug() << "[RendererFactory] Creating renderer backend:" << PlatformDetector::backendToString(backend);
    
    switch (backend) {
        case RendererBackend::Metal:
            return createMetalRenderer(parent);
        case RendererBackend::OpenGL:
            return createOpenGLRenderer(parent);
        case RendererBackend::DirectX12:
            return createDirectX12Renderer(parent);
        case RendererBackend::Software:
            return createSoftwareRenderer(parent);
        default:
            qCritical() << "[RendererFactory] Unknown backend requested";
            return nullptr;
    }
}

RendererBackend RendererFactory::detectAndCreate(GPURenderer** renderer, QObject* parent) {
    RendererBackend backend = PlatformDetector::detectBestBackend();
    *renderer = createRenderer(backend, parent);
    return backend;
}

GPURenderer* RendererFactory::createMetalRenderer(QObject* parent) {
#ifdef Q_OS_MACOS
    return new MetalRenderer(parent);
#else
    qWarning() << "[RendererFactory] Metal backend not available on this platform, falling back to OpenGL";
    return createOpenGLRenderer(parent);
#endif
}

GPURenderer* RendererFactory::createOpenGLRenderer(QObject* parent) {
    return new OpenGLRenderer(parent);
}

GPURenderer* RendererFactory::createDirectX12Renderer(QObject* parent) {
    qWarning() << "[RendererFactory] DirectX12 backend not yet implemented, falling back to OpenGL";
    return createOpenGLRenderer(parent);
}

GPURenderer* RendererFactory::createSoftwareRenderer(QObject* parent) {
    qWarning() << "[RendererFactory] Software renderer not yet implemented, falling back to OpenGL";
    return createOpenGLRenderer(parent);
}
