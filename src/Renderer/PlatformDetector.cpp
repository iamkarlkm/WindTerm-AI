#include "PlatformDetector.h"
#include <QOpenGLContext>
#include <QLibrary>
#include <QDebug>

#ifdef Q_OS_MACOS
#import <Metal/Metal.h>
#endif

#ifdef Q_OS_WIN
#include <d3d12.h>
#endif

RendererBackend PlatformDetector::detectBestBackend() {
    if (isMetalAvailable()) {
        return RendererBackend::Metal;
    }
    if (isDirectX12Available()) {
        return RendererBackend::DirectX12;
    }
    if (isVulkanAvailable()) {
        return RendererBackend::Vulkan;
    }
    if (isOpenGLAvailable()) {
        return RendererBackend::OpenGL;
    }
    return RendererBackend::Software;
}

QString PlatformDetector::backendToString(RendererBackend backend) {
    switch (backend) {
        case RendererBackend::Auto: return QStringLiteral("Auto");
        case RendererBackend::Metal: return QStringLiteral("Metal");
        case RendererBackend::OpenGL: return QStringLiteral("OpenGL");
        case RendererBackend::DirectX12: return QStringLiteral("DirectX12");
        case RendererBackend::Vulkan: return QStringLiteral("Vulkan");
        case RendererBackend::Software: return QStringLiteral("Software");
        default: return QStringLiteral("Unknown");
    }
}

bool PlatformDetector::isBackendAvailable(RendererBackend backend) {
    switch (backend) {
        case RendererBackend::Auto: return true;
        case RendererBackend::Metal: return isMetalAvailable();
        case RendererBackend::OpenGL: return isOpenGLAvailable();
        case RendererBackend::DirectX12: return isDirectX12Available();
        case RendererBackend::Vulkan: return isVulkanAvailable();
        case RendererBackend::Software: return true;
        default: return false;
    }
}

bool PlatformDetector::isMetalAvailable() {
#ifdef Q_OS_MACOS
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    bool available = (device != nil);
    if (device) [device release];
    return available;
#else
    return false;
#endif
}

bool PlatformDetector::isOpenGLAvailable() {
    QOpenGLContext ctx;
    return ctx.create();
}

bool PlatformDetector::isDirectX12Available() {
#ifdef Q_OS_WIN
    ID3D12Device* device = nullptr;
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    if (SUCCEEDED(hr) && device) {
        device->Release();
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool PlatformDetector::isVulkanAvailable() {
#ifdef Q_OS_LINUX
    return QLibrary::resolve("vulkan", "vkEnumerateInstanceExtensionProperties") != nullptr;
#elif defined(Q_OS_WIN)
    return QLibrary::resolve("vulkan-1", "vkEnumerateInstanceExtensionProperties") != nullptr;
#else
    return false;
#endif
}
