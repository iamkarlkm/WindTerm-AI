#include "MemoryFragmentStore.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

MemoryFragmentStore* MemoryFragmentStore::s_instance = nullptr;

MemoryFragmentStore::MemoryFragmentStore(QObject* parent)
    : QObject(parent), m_initialized(false) {}

MemoryFragmentStore::~MemoryFragmentStore() {
    if (m_database.isOpen()) {
        m_database.close();
    }
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

MemoryFragmentStore* MemoryFragmentStore::instance(QObject* parent) {
    if (!s_instance) {
        s_instance = new MemoryFragmentStore(parent);
    }
    return s_instance;
}

bool MemoryFragmentStore::initialize(const QString& dbPath) {
    if (m_initialized) return true;
    
    QString path = dbPath.isEmpty() ? defaultDbPath() : dbPath;
    
    QDir dbDir = QFileInfo(path).absoluteDir();
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(".")) {
            emit databaseError(QStringLiteral("无法创建数据库目录: ") + dbDir.absolutePath());
            return false;
        }
    }
    
    m_database = QSqlDatabase::addDatabase("QSQLITE", "MemoryFragments");
    m_database.setDatabaseName(path);
    
    if (!m_database.open()) {
        emit databaseError(m_database.lastError().text());
        return false;
    }
    
    if (!createTables()) {
        return false;
    }
    
    m_initialized = true;
    qDebug() << "[MemoryFragmentStore] Initialized:" << path;
    return true;
}

bool MemoryFragmentStore::createTables() {
    QSqlQuery query(m_database);
    
    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS memory_fragments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT DEFAULT '',
            content TEXT NOT NULL,
            terminal_type TEXT DEFAULT '',
            working_directory TEXT DEFAULT '',
            session_id TEXT DEFAULT '',
            command_history TEXT DEFAULT '',
            source_type TEXT DEFAULT 'manual',
            source_remark TEXT DEFAULT '',
            created_at TEXT DEFAULT (datetime('now')),
            updated_at TEXT DEFAULT (datetime('now'))
        )
    )";
    
    if (!query.exec(createTable)) {
        emit databaseError(query.lastError().text());
        return false;
    }
    
    QString createIndex1 = R"(
        CREATE INDEX IF NOT EXISTS idx_created_at ON memory_fragments(created_at DESC)
    )";
    if (!query.exec(createIndex1)) {
        emit databaseError(query.lastError().text());
        return false;
    }
    
    QString createIndex2 = R"(
        CREATE INDEX IF NOT EXISTS idx_source_type ON memory_fragments(source_type)
    )";
    if (!query.exec(createIndex2)) {
        emit databaseError(query.lastError().text());
        return false;
    }
    
    return true;
}

QString MemoryFragmentStore::defaultDbPath() const {
#ifdef Q_OS_WIN
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#endif
    return dataDir + "/memory_fragments.db";
}

qint64 MemoryFragmentStore::createFragment(const MemoryFragment& fragment) {
    if (!m_initialized) return -1;
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO memory_fragments (
            title, content, terminal_type, working_directory, 
            session_id, command_history, source_type, source_remark,
            created_at, updated_at
        ) VALUES (
            :title, :content, :terminal_type, :working_directory,
            :session_id, :command_history, :source_type, :source_remark,
            :created_at, :updated_at
        )
    )");
    
    query.bindValue(":title", fragment.title);
    query.bindValue(":content", fragment.content);
    query.bindValue(":terminal_type", fragment.terminalType);
    query.bindValue(":working_directory", fragment.workingDirectory);
    query.bindValue(":session_id", fragment.sessionId);
    query.bindValue(":command_history", fragment.commandHistory);
    query.bindValue(":source_type", fragment.sourceType);
    query.bindValue(":source_remark", fragment.sourceRemark);
    query.bindValue(":created_at", fragment.createdAt.toString(Qt::ISODate));
    query.bindValue(":updated_at", fragment.updatedAt.toString(Qt::ISODate));
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return -1;
    }
    
    qint64 id = query.lastInsertId().toLongLong();
    emit fragmentCreated(id);
    return id;
}

bool MemoryFragmentStore::updateFragment(const MemoryFragment& fragment) {
    if (!m_initialized || fragment.id <= 0) return false;
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        UPDATE memory_fragments SET
            title = :title,
            content = :content,
            terminal_type = :terminal_type,
            working_directory = :working_directory,
            session_id = :session_id,
            command_history = :command_history,
            source_type = :source_type,
            source_remark = :source_remark,
            updated_at = :updated_at
        WHERE id = :id
    )");
    
    query.bindValue(":id", fragment.id);
    query.bindValue(":title", fragment.title);
    query.bindValue(":content", fragment.content);
    query.bindValue(":terminal_type", fragment.terminalType);
    query.bindValue(":working_directory", fragment.workingDirectory);
    query.bindValue(":session_id", fragment.sessionId);
    query.bindValue(":command_history", fragment.commandHistory);
    query.bindValue(":source_type", fragment.sourceType);
    query.bindValue(":source_remark", fragment.sourceRemark);
    query.bindValue(":updated_at", QDateTime::currentDateTime().toString(Qt::ISODate));
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    
    emit fragmentUpdated(fragment.id);
    return true;
}

bool MemoryFragmentStore::deleteFragment(qint64 id) {
    if (!m_initialized || id <= 0) return false;
    
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM memory_fragments WHERE id = :id");
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    
    emit fragmentDeleted(id);
    return true;
}

QList<MemoryFragment> MemoryFragmentStore::loadAll() {
    QList<MemoryFragment> fragments;
    
    if (!m_initialized) return fragments;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM memory_fragments ORDER BY created_at DESC");
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return fragments;
    }
    
    while (query.next()) {
        MemoryFragment fragment;
        fragment.id = query.value("id").toLongLong();
        fragment.title = query.value("title").toString();
        fragment.content = query.value("content").toString();
        fragment.terminalType = query.value("terminal_type").toString();
        fragment.workingDirectory = query.value("working_directory").toString();
        fragment.sessionId = query.value("session_id").toString();
        fragment.commandHistory = query.value("command_history").toString();
        fragment.sourceType = query.value("source_type").toString();
        fragment.sourceRemark = query.value("source_remark").toString();
        fragment.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
        fragment.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
        
        fragments.append(fragment);
    }
    
    return fragments;
}

QList<MemoryFragment> MemoryFragmentStore::search(const QString& keyword) {
    QList<MemoryFragment> fragments;
    
    if (!m_initialized || keyword.isEmpty()) return loadAll();
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT * FROM memory_fragments 
        WHERE title LIKE :keyword OR content LIKE :keyword OR working_directory LIKE :keyword
        ORDER BY created_at DESC
    )");
    
    QString likeKeyword = "%" + keyword + "%";
    query.bindValue(":keyword", likeKeyword);
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return fragments;
    }
    
    while (query.next()) {
        MemoryFragment fragment;
        fragment.id = query.value("id").toLongLong();
        fragment.title = query.value("title").toString();
        fragment.content = query.value("content").toString();
        fragment.terminalType = query.value("terminal_type").toString();
        fragment.workingDirectory = query.value("working_directory").toString();
        fragment.sessionId = query.value("session_id").toString();
        fragment.commandHistory = query.value("command_history").toString();
        fragment.sourceType = query.value("source_type").toString();
        fragment.sourceRemark = query.value("source_remark").toString();
        fragment.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
        fragment.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
        
        fragments.append(fragment);
    }
    
    return fragments;
}

MemoryFragment MemoryFragmentStore::getFragment(qint64 id) {
    MemoryFragment fragment;
    
    if (!m_initialized || id <= 0) return fragment;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM memory_fragments WHERE id = :id");
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return fragment;
    }
    
    if (query.next()) {
        fragment.id = query.value("id").toLongLong();
        fragment.title = query.value("title").toString();
        fragment.content = query.value("content").toString();
        fragment.terminalType = query.value("terminal_type").toString();
        fragment.workingDirectory = query.value("working_directory").toString();
        fragment.sessionId = query.value("session_id").toString();
        fragment.commandHistory = query.value("command_history").toString();
        fragment.sourceType = query.value("source_type").toString();
        fragment.sourceRemark = query.value("source_remark").toString();
        fragment.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
        fragment.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
    }
    
    return fragment;
}

qint64 MemoryFragmentStore::count() const {
    if (!m_initialized) return 0;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM memory_fragments");
    
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    
    return 0;
}
