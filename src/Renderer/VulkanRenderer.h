#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include "Renderer/GPURenderer.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
#define VULKAN_RENDERER_AVAILABLE
#endif

#ifdef VULKAN_RENDERER_AVAILABLE

struct VulkanRendererPrivate;

class VulkanRenderer : public GPURenderer {
    Q_OBJECT
public:
    explicit VulkanRenderer(QObject* parent = nullptr);
    ~VulkanRenderer() override;
    
    bool initialize() override;
    void resize(int width, int height) override;
    void render() override;
    void finalize() override;
    
protected:
    void initShaders() override;
    void initBuffers() override;
    void renderGlyphs() override;
    
private:
    VulkanRendererPrivate* d;
};

#else

class VulkanRenderer : public GPURenderer {
    Q_OBJECT
public:
    explicit VulkanRenderer(QObject* parent = nullptr) : GPURenderer(parent) {
        qWarning() << "[VulkanRenderer] Vulkan is not available";
    }
};

#endif

#endif
