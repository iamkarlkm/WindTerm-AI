#include "DirectX12Renderer.h"

#ifdef Q_OS_WIN

#include <QDebug>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

struct DirectX12RendererPrivate {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<IDXGISwapChain3> swapChain;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
    
    HANDLE frameEvent;
    UINT frameIndex;
    UINT rtvDescriptorSize;
    
    struct FrameResource {
        ComPtr<ID3D12Resource> renderTarget;
        UINT64 fenceValue;
    };
    
    static const int FrameCount = 2;
    FrameResource frames[FrameCount];
    
    ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent;
    
    int width;
    int height;
};

DirectX12Renderer::DirectX12Renderer(QObject* parent)
    : GPURenderer(parent), d(new DirectX12RendererPrivate()) {
    d->frameIndex = 0;
    d->width = 0;
    d->height = 0;
}

DirectX12Renderer::~DirectX12Renderer() {
    finalize();
    delete d;
}

bool DirectX12Renderer::initialize() {
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d->device));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create D3D12 device";
        return false;
    }
    
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    hr = d->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d->commandQueue));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create command queue";
        return false;
    }
    
    hr = d->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
                                           IID_PPV_ARGS(&d->commandAllocator));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create command allocator";
        return false;
    }
    
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = DirectX12RendererPrivate::FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    hr = d->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&d->rtvHeap));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create RTV heap";
        return false;
    }
    
    d->rtvDescriptorSize = d->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    hr = d->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d->fence));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create fence";
        return false;
    }
    
    if (!m_glyphAtlas.initialize()) {
        qCritical() << "[DirectX12Renderer] Failed to initialize glyph atlas";
        return false;
    }
    
    initShaders();
    initBuffers();
    
    qDebug() << "[DirectX12Renderer] Initialized successfully";
    return true;
}

void DirectX12Renderer::resize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
    d->width = width;
    d->height = height;
}

void DirectX12Renderer::render() {
    d->commandAllocator->Reset();
    d->commandList->Reset(d->commandAllocator.Get(), d->pipelineState.Get());
    
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = d->frames[d->frameIndex].renderTarget.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    
    d->commandList->ResourceBarrier(1, &barrier);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += d->frameIndex * d->rtvDescriptorSize;
    
    float clearColor[] = {
        m_backgroundColor.redF(),
        m_backgroundColor.greenF(),
        m_backgroundColor.blueF(),
        1.0f
    };
    d->commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(d->width);
    viewport.Height = static_cast<float>(d->height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    d->commandList->RSSetViewports(1, &viewport);
    
    renderGlyphs();
    
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    d->commandList->ResourceBarrier(1, &barrier);
    
    d->commandList->Close();
    
    ID3D12CommandList* ppCommandLists[] = { d->commandList.Get() };
    d->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    
    d->swapChain->Present(1, 0);
    
    UINT64 fenceValue = d->frames[d->frameIndex].fenceValue;
    d->commandQueue->Signal(d->fence.Get(), fenceValue);
    d->frameIndex = d->swapChain->GetCurrentBackBufferIndex();
}

void DirectX12Renderer::finalize() {
    m_glyphAtlas.finalize();
    
    d->commandList.Reset();
    d->commandAllocator.Reset();
    d->pipelineState.Reset();
    d->rootSignature.Reset();
    d->rtvHeap.Reset();
    d->swapChain.Reset();
    d->commandQueue.Reset();
    d->fence.Reset();
    d->device.Reset();
}

void DirectX12Renderer::initShaders() {
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> fragmentShader;
    ComPtr<ID3DBlob> error;
    
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = 0;
    rootSignatureDesc.pParameters = nullptr;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    ComPtr<ID3DBlob> signature;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, 
                                             &signature, &error);
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to serialize root signature";
        return;
    }
    
    d->device->CreateRootSignature(0, signature->GetBufferPointer(), 
                                   signature->GetBufferSize(), IID_PPV_ARGS(&d->rootSignature));
}

void DirectX12Renderer::initBuffers() {
    HRESULT hr = d->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
                                              d->commandAllocator.Get(), nullptr, 
                                              IID_PPV_ARGS(&d->commandList));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create command list";
        return;
    }
    d->commandList->Close();
}

void DirectX12Renderer::renderGlyphs() {
    if (m_vertices.isEmpty()) return;
}

#else

#endif
