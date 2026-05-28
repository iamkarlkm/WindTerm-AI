#include "DirectX12Renderer.h"

#ifdef Q_OS_WIN

#include <QDebug>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxgiformat.h>
#include <wrl/client.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12_simple.h"

using namespace Microsoft::WRL;
using namespace DirectX;

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
    
    hr = d->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d->fence));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create fence";
        return false;
    }
    
    d->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!d->fenceEvent) {
        qCritical() << "[DirectX12Renderer] Failed to create fence event";
        return false;
    }
    
    if (!m_glyphAtlas.initialize()) {
        qCritical() << "[DirectX12Renderer] Failed to initialize glyph atlas";
        return false;
    }
    
    qDebug() << "[DirectX12Renderer] Device initialized successfully";
    return true;
}

void DirectX12Renderer::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    
    m_viewportWidth = width;
    m_viewportHeight = height;
    d->width = width;
    d->height = height;
    
    if (d->swapChain.Get()) {
        d->swapChain.Reset();
        d->frames[0].renderTarget.Reset();
        d->frames[1].renderTarget.Reset();
    }
    
    ComPtr<IDXGIFactory4> dxgiFactory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create DXGI factory";
        return;
    }
    
    ComPtr<IDXGIAdapter1> adapter;
    hr = dxgiFactory->EnumAdapterByGpuPreference(0, DXGI_ADAPTER_GPU_PREFERENCE_HIGH_PERFORMANCE, 
                                                  IID_PPV_ARGS(&adapter));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to enumerate adapter";
        return;
    }
    
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = DirectX12RendererPrivate::FrameCount;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.Stereo = FALSE;
    
    ComPtr<IDXGISwapChain1> tempSwapChain;
    hr = dxgiFactory->CreateSwapChainForHwnd(d->commandQueue.Get(), 
                                              reinterpret_cast<HWND>(winId()),
                                              &swapChainDesc, nullptr, nullptr, 
                                              &tempSwapChain);
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create swap chain";
        return;
    }
    
    hr = tempSwapChain.As(&d->swapChain);
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to upgrade swap chain";
        return;
    }
    
    dxgiFactory->MakeWindowAssociation(reinterpret_cast<HWND>(winId()), 
                                        DXGI_MWA_NO_ALT_ENTER);
    
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = DirectX12RendererPrivate::FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    hr = d->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&d->rtvHeap));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create RTV heap";
        return;
    }
    
    d->rtvDescriptorSize = d->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(d->rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < DirectX12RendererPrivate::FrameCount; i++) {
        hr = d->swapChain->GetBuffer(i, IID_PPV_ARGS(&d->frames[i].renderTarget));
        if (FAILED(hr)) {
            qCritical() << "[DirectX12Renderer] Failed to get back buffer" << i;
            return;
        }
        
        d->device->CreateRenderTargetView(d->frames[i].renderTarget.Get(), nullptr, rtvHandle);
        d->frames[i].fenceValue = 0;
        rtvHandle.Offset(1, d->rtvDescriptorSize);
    }
    
    d->frameIndex = d->swapChain->GetCurrentBackBufferIndex();
    
    initShaders();
    initBuffers();
    
    qDebug() << "[DirectX12Renderer] Resize to" << width << "x" << height;
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
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = 0;
    rootSignatureDesc.pParameters = nullptr;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, 
                                             &signature, &error);
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to serialize root signature";
        return;
    }
    
    hr = d->device->CreateRootSignature(0, signature->GetBufferPointer(), 
                                       signature->GetBufferSize(), 
                                       IID_PPV_ARGS(&d->rootSignature));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create root signature";
        return;
    }
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = d->rootSignature.Get();
    
    BYTE* shaderBlob = nullptr;
    SIZE_T shaderBlobSize = 0;
    
#ifdef _DEBUG
    const wchar_t* shaderPath = L"shaders/compiled/sdf_glyph_vs.cso";
#else
    const wchar_t* shaderPath = L"shaders/compiled/sdf_glyph_vs.cso";
#endif
    
    HANDLE hFile = CreateFileW(shaderPath, GENERIC_READ, FILE_SHARE_READ, nullptr, 
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        shaderBlobSize = GetFileSize(hFile, nullptr);
        shaderBlob = new BYTE[shaderBlobSize];
        DWORD bytesRead;
        ReadFile(hFile, shaderBlob, shaderBlobSize, &bytesRead, nullptr);
        CloseHandle(hFile);
        
        psoDesc.VS = { shaderBlob, shaderBlobSize };
    }
    
    D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
    rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0] = rtBlendDesc;
    
    psoDesc.BlendState = blendDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    
    psoDesc.NodeMask = 0;
    psoDesc.CachedPSO = { nullptr, 0 };
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    
    hr = d->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&d->pipelineState));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create pipeline state object, hr=" << hex << hr;
    } else {
        qDebug() << "[DirectX12Renderer] Pipeline state created successfully";
    }
    
    if (shaderBlob) {
        delete[] shaderBlob;
    }
}

void DirectX12Renderer::initBuffers() {
    const UINT vertexBufferSize = 1024 * 1024;
    
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CreationNodeMask = 1;
    heapProp.VisibleNodeMask = 1;
    
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resourceDesc.Width = vertexBufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    HRESULT hr = d->device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, 
                                                    &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 
                                                    nullptr, IID_PPV_ARGS(&m_vertexBuffer));
    if (FAILED(hr)) {
        qCritical() << "[DirectX12Renderer] Failed to create vertex buffer";
        return;
    }
    
    D3D12_CONSTANT_BUFFER_DESC cbDesc = {};
    cbDesc.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    cbDesc.SizeInBytes = (vertexBufferSize + 255) & ~255;
    
    qDebug() << "[DirectX12Renderer] Vertex buffer created, size:" << vertexBufferSize;
}

void DirectX12Renderer::renderGlyphs() {
    if (m_vertices.isEmpty()) return;
    
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    vbv.SizeInBytes = static_cast<UINT>(m_vertices.size() * sizeof(VertexData));
    vbv.StrideInBytes = sizeof(VertexData);
    
    d->commandList->IASetVertexBuffers(0, 1, &vbv);
    d->commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    d->commandList->SetGraphicsRootSignature(d->rootSignature.Get());
    d->commandList->SetPipelineState(d->pipelineState.Get());
    
    void* pMappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    
    if (SUCCEEDED(m_vertexBuffer->Map(0, &readRange, &pMappedData))) {
        memcpy(pMappedData, m_vertices.constData(), 
               m_vertices.size() * sizeof(VertexData));
        m_vertexBuffer->Unmap(0, nullptr);
    }
    
    d->commandList->DrawInstanced(static_cast<UINT>(m_vertices.size()), 1, 0, 0);
    
    m_vertices.clear();
}

#else

#endif
