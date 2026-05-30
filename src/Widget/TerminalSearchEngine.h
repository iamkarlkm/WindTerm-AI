#ifndef TERMINAL_SEARCH_ENGINE_H
#define TERMINAL_SEARCH_ENGINE_H

#include <QPoint>
#include <QObject>
#include <QVector>
#include <QRegularExpression>

struct SearchMatch {
    int row;
    int col;
    int length;
    QString matchedText;
};

class TerminalSearchEngine : public QObject {
    Q_OBJECT
public:
    explicit TerminalSearchEngine(QObject* parent = nullptr);
    
    // 搜索设置
    void setSearchText(const QString& text);
    void setCaseSensitive(bool sensitive);
    void setUseRegex(bool useRegex);
    void setMatchWholeWord(bool match);
    void setReverseSearch(bool reverse);
    
    // 执行搜索
    QVector<SearchMatch> search(const QVector<QVector<QPair<QChar, void*>>>& buffer);
    
    // 查找下一个/上一个
    SearchMatch findNext(int currentRow, int currentCol);
    SearchMatch findPrevious(int currentRow, int currentCol);
    
    // 高亮管理
    void clearHighlights();
    QVector<SearchMatch> currentHighlights() const { return m_currentMatches; }
    
    // 统计
    int matchCount() const { return m_currentMatches.size(); }
    
    // 增量搜索
    void startIncrementalSearch();
    void addToIncrementalSearch(const QString& text);
    void endIncrementalSearch();
    
signals:
    void searchStarted();
    void searchCompleted(int matchCount);
    void matchFound(const SearchMatch& match);
    void noMoreMatches();
    void wrapAround();

private:
    bool matchAtPosition(const QString& text, int row, int col, const QVector<QVector<QPair<QChar, void*>>>& buffer);
    QString extractText(int row, int col, int length, const QVector<QVector<QPair<QChar, void*>>>& buffer);
    
    QString m_searchText;
    bool m_caseSensitive;
    bool m_useRegex;
    bool m_matchWholeWord;
    bool m_reverseSearch;
    
    QRegularExpression m_regex;
    QVector<SearchMatch> m_currentMatches;
    int m_currentMatchIndex;
    
    // 增量搜索状态
    bool m_incrementalMode;
    QString m_incrementalText;
};

#endif
