#include "AnsiParser.h"
#include "ColorPalette.h"
#include "AnsiEscapeCodes.h"
#include <QDebug>

AnsiParser::AnsiParser()
    : m_state(AnsiState::Normal), m_cursorRow(0), m_cursorCol(0),
      m_screenRows(24), m_screenCols(80), m_useAlternateBuffer(false) {
    m_currentStyle = TextStyle::defaultStyle();
    m_currentLine.resize(m_screenCols);
    m_screenBuffer.resize(m_screenRows);
    for (int i = 0; i < m_screenRows; i++) {
        m_screenBuffer[i].resize(m_screenCols);
    }
    m_normalBuffer = m_screenBuffer;
}

void AnsiParser::parse(const QByteArray& data) {
    QString text = QString::fromUtf8(data);
    parse(text);
}

void AnsiParser::parse(const QString& text) {
    for (const QChar& ch : text) {
        processChar(ch);
    }
}

void AnsiParser::processChar(QChar ch) {
    switch (m_state) {
        case AnsiState::Normal:
            if (ch == '\x1b') {
                m_state = AnsiState::Escape;
            } else if (ch == '\n') {
                m_cursorCol = 0;
                m_cursorRow++;
                if (m_cursorRow >= m_screenRows) {
                    scrollUp(1);
                    m_cursorRow = m_screenRows - 1;
                }
            } else if (ch == '\r') {
                m_cursorCol = 0;
            } else if (ch == '\t') {
                int nextTab = ((m_cursorCol / 8) + 1) * 8;
                m_cursorCol = qMin(nextTab, m_screenCols - 1);
            } else if (ch == '\x08' || ch == '\x7f') {
                if (m_cursorCol > 0) m_cursorCol--;
            } else {
                if (m_cursorCol < m_screenCols && m_cursorRow < m_screenRows) {
                    m_currentLine[m_cursorCol] = qMakePair(ch, m_currentStyle);
                    m_cursorCol++;
                }
            }
            break;
            
        case AnsiState::Escape:
            handleEscape(ch);
            break;
            
        case AnsiState::Csi:
            handleCsi(ch);
            break;
            
        case AnsiState::Osc:
            handleOsc(ch);
            break;
            
        default:
            m_state = AnsiState::Normal;
            break;
    }
}

void AnsiParser::handleEscape(QChar ch) {
    if (ch == '[') {
        m_state = AnsiState::Csi;
        m_csiParams.clear();
    } else if (ch == ']') {
        m_state = AnsiState::Osc;
        m_oscParams.clear();
    } else if (ch == '7') {
        m_lastCommand = {TerminalCommand::SaveCursorPosition};
        if (commandCallback) commandCallback(m_lastCommand);
        m_state = AnsiState::Normal;
    } else if (ch == '8') {
        m_lastCommand = {TerminalCommand::RestoreCursorPosition};
        if (commandCallback) commandCallback(m_lastCommand);
        m_state = AnsiState::Normal;
    } else if (ch == 'D') {
        m_lastCommand = {TerminalCommand::Index};
        if (commandCallback) commandCallback(m_lastCommand);
        m_state = AnsiState::Normal;
    } else if (ch == 'M') {
        m_lastCommand = {TerminalCommand::ReverseIndex};
        if (commandCallback) commandCallback(m_lastCommand);
        m_state = AnsiState::Normal;
    } else if (ch == 'E') {
        m_lastCommand = {TerminalCommand::NextLine};
        if (commandCallback) commandCallback(m_lastCommand);
        m_state = AnsiState::Normal;
    } else {
        m_state = AnsiState::Normal;
    }
}

void AnsiParser::handleCsi(QChar ch) {
    if (ch.isDigit() || ch == ';') {
        m_csiParams.append(ch);
    } else {
        AnsiCommand cmd;
        cmd.command = TerminalCommand::None;
        
        if (!m_csiParams.isEmpty()) {
            QStringList parts = m_csiParams.split(';');
            for (const QString& part : parts) {
                bool ok;
                int val = part.toInt(&ok);
                cmd.parameters.append(ok ? val : 0);
            }
        }
        
        switch (ch.unicode()) {
            case 'A': cmd.command = TerminalCommand::CursorUp; break;
            case 'B': cmd.command = TerminalCommand::CursorDown; break;
            case 'C': cmd.command = TerminalCommand::CursorForward; break;
            case 'D': cmd.command = TerminalCommand::CursorBackward; break;
            case 'E': cmd.command = TerminalCommand::CursorNextLine; break;
            case 'F': cmd.command = TerminalCommand::CursorPreviousLine; break;
            case 'G': case '`': cmd.command = TerminalCommand::CursorHorizontalAbsolute; break;
            case 'H': case 'f': cmd.command = TerminalCommand::CursorPosition; break;
            case 'J': cmd.command = TerminalCommand::EraseInDisplay; break;
            case 'K': cmd.command = TerminalCommand::EraseInLine; break;
            case 'S': cmd.command = TerminalCommand::ScrollUp; break;
            case 'T': cmd.command = TerminalCommand::ScrollDown; break;
            case 'm': cmd.command = TerminalCommand::SetGraphicsMode; break;
            case 'h': cmd.command = TerminalCommand::SetMode; break;
            case 'l': cmd.command = TerminalCommand::ResetMode; break;
            case 'n': cmd.command = TerminalCommand::DeviceStatusReport; break;
            case 'g': cmd.command = TerminalCommand::TabClear; break;
            default: cmd.command = TerminalCommand::None; break;
        }
        
        m_lastCommand = cmd;
        
        switch (cmd.command) {
            case TerminalCommand::CursorUp: moveCursorUp(cmd.param(0, 1)); break;
            case TerminalCommand::CursorDown: moveCursorDown(cmd.param(0, 1)); break;
            case TerminalCommand::CursorForward: moveCursorForward(cmd.param(0, 1)); break;
            case TerminalCommand::CursorBackward: moveCursorBackward(cmd.param(0, 1)); break;
            case TerminalCommand::CursorNextLine:
                m_cursorCol = 0;
                moveCursorDown(cmd.param(0, 1));
                break;
            case TerminalCommand::CursorPreviousLine:
                m_cursorCol = 0;
                moveCursorUp(cmd.param(0, 1));
                break;
            case TerminalCommand::CursorHorizontalAbsolute:
                m_cursorCol = qBound(0, cmd.param(0, 1) - 1, m_screenCols - 1);
                break;
            case TerminalCommand::CursorPosition:
                setCursorPosition(cmd.param(0, 1) - 1, cmd.param(1, 1) - 1);
                break;
            case TerminalCommand::EraseInDisplay: eraseInDisplay(cmd.param(0, 0)); break;
            case TerminalCommand::EraseInLine: eraseInLine(cmd.param(0, 0)); break;
            case TerminalCommand::ScrollUp: scrollUp(cmd.param(0, 1)); break;
            case TerminalCommand::ScrollDown: scrollDown(cmd.param(0, 1)); break;
            case TerminalCommand::SetGraphicsMode: applyGraphicsMode(cmd.parameters); break;
            default: break;
        }
        
        if (commandCallback) commandCallback(m_lastCommand);
        
        m_state = AnsiState::Normal;
        m_csiParams.clear();
    }
}

void AnsiParser::handleOsc(QChar ch) {
    if (ch == '\x07' || ch == '\x1b') {
        if (m_oscParams.startsWith("8;")) {
            int semiPos = m_oscParams.indexOf(';', 2);
            QString uri;
            if (semiPos > 0) {
                uri = m_oscParams.mid(semiPos + 1);
            } else {
                uri = m_oscParams.mid(2);
            }
            
            TextStyle newStyle = m_currentStyle;
            newStyle.hyperlink = uri;
            
            TextStyle oldStyle = m_currentStyle;
            m_currentStyle = newStyle;
            
            if (styleCallback) styleCallback(oldStyle, newStyle);
        } else if (m_oscParams.startsWith("8;;")) {
            TextStyle newStyle = m_currentStyle;
            newStyle.hyperlink.clear();
            
            TextStyle oldStyle = m_currentStyle;
            m_currentStyle = newStyle;
            
            if (styleCallback) styleCallback(oldStyle, newStyle);
        } else {
            m_lastCommand = {TerminalCommand::SetWindowTitle};
            m_lastCommand.oscString = m_oscParams;
            if (commandCallback) commandCallback(m_lastCommand);
        }
        
        m_state = AnsiState::Normal;
        m_oscParams.clear();
    } else {
        m_oscParams.append(ch);
    }
}

void AnsiParser::moveCursorUp(int n) {
    m_cursorRow = qMax(0, m_cursorRow - n);
}

void AnsiParser::moveCursorDown(int n) {
    m_cursorRow = qMin(m_screenRows - 1, m_cursorRow + n);
}

void AnsiParser::moveCursorForward(int n) {
    m_cursorCol = qMin(m_screenCols - 1, m_cursorCol + n);
}

void AnsiParser::moveCursorBackward(int n) {
    m_cursorCol = qMax(0, m_cursorCol - n);
}

void AnsiParser::setCursorPosition(int row, int col) {
    m_cursorRow = qBound(0, row, m_screenRows - 1);
    m_cursorCol = qBound(0, col, m_screenCols - 1);
}

void AnsiParser::eraseInDisplay(int mode) {
    switch (mode) {
        case 0:
            for (int col = m_cursorCol; col < m_screenCols; col++) {
                m_currentLine[col] = qMakePair(QChar(' '), m_currentStyle);
            }
            for (int row = m_cursorRow + 1; row < m_screenRows; row++) {
                m_screenBuffer[row].fill(qMakePair(QChar(' '), m_currentStyle));
            }
            break;
        case 1:
            for (int row = 0; row < m_cursorRow; row++) {
                m_screenBuffer[row].fill(qMakePair(QChar(' '), m_currentStyle));
            }
            for (int col = 0; col <= m_cursorCol; col++) {
                m_currentLine[col] = qMakePair(QChar(' '), m_currentStyle);
            }
            break;
        case 2:
            m_currentLine.fill(qMakePair(QChar(' '), m_currentStyle));
            for (int row = 0; row < m_screenRows; row++) {
                m_screenBuffer[row].fill(qMakePair(QChar(' '), m_currentStyle));
            }
            m_cursorRow = 0;
            m_cursorCol = 0;
            break;
    }
}

void AnsiParser::eraseInLine(int mode) {
    switch (mode) {
        case 0:
            for (int col = m_cursorCol; col < m_screenCols; col++) {
                m_currentLine[col] = qMakePair(QChar(' '), m_currentStyle);
            }
            break;
        case 1:
            for (int col = 0; col <= m_cursorCol; col++) {
                m_currentLine[col] = qMakePair(QChar(' '), m_currentStyle);
            }
            break;
        case 2:
            m_currentLine.fill(qMakePair(QChar(' '), m_currentStyle));
            break;
    }
}

void AnsiParser::scrollUp(int n) {
    for (int i = 0; i < n && i < m_screenRows; i++) {
        m_screenBuffer.removeFirst();
        m_screenBuffer.append(QVector<QPair<QChar, TextStyle>>(m_screenCols, 
            qMakePair(QChar(' '), m_currentStyle)));
    }
}

void AnsiParser::scrollDown(int n) {
    for (int i = 0; i < n && i < m_screenRows; i++) {
        m_screenBuffer.removeLast();
        m_screenBuffer.prepend(QVector<QPair<QChar, TextStyle>>(m_screenCols, 
            qMakePair(QChar(' '), m_currentStyle)));
    }
}

void AnsiParser::applyGraphicsMode(const QVector<int>& params) {
    if (params.isEmpty() || params[0] == 0) {
        m_currentStyle.reset();
        if (params.isEmpty()) return;
    }
    
    int i = 0;
    while (i < params.size()) {
        int code = params[i];
        
        switch (code) {
            case 0: m_currentStyle.reset(); break;
            case 1: m_currentStyle.bold = true; break;
            case 2: m_currentStyle.faint = true; break;
            case 3: m_currentStyle.italic = true; break;
            case 4: m_currentStyle.underline = true; break;
            case 5: m_currentStyle.blink = true; break;
            case 7: m_currentStyle.reverse = true; break;
            case 8: m_currentStyle.hidden = true; break;
            case 9: m_currentStyle.strikeThrough = true; break;
            case 22: m_currentStyle.bold = false; m_currentStyle.faint = false; break;
            case 23: m_currentStyle.italic = false; break;
            case 24: m_currentStyle.underline = false; break;
            case 25: m_currentStyle.blink = false; break;
            case 27: m_currentStyle.reverse = false; break;
            case 28: m_currentStyle.hidden = false; break;
            case 29: m_currentStyle.strikeThrough = false; break;
            
            case 30: m_currentStyle.foreground = QColor(0, 0, 0); break;
            case 31: m_currentStyle.foreground = QColor(205, 0, 0); break;
            case 32: m_currentStyle.foreground = QColor(0, 205, 0); break;
            case 33: m_currentStyle.foreground = QColor(205, 205, 0); break;
            case 34: m_currentStyle.foreground = QColor(0, 0, 238); break;
            case 35: m_currentStyle.foreground = QColor(205, 0, 205); break;
            case 36: m_currentStyle.foreground = QColor(0, 205, 205); break;
            case 37: m_currentStyle.foreground = QColor(229, 229, 229); break;
            case 38:
                if (i + 1 < params.size() && params[i + 1] == AnsiCodes::SGR::COLOR_256_SUBCODE) {
                    if (i + 2 < params.size()) {
                        int colorIndex = params[i + 2];
                        m_currentStyle.foreground = ColorPalette::get256Color(colorIndex);
                        i += 2;
                    }
                } else if (i + 1 < params.size() && params[i + 1] == AnsiCodes::SGR::COLOR_TRUECOLOR_SUBCODE) {
                    if (i + 4 < params.size()) {
                        m_currentStyle.foreground = ColorPalette::getTrueColor(params[i + 2], params[i + 3], params[i + 4]);
                        i += 4;
                    }
                }
                break;
            case 39: m_currentStyle.foreground = TextStyle::defaultStyle().foreground; break;
            
            case 40: m_currentStyle.background = QColor(0, 0, 0); break;
            case 41: m_currentStyle.background = QColor(205, 0, 0); break;
            case 42: m_currentStyle.background = QColor(0, 205, 0); break;
            case 43: m_currentStyle.background = QColor(205, 205, 0); break;
            case 44: m_currentStyle.background = QColor(0, 0, 238); break;
            case 45: m_currentStyle.background = QColor(205, 0, 205); break;
            case 46: m_currentStyle.background = QColor(0, 205, 205); break;
            case 47: m_currentStyle.background = QColor(229, 229, 229); break;
            case 48:
                if (i + 1 < params.size() && params[i + 1] == AnsiCodes::SGR::COLOR_256_SUBCODE) {
                    if (i + 2 < params.size()) {
                        int colorIndex = params[i + 2];
                        m_currentStyle.background = ColorPalette::get256Color(colorIndex);
                        i += 2;
                    }
                } else if (i + 1 < params.size() && params[i + 1] == AnsiCodes::SGR::COLOR_TRUECOLOR_SUBCODE) {
                    if (i + 4 < params.size()) {
                        m_currentStyle.background = ColorPalette::getTrueColor(params[i + 2], params[i + 3], params[i + 4]);
                        i += 4;
                    }
                }
                break;
            case 49: m_currentStyle.background = TextStyle::defaultStyle().background; break;
            
            case 90: m_currentStyle.foreground = QColor(127, 127, 127); break;
            case 91: m_currentStyle.foreground = QColor(255, 0, 0); break;
            case 92: m_currentStyle.foreground = QColor(0, 255, 0); break;
            case 93: m_currentStyle.foreground = QColor(255, 255, 0); break;
            case 94: m_currentStyle.foreground = QColor(92, 92, 255); break;
            case 95: m_currentStyle.foreground = QColor(255, 0, 255); break;
            case 96: m_currentStyle.foreground = QColor(0, 255, 255); break;
            case 97: m_currentStyle.foreground = QColor(255, 255, 255); break;
            
            case 100: m_currentStyle.background = QColor(127, 127, 127); break;
            case 101: m_currentStyle.background = QColor(255, 0, 0); break;
            case 102: m_currentStyle.background = QColor(0, 255, 0); break;
            case 103: m_currentStyle.background = QColor(255, 255, 0); break;
            case 104: m_currentStyle.background = QColor(92, 92, 255); break;
            case 105: m_currentStyle.background = QColor(255, 0, 255); break;
            case 106: m_currentStyle.background = QColor(0, 255, 255); break;
            case 107: m_currentStyle.background = QColor(255, 255, 255); break;
        }
        
        i++;
    }
}

void AnsiParser::clear() {
    m_state = AnsiState::Normal;
    m_csiParams.clear();
    m_oscParams.clear();
    m_cursorRow = 0;
    m_cursorCol = 0;
    m_currentStyle.reset();
    m_currentLine.fill(qMakePair(QChar(' '), m_currentStyle));
    for (int i = 0; i < m_screenRows; i++) {
        m_screenBuffer[i].fill(qMakePair(QChar(' '), m_currentStyle));
    }
}

void AnsiParser::onNewLine(const QVector<QPair<QChar, TextStyle>>& line) {
    if (lineCallback) lineCallback(line);
}

void AnsiParser::onCommand(const AnsiCommand& cmd) {
    if (commandCallback) commandCallback(cmd);
}

void AnsiParser::onStyleChange(const TextStyle& oldStyle, const TextStyle& newStyle) {
    if (styleCallback) styleCallback(oldStyle, newStyle);
}

// Extended CSI handlers for better ANSI support

void AnsiParser::handleCsiCursor(const QString& params) {
    QStringList p = params.split(';');
    
    if (p.isEmpty() || p[0].isEmpty()) {
        // Default cursor position
        m_cursorRow = 0;
        m_cursorCol = 0;
        return;
    }
    
    if (p.size() == 1) {
        // Single parameter - column only (GCH)
        int col = p[0].toInt() - 1;
        m_cursorCol = qMax(0, qMin(col, m_screenCols - 1));
    } else {
        // Row and column
        int row = p[0].toInt() - 1;
        int col = p[1].toInt() - 1;
        m_cursorRow = qMax(0, qMin(row, m_screenRows - 1));
        m_cursorCol = qMax(0, qMin(col, m_screenCols - 1));
    }
}

void AnsiParser::handleCsiErase(const QString& params) {
    QString param = params.isEmpty() ? "0" : params;
    
    if (param == "0" || param.isEmpty()) {
        // Erase from cursor to end of screen
        for (int row = m_cursorRow; row < m_screenRows; row++) {
            int startCol = (row == m_cursorRow) ? m_cursorCol : 0;
            for (int col = startCol; col < m_screenCols; col++) {
                m_screenBuffer[row][col] = qMakePair(QChar(' '), m_currentStyle);
            }
        }
    } else if (param == "1") {
        // Erase from start to cursor
        for (int row = 0; row <= m_cursorRow; row++) {
            int endCol = (row == m_cursorRow) ? m_cursorCol : m_screenCols - 1;
            for (int col = 0; col <= endCol; col++) {
                m_screenBuffer[row][col] = qMakePair(QChar(' '), m_currentStyle);
            }
        }
    } else if (param == "2") {
        // Erase entire screen
        for (int row = 0; row < m_screenRows; row++) {
            for (int col = 0; col < m_screenCols; col++) {
                m_screenBuffer[row][col] = qMakePair(QChar(' '), m_currentStyle);
            }
        }
        m_cursorRow = 0;
        m_cursorCol = 0;
    }
}

void AnsiParser::handleCsiEraseLine(const QString& params) {
    QString param = params.isEmpty() ? "0" : params;
    
    if (param == "0" || param.isEmpty()) {
        // Erase from cursor to end of line
        for (int col = m_cursorCol; col < m_screenCols; col++) {
            m_screenBuffer[m_cursorRow][col] = qMakePair(QChar(' '), m_currentStyle);
        }
    } else if (param == "1") {
        // Erase from start to cursor
        for (int col = 0; col <= m_cursorCol; col++) {
            m_screenBuffer[m_cursorRow][col] = qMakePair(QChar(' '), m_currentStyle);
        }
    } else if (param == "2") {
        // Erase entire line
        for (int col = 0; col < m_screenCols; col++) {
            m_screenBuffer[m_cursorRow][col] = qMakePair(QChar(' '), m_currentStyle);
        }
    }
}

void AnsiParser::handleCsiScroll(const QString& params) {
    int count = params.isEmpty() ? 1 : params.toInt();
    if (count <= 0) count = 1;
    
    scrollUp(count);
}

void AnsiParser::handleCsiScrollDown(const QString& params) {
    int count = params.isEmpty() ? 1 : params.toInt();
    if (count <= 0) count = 1;
    
    // Scroll down by inserting blank lines at top
    for (int i = 0; i < count; i++) {
        m_screenBuffer.prepend(m_currentLine);
        if (m_screenBuffer.size() > m_screenRows) {
            m_screenBuffer.removeLast();
        }
    }
}
