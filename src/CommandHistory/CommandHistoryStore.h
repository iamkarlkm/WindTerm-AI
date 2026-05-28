#ifndef COMMAND_HISTORY_STORE_H
#define COMMAND_HISTORY_STORE_H

#include <QObject>
#include <QSqlDatabase>
#include <QVector>
#include <QDateTime>

struct CommandHistoryEntry {
    qint64 id;
    QString command;
    QString workingDirectory;
    QString sessionType;
    QDateTime timestamp;
    int usageCount;
    
    CommandHistoryEntry() : id(0), usageCount(1) {
        timestamp = QDateTime::currentDateTime();
    }
};

class CommandHistoryStore : public QObject {
    Q_OBJECT
public:
    explicit CommandHistoryStore(QObject* parent = nullptr);
    
    static CommandHistoryStore* instance(QObject* parent = nullptr);
    
    bool initialize();
    bool isInitialized() const { return m_initialized; }
    
    void addCommand(const QString& command, const QString& workingDir = QString(), const QString& sessionType = "local");
    QVector<CommandHistoryEntry> search(const QString& query, int limit = 50) const;
    QVector<CommandHistoryEntry> recent(int limit = 50) const;
    bool deleteEntry(qint64 id);
    void clearHistory();
    
    QString databasePath() const;
    
signals:
    void commandAdded(const CommandHistoryEntry& entry);
    
private:
    bool createTables();
    
    QSqlDatabase m_database;
    bool m_initialized;
    
    static CommandHistoryStore* s_instance;
};

#endif
