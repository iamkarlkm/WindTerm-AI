#include "OpenGLRenderer.h"
#include "SDFShader.h"
#include <QOpenGLShader>
#include <QDebug>

OpenGLRenderer::OpenGLRenderer(QObject* parent)
    : GPURenderer(parent), m_posAttr(-1), m_texAttr(-1), m_colorAttr(-1), 
      m_textureUniform(-1), m_sdfThresholdUniform(-1) {}

void OpenGLRenderer::initShaders() {
    QOpenGLShader vertex(QOpenGLShader::Vertex);
    vertex.compileSourceCode(sdfVertexShader);
    
    QOpenGLShader fragment(QOpenGLShader::Fragment);
    fragment.compileSourceCode(sdfFragmentShader);
    
    m_shaderProgram.addShader(&vertex);
    m_shaderProgram.addShader(&fragment);
    m_shaderProgram.link();
    
    m_posAttr = m_shaderProgram.attributeLocation("aPos");
    m_texAttr = m_shaderProgram.attributeLocation("aTexCoord");
    m_colorAttr = m_shaderProgram.attributeLocation("aColor");
    m_textureUniform = m_shaderProgram.uniformLocation("uTexture");
    m_sdfThresholdUniform = m_shaderProgram.uniformLocation("uSDFThreshold");
    
    qDebug() << "[OpenGLRenderer] SDF shaders initialized";
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
    m_shaderProgram.setUniformValue("uSDFThreshold", 0.5f);
    
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
