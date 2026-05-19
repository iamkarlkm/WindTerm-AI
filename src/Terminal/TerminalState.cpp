#include "TerminalState.h"
#include <QDebug>
#include <QApplication>
#include <QClipboard>

TerminalState::TerminalState(int rows, int cols, QObject* parent)
    : QObject(parent), m_rows(rows), m_cols(cols) {
    m_screen.resize(m_rows);
    for (int i = 0; i < m_rows; i++) {
        m_screen[i].resize(m_cols);
    }
    m_scrollRegion = {0, m_rows, 0, m_cols};
    
    m_parser.commandCallback = [this](const AnsiCommand& cmd) {
        handleCommand(cmd);
    };
    m_parser.lineCallback = [this](const QVector<QPair<QChar, TextStyle>>& line) {
        handleLine(line);
    };
    m_parser.styleCallback = [this](const TextStyle& oldStyle, const TextStyle& newStyle) {
        handleStyleChange(oldStyle, newStyle);
    };
}

void TerminalState::resize(int rows, int cols) {
    if (rows == m_rows && cols == m_cols) return;
    
    QVector<QVector<TerminalCell>> newScreen(rows);
    for (int i = 0; i < rows; i++) {
        newScreen[i].resize(cols);
        int copyCols = qMin(cols, m_cols);
        if (i < m_rows) {
            for (int j = 0; j < copyCols; j++) {
                newScreen[i][j] = m_screen[i][j];
            }
        }
    }
    
    m_screen = newScreen;
    m_rows = rows;
    m_cols = cols;
    m_scrollRegion = {0, m_rows, 0, m_cols};
    m_cursor.row = qMin(m_cursor.row, m_rows - 1);
    m_cursor.col = qMin(m_cursor.col, m_cols - 1);
}

void TerminalState::write(const QByteArray& data) {
    m_parser.parse(data);
    emit screenUpdated();
}

void TerminalState::write(const QString& text) {
    write(text.toUtf8());
}

const QVector<TerminalCell>& TerminalState::line(int row) const {
    if (row >= 0 && row < m_rows) {
        return m_screen[row];
    }
    static QVector<TerminalCell> emptyLine;
    return emptyLine;
}

void TerminalState::setCursorVisible(bool visible) {
    m_cursor.visible = visible;
}

void TerminalState::setScrollRegion(int top, int bottom, int left, int right) {
    m_scrollRegion.top = qBound(0, top, m_rows - 1);
    m_scrollRegion.bottom = qBound(top + 1, bottom, m_rows);
    m_scrollRegion.left = qBound(0, left, m_cols - 1);
    m_scrollRegion.right = qBound(left + 1, right, m_cols);
}

QString TerminalState::getLineText(int row) const {
    if (row < 0 || row >= m_rows) return QString();
    
    QString text;
    for (int col = m_cols - 1; col >= 0; col--) {
        if (m_screen[row][col].character != ' ') {
            text.resize(col + 1);
            break;
        }
    }
    
    for (int col = 0; col < text.length(); col++) {
        text[col] = m_screen[row][col].character;
    }
    
    return text;
}

QString TerminalState::getSelectedText(int startRow, int startCol, int endRow, int endCol) const {
    QStringList lines;
    
    if (startRow == endRow) {
        QString line;
        for (int col = qMin(startCol, endCol); col < qMax(startCol, endCol); col++) {
            if (col >= 0 && col < m_cols) {
                line.append(m_screen[startRow][col].character);
            }
        }
        return line;
    }
    
    for (int row = startRow; row <= endRow && row < m_rows; row++) {
        QString line;
        int colStart = (row == startRow) ? startCol : 0;
        int colEnd = (row == endRow) ? endCol : m_cols;
        
        for (int col = colStart; col < colEnd && col < m_cols; col++) {
            if (col >= 0) {
                line.append(m_screen[row][col].character);
            }
        }
        lines.append(line);
    }
    
    return lines.join('\n');
}

void TerminalState::copyToClipboard() const {
    if (QApplication::clipboard()) {
        QStringList lines;
        for (int row = 0; row < m_rows; row++) {
            lines.append(getLineText(row));
        }
        QApplication::clipboard()->setText(lines.join('\n'));
    }
}

void TerminalState::clear() {
    for (int row = 0; row < m_rows; row++) {
        for (int col = 0; col < m_cols; col++) {
            m_screen[row][col] = TerminalCell();
        }
    }
    m_cursor.row = 0;
    m_cursor.col = 0;
    emit screenUpdated();
}

void TerminalState::reset() {
    clear();
    m_cursor.visible = true;
    m_cursor.blinking = true;
    m_currentStyle.reset();
    m_scrollRegion = {0, m_rows, 0, m_cols};
    m_parser.clear();
}

void TerminalState::clearScrollback() {
    m_scrollbackBuffer.clear();
    emit scrollbackChanged(0);
}

void TerminalState::insertChar(QChar ch, const TextStyle& style) {
    if (m_cursor.row >= 0 && m_cursor.row < m_rows && 
        m_cursor.col >= 0 && m_cursor.col < m_cols) {
        m_screen[m_cursor.row][m_cursor.col] = TerminalCell(ch, style);
    }
}

void TerminalState::moveCursor(int row, int col) {
    m_cursor.row = qBound(0, row, m_rows - 1);
    m_cursor.col = qBound(0, col, m_cols - 1);
    emit cursorMoved(m_cursor.row, m_cursor.col);
}

void TerminalState::scrollUp(int n) {
    if (!m_scrollRegion.isValid()) return;
    
    for (int i = 0; i < n; i++) {
        QVector<TerminalCell> line = m_screen[m_scrollRegion.top];
        
        for (int row = m_scrollRegion.top; row < m_scrollRegion.bottom - 1; row++) {
            m_screen[row] = m_screen[row + 1];
        }
        
        m_screen[m_scrollRegion.bottom - 1].fill(TerminalCell());
        
        if (!line.isEmpty()) {
            m_scrollbackBuffer.append(line);
            if (m_scrollbackBuffer.size() > m_maxScrollback) {
                m_scrollbackBuffer.removeFirst();
            }
            emit scrollbackChanged(m_scrollbackBuffer.size());
        }
    }
}

void TerminalState::scrollDown(int n) {
    if (!m_scrollRegion.isValid()) return;
    
    for (int i = 0; i < n; i++) {
        QVector<TerminalCell> line = m_screen[m_scrollRegion.bottom - 1];
        
        for (int row = m_scrollRegion.bottom - 1; row > m_scrollRegion.top; row--) {
            m_screen[row] = m_screen[row - 1];
        }
        
        m_screen[m_scrollRegion.top].fill(TerminalCell());
    }
}

void TerminalState::eraseInDisplay(int mode) {
    switch (mode) {
        case 0:
            for (int col = m_cursor.col; col < m_cols; col++) {
                m_screen[m_cursor.row][col] = TerminalCell();
            }
            for (int row = m_cursor.row + 1; row < m_rows; row++) {
                m_screen[row].fill(TerminalCell());
            }
            break;
        case 1:
            for (int row = 0; row < m_cursor.row; row++) {
                m_screen[row].fill(TerminalCell());
            }
            for (int col = 0; col <= m_cursor.col; col++) {
                m_screen[m_cursor.row][col] = TerminalCell();
            }
            break;
        case 2:
            for (int row = 0; row < m_rows; row++) {
                m_screen[row].fill(TerminalCell());
            }
            m_cursor.row = 0;
            m_cursor.col = 0;
            break;
    }
}

void TerminalState::eraseInLine(int mode) {
    switch (mode) {
        case 0:
            for (int col = m_cursor.col; col < m_cols; col++) {
                m_screen[m_cursor.row][col] = TerminalCell();
            }
            break;
        case 1:
            for (int col = 0; col <= m_cursor.col; col++) {
                m_screen[m_cursor.row][col] = TerminalCell();
            }
            break;
        case 2:
            m_screen[m_cursor.row].fill(TerminalCell());
            break;
    }
}

void TerminalState::lineFeed() {
    m_cursor.row++;
    if (m_cursor.row >= m_scrollRegion.bottom) {
        scrollUp(1);
        m_cursor.row = m_scrollRegion.bottom - 1;
    }
}

void TerminalState::carriageReturn() {
    m_cursor.col = 0;
}

void TerminalState::backspace() {
    if (m_cursor.col > 0) {
        m_cursor.col--;
    } else if (m_cursor.row > 0) {
        m_cursor.row--;
        m_cursor.col = m_cols - 1;
    }
}

void TerminalState::tab() {
    int nextTab = ((m_cursor.col / 8) + 1) * 8;
    m_cursor.col = qMin(nextTab, m_cols - 1);
}

void TerminalState::handleCommand(const AnsiCommand& cmd) {
    switch (cmd.command) {
        case TerminalCommand::CursorUp:
            moveCursor(m_cursor.row - cmd.param(0, 1), m_cursor.col);
            break;
        case TerminalCommand::CursorDown:
            moveCursor(m_cursor.row + cmd.param(0, 1), m_cursor.col);
            break;
        case TerminalCommand::CursorForward:
            moveCursor(m_cursor.row, m_cursor.col + cmd.param(0, 1));
            break;
        case TerminalCommand::CursorBackward:
            moveCursor(m_cursor.row, m_cursor.col - cmd.param(0, 1));
            break;
        case TerminalCommand::CursorPosition:
            moveCursor(cmd.param(0, 1) - 1, cmd.param(1, 1) - 1);
            break;
        case TerminalCommand::EraseInDisplay:
            eraseInDisplay(cmd.param(0, 0));
            break;
        case TerminalCommand::EraseInLine:
            eraseInLine(cmd.param(0, 0));
            break;
        case TerminalCommand::ScrollUp:
            scrollUp(cmd.param(0, 1));
            break;
        case TerminalCommand::ScrollDown:
            scrollDown(cmd.param(0, 1));
            break;
        case TerminalCommand::SetGraphicsMode:
            break;
        case TerminalCommand::SaveCursorPosition:
            break;
        case TerminalCommand::RestoreCursorPosition:
            break;
        case TerminalCommand::SetWindowTitle:
            m_title = cmd.oscString;
            emit titleChanged(m_title);
            break;
        default:
            break;
    }
}

void TerminalState::handleLine(const QVector<QPair<QChar, TextStyle>>& line) {
}

void TerminalState::handleStyleChange(const TextStyle& oldStyle, const TextStyle& newStyle) {
    m_currentStyle = newStyle;
}
