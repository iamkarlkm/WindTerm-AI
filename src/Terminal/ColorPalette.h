#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H

#include <QColor>
#include <QVector>
#include <QHash>

class ColorPalette {
public:
    ColorPalette();
    
    static QColor getAnsiColor(int index);
    static QColor get256Color(int index);
    static QColor getTrueColor(int r, int g, int b);
    
    static QColor parseColor(const QString& colorSpec);
    static QString toAnsiCode(const QColor& color, bool isBackground = false);
    
    static const int ANSI_COLORS = 16;
    static const int COLOR_256_COUNT = 256;
    
private:
    static void init256Palette();
    static QVector<QColor> m_256Palette;
    static bool m_paletteInitialized;
};

#endif
