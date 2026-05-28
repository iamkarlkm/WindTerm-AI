#include "UnicodeUtil.h"
#include <QRegularExpression>

namespace {

bool isCJK(quint32 cp) {
    return (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0x3400 && cp <= 0x4DBF) ||
           (cp >= 0x20000 && cp <= 0x2A6DF) ||
           (cp >= 0x2A700 && cp <= 0x2B73F) ||
           (cp >= 0x2B740 && cp <= 0x2B81F);
}

bool isFullWidth(quint32 cp) {
    return (cp >= 0xFF01 && cp <= 0xFF5F) ||
           (cp >= 0xFFE0 && cp <= 0xFFE6);
}

bool isHangulSyllable(quint32 cp) {
    return cp >= 0xAC00 && cp <= 0xD7A3;
}

}

int UnicodeUtil::charDisplayWidth(quint32 cp) {
    if (isCombiningCharacter(cp) || isZeroWidth(cp)) {
        return 0;
    }
    
    if (isCJK(cp) || isHangulSyllable(cp) || isFullWidth(cp)) {
        return 2;
    }
    
    if (cp >= 0x1F300 && cp <= 0x1F9FF) {
        return 2;
    }
    
    if (cp >= 0x2000 && cp <= 0x25FF) {
        if (cp >= 0x2000 && cp <= 0x200A) return 1;
        if (cp == 0x200B) return 0;
        if (cp == 0x200C) return 0;
        if (cp == 0x200D) return 0;
        return 1;
    }
    
    if (cp < 128) {
        return 1;
    }
    
    return 1;
}

bool UnicodeUtil::isCombiningCharacter(quint32 cp) {
    return (cp >= 0x0300 && cp <= 0x036F) ||
           (cp >= 0x1AB0 && cp <= 0x1AFF) ||
           (cp >= 0x1DC0 && cp <= 0x1DFF) ||
           (cp >= 0x20D0 && cp <= 0x20FF) ||
           (cp >= 0xFE20 && cp <= 0xFE2F);
}

bool UnicodeUtil::isEmoji(quint32 cp) {
    if (cp >= 0x1F300 && cp <= 0x1F9FF) return true;
    if (cp >= 0x1FA00 && cp <= 0x1FAFF) return true;
    if (cp >= 0x2600 && cp <= 0x26FF) return true;
    if (cp >= 0x2700 && cp <= 0x27BF) return true;
    if (cp >= 0xFE00 && cp <= 0xFE0F) return true;
    if (cp >= 0x1F100 && cp <= 0x1F1FF) return true;
    
    return cp == 0x200D;
}

bool UnicodeUtil::isZeroWidth(quint32 cp) {
    return cp == 0x200B ||
           cp == 0x200C ||
           cp == 0x200D ||
           cp == 0xFEFF ||
           cp == 0x00AD;
}

QVector<UnicodeChar> UnicodeUtil::parseString(const QString& text) {
    QVector<UnicodeChar> result;
    
    const ushort* utf16 = reinterpret_cast<const ushort*>(text.utf16());
    int len = text.length();
    
    for (int i = 0; i < len; ++i) {
        quint32 cp = utf16[i];
        
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < len) {
            quint32 low = utf16[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                ++i;
            }
        }
        
        UnicodeChar ch;
        ch.codepoint = cp;
        ch.displayWidth = charDisplayWidth(cp);
        ch.isCombining = isCombiningCharacter(cp);
        ch.isEmoji = isEmoji(cp);
        ch.isZeroWidth = isZeroWidth(cp);
        
        result.append(ch);
    }
    
    return result;
}

QString UnicodeUtil::normalizeToNFC(const QString& text) {
    return text.normalized(QString::NormalizationForm_C);
}

int UnicodeUtil::stringDisplayWidth(const QString& text) {
    QVector<UnicodeChar> chars = parseString(text);
    int totalWidth = 0;
    
    for (const UnicodeChar& ch : chars) {
        if (!ch.isZeroWidth && !ch.isCombining) {
            totalWidth += ch.displayWidth;
        }
    }
    
    return totalWidth;
}
