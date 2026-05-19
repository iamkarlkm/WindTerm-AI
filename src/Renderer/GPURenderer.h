#ifndef GPU_RENDERER_H
#define GPU_RENDERER_H

#include <QObject>
#include <QString>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <QCache>

struct GlyphInfo {
    quint32 codepoint;
    float x, y;
    float width, height;
    float texX, texY;
    float texWidth, texHeight;
};

struct VertexData {
    float position[2];
    float texCoord[2];
    float color[4];
};

struct GlyphKey {
    quint32 codepoint;
    QString fontFamily;
    int fontSize;
    
    bool operator==(const GlyphKey& other) const {
        return codepoint == other.codepoint && 
               fontFamily == other.fontFamily && 
               fontSize == other.fontSize;
    }
};

inline uint qHash(const GlyphKey& key, uint seed = 0) {
    uint h = qHash(key.codepoint, seed);
    h ^= qHash(key.fontFamily, seed) << 1;
    h ^= qHash(key.fontSize, seed) << 2;
    return h;
}

class GlyphAtlas {
public:
    GlyphAtlas(int width = 2048, int height = 2048);
    ~GlyphAtlas();
    
    bool initialize();
    void finalize();
    
    GlyphInfo* getGlyph(quint32 codepoint, const QString& fontFamily, int fontSize);
    void uploadGlyph(quint32 codepoint, const QString& fontFamily, int fontSize, const QImage& glyphImage);
    
    QOpenGLTexture* texture() const { return m_texture; }
    
private:
    QOpenGLTexture* m_texture;
    int m_width, m_height;
    int m_cursorX, m_cursorY;
    int m_rowHeight;
    QHash<GlyphKey, GlyphInfo*> m_glyphCache;
};

class GPURenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit GPURenderer(QObject* parent = nullptr);
    virtual ~GPURenderer();
    
    virtual bool initialize();
    virtual void resize(int width, int height);
    virtual void render();
    virtual void finalize();
    
    void setFontFamily(const QString& family);
    void setFontSize(int size);
    void setBackgroundColor(const QColor& color);
    void setForegroundColor(const QColor& color);
    
    void appendText(const QString& text, int x, int y);
    void clear();
    
protected:
    virtual void initShaders() = 0;
    virtual void initBuffers() = 0;
    virtual void renderGlyphs() = 0;
    
    GlyphAtlas m_glyphAtlas;
    QOpenGLShaderProgram m_shaderProgram;
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLVertexArrayObject m_vao;
    
    QVector<VertexData> m_vertices;
    
    QString m_fontFamily;
    int m_fontSize;
    QColor m_backgroundColor;
    QColor m_foregroundColor;
    
    int m_viewportWidth;
    int m_viewportHeight;
};

#endif
