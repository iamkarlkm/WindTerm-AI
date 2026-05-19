#include "RendererFactory.h"
#include "Renderer/OpenGLRenderer.h"
#include "Renderer/PlatformDetector.h"
#include <QDebug>

#ifdef Q_OS_MACOS
#include "Renderer/MetalRenderer.h"
#endif

#ifdef Q_OS_WIN
#include "Renderer/DirectX12Renderer.h"
#endif

#include "Renderer/VulkanRenderer.h"

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
        case RendererBackend::Vulkan:
            return createVulkanRenderer(parent);
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
#ifdef Q_OS_WIN
    return new DirectX12Renderer(parent);
#else
    qWarning() << "[RendererFactory] DirectX12 backend not available on this platform, falling back to OpenGL";
    return createOpenGLRenderer(parent);
#endif
}

GPURenderer* RendererFactory::createVulkanRenderer(QObject* parent) {
#ifdef VULKAN_AVAILABLE
    return new VulkanRenderer(parent);
#else
    qWarning() << "[RendererFactory] Vulkan backend not available on this platform, falling back to OpenGL";
    return createOpenGLRenderer(parent);
#endif
}

GPURenderer* RendererFactory::createSoftwareRenderer(QObject* parent) {
    qWarning() << "[RendererFactory] Software renderer not yet implemented, falling back to OpenGL";
    return createOpenGLRenderer(parent);
}
