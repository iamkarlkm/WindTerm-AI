#ifndef SDF_GLYPH_GENERATOR_H
#define SDF_GLYPH_GENERATOR_H

#include <QImage>
#include <QVector>

class SDFGlyphGenerator {
public:
    SDFGlyphGenerator(int spread = 4);
    
    QImage generateSDF(const QImage& sourceImage);
    static QImage generateAlphaMask(const QChar& ch, const QFont& font, int size);
    
private:
    void computeDistanceTransform(const QVector<float>& input, QVector<float>& output, int width, int height);
    float euclideanDistance(int x1, int y1, int x2, int y2);
    float sampleDistance(const QVector<float>& dist, int x, int y, int width, int height);
    
    int m_spread;
};

#endif
