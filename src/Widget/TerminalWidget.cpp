#include "TerminalWidget.h"
#include <QDebug>
#include <QApplication>
#include <QRegularExpression>
#include <QFontMetrics>

TerminalWidget::TerminalWidget(QWidget* parent)
    : QOpenGLWidget(parent), m_renderer(nullptr), m_buffer(nullptr),
      m_renderTimer(nullptr), m_cursorBlinkTimer(nullptr),
      m_cursorPos{0, 0}, m_cursorVisible(true), m_cursorBlinkState(true) {
    
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_InputMethodEnabled);
    
    m_buffer = new CircularTextBuffer(m_config.bufferCapacity);
    
    m_cursorBlinkTimer = new QTimer(this);
    m_cursorBlinkTimer->setInterval(500);
    connect(m_cursorBlinkTimer, &QTimer::timeout, this, &TerminalWidget::onCursorBlink);
    
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(16);
    connect(m_renderTimer, &QTimer::timeout, this, &TerminalWidget::onRenderTimer);
    
    m_displayLines.resize(m_config.rows);
}

TerminalWidget::TerminalWidget(const TerminalConfig& config, QWidget* parent)
    : TerminalWidget(parent) {
    m_config = config;
    m_buffer->setCapacity(m_config.bufferCapacity);
    m_displayLines.resize(m_config.rows);
}

TerminalWidget::~TerminalWidget() {
    if (m_renderer) {
        makeCurrent();
        m_renderer->finalize();
        delete m_renderer;
    }
    delete m_buffer;
}

void TerminalWidget::initializeGL() {
    initRenderer();
    
    m_renderTimer->start();
    m_cursorBlinkTimer->start();
    
    m_lastFpsUpdate = QDateTime::currentMSecsSinceEpoch();
    m_initialized = true;
    
    qDebug() << "[TerminalWidget] Initialized with backend:" 
             << PlatformDetector::backendToString(m_config.backend);
}

void TerminalWidget::initRenderer() {
    if (m_renderer) {
        m_renderer->finalize();
        delete m_renderer;
    }
    
    m_renderer = RendererFactory::createRenderer(m_config.backend, this);
    if (!m_renderer) {
        qCritical() << "[TerminalWidget] Failed to create renderer";
        return;
    }
    
    if (!m_renderer->initialize()) {
        qCritical() << "[TerminalWidget] Failed to initialize renderer";
        return;
    }
    
    m_renderer->setFontFamily(m_config.fontFamily);
    m_renderer->setFontSize(m_config.fontSize);
    m_renderer->setBackgroundColor(m_config.backgroundColor);
    m_renderer->setForegroundColor(m_config.foregroundColor);
}

void TerminalWidget::resizeGL(int w, int h) {
    if (!m_renderer) return;
    
    m_renderer->resize(w, h);
    
    int newCols = w / charWidth();
    int newRows = h / charHeight();
    
    if (newCols != m_config.columns || newRows != m_config.rows) {
        m_config.columns = qMax(1, newCols);
        m_config.rows = qMax(1, newRows);
        m_displayLines.resize(m_config.rows);
        updateScrollPosition();
    }
}

void TerminalWidget::paintGL() {
    if (!m_renderer || !m_initialized) return;
    
    m_renderer->clear();
    
    renderText();
    renderCursor();
    renderSelection();
    
    m_renderer->render();
    
    m_frameCount++;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastFpsUpdate >= 1000) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_lastFpsUpdate = now;
    }
}

void TerminalWidget::renderText() {
    int cw = charWidth();
    int ch = charHeight();
    
    int startLine = m_scrollOffset;
    for (int row = 0; row < m_config.rows; row++) {
        int bufferIdx = startLine + row;
        QString lineText;
        
        if (bufferIdx < m_buffer->size()) {
            lineText = m_buffer->lineAt(bufferIdx).text;
        }
        
        if (!lineText.isEmpty()) {
            int x = 0;
            int y = row * ch;
            m_renderer->appendText(lineText, x, y);
        }
    }
}

void TerminalWidget::renderCursor() {
    if (!m_cursorVisible || !m_cursorBlinkState) return;
    
    int cw = charWidth();
    int ch = charHeight();
    int x = m_cursorPos.x * cw;
    int y = m_cursorPos.y * ch;
    
    m_renderer->appendText(QStringLiteral("\u2588"), x, y);
}

void TerminalWidget::renderSelection() {
    if (!m_selection.active) return;
}

void TerminalWidget::write(const QString& text) {
    QMetaObject::invokeMethod(this, [this, text]() {
        m_buffer->append(text);
        updateScrollPosition();
        update();
    }, Qt::QueuedConnection);
}

void TerminalWidget::write(const QByteArray& data) {
    write(QString::fromUtf8(data));
}

void TerminalWidget::clear() {
    m_buffer->clear();
    m_cursorPos = {0, 0};
    m_scrollOffset = 0;
    m_selection.active = false;
    m_selectedText.clear();
    update();
}

void TerminalWidget::scrollToTop() {
    m_scrollOffset = qMax(0, m_buffer->size() - m_config.rows);
    updateScrollPosition();
    update();
}

void TerminalWidget::scrollToBottom() {
    m_scrollOffset = 0;
    updateScrollPosition();
    update();
}

void TerminalWidget::scroll(int lines) {
    m_scrollOffset = qBound(0, m_scrollOffset + lines, 
                           qMax(0, m_buffer->size() - m_config.rows));
    updateScrollPosition();
    update();
}

int TerminalWidget::scrollbackSize() const {
    return qMax(0, m_buffer->size() - m_config.rows);
}

void TerminalWidget::setConfig(const TerminalConfig& config) {
    bool fontChanged = (m_config.fontFamily != config.fontFamily || 
                       m_config.fontSize != config.fontSize);
    m_config = config;
    
    if (m_initialized && fontChanged && m_renderer) {
        m_renderer->setFontFamily(m_config.fontFamily);
        m_renderer->setFontSize(m_config.fontSize);
    }
}

void TerminalWidget::updateScrollPosition() {
    m_maxScrollback = qMax(0, m_buffer->size() - m_config.rows);
    emit scrollPositionChanged(m_scrollOffset, m_maxScrollback);
}

int TerminalWidget::charWidth() const {
    return m_config.fontSize;
}

int TerminalWidget::charHeight() const {
    return m_config.fontSize + 2;
}

int TerminalWidget::linePixelWidth() const {
    return m_config.columns * charWidth();
}

QPoint TerminalWidget::pixelToChar(const QPoint& pixel) const {
    return QPoint(pixel.x() / charWidth(), pixel.y() / charHeight());
}

QPoint TerminalWidget::charToPixel(int col, int row) const {
    return QPoint(col * charWidth(), row * charHeight());
}

void TerminalWidget::selectWord(int col, int row) {
    if (row >= m_buffer->size()) return;
    
    QString line = m_buffer->lineAt(row).text;
    if (col >= line.length()) return;
    
    QRegularExpression wordRegex(QStringLiteral(R"(\w+)"));
    int wordStart = 0;
    int wordEnd = line.length();
    
    for (int i = col; i >= 0; i--) {
        if (!line[i].isLetterOrNumber()) {
            wordStart = i + 1;
            break;
        }
    }
    
    for (int i = col; i < line.length(); i++) {
        if (!line[i].isLetterOrNumber()) {
            wordEnd = i;
            break;
        }
    }
    
    m_selection.active = true;
    m_selection.startCol = wordStart;
    m_selection.startRow = row;
    m_selection.endCol = wordEnd;
    m_selection.endRow = row;
    m_selectedText = line.mid(wordStart, wordEnd - wordStart);
    
    emit selectionChanged(m_selectedText);
}

void TerminalWidget::selectLine(int row) {
    if (row >= m_buffer->size()) return;
    
    QString line = m_buffer->lineAt(row).text;
    m_selection.active = true;
    m_selection.startCol = 0;
    m_selection.startRow = row;
    m_selection.endCol = line.length();
    m_selection.endRow = row;
    m_selectedText = line;
    
    emit selectionChanged(m_selectedText);
}

void TerminalWidget::updateSelection(int col, int row) {
    if (!m_selection.active) return;
    
    m_selection.endCol = col;
    m_selection.endRow = row;
    
    QStringList selected;
    int startRow = qMin(m_selection.startRow, m_selection.endRow);
    int endRow = qMax(m_selection.startRow, m_selection.endRow);
    
    for (int r = startRow; r <= endRow && r < m_buffer->size(); r++) {
        QString line = m_buffer->lineAt(r).text;
        if (r == startRow && r == endRow) {
            selected.append(line.mid(
                qMin(m_selection.startCol, m_selection.endCol),
                qAbs(m_selection.endCol - m_selection.startCol)
            ));
        } else if (r == startRow) {
            selected.append(line.mid(m_selection.startCol));
        } else if (r == endRow) {
            selected.append(line.left(m_selection.endCol));
        } else {
            selected.append(line);
        }
    }
    
    m_selectedText = selected.join(QStringLiteral("\n"));
    emit selectionChanged(m_selectedText);
}

void TerminalWidget::copyToClipboard() {
    if (!m_selectedText.isEmpty()) {
        QApplication::clipboard()->setText(m_selectedText);
    }
}

void TerminalWidget::pasteFromClipboard() {
    QString text = QApplication::clipboard()->text();
    if (!text.isEmpty()) {
        emit dataAvailable(text.toUtf8());
    }
}

void TerminalWidget::handleCtrlKey(int key) {
    switch (key) {
        case Qt::Key_C:
            emit dataAvailable(QByteArray("\x03"));
            break;
        case Qt::Key_D:
            emit dataAvailable(QByteArray("\x04"));
            break;
        case Qt::Key_L:
            emit dataAvailable(QByteArray("\x0c"));
            break;
        case Qt::Key_U:
            emit dataAvailable(QByteArray("\x15"));
            break;
        case Qt::Key_V:
            pasteFromClipboard();
            break;
        case Qt::Key_W:
            emit dataAvailable(QByteArray("\x17"));
            break;
        default:
            if (key >= Qt::Key_A && key <= Qt::Key_Z) {
                char ctrl = key - Qt::Key_A + 1;
                emit dataAvailable(QByteArray(1, ctrl));
            }
            break;
    }
}

void TerminalWidget::handleAltKey(int key) {
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        QByteArray seq = "\x1b";
        seq.append(key - Qt::Key_A + 'a');
        emit dataAvailable(seq);
    }
}

QByteArray TerminalWidget::encodeKey(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        return "\r";
    }
    
    if (event->key() == Qt::Key_Backspace) {
        return "\x7f";
    }
    
    if (event->key() == Qt::Key_Escape) {
        return "\x1b";
    }
    
    if (event->key() == Qt::Key_Tab) {
        return "\t";
    }
    
    if (event->key() == Qt::Key_Up) {
        return event->modifiers() & Qt::ShiftModifier ? "\x1b[1;2A" : "\x1b[A";
    }
    if (event->key() == Qt::Key_Down) {
        return event->modifiers() & Qt::ShiftModifier ? "\x1b[1;2B" : "\x1b[B";
    }
    if (event->key() == Qt::Key_Right) {
        return event->modifiers() & Qt::ShiftModifier ? "\x1b[1;2C" : "\x1b[C";
    }
    if (event->key() == Qt::Key_Left) {
        return event->modifiers() & Qt::ShiftModifier ? "\x1b[1;2D" : "\x1b[D";
    }
    
    if (event->key() == Qt::Key_Home) {
        return "\x1b[H";
    }
    if (event->key() == Qt::Key_End) {
        return "\x1b[F";
    }
    if (event->key() == Qt::Key_PageUp) {
        return "\x1b[5~";
    }
    if (event->key() == Qt::Key_PageDown) {
        return "\x1b[6~";
    }
    if (event->key() == Qt::Key_Insert) {
        return "\x1b[2~";
    }
    if (event->key() == Qt::Key_Delete) {
        return "\x1b[3~";
    }
    
    if (event->key() >= Qt::Key_F1 && event->key() <= Qt::Key_F12) {
        int f = event->key() - Qt::Key_F1 + 1;
        if (f <= 4) {
            return QString("\x1bOP%1").arg(QChar('0' + f - 1)).toUtf8();
        } else {
            return QString("\x1b[15~%1").arg(f - 1).toUtf8();
        }
    }
    
    QString text = event->text();
    if (!text.isEmpty()) {
        QChar ch = text[0];
        ushort code = ch.unicode();
        if (code >= 0x20 && code != 0x7f) {
            return text.toUtf8();
        }
    }
    
    return QByteArray();
}

QByteArray TerminalWidget::encodeMouse(QMouseEvent* event) {
    int button = 0;
    if (event->button() == Qt::LeftButton) button = 0;
    else if (event->button() == Qt::MiddleButton) button = 1;
    else if (event->button() == Qt::RightButton) button = 2;
    
    int x = event->pos().x() / charWidth() + 32;
    int y = event->pos().y() / charHeight() + 32;
    
    QByteArray seq = "\x1b[M";
    seq.append(static_cast<char>(32 + button));
    seq.append(static_cast<char>(x));
    seq.append(static_cast<char>(y));
    
    return seq;
}

void TerminalWidget::keyPressEvent(QKeyEvent* event) {
    Qt::KeyboardModifiers mods = event->modifiers();
    
    if (mods & Qt::ControlModifier) {
        if (mods & Qt::ShiftModifier && event->key() == Qt::Key_C) {
            copyToClipboard();
            return;
        }
        if (mods & Qt::ShiftModifier && event->key() == Qt::Key_V) {
            pasteFromClipboard();
            return;
        }
        handleCtrlKey(event->key());
        return;
    }
    
    if (mods & Qt::AltModifier) {
        handleAltKey(event->key());
        return;
    }
    
    QByteArray seq = encodeKey(event);
    if (!seq.isEmpty()) {
        emit dataAvailable(seq);
    }
    
    m_cursorBlinkState = true;
    m_cursorVisible = true;
    if (m_cursorBlinkTimer->isActive()) {
        m_cursorBlinkTimer->start();
    }
}

void TerminalWidget::keyReleaseEvent(QKeyEvent* event) {
    QOpenGLWidget::keyReleaseEvent(event);
}

void TerminalWidget::mousePressEvent(QMouseEvent* event) {
    QPoint charPos = pixelToChar(event->pos());
    
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() & Qt::ControlModifier) {
            selectWord(charPos.x(), charPos.y());
        } else if (event->modifiers() & Qt::ShiftModifier) {
            selectLine(charPos.y());
        } else {
            m_selection.active = true;
            m_selection.startCol = charPos.x();
            m_selection.startRow = charPos.y();
            m_selection.endCol = charPos.x();
            m_selection.endRow = charPos.y();
        }
        m_mouseSelecting = true;
        m_lastMousePos = event->pos();
        update();
    }
}

void TerminalWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_mouseSelecting) {
        m_mouseSelecting = false;
        if (!m_selectedText.isEmpty()) {
            emit selectionChanged(m_selectedText);
        }
    }
}

void TerminalWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_mouseSelecting) {
        QPoint charPos = pixelToChar(event->pos());
        updateSelection(charPos.x(), charPos.y());
        update();
    }
    m_lastMousePos = event->pos();
}

void TerminalWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y() / 120;
    scroll(-delta * 3);
    event->accept();
}

void TerminalWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    
    QAction* copyAction = menu.addAction(QStringLiteral("Copy"));
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(!m_selectedText.isEmpty());
    connect(copyAction, &QAction::triggered, this, &TerminalWidget::copyToClipboard);
    
    QAction* pasteAction = menu.addAction(QStringLiteral("Paste"));
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &TerminalWidget::pasteFromClipboard);
    
    menu.addSeparator();
    
    QAction* clearAction = menu.addAction(QStringLiteral("Clear"));
    connect(clearAction, &QAction::triggered, this, &TerminalWidget::clear);
    
    QAction* selectAllAction = menu.addAction(QStringLiteral("Select All"));
    connect(selectAllAction, &QAction::triggered, this, [this]() {
        m_selection.active = true;
        m_selection.startCol = 0;
        m_selection.startRow = 0;
        m_selection.endCol = m_config.columns;
        m_selection.endRow = m_buffer->size();
        
        QStringList allLines;
        for (int i = 0; i < m_buffer->size(); i++) {
            allLines.append(m_buffer->lineAt(i).text);
        }
        m_selectedText = allLines.join(QStringLiteral("\n"));
        emit selectionChanged(m_selectedText);
        update();
    });
    
    menu.exec(event->globalPos());
}

void TerminalWidget::focusInEvent(QFocusEvent* event) {
    m_cursorVisible = true;
    m_cursorBlinkState = true;
    m_cursorBlinkTimer->start();
    update();
    QOpenGLWidget::focusInEvent(event);
}

void TerminalWidget::focusOutEvent(QFocusEvent* event) {
    m_cursorVisible = false;
    m_cursorBlinkTimer->stop();
    update();
    QOpenGLWidget::focusOutEvent(event);
}

void TerminalWidget::inputMethodEvent(QInputMethodEvent* event) {
    QString text = event->commitString();
    if (!text.isEmpty()) {
        emit dataAvailable(text.toUtf8());
    }
    event->accept();
}

void TerminalWidget::onCursorBlink() {
    m_cursorBlinkState = !m_cursorBlinkState;
    update();
}

void TerminalWidget::onRenderTimer() {
    update();
}
