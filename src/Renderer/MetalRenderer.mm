#include "MetalRenderer.h"

#ifdef Q_OS_MACOS

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <QDebug>

struct MetalRendererPrivate {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLRenderPipelineState> pipelineState;
    id<MTLBuffer> vertexBuffer;
    id<MTLTexture> glyphTexture;
    
    CAMetalLayer* metalLayer;
    NSUInteger width;
    NSUInteger height;
};

MetalRenderer::MetalRenderer(QObject* parent)
    : GPURenderer(parent), d(new MetalRendererPrivate()) {
    d->device = MTLCreateSystemDefaultDevice();
    d->commandQueue = [d->device newCommandQueue];
    d->width = 0;
    d->height = 0;
}

MetalRenderer::~MetalRenderer() {
    finalize();
    delete d;
}

bool MetalRenderer::initialize() {
    if (!d->device) {
        qCritical() << "[MetalRenderer] Metal is not supported on this device";
        return false;
    }
    
    if (!m_glyphAtlas.initialize()) {
        qCritical() << "[MetalRenderer] Failed to initialize glyph atlas";
        return false;
    }
    
    initShaders();
    initBuffers();
    
    qDebug() << "[MetalRenderer] Initialized successfully";
    return true;
}

void MetalRenderer::resize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
    d->width = width;
    d->height = height;
}

void MetalRenderer::render() {
    if (!d->commandQueue) return;
    
    id<MTLCommandBuffer> commandBuffer = [d->commandQueue commandBuffer];
    
    MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    passDescriptor.colorAttachments[0].texture = d->metalLayer.nextDrawable.texture;
    passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(
        m_backgroundColor.redF(),
        m_backgroundColor.greenF(),
        m_backgroundColor.blueF(),
        1.0
    );
    passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
    [encoder setRenderPipelineState:d->pipelineState];
    
    renderGlyphs();
    
    [encoder endEncoding];
    [commandBuffer presentDrawable:d->metalLayer.nextDrawable];
    [commandBuffer commit];
}

void MetalRenderer::finalize() {
    m_glyphAtlas.finalize();
    
    if (d->pipelineState) {
        [d->pipelineState release];
        d->pipelineState = nil;
    }
    if (d->vertexBuffer) {
        [d->vertexBuffer release];
        d->vertexBuffer = nil;
    }
    if (d->glyphTexture) {
        [d->glyphTexture release];
        d->glyphTexture = nil;
    }
}

void MetalRenderer::initShaders() {
    NSString* librarySource = @""
        "#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "\n"
        "struct VertexIn {\n"
        "    float2 position [[attribute(0)]];\n"
        "    float2 texCoord [[attribute(1)]];\n"
        "    float4 color [[attribute(2)]];\n"
        "};\n"
        "\n"
        "struct VertexOut {\n"
        "    float4 position [[position]];\n"
        "    float2 texCoord;\n"
        "    float4 color;\n"
        "};\n"
        "\n"
        "vertex VertexOut vertexShader(VertexIn in [[stage_in]],\n"
        "                              constant float2& resolution [[buffer(1)]]) {\n"
        "    VertexOut out;\n"
        "    float2 normalized = (in.position / resolution) * 2.0 - 1.0;\n"
        "    out.position = float4(normalized.x, -normalized.y, 0.0, 1.0);\n"
        "    out.texCoord = in.texCoord;\n"
        "    out.color = in.color;\n"
        "    return out;\n"
        "}\n"
        "\n"
        "fragment float4 fragmentShader(VertexOut in [[stage_in]],\n"
        "                               texture2d<float> tex [[texture(0)]]) {\n"
        "    constexpr sampler samplr;\n"
        "    float4 texColor = tex.sample(samplr, in.texCoord);\n"
        "    if (texColor.a < 0.01) discard_fragment();\n"
        "    return float4(in.color.rgb * texColor.a, 1.0);\n"
        "}\n";
    
    NSError* error = nil;
    id<MTLLibrary> library = [d->device newLibraryWithSource:librarySource options:nil error:&error];
    if (!library) {
        qCritical() << "[MetalRenderer] Failed to compile shaders:" << QString::fromNSString(error.localizedDescription);
        return;
    }
    
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    
    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    d->pipelineState = [d->device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!d->pipelineState) {
        qCritical() << "[MetalRenderer] Failed to create pipeline state:" << QString::fromNSString(error.localizedDescription);
    }
    
    [library release];
    [vertexFunction release];
    [fragmentFunction release];
    [pipelineDescriptor release];
}

void MetalRenderer::initBuffers() {
    const size_t bufferSize = 1024 * sizeof(VertexData);
    d->vertexBuffer = [d->device newBufferWithLength:bufferSize options:MTLResourceStorageModeShared];
}

void MetalRenderer::renderGlyphs() {
    if (m_vertices.isEmpty()) return;
    
    const size_t bufferSize = m_vertices.size() * sizeof(VertexData);
    if (d->vertexBuffer.length < bufferSize) {
        [d->vertexBuffer release];
        d->vertexBuffer = [d->device newBufferWithLength:bufferSize options:MTLResourceStorageModeShared];
    }
    
    memcpy(d->vertexBuffer.contents, m_vertices.constData(), bufferSize);
}

#else

MetalRenderer::MetalRenderer(QObject* parent)
    : GPURenderer(parent) {
    qWarning() << "[MetalRenderer] Metal is only available on macOS";
}

#endif
