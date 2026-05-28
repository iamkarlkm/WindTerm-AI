// Background Quad Vertex Shader for DirectX12

cbuffer QuadConstants : register(b0)
{
    float4x4 projectionMatrix;
    float4 backgroundColor;
};

struct VSInput
{
    float2 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput BackgroundVS(VSInput input)
{
    VSOutput output;
    float4 pos = float4(input.position.x, input.position.y, 0.0, 1.0);
    output.position = mul(pos, projectionMatrix);
    return output;
}
