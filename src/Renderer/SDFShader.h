#ifndef SDF_SHADER_H
#define SDF_SHADER_H

const char* sdfVertexShader = R"(
    #version 330 core
    
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aTexCoord;
    layout(location = 2) in vec4 aColor;
    
    out vec2 TexCoord;
    out vec4 Color;
    
    uniform vec2 uResolution;
    
    void main() {
        vec2 normalizedPos = (aPos / uResolution) * 2.0 - 1.0;
        gl_Position = vec4(normalizedPos.x, -normalizedPos.y, 0.0, 1.0);
        TexCoord = aTexCoord;
        Color = aColor;
    }
)";

const char* sdfFragmentShader = R"(
    #version 330 core
    
    in vec2 TexCoord;
    in vec4 Color;
    
    out vec4 FragColor;
    
    uniform sampler2D uTexture;
    uniform vec4 uBackgroundColor;
    uniform float uSDFThreshold;
    
    void main() {
        float distance = texture(uTexture, TexCoord).a;
        
        float smoothing = fwidth(distance);
        float alpha = smoothstep(uSDFThreshold - smoothing, uSDFThreshold + smoothing, distance);
        
        if (alpha < 0.01) {
            discard;
        }
        
        vec3 blendedColor = mix(uBackgroundColor.rgb, Color.rgb, alpha);
        FragColor = vec4(blendedColor, 1.0);
    }
)";

#endif
