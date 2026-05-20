#ifndef TERMINAL_PANE_H
#define TERMINAL_PANE_H

#include <QOpenGLWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QClipboard>
#include <QMenu>

#include "Terminal/TerminalSession.h"
#include "Renderer/GPURenderer.h"
#include "Renderer/RendererFactory.h"

class MemoryFragmentStore;

enum class PaneBorder {
    None,
    Top,
    Right,
    Bottom,
    Left
};

class TerminalPane : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit TerminalPane(QWidget* parent = nullptr);
    ~TerminalPane() override;
    
    bool startShell(const QString& shell = QString(), const QString& workDir = QString());
    void stop();
    void setFontFamily(const QString& family);
    void setFontSize(int size);
    void setColors(const QColor& bg, const QColor& fg);
    
    void write(const QByteArray& data);
    void resizeTerminal(int rows, int cols);
    
    TerminalSession* session() { return m_session; }
    bool isActive() const { return m_isActive; }
    void setActive(bool active);
    
    int paneId() const { return m_paneId; }
    void setPaneId(int id) { m_paneId = id; }
    
    void setBorder(PaneBorder border);
    PaneBorder border() const { return m_border; }
    
signals:
    void focusRequested();
    void closeRequested();
    void splitRequested(Qt::Orientation orientation);
    void dataAvailable(const QByteArray& data);
    void titleChanged(const QString& title);
    void paneResized(int rows, int cols);
    
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    
private slots:
    void onScreenUpdated();
    void onCursorMoved(int row, int col);
    void onSessionTitleChanged(const QString& title);
    void onProcessFinished(int exitCode);
    
private:
    void renderContent();
    void renderCursor();
    void renderSelection();
    void renderBorder();
    void calculateCharDimensions();
    void updateTerminalSize();
    QPoint pixelToChar(const QPoint& pixel) const;
    
    bool hasValidSelection() const;
    QString selectedText() const;
    void copySelectedText();
    void pasteFromClipboard();
    void selectWord(int col, int row);
    void selectLine(int row);
    
    MemoryFragmentStore* memoryStore() const;
    void saveSelectionAsMemory();
    void pasteClipboardAsMemory();
    void createNewMemory();
    void openMemoryViewer();
    
    TerminalSession* m_session;
    GPURenderer* m_renderer;
    
    QTimer* m_renderTimer;
    QTimer* m_cursorBlinkTimer;
    
    int m_paneId;
    bool m_isActive;
    bool m_initialized;
    
    int m_charWidth;
    int m_charHeight;
    int m_rows;
    int m_cols;
    
    CursorInfo m_cursor;
    bool m_cursorVisible;
    bool m_cursorBlinkState;
    
    PaneBorder m_border;
    
    struct Selection {
        bool active = false;
        int startRow = 0;
        int startCol = 0;
        int endRow = 0;
        int endCol = 0;
    } m_selection;
    
    QPoint m_lastMousePos;
    bool m_mouseSelecting = false;
    int m_scrollOffset = 0;
    
    qint64 m_lastClickTime = 0;
    int m_clickCount = 0;
    
    QColor m_activeBorderColor;
    QColor m_inactiveBorderColor;
};

#endif
