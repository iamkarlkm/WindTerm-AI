#ifndef THEME_CONFIG_H
#define THEME_CONFIG_H

#include <QString>
#include <QColor>
#include <QVector>
#include <QJsonObject>

struct ThemeConfig {
    QString name;
    
    // Base colors
    QColor foreground;
    QColor background;
    QColor cursor;
    QColor selection;
    QColor cursorText;
    
    // Standard 16 ANSI colors
    QColor black;
    QColor red;
    QColor green;
    QColor yellow;
    QColor blue;
    QColor magenta;
    QColor cyan;
    QColor white;
    
    // Bright variants
    QColor brightBlack;
    QColor brightRed;
    QColor brightGreen;
    QColor brightYellow;
    QColor brightBlue;
    QColor brightMagenta;
    QColor brightCyan;
    QColor brightWhite;
    
    // Font settings
    QString fontFamily;
    int fontSize;
    
    // Background image
    QString backgroundImage;
    double backgroundOpacity;
    
    ThemeConfig();
    
    bool isValid() const { return !name.isEmpty(); }
    
    QColor ansiColor(int index) const;
    
    QJsonObject toJson() const;
    static ThemeConfig fromJson(const QJsonObject& json);
    
    static ThemeConfig defaultTheme();
    static ThemeConfig draculaTheme();
    static ThemeConfig monokaiTheme();
    static ThemeConfig solarizedDarkTheme();
};

#endif
