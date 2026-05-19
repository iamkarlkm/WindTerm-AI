#include "CircularTextBuffer.h"
#include <QDateTime>
#include <algorithm>

CircularTextBuffer::CircularTextBuffer(int capacity)
    : m_head(0), m_capacity(capacity), m_count(0), m_nextLineNumber(1) {
    m_lines.resize(capacity);
}

void CircularTextBuffer::append(const QString& text) {
    QMutexLocker locker(&m_mutex);
    
    QStringList lines = text.split('\n');
    if (lines.isEmpty()) return;
    
    for (const QString& line : lines) {
        int idx = (m_head + m_count) % m_capacity;
        
        if (m_count == m_capacity) {
            m_head = (m_head + 1) % m_capacity;
        } else {
            m_count++;
        }
        
        m_lines[idx] = LineData{
            line,
            m_nextLineNumber++,
            false,
            QDateTime::currentMSecsSinceEpoch()
        };
    }
}

void CircularTextBuffer::appendLines(const QStringList& lines) {
    QMutexLocker locker(&m_mutex);
    
    for (const QString& line : lines) {
        int idx = (m_head + m_count) % m_capacity;
        
        if (m_count == m_capacity) {
            m_head = (m_head + 1) % m_capacity;
        } else {
            m_count++;
        }
        
        m_lines[idx] = LineData{
            line,
            m_nextLineNumber++,
            false,
            QDateTime::currentMSecsSinceEpoch()
        };
    }
}

const LineData& CircularTextBuffer::lineAt(int index) const {
    QMutexLocker locker(&m_mutex);
    
    if (index < 0 || index >= m_count) {
        static LineData empty{"", 0, false, 0};
        return empty;
    }
    
    int idx = (m_head + index) % m_capacity;
    return m_lines[idx];
}

int CircularTextBuffer::size() const {
    QMutexLocker locker(&m_mutex);
    return m_count;
}

bool CircularTextBuffer::isEmpty() const {
    return m_count == 0;
}

QStringList CircularTextBuffer::getLines(int startIndex, int count) const {
    QMutexLocker locker(&m_mutex);
    
    QStringList result;
    int end = qMin(startIndex + count, m_count);
    
    for (int i = startIndex; i < end; i++) {
        int idx = (m_head + i) % m_capacity;
        result.append(m_lines[idx].text);
    }
    
    return result;
}

QString CircularTextBuffer::getLastNLines(int n) const {
    QMutexLocker locker(&m_mutex);
    
    int start = qMax(0, m_count - n);
    QStringList lines;
    
    for (int i = start; i < m_count; i++) {
        int idx = (m_head + i) % m_capacity;
        lines.append(m_lines[idx].text);
    }
    
    return lines.join('\n');
}

void CircularTextBuffer::clear() {
    QMutexLocker locker(&m_mutex);
    m_head = 0;
    m_count = 0;
    m_nextLineNumber = 1;
}

void CircularTextBuffer::setCapacity(int capacity) {
    QMutexLocker locker(&m_mutex);
    resize(capacity);
}

int CircularTextBuffer::findLineContaining(int position) const {
    QMutexLocker locker(&m_mutex);
    
    int currentPos = 0;
    for (int i = 0; i < m_count; i++) {
        int idx = (m_head + i) % m_capacity;
        int lineLen = m_lines[idx].text.length() + 1;
        
        if (currentPos + lineLen > position) {
            return i;
        }
        
        currentPos += lineLen;
    }
    
    return m_count - 1;
}

QString CircularTextBuffer::getVisibleText(int startLine, int endLine) const {
    QStringList lines = getLines(startLine, endLine - startLine);
    return lines.join('\n');
}

void CircularTextBuffer::resize(int newCapacity) {
    if (newCapacity == m_capacity) return;
    
    QVector<LineData> newLines(newCapacity);
    
    int copyCount = qMin(m_count, newCapacity);
    int srcStart = (m_head + m_count - copyCount) % m_capacity;
    
    for (int i = 0; i < copyCount; i++) {
        int srcIdx = (srcStart + i) % m_capacity;
        newLines[i] = m_lines[srcIdx];
    }
    
    m_lines = std::move(newLines);
    m_head = 0;
    m_count = copyCount;
    m_capacity = newCapacity;
}
