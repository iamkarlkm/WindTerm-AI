#include "BatchRenderer.h"
#include <QDebug>

BatchRenderer::BatchRenderer(QObject* parent)
    : QObject(parent), m_inBatch(false), m_flushCount(0) {
}

void BatchRenderer::beginBatch() {
    if (!m_inBatch) {
        m_batches.clear();
        m_inBatch = true;
    }
}

void BatchRenderer::appendBackground(int x, int y, int width, int height, const QColor& color) {
    if (!m_inBatch) return;
    
    RenderBatch batch;
    batch.x = x;
    batch.y = y;
    batch.width = width;
    batch.height = height;
    batch.backgroundColor = color;
    batch.isBackground = true;
    batch.isText = false;
    
    m_batches.append(batch);
}

void BatchRenderer::appendText(int x, int y, const QString& text, const QColor& fgColor) {
    if (!m_inBatch) return;
    
    RenderBatch batch;
    batch.x = x;
    batch.y = y;
    batch.text = text;
    batch.foregroundColor = fgColor;
    batch.isText = true;
    batch.isBackground = false;
    
    m_batches.append(batch);
}

void BatchRenderer::flush() {
    if (!m_inBatch || m_batches.isEmpty()) return;
    
    m_flushCount++;
    emit batchReady(m_batches);
    
    m_batches.clear();
    m_inBatch = false;
}

void BatchRenderer::clear() {
    m_batches.clear();
    m_inBatch = false;
}

#include "BatchRenderer.moc"
