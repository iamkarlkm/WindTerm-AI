#ifndef PLATFORM_DETECTOR_H
#define PLATFORM_DETECTOR_H

#include <QString>

enum class RendererBackend {
    Auto,
    Metal,
    OpenGL,
    DirectX12,
    Vulkan,
    Software
};

class PlatformDetector {
public:
    static RendererBackend detectBestBackend();
    static QString backendToString(RendererBackend backend);
    static bool isBackendAvailable(RendererBackend backend);
    
private:
    static bool isMetalAvailable();
    static bool isOpenGLAvailable();
    static bool isDirectX12Available();
    static bool isVulkanAvailable();
};

#endif
