// SDF Glyph Vertex Shader for DirectX12
// Compile with: fxc /T vs_5_0 /O3 sdf_glyph.vs.hlsl /Fosdf_glyph.vs.cso

cbuffer GlyphConstants : register(b0)
{
    float4x4 projectionMatrix;
    float2 glyphSize;
    float2 textureSize;
    float4 color;
};

struct VSInput
{
    float2 position : POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

VSOutput SDFGlyphVS(VSInput input)
{
    VSOutput output;
    
    float4 pos = float4(input.position.x, input.position.y, 0.0, 1.0);
    output.position = mul(pos, projectionMatrix);
    output.texCoord = input.texCoord;
    output.color = input.color;
    
    return output;
}
