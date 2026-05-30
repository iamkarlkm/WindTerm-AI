#include "ColorPalette.h"
#include <QtMath>

QVector<QColor> ColorPalette::m_256Palette;
bool ColorPalette::m_paletteInitialized = false;

ColorPalette::ColorPalette() {
    if (!m_paletteInitialized) {
        init256Palette();
    }
}

void ColorPalette::init256Palette() {
    if (m_paletteInitialized) return;
    
    m_256Palette.reserve(COLOR_256_COUNT);
    
    // 0-15: Standard ANSI colors
    m_256Palette << QColor(0, 0, 0)       // 0: Black
                 << QColor(128, 0, 0)     // 1: Red
                 << QColor(0, 128, 0)     // 2: Green
                 << QColor(128, 128, 0)   // 3: Yellow
                 << QColor(0, 0, 128)     // 4: Blue
                 << QColor(128, 0, 128)   // 5: Magenta
                 << QColor(0, 128, 128)   // 6: Cyan
                 << QColor(192, 192, 192) // 7: White
                 << QColor(128, 128, 128) // 8: Bright Black
                 << QColor(255, 0, 0)     // 9: Bright Red
                 << QColor(0, 255, 0)     // 10: Bright Green
                 << QColor(255, 255, 0)   // 11: Bright Yellow
                 << QColor(0, 0, 255)     // 12: Bright Blue
                 << QColor(255, 0, 255)   // 13: Bright Magenta
                 << QColor(0, 255, 255)   // 14: Bright Cyan
                 << QColor(255, 255, 255);// 15: Bright White
    
    // 16-231: 6x6x6 color cube
    const int colorSteps[6] = {0, 95, 135, 175, 215, 255};
    for (int r = 0; r < 6; r++) {
        for (int g = 0; g < 6; g++) {
            for (int b = 0; b < 6; b++) {
                m_256Palette.append(QColor(colorSteps[r], colorSteps[g], colorSteps[b]));
            }
        }
    }
    
    // 232-255: Grayscale ramp (24 steps from black to white)
    for (int i = 0; i < 24; i++) {
        int gray = 8 + i * 10;
        m_256Palette.append(QColor(gray, gray, gray));
    }
    
    m_paletteInitialized = true;
}

QColor ColorPalette::getAnsiColor(int index) {
    if (index >= 0 && index < ANSI_COLORS) {
        return m_256Palette[index];
    }
    return m_256Palette[0]; // Default to black
}

QColor ColorPalette::get256Color(int index) {
    if (index >= 0 && index < COLOR_256_COUNT) {
        return m_256Palette[index];
    }
    return m_256Palette[0];
}

QColor ColorPalette::getTrueColor(int r, int g, int b) {
    return QColor(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
}

QColor ColorPalette::parseColor(const QString& colorSpec) {
    // Handle #RRGGBB format
    if (colorSpec.startsWith('#')) {
        return QColor(colorSpec);
    }
    
    // Handle rgb(r,g,b) format
    if (colorSpec.startsWith("rgb(")) {
        QString content = colorSpec.mid(4, colorSpec.length() - 5);
        QStringList parts = content.split(',');
        if (parts.size() == 3) {
            return QColor(parts[0].toInt(), parts[1].toInt(), parts[2].toInt());
        }
    }
    
    // Handle color name
    return QColor(colorSpec);
}

QString ColorPalette::toAnsiCode(const QColor& color, bool isBackground) {
    int baseCode = isBackground ? 48 : 38;
    
    // Find closest 256-color match
    int closestIndex = 0;
    int minDistance = INT_MAX;
    
    for (int i = 0; i < COLOR_256_COUNT; i++) {
        const QColor& paletteColor = m_256Palette[i];
        int dr = color.red() - paletteColor.red();
        int dg = color.green() - paletteColor.green();
        int db = color.blue() - paletteColor.blue();
        int distance = dr*dr + dg*dg + db*db;
        
        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
    }
    
    // Use 256-color mode
    return QString("%1;5;%2").arg(baseCode).arg(closestIndex);
}

#include "ColorPalette.moc"
