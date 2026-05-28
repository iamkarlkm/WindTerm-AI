#include "CommandHistoryStore.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

CommandHistoryStore* CommandHistoryStore::s_instance = nullptr;

CommandHistoryStore::CommandHistoryStore(QObject* parent)
    : QObject(parent), m_initialized(false) {
}

CommandHistoryStore* CommandHistoryStore::instance(QObject* parent) {
    if (!s_instance) {
        s_instance = new CommandHistoryStore(parent);
    }
    return s_instance;
}

bool CommandHistoryStore::initialize() {
    if (m_initialized) return true;
    
    QString dbPath = databasePath();
    QDir().mkpath(QFileInfo(dbPath).absolutePath());
    
    m_database = QSqlDatabase::addDatabase("QSQLITE", "CommandHistory");
    m_database.setDatabaseName(dbPath);
    
    if (!m_database.open()) {
        qWarning() << "[CommandHistoryStore] Failed to open database:" << m_database.lastError().text();
        return false;
    }
    
    if (!createTables()) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void CommandHistoryStore::addCommand(const QString& command, const QString& workingDir, const QString& sessionType) {
    if (!m_initialized || command.trimmed().isEmpty()) return;
    
    QSqlQuery query(m_database);
    
    query.prepare("SELECT id, usage_count FROM command_history WHERE command = :cmd");
    query.bindValue(":cmd", command.trimmed());
    
    if (query.exec() && query.next()) {
        qint64 id = query.value(0).toLongLong();
        int count = query.value(1).toInt();
        
        query.prepare("UPDATE command_history SET usage_count = :count, last_used = :timestamp WHERE id = :id");
        query.bindValue(":count", count + 1);
        query.bindValue(":timestamp", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.bindValue(":id", id);
        query.exec();
    } else {
        query.prepare("INSERT INTO command_history (command, working_directory, session_type, created_at, last_used, usage_count) "
                      "VALUES (:cmd, :dir, :type, :created, :lastUsed, :count)");
        query.bindValue(":cmd", command.trimmed());
        query.bindValue(":dir", workingDir);
        query.bindValue(":type", sessionType);
        QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
        query.bindValue(":created", now);
        query.bindValue(":lastUsed", now);
        query.bindValue(":count", 1);
        query.exec();
    }
}

QVector<CommandHistoryEntry> CommandHistoryStore::search(const QString& query, int limit) const {
    QVector<CommandHistoryEntry> results;
    if (!m_initialized) return results;
    
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(
        "SELECT id, command, working_directory, session_type, created_at, last_used, usage_count "
        "FROM command_history "
        "WHERE command LIKE :query "
        "ORDER BY usage_count DESC, last_used DESC "
        "LIMIT :limit"
    );
    sqlQuery.bindValue(":query", "%" + query + "%");
    sqlQuery.bindValue(":limit", limit);
    
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            CommandHistoryEntry entry;
            entry.id = sqlQuery.value(0).toLongLong();
            entry.command = sqlQuery.value(1).toString();
            entry.workingDirectory = sqlQuery.value(2).toString();
            entry.sessionType = sqlQuery.value(3).toString();
            entry.timestamp = QDateTime::fromString(sqlQuery.value(4).toString(), Qt::ISODate);
            entry.usageCount = sqlQuery.value(6).toInt();
            results.append(entry);
        }
    }
    
    return results;
}

QVector<CommandHistoryEntry> CommandHistoryStore::recent(int limit) const {
    QVector<CommandHistoryEntry> results;
    if (!m_initialized) return results;
    
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(
        "SELECT id, command, working_directory, session_type, created_at, last_used, usage_count "
        "FROM command_history "
        "ORDER BY last_used DESC "
        "LIMIT :limit"
    );
    sqlQuery.bindValue(":limit", limit);
    
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            CommandHistoryEntry entry;
            entry.id = sqlQuery.value(0).toLongLong();
            entry.command = sqlQuery.value(1).toString();
            entry.workingDirectory = sqlQuery.value(2).toString();
            entry.sessionType = sqlQuery.value(3).toString();
            entry.timestamp = QDateTime::fromString(sqlQuery.value(4).toString(), Qt::ISODate);
            entry.usageCount = sqlQuery.value(6).toInt();
            results.append(entry);
        }
    }
    
    return results;
}

bool CommandHistoryStore::deleteEntry(qint64 id) {
    if (!m_initialized) return false;
    
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM command_history WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

void CommandHistoryStore::clearHistory() {
    if (!m_initialized) return;
    
    QSqlQuery query(m_database);
    query.exec("DELETE FROM command_history");
}

QString CommandHistoryStore::databasePath() const {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataDir + "/command_history.db";
}

bool CommandHistoryStore::createTables() {
    QSqlQuery query(m_database);
    
    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS command_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            command TEXT NOT NULL,
            working_directory TEXT,
            session_type TEXT DEFAULT 'local',
            created_at TEXT NOT NULL,
            last_used TEXT NOT NULL,
            usage_count INTEGER DEFAULT 1
        )
    )";
    
    if (!query.exec(createTable)) {
        qWarning() << "[CommandHistoryStore] Failed to create table:" << query.lastError().text();
        return false;
    }
    
    QString createIndex = "CREATE INDEX IF NOT EXISTS idx_command ON command_history(command)";
    query.exec(createIndex);
    
    QString createIndex2 = "CREATE INDEX IF NOT EXISTS idx_last_used ON command_history(last_used)";
    query.exec(createIndex2);
    
    return true;
}
