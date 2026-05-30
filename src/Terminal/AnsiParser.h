#ifndef ANSI_PARSER_H
#define ANSI_PARSER_H

#include <QString>
#include <QVector>
#include <QColor>
#include <functional>

enum class AnsiState {
    Normal,
    Escape,
    Csi,
    Osc,
    Dcs,
    Apc
    
};

enum class TerminalCommand {
    None,
    CursorUp,
    CursorDown,
    CursorForward,
    CursorBackward,
    CursorNextLine,
    CursorPreviousLine,
    CursorPosition,
    CursorHorizontalAbsolute,
    EraseInDisplay,
    EraseInLine,
    ScrollUp,
    ScrollDown,
    SetGraphicsMode,
    SetMode,
    ResetMode,
    DeviceStatusReport,
    SaveCursorPosition,
    RestoreCursorPosition,
    SetWindowTitle,
    TabSet,
    TabClear,
    Index,
    NextLine,
    ReverseIndex,
    FullReset,
    KeypadApplicationMode,
    KeypadNumericMode
    
};

struct AnsiCommand {
    TerminalCommand command = TerminalCommand::None;
    QVector<int> parameters;
    QString oscString;
    
    bool hasParam(int index, int defaultValue = 0) const {
        return index < parameters.size() ? parameters[index] : defaultValue;
    }
    
    int param(int index, int defaultValue = 0) const {
        if (index < parameters.size()) {
            return parameters[index] == 0 ? defaultValue : parameters[index];
        }
        return defaultValue;
    }
    
};

enum class TextAttribute {
    Reset,
    Bold,
    Faint,
    Italic,
    Underline,
    Blink,
    Reverse,
    Hidden,
    StrikeThrough,
    DefaultForeground,
    DefaultBackground,
    BrightForeground,
    BrightBackground,
    Foreground256,
    Background256,
    ForegroundRGB,
    BackgroundRGB
    
};

struct TextStyle {
    bool bold = false;
    bool faint = false;
    bool italic = false;
    bool underline = false;
    bool blink = false;
    bool reverse = false;
    bool hidden = false;
    bool strikeThrough = false;
    
    QColor foreground;
    QColor background;
    QString hyperlink;
    
    static TextStyle defaultStyle() {
        TextStyle style;
        style.foreground = QColor(200, 200, 200);
        style.background = QColor(30, 30, 30);
        return style;
    }
    
    void reset() {
        *this = defaultStyle();
    }
    
    bool hasHyperlink() const { return !hyperlink.isEmpty(); }
    
    bool operator==(const TextStyle& other) const {
        return bold == other.bold &&
               faint == other.faint &&
               italic == other.italic &&
               underline == other.underline &&
               blink == other.blink &&
               reverse == other.reverse &&
               hidden == other.hidden &&
               strikeThrough == other.strikeThrough &&
               foreground == other.foreground &&
               background == other.background;
    }
    
    bool operator!=(const TextStyle& other) const {
        return !(*this == other);
    }
    
};

class AnsiParser {
public:
    AnsiParser();
    
    void parse(const QByteArray& data);
    void parse(const QString& text);
    
    const QVector<QPair<QChar, TextStyle>>& currentLine() const { return m_currentLine; }
    const AnsiCommand& lastCommand() const { return m_lastCommand; }
    TextStyle currentStyle() const { return m_currentStyle; }
    
    void clear();
    
    // Callback hooks
    void onNewLine(const QVector<QPair<QChar, TextStyle>>& line);
    void onCommand(const AnsiCommand& cmd);
    void onStyleChange(const TextStyle& oldStyle, const TextStyle& newStyle);
    
    // Function objects for callbacks
    std::function<void(const QVector<QPair<QChar, TextStyle>>&)> lineCallback;
    std::function<void(const AnsiCommand&)> commandCallback;
    std::function<void(const TextStyle&, const TextStyle&)> styleCallback;
    
private:
    void processChar(QChar ch);
    void handleEscape(QChar ch);
    void handleCsi(QChar ch);
    void handleOsc(QChar ch);
    void executeCommand();
    void applyGraphicsMode(const QVector<int>& params);
    
    void moveCursorUp(int n);
    void moveCursorDown(int n);
    void moveCursorForward(int n);
    void moveCursorBackward(int n);
    void setCursorPosition(int row, int col);
    void eraseInDisplay(int mode);
    void eraseInLine(int mode);
    void scrollUp(int n);
    void scrollDown(int n);
    
    // Extended CSI handlers
    void handleCsiCursor(const QString& params);
    void handleCsiErase(const QString& params);
    void handleCsiEraseLine(const QString& params);
    void handleCsiScroll(const QString& params);
    void handleCsiScrollDown(const QString& params);
    void handleDecPrivateMode(const QString& params, bool set);
    void handleOscCommand(const QString& params);
    void parseColorPaletteChange(const QString& params);
    void parseHyperlink(const QString& params);
    void handleDcsSequence(const QString& params);
    void handleApcSequence(const QString& params);
    
    AnsiState m_state;
    QString m_csiParams;
    QString m_oscParams;
    AnsiCommand m_lastCommand;
    TextStyle m_currentStyle;
    QVector<QPair<QChar, TextStyle>> m_currentLine;
    QVector<QVector<QPair<QChar, TextStyle>>> m_screenBuffer;
    
    int m_cursorRow;
    int m_cursorCol;
    int m_screenRows;
    int m_screenCols;
    
    bool m_useAlternateBuffer;
    QVector<QVector<QPair<QChar, TextStyle>>> m_normalBuffer;
    QVector<QVector<QPair<QChar, TextStyle>>> m_alternateBuffer;
    
    // Saved cursor state for alternate screen buffer
    int m_savedCursorRow;
    int m_savedCursorCol;
    TextStyle m_savedStyle;
    
    // Mode flags
    bool m_bracketedPasteMode;
    bool m_cursorVisible;
    
};

#endif


