#include "TerminalPane.h"
#include <QDebug>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QDir>
#include <QDateTime>

TerminalPane::TerminalPane(QWidget* parent)
    : QOpenGLWidget(parent), m_session(nullptr), m_renderer(nullptr),
      m_renderTimer(nullptr), m_cursorBlinkTimer(nullptr),
      m_paneId(0), m_isActive(false), m_initialized(false),
      m_charWidth(8), m_charHeight(16), m_rows(24), m_cols(80),
      m_cursorVisible(true), m_cursorBlinkState(true),
      m_border(PaneBorder::None),
      m_activeBorderColor(100, 150, 255),
      m_inactiveBorderColor(80, 80, 80) {
    
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
    if (m_renderer) {
        m_renderer->setFontFamily(family);
    }
    calculateCharDimensions();
    updateTerminalSize();
}

void TerminalPane::setFontSize(int size) {
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
        m_renderer->setFontFamily(QStringLiteral("Monospace"));
        m_renderer->setFontSize(14);
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
    renderSelection();
    
    if (m_isActive && m_cursorVisible && m_cursorBlinkState) {
        renderCursor();
    }
    
    renderBorder();
    
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

void TerminalPane::renderCursor() {
    if (!m_session) return;
    
    int x = m_cursor.col * m_charWidth;
    int y = m_cursor.row * m_charHeight;
    
    const QVector<StyledChar>& line = m_session->line(m_cursor.row + m_scrollOffset);
    if (line.size() > m_cursor.col) {
        const StyledChar& sc = line[m_cursor.col];
        QColor cursorColor = sc.reverse ? sc.foreground : sc.background;
        if (!cursorColor.isValid()) {
            cursorColor = m_renderer->foregroundColor();
        }
        m_renderer->appendBackground(x, y, m_charWidth, m_charHeight, cursorColor);
        
        QString ch(sc.character);
        if (!ch.isEmpty() && ch != QStringLiteral(" ")) {
            QColor chColor = sc.reverse ? sc.background : sc.foreground;
            if (!chColor.isValid()) {
                chColor = m_renderer->backgroundColor();
            }
            m_renderer->appendText(ch, x, y, chColor);
        }
    }
}

void TerminalPane::renderSelection() {
    if (!m_selection.active) return;
    
    QColor selectionColor(100, 150, 255, 100);
    
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

void TerminalPane::calculateCharDimensions() {
    QFontMetrics fm(QFont(QStringLiteral("Monospace"), 14));
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
        case Qt::Key_Return: case Qt::Key_Enter: seq = "\r"; break;
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
    for (int row = startRow; row <= endRow && row < m_session->rows(); row++) {
        const QVector<StyledChar>& line = m_session->line(row);
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
    QClipboard* clipboard = QApplication::clipboard();
    if (clipboard && clipboard->text().isEmpty()) {
        return;
    }
    write(clipboard->text().toUtf8());
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
