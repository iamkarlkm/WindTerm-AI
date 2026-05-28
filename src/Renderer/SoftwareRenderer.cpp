#include "SoftwareRenderer.h"
#include <algorithm>

SoftwareRenderer::SoftwareRenderer(QObject* parent)
    : GPURenderer(parent)
    , m_initialized(false) {
}

SoftwareRenderer::~SoftwareRenderer() {
    finalize();
}

bool SoftwareRenderer::initialize() {
    if (m_initialized) return true;
    
    m_font.setFamily("monospace");
    m_font.setPointSize(14);
    m_font.setStyleHint(QFont::Monospace);
    
    m_initialized = true;
    return true;
}

void SoftwareRenderer::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    
    if (m_frameBuffer.size() != QSize(width, height)) {
        m_frameBuffer = QImage(QSize(width, height), QImage::Format_ARGB32_Premultiplied);
        m_frameBuffer.fill(Qt::black);
    }
}

void SoftwareRenderer::render() {
    if (!m_initialized || m_frameBuffer.isNull()) return;
    
    m_painter.begin(&m_frameBuffer);
    m_painter.setRenderHint(QPainter::Antialiasing);
    m_painter.setFont(m_font);
    
    renderGlyphs();
    
    m_painter.end();
}

void SoftwareRenderer::finalize() {
    m_frameBuffer = QImage();
    m_initialized = false;
}

void SoftwareRenderer::renderGlyphs() {
    if (m_vertices.isEmpty()) return;
    
    for (int i = 0; i < m_vertices.size(); i += 4) {
        const VertexData& v0 = m_vertices[i];
        const VertexData& v1 = m_vertices[i + 1];
        const VertexData& v2 = m_vertices[i + 2];
        const VertexData& v3 = m_vertices[i + 3];
        
        float minX = v0.position[0];
        if (v1.position[0] < minX) minX = v1.position[0];
        if (v2.position[0] < minX) minX = v2.position[0];
        if (v3.position[0] < minX) minX = v3.position[0];
        
        float minY = v0.position[1];
        if (v1.position[1] < minY) minY = v1.position[1];
        if (v2.position[1] < minY) minY = v2.position[1];
        if (v3.position[1] < minY) minY = v3.position[1];
        
        float width = qAbs(v1.position[0] - v0.position[0]);
        float height = qAbs(v2.position[1] - v0.position[1]);
        
        QRectF rect(minX, minY, width, height);
        
        QColor color(v0.color[0] * 255, v0.color[1] * 255, v0.color[2] * 255, v0.color[3] * 255);
        m_painter.fillRect(rect, color);
    }
}

#include "SoftwareRenderer.moc"
