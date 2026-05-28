// Background Quad Pixel Shader for DirectX12

cbuffer QuadConstants : register(b0)
{
    float4x4 projectionMatrix;
    float4 backgroundColor;
};

float4 BackgroundPS() : SV_TARGET
{
    return backgroundColor;
}
