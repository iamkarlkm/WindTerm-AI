#include "TerminalPane.h"
#include "TerminalWidget.h"
#include "ThemeDialog.h"
#include "MemoryViewerDialog.h"
#include "MemoryEditorDialog.h"
#include "MemoryFragment/MemoryFragmentStore.h"
#include "MemoryFragment/MemoryFragment.h"
#include "AiIntegration/AiClient.h"
#include <QDebug>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QMimeData>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDesktopServices>
#include <QCursor>

TerminalPane::TerminalPane(QWidget* parent)
    : QOpenGLWidget(parent), m_session(nullptr), m_renderer(nullptr),
      m_renderTimer(nullptr), m_cursorBlinkTimer(nullptr),
      m_paneId(0), m_isActive(false), m_initialized(false),
      m_charWidth(8), m_charHeight(16), m_rows(24), m_cols(80),
      m_cursorVisible(true), m_cursorBlinkState(true),
      m_border(PaneBorder::None),
      m_activeBorderColor(100, 150, 255),
      m_inactiveBorderColor(80, 80, 80), m_fontSize(14), m_currentMatchIndex(-1),
      m_scrollBar(nullptr), m_scrollBarVisible(false), m_hoveredUrlIndex(-1),
      m_bellActive(false), m_bellFlashCount(0) {
    
    m_session = new TerminalSession(this);
    
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_InputMethodEnabled);
    
    m_cursorBlinkTimer = new QTimer(this);
    m_cursorBlinkTimer->setInterval(500);
    connect(m_cursorBlinkTimer, &QTimer::timeout, this, [this]() {
        m_cursorBlinkState = !m_cursorBlinkState;
        update();
    });
    
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(16);
    connect(m_renderTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    
    connect(m_session, &TerminalSession::screenUpdated, this, &TerminalPane::onScreenUpdated);
    connect(m_session, &TerminalSession::cursorMoved, this, &TerminalPane::onCursorMoved);
    connect(m_session, &TerminalSession::titleChanged, this, &TerminalPane::onSessionTitleChanged);
    connect(m_session, &TerminalSession::processFinished, this, &TerminalPane::onProcessFinished);
    connect(m_session, &TerminalSession::bellRequested, this, &TerminalPane::onBell);
    
    m_scrollBar = new QScrollBar(Qt::Vertical, this);
    m_scrollBar->hide();
    connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int value) {
        m_scrollOffset = qBound(0, value, m_session->scrollbackSize());
        update();
    });
}

TerminalPane::~TerminalPane() {
    stop();
    if (m_renderer) {
        makeCurrent();
        m_renderer->finalize();
        delete m_renderer;
    }
}

bool TerminalPane::startShell(const QString& shell, const QString& workDir) {
    PtyConfig config;
    config.shell = shell.isEmpty() ? QStringLiteral("/bin/bash") : shell;
    config.workingDirectory = workDir.isEmpty() ? QDir::homePath() : workDir;
    config.rows = m_rows;
    config.cols = m_cols;
    
    bool result = m_session->start(config);
    if (result) {
        m_renderTimer->start();
        m_cursorBlinkTimer->start();
    }
    return result;
}

void TerminalPane::stop() {
    m_session->stop();
    m_renderTimer->stop();
    m_cursorBlinkTimer->stop();
}

void TerminalPane::setFontFamily(const QString& family) {
    m_fontFamily = family;
    if (m_renderer) {
        m_renderer->setFontFamily(family);
    }
    calculateCharDimensions();
    updateTerminalSize();
}

void TerminalPane::setFontSize(int size) {
    m_fontSize = size;
    if (m_renderer) {
        m_renderer->setFontSize(size);
    }
    calculateCharDimensions();
    updateTerminalSize();
}

void TerminalPane::setColors(const QColor& bg, const QColor& fg) {
    if (m_renderer) {
        m_renderer->setBackgroundColor(bg);
        m_renderer->setForegroundColor(fg);
    }
    update();
}

void TerminalPane::setTheme(const ThemeConfig& theme) {
    m_theme = theme;
    m_fontFamily = theme.fontFamily;
    m_fontSize = theme.fontSize;
    
    if (m_renderer) {
        m_renderer->setBackgroundColor(theme.background);
        m_renderer->setForegroundColor(theme.foreground);
        m_renderer->setFontFamily(theme.fontFamily);
        m_renderer->setFontSize(theme.fontSize);
    }
    
    m_activeBorderColor = theme.cursor;
    calculateCharDimensions();
    updateTerminalSize();
    update();
}

void TerminalPane::write(const QByteArray& data) {
    m_session->write(data);
}

void TerminalPane::resizeTerminal(int rows, int cols) {
    m_rows = rows;
    m_cols = cols;
    m_session->resize(rows, cols);
    emit paneResized(rows, cols);
}

void TerminalPane::setActive(bool active) {
    m_isActive = active;
    if (active) {
        m_cursorVisible = true;
        m_cursorBlinkTimer->start();
    } else {
        m_cursorVisible = false;
        m_cursorBlinkTimer->stop();
    }
    update();
}

void TerminalPane::setBorder(PaneBorder border) {
    m_border = border;
    update();
}

void TerminalPane::initializeGL() {
    m_renderer = RendererFactory::createRenderer(RendererBackend::Auto, this);
    if (m_renderer && m_renderer->initialize()) {
        m_fontFamily = QStringLiteral("Monospace");
        m_fontSize = 14;
        m_renderer->setFontFamily(m_fontFamily);
        m_renderer->setFontSize(m_fontSize);
        m_renderer->setBackgroundColor(QColor(30, 30, 30));
        m_renderer->setForegroundColor(QColor(200, 200, 200));
    }
    
    calculateCharDimensions();
    m_initialized = true;
}

void TerminalPane::resizeGL(int w, int h) {
    if (m_renderer) {
        m_renderer->resize(w, h);
    }
    calculateCharDimensions();
    updateTerminalSize();
}

void TerminalPane::paintGL() {
    if (!m_renderer || !m_initialized) return;
    
    m_renderer->clear();
    renderContent();
    renderSearchHighlights();
    renderHyperlinks();
    renderSelection();
    
    if (m_isActive && m_cursorVisible && m_cursorBlinkState) {
        renderCursor();
    }
    
    renderBorder();
    renderBellFlash();
    
    m_renderer->render();
}

void TerminalPane::renderContent() {
    if (!m_session) return;
    
    QColor defaultBg = m_renderer->backgroundColor();
    QColor defaultFg = m_renderer->foregroundColor();
    
    int startRow = m_scrollOffset;
    for (int row = 0; row < m_rows && (startRow + row) < m_session->rows(); row++) {
        const QVector<StyledChar>& line = m_session->line(startRow + row);
        for (int col = 0; col < line.size() && col < m_cols; col++) {
            const StyledChar& sc = line[col];
            
            int x = col * m_charWidth;
            int y = row * m_charHeight;
            
            QColor fg = sc.foreground.isValid() ? sc.foreground : defaultFg;
            QColor bg = sc.background.isValid() ? sc.background : defaultBg;
            
            if (sc.reverse) {
                qSwap(fg, bg);
            }
            
            if (bg != defaultBg) {
                m_renderer->appendBackground(x, y, m_charWidth, m_charHeight, bg);
            }
            
            QString ch(sc.character);
            if (sc.hidden) {
                ch = QStringLiteral(" ");
            }
            
            if (!ch.isEmpty() && ch != QStringLiteral(" ")) {
                m_renderer->appendText(ch, x, y, fg);
            }
            
            if (sc.underline) {
                m_renderer->appendBackground(x, y + m_charHeight - 1, m_charWidth, 1, fg);
            }
            
            if (sc.strikeThrough) {
                m_renderer->appendBackground(x, y + m_charHeight / 2, m_charWidth, 1, fg);
            }
        }
    }
}

void TerminalPane::renderSearchHighlights() {
    if (m_searchMatches.isEmpty()) return;
    
    QColor searchColor(255, 255, 0, 80);
    QColor currentSearchColor(255, 165, 0, 120);
    
    for (int i = 0; i < m_searchMatches.size(); i++) {
        const SearchMatch& match = m_searchMatches[i];
        int row = match.row - m_scrollOffset;
        
        if (row < 0 || row >= m_rows) continue;
        
        QColor color = (i == m_currentMatchIndex) ? currentSearchColor : searchColor;
        
        for (int c = 0; c < match.length && (match.col + c) < m_cols; c++) {
            int x = (match.col + c) * m_charWidth;
            int y = row * m_charHeight;
            m_renderer->appendBackground(x, y, m_charWidth, m_charHeight, color);
        }
    }
}

void TerminalPane::renderHyperlinks() {
    for (int i = 0; i < m_urlMatches.size(); i++) {
        const UrlMatch& match = m_urlMatches[i];
        int row = match.row - m_scrollOffset;
        
        if (row < 0 || row >= m_rows) continue;
        if (!match.isHyperlink) continue;
        
        for (int c = 0; c < match.length && (match.col + c) < m_cols; c++) {
            int x = (match.col + c) * m_charWidth;
            int y = row * m_charHeight + m_charHeight - 1;
            
            if (i == m_hoveredUrlIndex) {
                m_renderer->appendBackground(x, y, m_charWidth, 2, QColor(0, 120, 215));
            } else {
                m_renderer->appendBackground(x, y, m_charWidth, 1, QColor(0, 120, 215, 180));
            }
        }
    }
}

void TerminalPane::renderCursor() {
    if (!m_session) return;
    
    int x = m_cursor.col * m_charWidth;
    int y = m_cursor.row * m_charHeight;
    
    QColor cursorColor = m_theme.cursor;
    QColor cursorTextColor = m_theme.cursorText;
    
    const QVector<StyledChar>& line = m_session->line(m_cursor.row + m_scrollOffset);
    if (line.size() > m_cursor.col) {
        const StyledChar& sc = line[m_cursor.col];
        m_renderer->appendBackground(x, y, m_charWidth, m_charHeight, cursorColor);
        
        QString ch(sc.character);
        if (!ch.isEmpty() && ch != QStringLiteral(" ")) {
            m_renderer->appendText(ch, x, y, cursorTextColor);
        }
    }
}

void TerminalPane::renderSelection() {
    if (!m_selection.active) return;
    
    QColor selectionColor = m_theme.selection;
    
    int startRow = qMin(m_selection.startRow, m_selection.endRow);
    int endRow = qMax(m_selection.startRow, m_selection.endRow);
    int startCol = qMin(m_selection.startCol, m_selection.endCol);
    int endCol = qMax(m_selection.startCol, m_selection.endCol);
    
    for (int row = startRow; row <= endRow && row < m_rows; row++) {
        int colStart = (row == startRow) ? startCol : 0;
        int colEnd = (row == endRow) ? endCol : m_cols;
        
        for (int col = colStart; col <= colEnd && col < m_cols; col++) {
            int x = col * m_charWidth;
            int y = row * m_charHeight;
            m_renderer->appendBackground(x, y, m_charWidth, m_charHeight, selectionColor);
        }
    }
}

void TerminalPane::renderBorder() {
    if (m_border == PaneBorder::None) return;
    
    QColor borderColor = m_isActive ? m_activeBorderColor : m_inactiveBorderColor;
    int borderWidth = 2;
    
    QRect rect = this->rect();
    switch (m_border) {
        case PaneBorder::Top:
            m_renderer->appendBackground(0, 0, rect.width(), borderWidth, borderColor);
            break;
        case PaneBorder::Right:
            m_renderer->appendBackground(rect.width() - borderWidth, 0, borderWidth, rect.height(), borderColor);
            break;
        case PaneBorder::Bottom:
            m_renderer->appendBackground(0, rect.height() - borderWidth, rect.width(), borderWidth, borderColor);
            break;
        case PaneBorder::Left:
            m_renderer->appendBackground(0, 0, borderWidth, rect.height(), borderColor);
            break;
        default:
            break;
    }
}

void TerminalPane::renderBellFlash() {
    if (!m_bellActive || m_bellFlashCount <= 0) return;
    
    QColor flashColor(255, 255, 0, 30);
    m_renderer->appendBackground(0, 0, width(), height(), flashColor);
}

void TerminalPane::calculateCharDimensions() {
    QString family = m_fontFamily.isEmpty() ? QStringLiteral("Monospace") : m_fontFamily;
    int size = m_fontSize > 0 ? m_fontSize : 14;
    QFontMetrics fm(QFont(family, size));
    m_charWidth = fm.horizontalAdvance(QStringLiteral("M"));
    m_charHeight = fm.height();
    
    if (m_charWidth <= 0) m_charWidth = 8;
    if (m_charHeight <= 0) m_charHeight = 16;
}

void TerminalPane::updateTerminalSize() {
    int newCols = width() / m_charWidth;
    int newRows = height() / m_charHeight;
    
    if (newCols > 0 && newRows > 0 && (newCols != m_cols || newRows != m_rows)) {
        resizeTerminal(newRows, newCols);
    }
}

QPoint TerminalPane::pixelToChar(const QPoint& pixel) const {
    return QPoint(pixel.x() / m_charWidth, pixel.y() / m_charHeight);
}

void TerminalPane::keyPressEvent(QKeyEvent* event) {
    Qt::KeyboardModifiers mods = event->modifiers();
    
    if (mods & Qt::ControlModifier && mods & Qt::ShiftModifier) {
        if (event->key() == Qt::Key_C) {
            copySelectedText();
            return;
        }
        if (event->key() == Qt::Key_V) {
            pasteFromClipboard();
            return;
        }
        if (event->key() == Qt::Key_H) {
            emit splitRequested(Qt::Horizontal);
            return;
        }
        if (event->key() == Qt::Key_R) {
            QWidget* p = parentWidget();
            while (p) {
                if (auto* widget = qobject_cast<TerminalWidget*>(p)) {
                    widget->showCommandSearchDialog();
                    return;
                }
                p = p->parentWidget();
            }
        }
        if (event->key() == Qt::Key_B) {
            QWidget* p = parentWidget();
            while (p) {
                if (auto* widget = qobject_cast<TerminalWidget*>(p)) {
                    widget->showBookmarksDialog();
                    return;
                }
                p = p->parentWidget();
            }
        }
        if (event->key() == Qt::Key_A) {
            QWidget* p = parentWidget();
            while (p) {
                if (auto* widget = qobject_cast<TerminalWidget*>(p)) {
                    widget->showAiAssistantDialog();
                    return;
                }
                p = p->parentWidget();
            }
        }
        if (event->key() == Qt::Key_F) {
            QWidget* p = parentWidget();
            while (p) {
                if (auto* widget = qobject_cast<TerminalWidget*>(p)) {
                    widget->showFileTransferDialog();
                    return;
                }
                p = p->parentWidget();
            }
        }
        if (event->key() == Qt::Key_G) {
            QWidget* p = parentWidget();
            while (p) {
                if (auto* widget = qobject_cast<TerminalWidget*>(p)) {
                    widget->showTerminalSearchDialog();
                    return;
                }
                p = p->parentWidget();
            }
        }
    }
    
    if (mods & Qt::ControlModifier) {
        QByteArray ctrl;
        if (event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z) {
            ctrl.append(static_cast<char>(event->key() - Qt::Key_A + 1));
            write(ctrl);
        }
        return;
    }
    
    QByteArray seq;
    switch (event->key()) {
        case Qt::Key_Return: case Qt::Key_Enter:
        seq = "\r";
        processAiTrigger();
        break;
        case Qt::Key_Backspace: seq = "\x7f"; break;
        case Qt::Key_Escape: seq = "\x1b"; break;
        case Qt::Key_Tab: seq = "\t"; break;
        case Qt::Key_Up: seq = mods & Qt::ShiftModifier ? "\x1b[1;2A" : "\x1b[A"; break;
        case Qt::Key_Down: seq = mods & Qt::ShiftModifier ? "\x1b[1;2B" : "\x1b[B"; break;
        case Qt::Key_Right: seq = mods & Qt::ShiftModifier ? "\x1b[1;2C" : "\x1b[C"; break;
        case Qt::Key_Left: seq = mods & Qt::ShiftModifier ? "\x1b[1;2D" : "\x1b[D"; break;
        case Qt::Key_Home: seq = "\x1b[H"; break;
        case Qt::Key_End: seq = "\x1b[F"; break;
        case Qt::Key_PageUp: seq = "\x1b[5~"; break;
        case Qt::Key_PageDown: seq = "\x1b[6~"; break;
        case Qt::Key_Insert: seq = "\x1b[2~"; break;
        case Qt::Key_Delete: seq = "\x1b[3~"; break;
        default:
            if (!event->text().isEmpty()) {
                seq = event->text().toUtf8();
            }
            break;
    }
    
    if (!seq.isEmpty()) {
        write(seq);
    }
    
    m_cursorBlinkState = true;
    m_cursorVisible = true;
    if (m_cursorBlinkTimer->isActive()) {
        m_cursorBlinkTimer->start();
    }
}

void TerminalPane::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() & Qt::ControlModifier) {
            openUrlAtPosition(event->x(), event->y());
            return;
        }
        
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        if (currentTime - m_lastClickTime < 500) {
            m_clickCount++;
        } else {
            m_clickCount = 1;
        }
        m_lastClickTime = currentTime;
        
        QPoint charPos = pixelToChar(event->pos());
        
        if (m_clickCount >= 3) {
            selectLine(charPos.y());
            m_selection.active = true;
            m_mouseSelecting = false;
        } else if (m_clickCount == 2) {
            selectWord(charPos.x(), charPos.y());
            m_selection.active = true;
            m_mouseSelecting = true;
        } else {
            m_selection.startRow = charPos.y();
            m_selection.startCol = charPos.x();
            m_selection.endRow = charPos.y();
            m_selection.endCol = charPos.x();
            m_selection.active = true;
            m_mouseSelecting = true;
        }
        
        m_lastMousePos = event->pos();
        emit focusRequested();
        update();
    }
}

void TerminalPane::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_mouseSelecting) {
        m_mouseSelecting = false;
        if (m_selection.active && hasValidSelection()) {
            copySelectedText();
        }
    }
}

void TerminalPane::mouseMoveEvent(QMouseEvent* event) {
    int row = event->y() / m_charHeight;
    int col = event->x() / m_charWidth;
    
    int hoveredIndex = -1;
    for (int i = 0; i < m_urlMatches.size(); i++) {
        const UrlMatch& match = m_urlMatches[i];
        int visibleRow = match.row - m_scrollOffset;
        if (visibleRow < 0 || visibleRow >= m_rows) continue;
        
        if (visibleRow == row && col >= match.col && col < match.col + match.length) {
            hoveredIndex = i;
            break;
        }
    }
    
    if (hoveredIndex != m_hoveredUrlIndex) {
        m_hoveredUrlIndex = hoveredIndex;
        if (m_hoveredUrlIndex >= 0) {
            setCursor(QCursor(Qt::PointingHandCursor));
        } else {
            setCursor(QCursor(Qt::ArrowCursor));
        }
        update();
    }
    
    if (m_mouseSelecting) {
        QPoint charPos = pixelToChar(event->pos());
        m_selection.endRow = qBound(0, charPos.y(), m_rows - 1);
        m_selection.endCol = qBound(0, charPos.x(), m_cols - 1);
        update();
    }
    m_lastMousePos = event->pos();
}

void TerminalPane::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y() / 120;
    m_scrollOffset = qBound(0, m_scrollOffset - delta * 3, m_session->scrollbackSize());
    update();
    event->accept();
}

void TerminalPane::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    
    bool hasSelection = hasValidSelection();
    
    if (hasSelection) {
        QAction* saveAsMemoryAction = menu.addAction(QStringLiteral("保存为记忆碎片"));
        connect(saveAsMemoryAction, &QAction::triggered, this, [this]() {
            saveSelectionAsMemory();
        });
        menu.addSeparator();
    }
    
    QAction* copyAction = menu.addAction(QStringLiteral("Copy"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &TerminalPane::copySelectedText);
    
    QAction* pasteAction = menu.addAction(QStringLiteral("Paste"));
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &TerminalPane::pasteFromClipboard);
    
    menu.addSeparator();
    
    QAction* selectAllAction = menu.addAction(QStringLiteral("Select All"));
    connect(selectAllAction, &QAction::triggered, this, [this]() {
        m_selection.active = true;
        m_selection.startRow = 0;
        m_selection.startCol = 0;
        m_selection.endRow = m_rows - 1;
        m_selection.endCol = m_cols - 1;
        update();
    });
    
    menu.addSeparator();
    
    QAction* pasteAsMemoryAction = menu.addAction(QStringLiteral("剪贴板粘贴为记忆碎片"));
    connect(pasteAsMemoryAction, &QAction::triggered, this, &TerminalPane::pasteClipboardAsMemory);
    
    QAction* newMemoryAction = menu.addAction(QStringLiteral("新增记忆碎片"));
    connect(newMemoryAction, &QAction::triggered, this, &TerminalPane::createNewMemory);
    
    menu.addSeparator();
    
    QAction* viewMemoriesAction = menu.addAction(QStringLiteral("查看记忆碎片"));
    connect(viewMemoriesAction, &QAction::triggered, this, &TerminalPane::openMemoryViewer);
    
    menu.addSeparator();
    
    QAction* themeAction = menu.addAction(QStringLiteral("主题设置"));
    connect(themeAction, &QAction::triggered, this, [this]() {
        QWidget* p = parentWidget();
        while (p) {
            if (auto* widget = qobject_cast<TerminalWidget*>(p)) {
                widget->showThemeDialog();
                break;
            }
            p = p->parentWidget();
        }
    });
    
    QAction* splitHAction = menu.addAction(QStringLiteral("Split Horizontal"));
    connect(splitHAction, &QAction::triggered, this, [this]() {
        emit splitRequested(Qt::Horizontal);
    });
    
    QAction* splitVAction = menu.addAction(QStringLiteral("Split Vertical"));
    connect(splitVAction, &QAction::triggered, this, [this]() {
        emit splitRequested(Qt::Vertical);
    });
    
    menu.addSeparator();
    
    QAction* closeAction = menu.addAction(QStringLiteral("Close Pane"));
    connect(closeAction, &QAction::triggered, this, &TerminalPane::closeRequested);
    
    menu.exec(event->globalPos());
}

void TerminalPane::focusInEvent(QFocusEvent* event) {
    setActive(true);
    QOpenGLWidget::focusInEvent(event);
}

void TerminalPane::focusOutEvent(QFocusEvent* event) {
    setActive(false);
    QOpenGLWidget::focusOutEvent(event);
}

void TerminalPane::onScreenUpdated() {
    m_cursor = m_session->cursor();
    
    int scrollbackSize = m_session->scrollbackSize();
    m_scrollBarVisible = scrollbackSize > 0;
    
    if (m_scrollBar) {
        if (m_scrollBarVisible) {
            m_scrollBar->setRange(0, scrollbackSize);
            m_scrollBar->setPageStep(m_rows);
            m_scrollBar->setSingleStep(3);
            m_scrollBar->setValue(m_scrollOffset);
            m_scrollBar->show();
            int barWidth = 14;
            m_scrollBar->setGeometry(width() - barWidth, 0, barWidth, height());
        } else {
            m_scrollBar->hide();
        }
    }
    
    detectUrls();
    update();
}

void TerminalPane::onCursorMoved(int row, int col) {
    m_cursor.row = row;
    m_cursor.col = col;
    update();
}

void TerminalPane::onSessionTitleChanged(const QString& title) {
    emit titleChanged(title);
}

void TerminalPane::onProcessFinished(int exitCode) {
    qDebug() << "[TerminalPane]" << m_paneId << "process finished with code:" << exitCode;
}

void TerminalPane::onBell() {
    QApplication::beep();
    
    m_bellActive = true;
    m_bellFlashCount = 3;
    
    QTimer* bellTimer = new QTimer(this);
    connect(bellTimer, &QTimer::timeout, this, [this, bellTimer]() {
        m_bellFlashCount--;
        update();
        
        if (m_bellFlashCount <= 0) {
            m_bellActive = false;
            bellTimer->stop();
            bellTimer->deleteLater();
        }
    });
    bellTimer->start(100);
}

MemoryFragmentStore* TerminalPane::memoryStore() const {
    QWidget* p = parentWidget();
    while (p) {
        if (auto* widget = qobject_cast<TerminalWidget*>(p)) {
            return widget->memoryStore();
        }
        p = p->parentWidget();
    }
    return MemoryFragmentStore::instance();
}

void TerminalPane::saveSelectionAsMemory() {
    QString text = selectedText();
    if (text.isEmpty()) return;
    
    MemoryFragmentStore* store = memoryStore();
    if (!store->isInitialized()) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("记忆碎片数据库未初始化。"));
        return;
    }
    
    MemoryFragmentContext context = MemoryFragmentContext::current();
    
    MemoryFragment fragment;
    fragment.content = text;
    fragment.sourceType = "selection";
    fragment.sourceRemark = QStringLiteral("从终端选择保存");
    fragment.terminalType = context.terminalType;
    fragment.workingDirectory = context.workingDirectory;
    fragment.sessionId = context.sessionId;
    
    MemoryEditorDialog editor(store, fragment, this);
    if (editor.exec() == QDialog::Accepted) {
        QMessageBox::information(this, QStringLiteral("保存成功"), QStringLiteral("记忆碎片已保存。"));
    }
}

void TerminalPane::pasteClipboardAsMemory() {
    QClipboard* clipboard = QApplication::clipboard();
    if (!clipboard || clipboard->text().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("剪贴板为空。"));
        return;
    }
    
    MemoryFragmentStore* store = memoryStore();
    if (!store->isInitialized()) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("记忆碎片数据库未初始化。"));
        return;
    }
    
    MemoryFragment fragment;
    fragment.content = clipboard->text();
    fragment.sourceType = "clipboard";
    fragment.sourceRemark = QStringLiteral("从剪贴板粘贴");
    
    MemoryEditorDialog editor(store, fragment, this);
    if (editor.exec() == QDialog::Accepted) {
        QMessageBox::information(this, QStringLiteral("保存成功"), QStringLiteral("记忆碎片已保存。"));
    }
}

void TerminalPane::createNewMemory() {
    MemoryFragmentStore* store = memoryStore();
    if (!store->isInitialized()) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("记忆碎片数据库未初始化。"));
        return;
    }
    
    MemoryFragmentContext context = MemoryFragmentContext::current();
    
    MemoryFragment fragment;
    fragment.sourceType = "manual";
    fragment.terminalType = context.terminalType;
    fragment.workingDirectory = context.workingDirectory;
    fragment.sessionId = context.sessionId;
    
    MemoryEditorDialog editor(store, fragment, this);
    if (editor.exec() == QDialog::Accepted) {
        QMessageBox::information(this, QStringLiteral("保存成功"), QStringLiteral("记忆碎片已保存。"));
    }
}

void TerminalPane::openMemoryViewer() {
    MemoryFragmentStore* store = memoryStore();
    if (!store->isInitialized()) {
        QMessageBox::warning(this, QStringLiteral("查看失败"), QStringLiteral("记忆碎片数据库未初始化。"));
        return;
    }
    
    MemoryViewerDialog viewer(store, this);
    viewer.exec();
}

bool TerminalPane::hasValidSelection() const {
    if (!m_selection.active) return false;
    return m_selection.startRow != m_selection.endRow || 
           m_selection.startCol != m_selection.endCol;
}

QString TerminalPane::selectedText() const {
    if (!hasValidSelection()) return QString();
    
    int startRow = qMin(m_selection.startRow, m_selection.endRow);
    int endRow = qMax(m_selection.startRow, m_selection.endRow);
    int startCol = qMin(m_selection.startCol, m_selection.endCol);
    int endCol = qMax(m_selection.startCol, m_selection.endCol);
    
    QString result;
    for (int row = startRow; row <= endRow && (m_scrollOffset + row) < m_session->rows(); row++) {
        const QVector<StyledChar>& line = m_session->line(m_scrollOffset + row);
        int colStart = (row == startRow) ? startCol : 0;
        int colEnd = (row == endRow) ? endCol : qMin(m_cols, line.size());
        
        for (int col = colStart; col < colEnd && col < line.size(); col++) {
            result.append(line[col].character);
        }
        
        if (row < endRow) {
            result.append('\n');
        }
    }
    
    return result;
}

void TerminalPane::copySelectedText() {
    QString text = selectedText();
    if (!text.isEmpty()) {
        QClipboard* clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(text);
        }
    }
}

void TerminalPane::pasteFromClipboard() {
    if (m_session) {
        m_session->pasteFromClipboard();
    }
}

void TerminalPane::selectWord(int col, int row) {
    if (!m_session || row < 0 || row >= m_session->rows()) return;
    
    const QVector<StyledChar>& line = m_session->line(row);
    if (col < 0 || col >= line.size()) return;
    
    QChar ch = line[col].character;
    bool isWordChar = !ch.isSpace() && ch != QLatin1Char('\0');
    
    int wordStart = col;
    int wordEnd = col;
    
    if (isWordChar) {
        while (wordStart > 0 && !line[wordStart - 1].character.isSpace()) {
            wordStart--;
        }
        while (wordEnd < line.size() - 1 && !line[wordEnd + 1].character.isSpace()) {
            wordEnd++;
        }
    }
    
    m_selection.active = true;
    m_selection.startRow = row;
    m_selection.endRow = row;
    m_selection.startCol = wordStart;
    m_selection.endCol = wordEnd;
}

void TerminalPane::selectLine(int row) {
    if (!m_session || row < 0 || row >= m_session->rows()) return;
    
    m_selection.active = true;
    m_selection.startRow = row;
    m_selection.endRow = row;
    m_selection.startCol = 0;
    m_selection.endCol = m_cols - 1;
}

void TerminalPane::processAiTrigger() {
    QWidget* p = parentWidget();
    TerminalWidget* widget = nullptr;
    while (p) {
        widget = qobject_cast<TerminalWidget*>(p);
        if (widget) break;
        p = p->parentWidget();
    }
    if (!widget || !widget->aiClient()) return;
    
    int lastRow = qMax(0, m_session->rows() - 1 + m_scrollOffset);
    const QVector<StyledChar>& line = m_session->line(lastRow);
    
    QString lineText;
    for (const auto& ch : line) {
        lineText += ch.character;
    }
    lineText = lineText.trimmed();
    
    if (lineText.startsWith("/ai ")) {
        QString query = lineText.mid(4).trimmed();
        if (query.isEmpty()) return;
        
        QString workingDir = m_session->ptyManager() ? m_session->ptyManager()->workingDirectory() : QString();
        QStringList recentCommands;
        
        widget->aiClient()->sendPrompt(query, workingDir);
    } else if (lineText.endsWith("!!")) {
        QString baseCmd = lineText.left(lineText.length() - 2).trimmed();
        QString query = baseCmd.isEmpty() ? 
            QStringLiteral("解释一下上一条命令的作用") : 
            QString("解释一下命令 '%1' 的作用").arg(baseCmd);
        
        QString workingDir = m_session->ptyManager() ? m_session->ptyManager()->workingDirectory() : QString();
        widget->aiClient()->sendPrompt(query, workingDir);
        
        widget->showAiAssistantDialog();
    }
}

void TerminalPane::searchInBuffer(const QString& text, bool forward) {
    if (!m_session || text.isEmpty()) {
        clearSearchHighlight();
        return;
    }
    
    m_searchText = text;
    m_searchMatches.clear();
    m_currentMatchIndex = -1;
    
    int totalRows = m_session->rows() + m_session->scrollbackSize();
    QString searchText = text.toLower();
    
    for (int row = 0; row < totalRows; row++) {
        const QVector<StyledChar>& line = m_session->line(row);
        QString lineText;
        for (const auto& ch : line) {
            lineText += ch.character;
        }
        
        int pos = 0;
        while ((pos = lineText.toLower().indexOf(searchText, pos)) != -1) {
            SearchMatch match;
            match.row = row;
            match.col = pos;
            match.length = searchText.length();
            m_searchMatches.append(match);
            pos += searchText.length();
        }
    }
    
    if (!m_searchMatches.isEmpty()) {
        if (forward) {
            m_currentMatchIndex = 0;
        } else {
            m_currentMatchIndex = m_searchMatches.size() - 1;
        }
        
        int matchRow = m_searchMatches[m_currentMatchIndex].row;
        int visibleStart = m_scrollOffset;
        int visibleEnd = m_scrollOffset + m_rows - 1;
        
        if (matchRow < visibleStart) {
            m_scrollOffset = matchRow;
        } else if (matchRow > visibleEnd) {
            m_scrollOffset = matchRow - m_rows + 1;
        }
        
        m_scrollOffset = qBound(0, m_scrollOffset, m_session->scrollbackSize());
    }
    
    update();
}

void TerminalPane::clearSearchHighlight() {
    m_searchText.clear();
    m_searchMatches.clear();
    m_currentMatchIndex = -1;
    update();
}

void TerminalPane::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void TerminalPane::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void TerminalPane::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();
    if (!mimeData->hasUrls()) return;
    
    QStringList paths;
    for (const QUrl& url : mimeData->urls()) {
        QString path = url.toLocalFile();
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }
    
    if (paths.isEmpty()) return;
    
    if (paths.size() == 1) {
        QString path = paths.first();
        if (path.contains(' ')) {
            path.replace(QLatin1String("'"), QLatin1String("'\\''"));
            path = QStringLiteral("'%1'").arg(path);
        }
        write(path.toUtf8());
    } else {
        for (QString path : paths) {
            if (path.contains(' ')) {
                path.replace(QLatin1String("'"), QLatin1String("'\\''"));
                path = QStringLiteral("'%1'").arg(path);
            }
            write(path.toUtf8());
            write(" ");
        }
    }
}

void TerminalPane::resizeEvent(QResizeEvent* event) {
    QOpenGLWidget::resizeEvent(event);
    
    if (m_scrollBar && m_scrollBarVisible) {
        int barWidth = 14;
        m_scrollBar->setGeometry(width() - barWidth, 0, barWidth, height());
    }
}

void TerminalPane::leaveEvent(QEvent* event) {
    m_hoveredUrlIndex = -1;
    setCursor(QCursor(Qt::ArrowCursor));
    QOpenGLWidget::leaveEvent(event);
}

void TerminalPane::detectUrls() {
    m_urlMatches.clear();
    
    if (!m_session) return;
    
    int totalRows = m_session->rows();
    
    for (int row = 0; row < totalRows && row < m_rows + m_scrollOffset; row++) {
        const QVector<StyledChar>& cells = m_session->line(row);
        QString lineText;
        int hyperlinkStart = -1;
        QString currentHyperlink;
        
        for (int col = 0; col < cells.size(); col++) {
            const StyledChar& cell = cells[col];
            lineText += cell.character;
            
            if (!cell.hyperlink.isEmpty()) {
                if (hyperlinkStart < 0) {
                    hyperlinkStart = col;
                    currentHyperlink = cell.hyperlink;
                }
            } else if (hyperlinkStart >= 0) {
                UrlMatch match;
                match.row = row;
                match.col = hyperlinkStart;
                match.length = col - hyperlinkStart;
                match.url = currentHyperlink;
                match.isFile = false;
                match.isHyperlink = true;
                m_urlMatches.append(match);
                
                hyperlinkStart = -1;
                currentHyperlink.clear();
            }
        }
        
        if (hyperlinkStart >= 0) {
            UrlMatch match;
            match.row = row;
            match.col = hyperlinkStart;
            match.length = cells.size() - hyperlinkStart;
            match.url = currentHyperlink;
            match.isFile = false;
            match.isHyperlink = true;
            m_urlMatches.append(match);
        }
        
        QVector<UrlMatch> regexMatches = m_urlDetector.findUrls(lineText, row, 0);
        for (const UrlMatch& m : regexMatches) {
            bool alreadyExists = false;
            for (const UrlMatch& existing : m_urlMatches) {
                if (existing.row == m.row && existing.col == m.col) {
                    alreadyExists = true;
                    break;
                }
            }
            if (!alreadyExists) {
                m_urlMatches.append(m);
            }
        }
    }
}

void TerminalPane::openUrlAtPosition(int x, int y) {
    int row = y / m_charHeight;
    int col = x / m_charWidth;
    
    for (int i = 0; i < m_urlMatches.size(); i++) {
        const UrlMatch& match = m_urlMatches[i];
        
        int visibleRow = match.row - m_scrollOffset;
        if (visibleRow < 0 || visibleRow >= m_rows) continue;
        
        if (visibleRow == row && col >= match.col && col < match.col + match.length) {
            QString url = match.url;
            
            if (match.isFile) {
                if (url.startsWith("~")) {
                    url = QDir::homePath() + url.mid(1);
                }
                QUrl fileUrl = QUrl::fromLocalFile(url);
                QDesktopServices::openUrl(fileUrl);
            } else {
                if (!url.startsWith("http") && !url.startsWith("ftp") && !url.startsWith("ssh")) {
                    url = "http://" + url;
                }
                QDesktopServices::openUrl(QUrl(url));
            }
            return;
        }
    }
}
