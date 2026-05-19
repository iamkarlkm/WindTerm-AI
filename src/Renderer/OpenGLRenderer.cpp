#include "OpenGLRenderer.h"
#include <QOpenGLShader>
#include <QDebug>

const char* vertexShaderSource = R"(
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

const char* fragmentShaderSource = R"(
    #version 330 core
    
    in vec2 TexCoord;
    in vec4 Color;
    
    out vec4 FragColor;
    
    uniform sampler2D uTexture;
    uniform vec4 uBackgroundColor;
    
    void main() {
        vec4 texColor = texture(uTexture, TexCoord);
        float alpha = texColor.a;
        
        if (alpha < 0.01) {
            discard;
        }
        
        vec3 blendedColor = mix(uBackgroundColor.rgb, Color.rgb, alpha);
        FragColor = vec4(blendedColor, 1.0);
    }
)";

OpenGLRenderer::OpenGLRenderer(QObject* parent)
    : GPURenderer(parent), m_posAttr(-1), m_texAttr(-1), m_colorAttr(-1), m_textureUniform(-1) {}

void OpenGLRenderer::initShaders() {
    QOpenGLShader vertex(QOpenGLShader::Vertex);
    vertex.compileSourceCode(vertexShaderSource);
    
    QOpenGLShader fragment(QOpenGLShader::Fragment);
    fragment.compileSourceCode(fragmentShaderSource);
    
    m_shaderProgram.addShader(&vertex);
    m_shaderProgram.addShader(&fragment);
    m_shaderProgram.link();
    
    m_posAttr = m_shaderProgram.attributeLocation("aPos");
    m_texAttr = m_shaderProgram.attributeLocation("aTexCoord");
    m_colorAttr = m_shaderProgram.attributeLocation("aColor");
    m_textureUniform = m_shaderProgram.uniformLocation("uTexture");
    
    qDebug() << "[OpenGLRenderer] Shaders initialized";
}

void OpenGLRenderer::initBuffers() {
    m_vao.create();
    m_vao.bind();
    
    m_vertexBuffer.create();
    m_vertexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    
    qDebug() << "[OpenGLRenderer] Buffers initialized";
}

void OpenGLRenderer::renderGlyphs() {
    if (m_vertices.isEmpty()) return;
    
    m_shaderProgram.bind();
    
    m_shaderProgram.setUniformValue("uResolution", 
                                    static_cast<float>(m_viewportWidth), 
                                    static_cast<float>(m_viewportHeight));
    m_shaderProgram.setUniformValue("uBackgroundColor",
                                    m_backgroundColor.redF(),
                                    m_backgroundColor.greenF(),
                                    m_backgroundColor.blueF(),
                                    1.0f);
    m_shaderProgram.setUniformValue("uTexture", 0);
    
    m_vertexBuffer.bind();
    m_vertexBuffer.allocate(m_vertices.constData(), 
                           m_vertices.size() * sizeof(VertexData));
    
    int offset = 0;
    m_shaderProgram.setAttributeBuffer(m_posAttr, GL_FLOAT, offset, 2, sizeof(VertexData));
    m_shaderProgram.enableAttributeArray(m_posAttr);
    offset += 2 * sizeof(float);
    
    m_shaderProgram.setAttributeBuffer(m_texAttr, GL_FLOAT, offset, 2, sizeof(VertexData));
    m_shaderProgram.enableAttributeArray(m_texAttr);
    offset += 2 * sizeof(float);
    
    m_shaderProgram.setAttributeBuffer(m_colorAttr, GL_FLOAT, offset, 4, sizeof(VertexData));
    m_shaderProgram.enableAttributeArray(m_colorAttr);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
    
    m_shaderProgram.disableAttributeArray(m_posAttr);
    m_shaderProgram.disableAttributeArray(m_texAttr);
    m_shaderProgram.disableAttributeArray(m_colorAttr);
    
    m_vertexBuffer.release();
    m_shaderProgram.release();
}
