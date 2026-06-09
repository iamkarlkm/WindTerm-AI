#ifndef SESSION_HISTORY_H
#define SESSION_HISTORY_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QDateTime>

struct HistoryEntry {
    QString id;
    QString sessionId;
    QString command;
    QString output;
    qint64 timestamp;
    int duration;
    int exitCode;
    QString workingDirectory;
    QString host;
    bool isFavorite;
    QStringList tags;
    
    HistoryEntry() : timestamp(0), duration(0), exitCode(0), isFavorite(false) {}
};

struct SearchQuery {
    QString text;
    bool useRegex;
    QDateTime startTime;
    QDateTime endTime;
    QString host;
    QStringList tags;
    int exitCode;
    bool favoritesOnly;
    int limit;
    
    SearchQuery() : useRegex(false), exitCode(-1), favoritesOnly(false), limit(100) {}
};

class SessionHistoryManager : public QObject {
    Q_OBJECT
public:
    explicit SessionHistoryManager(QObject* parent = nullptr);
    static SessionHistoryManager* instance();
    
    void addEntry(const HistoryEntry& entry);
    void deleteEntry(const QString& id);
    void updateEntry(const QString& id, const HistoryEntry& entry);
    
    HistoryEntry getEntry(const QString& id) const;
    QList<HistoryEntry> searchHistory(const SearchQuery& query) const;
    QList<HistoryEntry> getSessionHistory(const QString& sessionId, int limit = 100) const;
    QList<HistoryEntry> getHostHistory(const QString& host, int limit = 100) const;
    QList<HistoryEntry> getFavoriteEntries() const;
    QList<HistoryEntry> getRecentCommands(int limit = 50) const;
    
    int getTotalCount() const;
    int getSessionCount(const QString& sessionId) const;
    QStringList getUniqueHosts() const;
    QStringList getUniqueTags() const;
    QMap<QString, int> getCommandFrequency(int limit = 20) const;
    
    void toggleFavorite(const QString& id);
    void addTag(const QString& id, const QString& tag);
    void removeTag(const QString& id, const QString& tag);
    
    void exportHistory(const QString& filePath, const QList<QString>& ids, const QString& format = "json");
    void exportToMarkdown(const QString& filePath, const QList<QString>& ids);
    void exportToHtml(const QString& filePath, const QList<QString>& ids);
    
    void clearHistory(const QString& sessionId = "");
    void cleanupOldHistory(int daysToKeep = 30);
    
    QStringList suggestCommands(const QString& prefix, int limit = 10) const;
    QString suggestNextCommand(const QString& currentCommand) const;

signals:
    void entryAdded(const HistoryEntry& entry);
    void entryDeleted(const QString& id);
    void entryUpdated(const QString& id);
    void historyCleared();

private:
    static SessionHistoryManager* s_instance;
    QList<HistoryEntry> m_entries;
    QMap<QString, HistoryEntry> m_entryMap;
    QMap<QString, QList<QString>> m_sessionIndex;
    QMap<QString, QList<QString>> m_hostIndex;
    QString m_historyFile;
    
    void saveHistory();
    void loadHistory();
    void rebuildIndex();
    bool matchRegex(const QString& text, const QString& pattern) const;
};

#endif
