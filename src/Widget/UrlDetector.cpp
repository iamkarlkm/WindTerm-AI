#include "UrlDetector.h"
#include <QFileInfo>
#include <QDir>

UrlDetector::UrlDetector() {
    m_urlPattern = QRegularExpression(
        R"(\b(?:https?|ftp|ssh)://[^\s<>"{}|\\^`\[\]]+)",
        QRegularExpression::CaseInsensitiveOption
    );
    
    m_filePathPattern = QRegularExpression(
        R"(\b(?:/|~|[A-Za-z]:)[\w./\-~]+(?:\.[\w]+)*)"
    );
    
    m_emailPattern = QRegularExpression(
        R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b)"
    );
}

bool UrlDetector::isUrl(const QString& text) {
    return text.startsWith("http://") || 
           text.startsWith("https://") || 
           text.startsWith("ftp://") ||
           text.startsWith("ssh://");
}

bool UrlDetector::isFilePath(const QString& text) {
    return text.startsWith("/") || text.startsWith("~");
}

QString UrlDetector::extractUrl(const QString& text, int start, int& length) {
    if (start < 0 || start >= text.length()) {
        length = 0;
        return QString();
    }
    
    QChar ch = text[start];
    if (!ch.isLetterOrNumber() && ch != QChar('/') && ch != QChar('~') && 
        ch != QChar('.') && ch != QChar('-')) {
        length = 0;
        return QString();
    }
    
    int end = start;
    while (end < text.length()) {
        QChar c = text[end];
        
        if (c.isLetterOrNumber() || c == QChar('/') || c == QChar('.') || 
            c == QChar('-') || c == QChar('_') || c == QChar('~') ||
            c == QChar(':') || c == QChar('?') || c == QChar('=') ||
            c == QChar('&') || c == QChar('%') || c == QChar('#')) {
            end++;
        } else if (c == QChar('@')) {
            end++;
        } else {
            break;
        }
    }
    
    while (end > start) {
        QChar c = text[end - 1];
        if (c == QChar('.') || c == QChar(',') || c == QChar('!') || 
            c == QChar('?') || c == QChar(':') || c == QChar(';')) {
            end--;
        } else {
            break;
        }
    }
    
    QString extracted = text.mid(start, end - start).trimmed();
    
    if (!extracted.isEmpty() && !extracted.startsWith("http") && 
        !extracted.startsWith("ftp") && !extracted.startsWith("ssh") &&
        !extracted.startsWith("/") && !extracted.startsWith("~")) {
        
        int atIndex = extracted.indexOf('@');
        if (atIndex > 0 && extracted.indexOf('.') > atIndex) {
            length = extracted.length();
            return extracted;
        }
        
        length = 0;
        return QString();
    }
    
    length = extracted.length();
    return extracted;
}

QVector<UrlMatch> UrlDetector::findUrls(const QString& text, int row, int offset) {
    QVector<UrlMatch> matches;
    
    QRegularExpressionMatchIterator it = m_urlPattern.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        
        UrlMatch urlMatch;
        urlMatch.row = row;
        urlMatch.col = match.capturedStart() + offset;
        urlMatch.length = match.capturedLength();
        urlMatch.url = match.captured();
        urlMatch.isFile = false;
        matches.append(urlMatch);
    }
    
    it = m_filePathPattern.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString path = match.captured();
        
        if (path.length() > 3 && (path.startsWith("/") || path.startsWith("~"))) {
            bool alreadyFound = false;
            for (const UrlMatch& existing : matches) {
                if (existing.col <= match.capturedStart() + offset &&
                    existing.col + existing.length >= match.capturedStart() + offset + match.capturedLength()) {
                    alreadyFound = true;
                    break;
                }
            }
            
            if (!alreadyFound) {
                UrlMatch urlMatch;
                urlMatch.row = row;
                urlMatch.col = match.capturedStart() + offset;
                urlMatch.length = match.capturedLength();
                urlMatch.url = path;
                urlMatch.isFile = true;
                matches.append(urlMatch);
            }
        }
    }
    
    return matches;
}
