#ifndef URL_DETECTOR_H
#define URL_DETECTOR_H

#include <QString>
#include <QVector>
#include <QRegularExpression>

struct UrlMatch {
    int row;
    int col;
    int length;
    QString url;
    bool isFile;
    bool isHyperlink;
};

class UrlDetector {
public:
    UrlDetector();
    
    QVector<UrlMatch> findUrls(const QString& text, int row, int offset = 0);
    
    static bool isUrl(const QString& text);
    static bool isFilePath(const QString& text);
    static QString extractUrl(const QString& text, int start, int& length);

private:
    QRegularExpression m_urlPattern;
    QRegularExpression m_filePathPattern;
    QRegularExpression m_emailPattern;
};

#endif
