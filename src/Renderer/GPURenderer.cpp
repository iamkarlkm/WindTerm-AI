#include "GPURenderer.h"
#include "SDFGlyphGenerator.h"
#include <QDebug>
#include <QFontDatabase>
#include <QPainter>

GlyphAtlas::GlyphAtlas(int width, int height)
    : m_texture(nullptr), m_width(width), m_height(height),
      m_cursorX(0), m_cursorY(0), m_rowHeight(0) {}

GlyphAtlas::~GlyphAtlas() {
    finalize();
    qDeleteAll(m_glyphCache);
    m_glyphCache.clear();
}

bool GlyphAtlas::initialize() {
    if (m_texture) return true;
    
    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->setSize(m_width, m_height);
    m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_texture->allocateStorage();
    
    QImage blank(m_width, m_height, QImage::Format_RGBA8888);
    blank.fill(Qt::transparent);
    m_texture->setData(blank);
    
    qDebug() << "[GlyphAtlas] Initialized:" << m_width << "x" << m_height;
    return true;
}

void GlyphAtlas::finalize() {
    if (m_texture) {
        delete m_texture;
        m_texture = nullptr;
    }
}

GlyphInfo* GlyphAtlas::getGlyph(quint32 codepoint, const QString& fontFamily, int fontSize) {
    GlyphKey key{codepoint, fontFamily, fontSize};
    if (m_glyphCache.contains(key)) {
        return m_glyphCache[key];
    }
    return nullptr;
}

void GlyphAtlas::uploadGlyph(quint32 codepoint, const QString& fontFamily, int fontSize, const QImage& glyphImage) {
    GlyphKey key{codepoint, fontFamily, fontSize};
    
    if (m_glyphCache.contains(key)) return;
    
    int glyphWidth = glyphImage.width();
    int glyphHeight = glyphImage.height();
    
    if (m_cursorX + glyphWidth > m_width) {
        m_cursorX = 0;
        m_cursorY += m_rowHeight;
        m_rowHeight = 0;
    }
    
    if (m_cursorY + glyphHeight > m_height) {
        qWarning() << "[GlyphAtlas] Atlas full, cannot upload more glyphs";
        return;
    }
    
    QImage subImage = glyphImage.copy(0, 0, glyphWidth, glyphHeight);
    m_texture->setData(subImage, QOpenGLTexture::DontGenerateMipMaps);
    
    GlyphInfo* info = new GlyphInfo{
        codepoint,
        0, 0,
        static_cast<float>(glyphWidth),
        static_cast<float>(glyphHeight),
        static_cast<float>(m_cursorX) / m_width,
        static_cast<float>(m_cursorY) / m_height,
        static_cast<float>(glyphWidth) / m_width,
        static_cast<float>(glyphHeight) / m_height
    };
    
    m_glyphCache[key] = info;
    m_cursorX += glyphWidth;
    m_rowHeight = qMax(m_rowHeight, glyphHeight);
}

GPURenderer::GPURenderer(QObject* parent)
    : QObject(parent), m_glVertexBuffer(nullptr), m_d3d12VertexBuffer(nullptr),
      m_fontSize(14), m_viewportWidth(0), m_viewportHeight(0) {
    m_backgroundColor = Qt::black;
    m_foregroundColor = Qt::white;
}

GPURenderer::~GPURenderer() {
}

bool GPURenderer::initialize() {
    initializeOpenGLFunctions();
    
    if (!m_glyphAtlas.initialize()) {
        qCritical() << "[GPURenderer] Failed to initialize glyph atlas";
        return false;
    }
    
    initShaders();
    initBuffers();
    
    qDebug() << "[GPURenderer] Initialized successfully";
    return true;
}

void GPURenderer::resize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
    glViewport(0, 0, width, height);
}

void GPURenderer::render() {
    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), 
                 m_backgroundColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    m_shaderProgram.bind();
    m_glyphAtlas.texture()->bind();
    
    renderGlyphs();
    
    m_glyphAtlas.texture()->release();
    m_shaderProgram.release();
}

void GPURenderer::finalize() {
    m_glyphAtlas.finalize();
    if (m_glVertexBuffer) {
        m_glVertexBuffer->destroy();
        delete m_glVertexBuffer;
        m_glVertexBuffer = nullptr;
    }
    m_d3d12VertexBuffer = nullptr;
}

void GPURenderer::setFontFamily(const QString& family) {
    m_fontFamily = family;
}

void GPURenderer::setFontSize(int size) {
    m_fontSize = size;
}

void GPURenderer::setBackgroundColor(const QColor& color) {
    m_backgroundColor = color;
}

void GPURenderer::setForegroundColor(const QColor& color) {
    m_foregroundColor = color;
}

void GPURenderer::appendText(const QString& text, int x, int y) {
    appendText(text, x, y, m_foregroundColor);
}

void GPURenderer::appendText(const QString& text, int x, int y, const QColor& fgColor) {
    QFontDatabase fontDb;
    QFont font(m_fontFamily, m_fontSize);
    
    float xPos = static_cast<float>(x);
    float yPos = static_cast<float>(y);
    
    for (const QChar& ch : text) {
        quint32 codepoint = ch.unicode();
        
        GlyphInfo* glyph = m_glyphAtlas.getGlyph(codepoint, m_fontFamily, m_fontSize);
        
        if (!glyph) {
            QImage alphaMask = SDFGlyphGenerator::generateAlphaMask(ch, font, m_fontSize);
            
            SDFGlyphGenerator sdfGen(4);
            QImage sdfImage = sdfGen.generateSDF(alphaMask);
            
            QImage glyphImage(sdfImage.size(), QImage::Format_RGBA8888);
            glyphImage.fill(Qt::transparent);
            
            for (int py = 0; py < sdfImage.height(); py++) {
                for (int px = 0; px < sdfImage.width(); px++) {
                    uchar dist = qGray(sdfImage.pixel(px, py));
                    glyphImage.setPixel(px, py, qRgba(255, 255, 255, dist));
                }
            }
            
            m_glyphAtlas.uploadGlyph(codepoint, m_fontFamily, m_fontSize, glyphImage);
            glyph = m_glyphAtlas.getGlyph(codepoint, m_fontFamily, m_fontSize);
        }
        
        if (glyph) {
            VertexData vertex[6];
            
            float r = fgColor.redF();
            float g = fgColor.greenF();
            float b = fgColor.blueF();
            
            vertex[0] = {{xPos, yPos + glyph->height}, 
                        {glyph->texX, glyph->texY + glyph->texHeight},
                        {r, g, b, 1.0f}};
            vertex[1] = {{xPos + glyph->width, yPos + glyph->height},
                        {glyph->texX + glyph->texWidth, glyph->texY + glyph->texHeight},
                        {r, g, b, 1.0f}};
            vertex[2] = {{xPos, yPos},
                        {glyph->texX, glyph->texY},
                        {r, g, b, 1.0f}};
            vertex[3] = vertex[2];
            vertex[4] = vertex[1];
            vertex[5] = {{xPos + glyph->width, yPos},
                        {glyph->texX + glyph->texWidth, glyph->texY},
                        {r, g, b, 1.0f}};
            
            m_vertices.append(vertex[0]);
            m_vertices.append(vertex[1]);
            m_vertices.append(vertex[2]);
            m_vertices.append(vertex[3]);
            m_vertices.append(vertex[4]);
            m_vertices.append(vertex[5]);
            
            xPos += glyph->width;
        }
    }
}

void GPURenderer::appendBackground(int x, int y, int width, int height, const QColor& color) {
    VertexData vertex[6];
    
    float r = color.redF();
    float g = color.greenF();
    float b = color.blueF();
    
    float left = static_cast<float>(x);
    float right = static_cast<float>(x + width);
    float top = static_cast<float>(y);
    float bottom = static_cast<float>(y + height);
    
    vertex[0] = {{left, bottom}, {0.0f, 0.0f}, {r, g, b, 1.0f}};
    vertex[1] = {{right, bottom}, {0.0f, 0.0f}, {r, g, b, 1.0f}};
    vertex[2] = {{left, top}, {0.0f, 0.0f}, {r, g, b, 1.0f}};
    vertex[3] = vertex[2];
    vertex[4] = vertex[1];
    vertex[5] = {{right, top}, {0.0f, 0.0f}, {r, g, b, 1.0f}};
    
    m_vertices.append(vertex[0]);
    m_vertices.append(vertex[1]);
    m_vertices.append(vertex[2]);
    m_vertices.append(vertex[3]);
    m_vertices.append(vertex[4]);
    m_vertices.append(vertex[5]);
}

void GPURenderer::clear() {
    m_vertices.clear();
}
