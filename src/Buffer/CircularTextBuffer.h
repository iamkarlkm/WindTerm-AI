#ifndef CIRCULAR_TEXT_BUFFER_H
#define CIRCULAR_TEXT_BUFFER_H

#include <QString>
#include <QVector>
#include <QMutex>
#include <QDateTime>

struct LineData {
    QString text;
    int lineNumber;
    bool isWrapped;
    qint64 timestamp;
};

class CircularTextBuffer {
public:
    explicit CircularTextBuffer(int capacity = 10000);
    
    void append(const QString& text);
    void appendLines(const QStringList& lines);
    
    const LineData& lineAt(int index) const;
    int size() const;
    bool isEmpty() const;
    
    QStringList getLines(int startIndex, int count) const;
    QString getLastNLines(int n) const;
    
    void clear();
    void setCapacity(int capacity);
    
    int findLineContaining(int position) const;
    QString getVisibleText(int startLine, int endLine) const;
    
private:
    void resize(int newCapacity);
    
    QVector<LineData> m_lines;
    int m_head;
    int m_capacity;
    int m_count;
    int m_nextLineNumber;
    mutable QMutex m_mutex;
};

#endif
