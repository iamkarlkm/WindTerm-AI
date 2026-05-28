#ifndef UNICODE_UTIL_H
#define UNICODE_UTIL_H

#include <QString>
#include <QVector>

struct UnicodeChar {
    quint32 codepoint;
    int displayWidth;
    bool isCombining;
    bool isEmoji;
    bool isZeroWidth;
};

class UnicodeUtil {
public:
    static int charDisplayWidth(quint32 codepoint);
    static bool isCombiningCharacter(quint32 codepoint);
    static bool isEmoji(quint32 codepoint);
    static bool isZeroWidth(quint32 codepoint);
    
    static QVector<UnicodeChar> parseString(const QString& text);
    static QString normalizeToNFC(const QString& text);
    
    static int stringDisplayWidth(const QString& text);
};

#endif
