#ifndef TERMINAL_WIDGET_H
#define TERMINAL_WIDGET_H

#include <QOpenGLWidget>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QClipboard>
#include <QMenu>

#include "Renderer/GPURenderer.h"
#include "Renderer/RendererFactory.h"
#include "Buffer/CircularTextBuffer.h"

struct TerminalConfig {
    QString fontFamily = QStringLiteral("Consolas");
    int fontSize = 14;
    int columns = 80;
    int rows = 24;
    int bufferCapacity = 10000;
    QColor backgroundColor = QColor(30, 30, 30);
    QColor foregroundColor = QColor(200, 200, 200);
    QColor cursorColor = QColor(200, 200, 200);
    bool enableBlinking = true;
    RendererBackend backend = RendererBackend::Auto;
};

class TerminalWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    explicit TerminalWidget(const TerminalConfig& config, QWidget* parent = nullptr);
    ~TerminalWidget() override;
    
    void write(const QString& text);
    void write(const QByteArray& data);
    void clear();
    void scrollToTop();
    void scrollToBottom();
    void scroll(int lines);
    
    int columns() const { return m_config.columns; }
    int rows() const { return m_config.rows; }
    int scrollbackSize() const;
    QString selectedText() const { return m_selectedText; }
    
    void setConfig(const TerminalConfig& config);
    TerminalConfig config() const { return m_config; }
    
    void copyToClipboard();
    void pasteFromClipboard();
    void selectWord(int col, int row);
    void selectLine(int row);
    
signals:
    void dataAvailable(const QByteArray& data);
    void selectionChanged(const QString& text);
    void urlClicked(const QString& url);
    void scrollPositionChanged(int current, int max);
    
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    
private slots:
    void onCursorBlink();
    void onRenderTimer();
    
private:
    struct CursorPosition {
        int x;
        int y;
    };
    
    void initRenderer();
    void renderFrame();
    void renderText();
    void renderCursor();
    void renderSelection();
    void updateScrollPosition();
    
    int charWidth() const;
    int charHeight() const;
    int linePixelWidth() const;
    
    QPoint pixelToChar(const QPoint& pixel) const;
    QPoint charToPixel(int col, int row) const;
    
    void updateSelection(int col, int row);
    
    void handleCtrlKey(int key);
    void handleAltKey(int key);
    QByteArray encodeKey(QKeyEvent* event);
    QByteArray encodeMouse(QMouseEvent* event);
    
    TerminalConfig m_config;
    GPURenderer* m_renderer;
    CircularTextBuffer* m_buffer;
    
    QTimer* m_renderTimer;
    QTimer* m_cursorBlinkTimer;
    
    CursorPosition m_cursorPos;
    bool m_cursorVisible;
    bool m_cursorBlinkState;
    
    struct Selection {
        bool active = false;
        int startCol = 0;
        int startRow = 0;
        int endCol = 0;
        int endRow = 0;
    } m_selection;
    QString m_selectedText;
    
    QPoint m_lastMousePos;
    bool m_mouseSelecting = false;
    
    int m_scrollOffset = 0;
    int m_maxScrollback = 0;
    
    bool m_initialized = false;
    int m_fps = 0;
    int m_frameCount = 0;
    qint64 m_lastFpsUpdate = 0;
    
    QVector<QString> m_displayLines;
};

#endif
