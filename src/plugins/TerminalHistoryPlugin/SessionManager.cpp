#include "SessionManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QDebug>
#include <QProcessEnvironment>

SessionManager::SessionManager(QObject* parent) : QObject(parent) {}

bool SessionManager::initialize() {
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/extensions/session_manager.db";
    QDir(QFileInfo(dbPath).absolutePath()).mkpath(".");
    m_db = QSqlDatabase::addDatabase("QSQLITE", "session_manager");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) { qCritical() << "Session DB open failed:" << m_db.lastError(); return false; }
    createTables();
    return true;
}

void SessionManager::shutdown() { if (m_db.isOpen()) m_db.close(); }

void SessionManager::createTables() {
    QSqlQuery query(m_db);
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id TEXT UNIQUE NOT NULL,
            host TEXT,
            port INTEGER,
            protocol TEXT,
            working_directory TEXT,
            last_command TEXT,
            last_successful_command TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_active_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            process_id INTEGER
        )
    )");
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS session_environment (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id TEXT NOT NULL,
            var_name TEXT NOT NULL,
            var_value TEXT,
            FOREIGN KEY (session_id) REFERENCES sessions(session_id) ON DELETE CASCADE
        )
    )");
    query.exec("CREATE INDEX IF NOT EXISTS idx_session_id ON sessions(session_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_last_active ON sessions(last_active_at)");
}

QString SessionManager::generateSessionId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void SessionManager::saveSession(const QString& sessionId, const QString& host, int port, const QString& protocol) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT OR REPLACE INTO sessions (session_id, host, port, protocol, created_at, last_active_at)
        VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
    )");
    query.addBindValue(sessionId.isEmpty() ? generateSessionId() : sessionId);
    query.addBindValue(host);
    query.addBindValue(port);
    query.addBindValue(protocol);
    query.exec();
}

void SessionManager::updateSession(const QString& sessionId, const QString& workingDir, const QString& command) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE sessions SET 
            working_directory = ?,
            last_command = ?,
            last_active_at = CURRENT_TIMESTAMP
        WHERE session_id = ?
    )");
    query.addBindValue(workingDir);
    query.addBindValue(command);
    query.addBindValue(sessionId);
    query.exec();
}

void SessionManager::markCommandSuccess(const QString& sessionId, const QString& command) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("UPDATE sessions SET last_successful_command = ?, last_active_at = CURRENT_TIMESTAMP WHERE session_id = ?");
    query.addBindValue(command);
    query.addBindValue(sessionId);
    query.exec();
}

void SessionManager::setEnvironment(const QString& sessionId, const QMap<QString, QString>& env) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM session_environment WHERE session_id = ?");
    query.addBindValue(sessionId);
    query.exec();
    
    query.prepare("INSERT INTO session_environment (session_id, var_name, var_value) VALUES (?, ?, ?)");
    for (auto it = env.begin(); it != env.end(); ++it) {
        query.addBindValue(sessionId);
        query.addBindValue(it.key());
        query.addBindValue(it.value());
        query.exec();
    }
}

void SessionManager::closeSession(const QString& sessionId) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("UPDATE sessions SET last_active_at = CURRENT_TIMESTAMP WHERE session_id = ?");
    query.addBindValue(sessionId);
    query.exec();
}

QList<SessionInfo> SessionManager::getAllSessions() {
    QList<SessionInfo> sessions;
    if (!m_db.isOpen()) return sessions;
    QSqlQuery query(m_db);
    query.exec("SELECT * FROM sessions ORDER BY last_active_at DESC");
    while (query.next()) {
        SessionInfo info;
        info.id = query.value("id").toInt();
        info.sessionId = query.value("session_id").toString();
        info.host = query.value("host").toString();
        info.port = query.value("port").toInt();
        info.protocol = query.value("protocol").toString();
        info.workingDirectory = query.value("working_directory").toString();
        info.lastCommand = query.value("last_command").toString();
        info.lastSuccessfulCommand = query.value("last_successful_command").toString();
        info.createdAt = query.value("created_at").toDateTime();
        info.lastActiveAt = query.value("last_active_at").toDateTime();
        info.processId = query.value("process_id").toLongLong();
        
        QSqlQuery envQuery(m_db);
        envQuery.prepare("SELECT var_name, var_value FROM session_environment WHERE session_id = ?");
        envQuery.addBindValue(info.sessionId);
        envQuery.exec();
        while (envQuery.next()) {
            info.environment[envQuery.value("var_name").toString()] = envQuery.value("var_value").toString();
        }
        sessions << info;
    }
    return sessions;
}

SessionInfo SessionManager::getSession(const QString& sessionId) {
    SessionInfo info;
    if (!m_db.isOpen()) return info;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM sessions WHERE session_id = ?");
    query.addBindValue(sessionId);
    if (query.exec() && query.next()) {
        info.id = query.value("id").toInt();
        info.sessionId = query.value("session_id").toString();
        info.host = query.value("host").toString();
        info.port = query.value("port").toInt();
        info.protocol = query.value("protocol").toString();
        info.workingDirectory = query.value("working_directory").toString();
        info.lastCommand = query.value("last_command").toString();
        info.lastSuccessfulCommand = query.value("last_successful_command").toString();
        info.createdAt = query.value("created_at").toDateTime();
        info.lastActiveAt = query.value("last_active_at").toDateTime();
        info.processId = query.value("process_id").toLongLong();
    }
    return info;
}

QList<SessionInfo> SessionManager::getRecentSessions(int limit) {
    QList<SessionInfo> sessions;
    if (!m_db.isOpen()) return sessions;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM sessions ORDER BY last_active_at DESC LIMIT ?");
    query.addBindValue(limit);
    query.exec();
    while (query.next()) {
        SessionInfo info;
        info.id = query.value("id").toInt();
        info.sessionId = query.value("session_id").toString();
        info.host = query.value("host").toString();
        info.workingDirectory = query.value("working_directory").toString();
        info.lastCommand = query.value("last_command").toString();
        info.lastSuccessfulCommand = query.value("last_successful_command").toString();
        info.lastActiveAt = query.value("last_active_at").toDateTime();
        sessions << info;
    }
    return sessions;
}

void SessionManager::deleteSession(const QString& sessionId) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM sessions WHERE session_id = ?");
    query.addBindValue(sessionId);
    query.exec();
}

void SessionManager::cleanupOldSessions(int days) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM sessions WHERE last_active_at < datetime('now', ?)");
    query.addBindValue(QString("-%1 days").arg(days));
    query.exec();
}
