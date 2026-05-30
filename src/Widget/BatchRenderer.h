#ifndef BATCH_RENDERER_H
#define BATCH_RENDERER_H

#include <QObject>
#include <QRect>
#include <QColor>
#include <QVector>

struct RenderBatch {
    int x, y;
    int width, height;
    QColor backgroundColor;
    QColor foregroundColor;
    QString text;
    bool isText;
    bool isBackground;
};

class BatchRenderer : public QObject {
    Q_OBJECT
public:
    explicit BatchRenderer(QObject* parent = nullptr);
    
    void beginBatch();
    void appendBackground(int x, int y, int width, int height, const QColor& color);
    void appendText(int x, int y, const QString& text, const QColor& fgColor);
    void flush();
    
    int batchCount() const { return m_batches.size(); }
    void clear();
    
signals:
    void batchReady(const QVector<RenderBatch>& batches);

private:
    QVector<RenderBatch> m_batches;
    bool m_inBatch;
    int m_flushCount;
};

#endif
