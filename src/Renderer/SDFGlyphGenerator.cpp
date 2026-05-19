#include "SDFGlyphGenerator.h"
#include <QPainter>
#include <QtMath>
#include <algorithm>
#include <limits>

SDFGlyphGenerator::SDFGlyphGenerator(int spread)
    : m_spread(spread) {}

QImage SDFGlyphGenerator::generateAlphaMask(const QChar& ch, const QFont& font, int size) {
    int glyphSize = size * 2;
    QImage image(glyphSize, glyphSize, QImage::Format_Grayscale8);
    image.fill(Qt::black);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(font);
    painter.setPen(Qt::white);
    
    QFontMetrics fm(font);
    QRect charRect = fm.boundingRect(ch);
    int x = (glyphSize - charRect.width()) / 2;
    int y = (glyphSize + fm.ascent()) / 2 - fm.ascent() / 2;
    
    painter.drawText(x, y, ch);
    painter.end();
    
    return image;
}

QImage SDFGlyphGenerator::generateSDF(const QImage& sourceImage) {
    if (sourceImage.isNull()) return sourceImage;
    
    int width = sourceImage.width();
    int height = sourceImage.height();
    int outputSize = width + m_spread * 2;
    
    QImage source = sourceImage.convertToFormat(QImage::Format_Grayscale8);
    if (source.isNull()) return sourceImage;
    
    QVector<float> insideDist(outputSize * outputSize, std::numeric_limits<float>::max());
    QVector<float> outsideDist(outputSize * outputSize, std::numeric_limits<float>::max());
    
    auto getPixel = [&](int x, int y) -> float {
        if (x < m_spread || x >= width + m_spread || y < m_spread || y >= height + m_spread) {
            return 0.0f;
        }
        uchar val = source.pixel(x - m_spread, y - m_spread);
        return val / 255.0f;
    };
    
    for (int y = 0; y < outputSize; y++) {
        for (int x = 0; x < outputSize; x++) {
            float alpha = getPixel(x, y);
            if (alpha > 0.5f) {
                insideDist[y * outputSize + x] = 0.0f;
            } else {
                outsideDist[y * outputSize + x] = 0.0f;
            }
        }
    }
    
    auto chamferDistance = [&](QVector<float>& dist, int w, int h) {
        float inf = std::numeric_limits<float>::max();
        
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float d = dist[y * w + x];
                if (d > 0) {
                    float minDist = inf;
                    if (x > 0) minDist = qMin(minDist, dist[y * w + (x-1)] + 1.0f);
                    if (y > 0) minDist = qMin(minDist, dist[(y-1) * w + x] + 1.0f);
                    if (x > 0 && y > 0) minDist = qMin(minDist, dist[(y-1) * w + (x-1)] + 1.414f);
                    if (x < w-1 && y > 0) minDist = qMin(minDist, dist[(y-1) * w + (x+1)] + 1.414f);
                    dist[y * w + x] = qMin(d, minDist);
                }
            }
        }
        
        for (int y = h - 1; y >= 0; y--) {
            for (int x = w - 1; x >= 0; x--) {
                float d = dist[y * w + x];
                if (d > 0) {
                    float minDist = inf;
                    if (x < w-1) minDist = qMin(minDist, dist[y * w + (x+1)] + 1.0f);
                    if (y < h-1) minDist = qMin(minDist, dist[(y+1) * w + x] + 1.0f);
                    if (x < w-1 && y < h-1) minDist = qMin(minDist, dist[(y+1) * w + (x+1)] + 1.414f);
                    if (x > 0 && y < h-1) minDist = qMin(minDist, dist[(y+1) * w + (x-1)] + 1.414f);
                    dist[y * w + x] = qMin(d, minDist);
                }
            }
        }
    };
    
    chamferDistance(insideDist, outputSize, outputSize);
    chamferDistance(outsideDist, outputSize, outputSize);
    
    QImage sdfImage(outputSize, outputSize, QImage::Format_Grayscale8);
    
    float maxDist = m_spread;
    
    for (int y = 0; y < outputSize; y++) {
        for (int x = 0; x < outputSize; x++) {
            float dist = insideDist[y * outputSize + x] - outsideDist[y * outputSize + x];
            float normalized = 0.5f + dist / (2.0f * maxDist);
            normalized = qBound(0.0f, normalized, 1.0f);
            uchar val = static_cast<uchar>(normalized * 255.0f);
            sdfImage.setPixel(x, y, qGray(qRgba(val, val, val, 255)));
        }
    }
    
    return sdfImage;
}

float SDFGlyphGenerator::euclideanDistance(int x1, int y1, int x2, int y2) {
    return qSqrt(qPow(x1 - x2, 2) + qPow(y1 - y2, 2));
}

float SDFGlyphGenerator::sampleDistance(const QVector<float>& dist, int x, int y, int width, int height) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return std::numeric_limits<float>::max();
    }
    return dist[y * width + x];
}
