#ifndef TERMINAL_STATE_H
#define TERMINAL_STATE_H

#include "Terminal/AnsiParser.h"
#include <QObject>
#include <QVector>
#include <QColor>

struct TerminalCell {
    QChar character;
    TextStyle style;
    QString hyperlink;
    
    TerminalCell() : character(' '), style(TextStyle::defaultStyle()) {}
    TerminalCell(QChar ch, const TextStyle& s) : character(ch), style(s), hyperlink(s.hyperlink) {}
    
    bool isEmpty() const {
        return character == ' ' && style == TextStyle::defaultStyle() && hyperlink.isEmpty();
    }
};

struct CursorInfo {
    int row = 0;
    int col = 0;
    bool visible = true;
    bool blinking = true;
};

struct ScrollRegion {
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;
    
    bool isValid() const { return top < bottom && left < right; }
    bool contains(int r, int c) const {
        return r >= top && r < bottom && c >= left && c < right;
    }
};

class TerminalState : public QObject {
    Q_OBJECT
public:
    explicit TerminalState(int rows = 24, int cols = 80, QObject* parent = nullptr);
    
    void resize(int rows, int cols);
    void write(const QByteArray& data);
    void write(const QString& text);
    
    const QVector<TerminalCell>& line(int row) const;
    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    
    CursorInfo cursor() const { return m_cursor; }
    void setCursorVisible(bool visible);
    
    void setScrollRegion(int top, int bottom, int left = 0, int right = 0);
    ScrollRegion scrollRegion() const { return m_scrollRegion; }
    
    QString getLineText(int row) const;
    QString getSelectedText(int startRow, int startCol, int endRow, int endCol) const;
    
    void copyToClipboard() const;
    void clear();
    void clearHistory();
    void reset();
    
    int scrollbackSize() const { return m_scrollbackBuffer.size(); }
    const QVector<QVector<TerminalCell>>& scrollbackBuffer() const { return m_scrollbackBuffer; }
    void clearScrollback();
    
    AnsiParser* parser() { return &m_parser; }
    
signals:
    void dataReceived(const QString& text);
    void cursorMoved(int row, int col);
    void screenUpdated();
    void scrollbackChanged(int size);
    void titleChanged(const QString& title);
    
private:
    void insertChar(QChar ch, const TextStyle& style);
    void moveCursor(int row, int col);
    void scrollUp(int n);
    void scrollDown(int n);
    void eraseInDisplay(int mode);
    void eraseInLine(int mode);
    void lineFeed();
    void carriageReturn();
    void backspace();
    void tab();
    
    void handleCommand(const AnsiCommand& cmd);
    void handleLine(const QVector<QPair<QChar, TextStyle>>& line);
    void handleStyleChange(const TextStyle& oldStyle, const TextStyle& newStyle);
    
    int m_rows;
    int m_cols;
    CursorInfo m_cursor;
    ScrollRegion m_scrollRegion;
    TextStyle m_currentStyle;
    QVector<QVector<TerminalCell>> m_screen;
    QVector<QVector<TerminalCell>> m_scrollbackBuffer;
    AnsiParser m_parser;
    QString m_title;
    
    int m_maxScrollback = 10000;
};

#endif
