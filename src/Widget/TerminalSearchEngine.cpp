#include "TerminalSearchEngine.h"
#include <QtMath>
#include <QDebug>

TerminalSearchEngine::TerminalSearchEngine(QObject* parent)
    : QObject(parent)
    , m_caseSensitive(false)
    , m_useRegex(false)
    , m_matchWholeWord(false)
    , m_reverseSearch(false)
    , m_currentMatchIndex(-1)
    , m_incrementalMode(false) {
}

void TerminalSearchEngine::setSearchText(const QString& text) {
    m_searchText = text;
    
    if (m_useRegex && !text.isEmpty()) {
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (!m_caseSensitive) {
            options |= QRegularExpression::CaseInsensitiveOption;
        }
        
        if (m_matchWholeWord) {
            m_regex = QRegularExpression("\\b" + QRegularExpression::escape(text) + "\\b", options);
        } else {
            m_regex = QRegularExpression(text, options);
        }
    }
}

void TerminalSearchEngine::setCaseSensitive(bool sensitive) {
    m_caseSensitive = sensitive;
    if (m_useRegex) {
        setSearchText(m_searchText);
    }
}

void TerminalSearchEngine::setUseRegex(bool useRegex) {
    m_useRegex = useRegex;
    if (useRegex) {
        setSearchText(m_searchText);
    }
}

void TerminalSearchEngine::setMatchWholeWord(bool match) {
    m_matchWholeWord = match;
    if (m_useRegex) {
        setSearchText(m_searchText);
    }
}

void TerminalSearchEngine::setReverseSearch(bool reverse) {
    m_reverseSearch = reverse;
}

QVector<SearchMatch> TerminalSearchEngine::search(
        const QVector<QVector<QPair<QChar, void*>>>& buffer) {
    
    emit searchStarted();
    m_currentMatches.clear();
    m_currentMatchIndex = -1;
    
    if (m_searchText.isEmpty()) {
        emit searchCompleted(0);
        return m_currentMatches;
    }
    
    int rows = buffer.size();
    if (rows == 0) {
        emit searchCompleted(0);
        return m_currentMatches;
    }
    
    int cols = buffer[0].size();
    
    // 将缓冲区转换为文本
    QString bufferText;
    QVector<QPoint> positions;
    
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            if (col < buffer[row].size()) {
                bufferText += buffer[row][col].first;
                positions.append(QPoint(row, col));
            }
        }
        bufferText += '\n';
        positions.append(QPoint(row, -1));  // 行尾标记
    }
    
    // 执行搜索
    if (m_useRegex) {
        QRegularExpressionMatchIterator it = m_regex.globalMatch(bufferText);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int startPos = match.capturedStart();
            int length = match.capturedLength();
            
            if (startPos >= 0 && startPos + length <= positions.size()) {
                SearchMatch sm;
                sm.row = positions[startPos].x();
                sm.col = positions[startPos].y();
                sm.length = length;
                sm.matchedText = match.captured();
                
                if (sm.col >= 0) {  // 有效的列位置
                    m_currentMatches.append(sm);
                }
            }
        }
    } else {
        // 普通文本搜索
        QString searchText = m_caseSensitive ? m_searchText : m_searchText.toLower();
        QString searchTextLower = searchText.toLower();
        
        int pos = 0;
        while ((pos = m_caseSensitive ? 
                     bufferText.indexOf(searchText, pos) :
                     bufferText.indexOf(searchTextLower, pos)) != -1) {
            
            if (pos + searchText.length() <= positions.size()) {
                SearchMatch sm;
                sm.row = positions[pos].x();
                sm.col = positions[pos].y();
                sm.length = searchText.length();
                sm.matchedText = bufferText.mid(pos, searchText.length());
                
                if (sm.col >= 0) {
                    m_currentMatches.append(sm);
                }
            }
            pos += searchText.length();
        }
    }
    
    emit searchCompleted(m_currentMatches.size());
    qDebug() << "[TerminalSearchEngine] Found" << m_currentMatches.size() << "matches";
    
    return m_currentMatches;
}

SearchMatch TerminalSearchEngine::findNext(int currentRow, int currentCol) {
    if (m_currentMatches.isEmpty()) {
        emit noMoreMatches();
        return SearchMatch();
    }
    
    // 查找下一个匹配
    for (int i = 0; i < m_currentMatches.size(); i++) {
        int idx = (m_currentMatchIndex + 1 + i) % m_currentMatches.size();
        const SearchMatch& match = m_currentMatches[idx];
        
        if (match.row > currentRow || (match.row == currentRow && match.col > currentCol)) {
            m_currentMatchIndex = idx;
            emit matchFound(match);
            return match;
        }
    }
    
    // 回绕到开头
    emit wrapAround();
    m_currentMatchIndex = 0;
    return m_currentMatches[0];
}

SearchMatch TerminalSearchEngine::findPrevious(int currentRow, int currentCol) {
    if (m_currentMatches.isEmpty()) {
        emit noMoreMatches();
        return SearchMatch();
    }
    
    // 查找上一个匹配
    for (int i = 0; i < m_currentMatches.size(); i++) {
        int idx = (m_currentMatchIndex - 1 - i + m_currentMatches.size()) % m_currentMatches.size();
        const SearchMatch& match = m_currentMatches[idx];
        
        if (match.row < currentRow || (match.row == currentRow && match.col < currentCol)) {
            m_currentMatchIndex = idx;
            emit matchFound(match);
            return match;
        }
    }
    
    // 回绕到末尾
    emit wrapAround();
    m_currentMatchIndex = m_currentMatches.size() - 1;
    return m_currentMatches.last();
}

void TerminalSearchEngine::clearHighlights() {
    m_currentMatches.clear();
    m_currentMatchIndex = -1;
}

void TerminalSearchEngine::startIncrementalSearch() {
    m_incrementalMode = true;
    m_incrementalText.clear();
}

void TerminalSearchEngine::addToIncrementalSearch(const QString& text) {
    if (!m_incrementalMode) return;
    
    m_incrementalText = text;
    setSearchText(m_incrementalText);
    
    // 这里应该触发实时搜索，但需要 buffer 参数
    // 实际使用时需要调用 search() 方法
}

void TerminalSearchEngine::endIncrementalSearch() {
    m_incrementalMode = false;
    m_incrementalText.clear();
}

#include "TerminalSearchEngine.moc"
