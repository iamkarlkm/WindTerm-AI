#ifndef D3DX12_SIMPLE_H
#define D3DX12_SIMPLE_H

#include <d3d12.h>

struct CD3DX12_DEFAULT {};
extern const CD3DX12_DEFAULT D3D12_DEFAULT;

struct CD3DX12_RASTERIZER_DESC : public D3D12_RASTERIZER_DESC {
    CD3DX12_RASTERIZER_DESC(D3D12_RASTERIZER_DESC desc) : D3D12_RASTERIZER_DESC(desc) {}
    
    CD3DX12_RASTERIZER_DESC(D3D12_FILL_MODE fillMode,
                           D3D12_CULL_MODE cullMode,
                           INT frontStencilFailOp,
                           INT frontStencilDepthFailOp,
                           INT frontStencilPassOp,
                           D3D12_COMPARISON_FUNC frontStencilFunc,
                           UINT stencilReadMask,
                           UINT stencilWriteMask,
                           INT backStencilFailOp,
                           INT backStencilDepthFailOp,
                           INT backStencilPassOp,
                           D3D12_COMPARISON_FUNC backStencilFunc,
                           INT biasClamp,
                           FLOAT slopeScaledDepthBias,
                           FLOAT depthBiasClamp,
                           FLOAT depthBias,
                           FLOAT depthClipWrite,
                           FLOAT depthClipRead,
                           UINT sampleMask) {
        FillMode = fillMode;
        CullMode = cullMode;
        FrontCounterClockwise = FALSE;
        DepthBias = static_cast<INT>(depthBias);
        DepthBiasClamp = depthBiasClamp;
        SlopeScaledDepthBias = slopeScaledDepthBias;
        DepthClipEnable = TRUE;
        MultisampleEnable = FALSE;
        ForcedSampleCount = FALSE;
        AntialiasedLineEnable = FALSE;
        ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }
    
    CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT) {
        FillMode = D3D12_FILL_MODE_SOLID;
        CullMode = D3D12_CULL_MODE_BACK;
        FrontCounterClockwise = FALSE;
        DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        DepthClipEnable = TRUE;
        MultisampleEnable = FALSE;
        ForcedSampleCount = FALSE;
        AntialiasedLineEnable = FALSE;
        ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }
};

#endif
