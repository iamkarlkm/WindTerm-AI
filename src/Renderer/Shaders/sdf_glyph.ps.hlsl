// SDF Glyph Pixel Shader for DirectX12
// Compile with: fxc /T ps_5_0 /O3 sdf_glyph.ps.hlsl /Fosdf_glyph.ps.cso

Texture2D sdfTexture : register(t0);
SamplerState sdfSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

float4 SDFGlyphPS(PSInput input) : SV_TARGET
{
    float sdf = sdfTexture.Sample(sdfSampler, input.texCoord).r;
    
    float distance = 1.0 - sdf;
    float pixel = fwidth(sdf);
    float alpha = smoothstep(-pixel, pixel, distance);
    
    float4 textColor = input.color;
    textColor.a *= alpha;
    
    return textColor;
}
