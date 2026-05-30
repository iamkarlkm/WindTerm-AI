#ifndef TERMINAL_THEME_H
#define TERMINAL_THEME_H

#include <QColor>
#include <QString>
#include <QVector>
#include <QMap>

struct ThemeColor {
    QColor foreground;
    QColor background;
    QColor cursor;
    QColor selection;
    QVector<QColor> ansiColors;  // 16 colors
    
    ThemeColor() {
        foreground = QColor(229, 229, 229);
        background = QColor(0, 0, 0);
        cursor = QColor(255, 255, 255);
        selection = QColor(61, 61, 255);
        
        // Default ANSI colors
        ansiColors = {
            QColor(0, 0, 0),       // Black
            QColor(205, 0, 0),     // Red
            QColor(0, 205, 0),     // Green
            QColor(205, 205, 0),   // Yellow
            QColor(0, 0, 238),     // Blue
            QColor(205, 0, 205),   // Magenta
            QColor(0, 205, 205),   // Cyan
            QColor(229, 229, 229), // White
            QColor(127, 127, 127), // Bright Black
            QColor(255, 0, 0),     // Bright Red
            QColor(0, 255, 0),     // Bright Green
            QColor(255, 255, 0),   // Bright Yellow
            QColor(92, 92, 255),   // Bright Blue
            QColor(255, 0, 255),   // Bright Magenta
            QColor(0, 255, 255),   // Bright Cyan
            QColor(255, 255, 255)  // Bright White
        };
    }
};

struct FontConfig {
    QString family;
    int size;
    bool bold;
    bool italic;
    
    FontConfig() : family("monospace"), size(14), bold(false), italic(false) {}
};

struct TerminalTheme {
    QString name;
    QString description;
    ThemeColor colors;
    FontConfig font;
    int opacity;  // 0-100
    bool smoothScroll;
    int cursorBlinkInterval;  // ms
    
    TerminalTheme() 
        : opacity(100), smoothScroll(true), cursorBlinkInterval(530) {}
    
    static TerminalTheme loadPreset(const QString& presetName);
    static QVector<QString> availablePresets();
    
    void saveToFile(const QString& filePath) const;
    static TerminalTheme loadFromFile(const QString& filePath);
};

#endif
