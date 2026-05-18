#include "SessionManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QDebug>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonArray>

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
            session_name TEXT,
            connection_type TEXT DEFAULT 'ssh',
            host TEXT,
            port INTEGER DEFAULT 22,
            username TEXT,
            working_directory TEXT,
            last_command TEXT,
            last_successful_command TEXT,
            shell_type TEXT,
            process_id INTEGER,
            auto_restore_command TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_active_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            is_active INTEGER DEFAULT 1
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
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS command_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id TEXT NOT NULL,
            command TEXT NOT NULL,
            working_directory TEXT,
            exit_code INTEGER DEFAULT 0,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (session_id) REFERENCES sessions(session_id) ON DELETE CASCADE
        )
    )");
    
    query.exec("CREATE INDEX IF NOT EXISTS idx_session_id ON sessions(session_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_last_active ON sessions(last_active_at)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_is_active ON sessions(is_active)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_cmd_session ON command_history(session_id)");
}

QString SessionManager::generateSessionId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString SessionManager::connectionTypeToString(ConnectionType type) {
    switch(type) {
        case ConnectionType::SSH: return "ssh";
        case ConnectionType::Telnet: return "telnet";
        case ConnectionType::Rlogin: return "rlogin";
        case ConnectionType::Serial: return "serial";
        case ConnectionType::TCP: return "tcp";
        case ConnectionType::UDP: return "udp";
        case ConnectionType::LocalShell: return "local";
        default: return "ssh";
    }
}

ConnectionType SessionManager::stringToConnectionType(const QString& str) {
    if (str == "telnet") return ConnectionType::Telnet;
    if (str == "rlogin") return ConnectionType::Rlogin;
    if (str == "serial") return ConnectionType::Serial;
    if (str == "tcp") return ConnectionType::TCP;
    if (str == "udp") return ConnectionType::UDP;
    if (str == "local") return ConnectionType::LocalShell;
    return ConnectionType::SSH;
}

QString SessionManager::createSession(const QString& name, ConnectionType type, const QString& host, int port, const QString& username) {
    if (!m_db.isOpen()) return QString();
    QString sessionId = generateSessionId();
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO sessions (session_id, session_name, connection_type, host, port, username, created_at, last_active_at)
        VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
    )");
    query.addBindValue(sessionId);
    query.addBindValue(name);
    query.addBindValue(connectionTypeToString(type));
    query.addBindValue(host);
    query.addBindValue(port);
    query.addBindValue(username);
    query.exec();
    return sessionId;
}

void SessionManager::updateSessionState(const QString& sessionId, const QString& workingDir, const QString& lastCmd) {
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
    query.addBindValue(lastCmd);
    query.addBindValue(sessionId);
    query.exec();
}

void SessionManager::markCommandSuccess(const QString& sessionId, const QString& command) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE sessions SET 
            last_successful_command = ?,
            auto_restore_command = ?,
            last_active_at = CURRENT_TIMESTAMP
        WHERE session_id = ?
    )");
    query.addBindValue(command);
    query.addBindValue(command);
    query.addBindValue(sessionId);
    query.exec();
}

void SessionManager::appendCommand(const QString& sessionId, const QString& command) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO command_history (session_id, command, working_directory, timestamp) VALUES (?, ?, ?, CURRENT_TIMESTAMP)");
    query.addBindValue(sessionId);
    query.addBindValue(command);
    
    QSqlQuery wdQuery(m_db);
    wdQuery.prepare("SELECT working_directory FROM sessions WHERE session_id = ?");
    wdQuery.addBindValue(sessionId);
    if (wdQuery.exec() && wdQuery.next()) {
        query.addBindValue(wdQuery.value(0).toString());
    } else {
        query.addBindValue(QString());
    }
    query.exec();
    
    query.prepare("UPDATE sessions SET command_history = (SELECT command_history FROM sessions WHERE session_id = ?) || ? || '\n' WHERE session_id = ?");
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

void SessionManager::setShellType(const QString& sessionId, const QString& shell) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("UPDATE sessions SET shell_type = ? WHERE session_id = ?");
    query.addBindValue(shell);
    query.addBindValue(sessionId);
    query.exec();
}

void SessionManager::closeSession(const QString& sessionId) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("UPDATE sessions SET is_active = 0, last_active_at = CURRENT_TIMESTAMP WHERE session_id = ?");
    query.addBindValue(sessionId);
    query.exec();
}

QList<SessionState> SessionManager::getAllSessions() {
    QList<SessionState> sessions;
    if (!m_db.isOpen()) return sessions;
    QSqlQuery query(m_db);
    query.exec("SELECT * FROM sessions ORDER BY last_active_at DESC");
    while (query.next()) {
        SessionState info;
        info.id = query.value("id").toInt();
        info.sessionId = query.value("session_id").toString();
        info.sessionName = query.value("session_name").toString();
        info.connectionType = stringToConnectionType(query.value("connection_type").toString());
        info.host = query.value("host").toString();
        info.port = query.value("port").toInt();
        info.username = query.value("username").toString();
        info.workingDirectory = query.value("working_directory").toString();
        info.lastCommand = query.value("last_command").toString();
        info.lastSuccessfulCommand = query.value("last_successful_command").toString();
        info.shellType = query.value("shell_type").toString();
        info.processId = query.value("process_id").toLongLong();
        info.createdAt = query.value("created_at").toDateTime();
        info.lastActiveAt = query.value("last_active_at").toDateTime();
        info.autoRestoreCommand = query.value("auto_restore_command").toString();
        
        QSqlQuery envQuery(m_db);
        envQuery.prepare("SELECT var_name, var_value FROM session_environment WHERE session_id = ?");
        envQuery.addBindValue(info.sessionId);
        envQuery.exec();
        while (envQuery.next()) {
            info.environment[envQuery.value("var_name").toString()] = envQuery.value("var_value").toString();
        }
        
        QSqlQuery cmdQuery(m_db);
        cmdQuery.prepare("SELECT command FROM command_history WHERE session_id = ? ORDER BY timestamp DESC LIMIT 50");
        cmdQuery.addBindValue(info.sessionId);
        cmdQuery.exec();
        while (cmdQuery.next()) {
            info.commandHistory << cmdQuery.value(0).toString();
        }
        sessions << info;
    }
    return sessions;
}

SessionState SessionManager::getSession(const QString& sessionId) {
    SessionState info;
    if (!m_db.isOpen()) return info;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM sessions WHERE session_id = ?");
    query.addBindValue(sessionId);
    if (query.exec() && query.next()) {
        info.id = query.value("id").toInt();
        info.sessionId = query.value("session_id").toString();
        info.sessionName = query.value("session_name").toString();
        info.connectionType = stringToConnectionType(query.value("connection_type").toString());
        info.host = query.value("host").toString();
        info.port = query.value("port").toInt();
        info.username = query.value("username").toString();
        info.workingDirectory = query.value("working_directory").toString();
        info.lastCommand = query.value("last_command").toString();
        info.lastSuccessfulCommand = query.value("last_successful_command").toString();
        info.shellType = query.value("shell_type").toString();
        info.createdAt = query.value("created_at").toDateTime();
        info.lastActiveAt = query.value("last_active_at").toDateTime();
        info.autoRestoreCommand = query.value("auto_restore_command").toString();
    }
    return info;
}

QList<SessionState> SessionManager::getRecentSessions(int limit) {
    QList<SessionState> sessions;
    if (!m_db.isOpen()) return sessions;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM sessions WHERE is_active = 1 ORDER BY last_active_at DESC LIMIT ?");
    query.addBindValue(limit);
    query.exec();
    while (query.next()) {
        SessionState info;
        info.id = query.value("id").toInt();
        info.sessionId = query.value("session_id").toString();
        info.sessionName = query.value("session_name").toString();
        info.host = query.value("host").toString();
        info.workingDirectory = query.value("working_directory").toString();
        info.lastSuccessfulCommand = query.value("last_successful_command").toString();
        info.autoRestoreCommand = query.value("auto_restore_command").toString();
        info.lastActiveAt = query.value("last_active_at").toDateTime();
        sessions << info;
    }
    return sessions;
}

QList<SessionState> SessionManager::getActiveSessions() {
    return getRecentSessions(100);
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
    query.prepare("DELETE FROM sessions WHERE last_active_at < datetime('now', ?) AND is_active = 0");
    query.addBindValue(QString("-%1 days").arg(days));
    query.exec();
}

QJsonObject SessionManager::getRestoreData(const QString& sessionId) {
    SessionState state = getSession(sessionId);
    QJsonObject obj;
    obj["session_id"] = state.sessionId;
    obj["session_name"] = state.sessionName;
    obj["connection_type"] = connectionTypeToString(state.connectionType);
    obj["host"] = state.host;
    obj["port"] = state.port;
    obj["username"] = state.username;
    obj["working_directory"] = state.workingDirectory;
    obj["last_successful_command"] = state.lastSuccessfulCommand;
    obj["auto_restore_command"] = state.autoRestoreCommand;
    obj["shell_type"] = state.shellType;
    
    QJsonObject envObj;
    for (auto it = state.environment.begin(); it != state.environment.end(); ++it) {
        envObj[it.key()] = it.value();
    }
    obj["environment"] = envObj;
    
    QJsonArray cmdArray;
    for (const auto& cmd : state.commandHistory) {
        cmdArray.append(cmd);
    }
    obj["command_history"] = cmdArray;
    
    return obj;
}

QString SessionManager::generateRestoreScript(const QString& sessionId) {
    SessionState state = getSession(sessionId);
    if (state.sessionId.isEmpty()) return QString();
    
    QString script;
    if (!state.workingDirectory.isEmpty()) {
        script += QString("cd \"%1\"\n").arg(state.workingDirectory);
    }
    
    for (auto it = state.environment.begin(); it != state.environment.end(); ++it) {
        if (it.key() == "PATH" || it.key() == "LD_LIBRARY_PATH") {
            script += QString("export %1=\"%2\"\n").arg(it.key()).arg(it.value());
        }
    }
    
    if (!state.autoRestoreCommand.isEmpty()) {
        script += state.autoRestoreCommand + "\n";
    }
    
    return script;
}
