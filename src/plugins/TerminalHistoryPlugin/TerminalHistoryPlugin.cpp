#include "TerminalHistoryPlugin.h"
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QHostInfo>
#include <QtDebug>
#include <QFile>
#include <QProcess>
#include <QClipboard>
#include <QApplication>

TerminalHistoryPlugin::TerminalHistoryPlugin(QObject* parent) 
    : TerminalEventHook(parent), m_currentIndex(-1) {}

TerminalHistoryPlugin::~TerminalHistoryPlugin() { shutdown(); }

bool TerminalHistoryPlugin::initialize() {
    initDatabase();
    if (!m_sessionManager.initialize()) return false;
    return m_db.isValid();
}

void TerminalHistoryPlugin::shutdown() {
    if (m_db.isOpen()) m_db.close();
    m_sessionManager.shutdown();
}

void TerminalHistoryPlugin::initDatabase() {
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/extensions/command_history.db";
    QDir(QFileInfo(dbPath).absolutePath()).mkpath(".");
    m_db = QSqlDatabase::addDatabase("QSQLITE", "terminal_history");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) { qCritical() << "DB open failed:" << m_db.lastError(); return; }
    QSqlQuery query(m_db);
    query.exec("CREATE TABLE IF NOT EXISTS command_history (id INTEGER PRIMARY KEY AUTOINCREMENT, command TEXT NOT NULL, working_directory TEXT, hostname TEXT, shell_type TEXT, session_id TEXT, exit_code INTEGER, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_working_dir ON command_history(working_directory)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_session_id ON command_history(session_id)");
}

bool TerminalHistoryPlugin::interceptKeyEvent(int key, int modifiers, const QString& text) {
    Q_UNUSED(text);
    // Ctrl+Shift+R: 恢复上次会话命令
    if (modifiers == (Qt::ControlModifier | Qt::ShiftModifier) && key == Qt::Key_R) {
        restoreLastSession();
        return true;
    }
    // Ctrl+Shift+L: 填充最后一次成功命令
    if (modifiers == (Qt::ControlModifier | Qt::ShiftModifier) && key == Qt::Key_L) {
        fillLastCommand();
        return true;
    }
    if (modifiers != Qt::NoModifier) return false;
    if (key == Qt::Key_Up) {
        QString cmd = queryHistoryByOffset(1);
        if (!cmd.isEmpty()) { emit commandReceived(cmd); return true; }
    } else if (key == Qt::Key_Down) {
        QString cmd = queryHistoryByOffset(-1);
        if (!cmd.isEmpty()) { emit commandReceived(cmd); return true; }
    }
    return false;
}

void TerminalHistoryPlugin::onCommandExecuted(const QString& command) {
    if (!command.trimmed().isEmpty()) {
        saveCommand(command);
        m_sessionManager.appendCommand(m_currentSessionId, command);
        if (!m_currentSessionId.isEmpty()) {
            m_sessionManager.updateSessionState(m_currentSessionId, m_currentWorkingDir, command);
        }
    }
}

void TerminalHistoryPlugin::onWorkingDirectoryChanged(const QString& path) {
    m_currentWorkingDir = path;
    if (!m_currentSessionId.isEmpty()) {
        m_sessionManager.updateSessionState(m_currentSessionId, path, QString());
    }
}

QString TerminalHistoryPlugin::getCommandHistory(int offset) {
    return queryHistoryByOffset(offset);
}

void TerminalHistoryPlugin::onSessionStart(const QString& sessionId, const QString& host, int port, const QString& protocol) {
    m_currentSessionId = sessionId;
    m_sessionManager.createSession(sessionId, ConnectionType::SSH, host, port, QString());
    captureEnvironment(sessionId);
}

void TerminalHistoryPlugin::onSessionEnd(const QString& sessionId) {
    m_sessionManager.closeSession(sessionId);
    if (m_currentSessionId == sessionId) m_currentSessionId.clear();
}

void TerminalHistoryPlugin::captureEnvironment(const QString& sessionId) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QMap<QString, QString> envMap;
    for (const auto& key : env.keys()) {
        if (key == "PATH" || key == "HOME" || key == "USER" || key == "SHELL" || 
            key == "PWD" || key == "OLDPWD" || key.startsWith("LC_") || key == "LANG") {
            envMap[key] = env.value(key);
        }
    }
    m_sessionManager.setEnvironment(sessionId, envMap);
    m_lastEnvironment = env;
}

void TerminalHistoryPlugin::sendTextToInput(const QString& text) {
    injectTextToTerminal(text);
}

void TerminalHistoryPlugin::clearInput() {
    injectTextToTerminal("\x08"); // Backspace 清空
}

void TerminalHistoryPlugin::injectTextToTerminal(const QString& text) {
    m_currentInput = text;
    emit textInputReceived(text);
    qDebug() << "[TerminalHistoryPlugin] Inject text:" << text;
}

void TerminalHistoryPlugin::restoreLastSession() {
    QList<SessionState> sessions = m_sessionManager.getRecentSessions(1);
    if (sessions.isEmpty()) {
        qDebug() << "[TerminalHistoryPlugin] No sessions to restore";
        return;
    }
    
    SessionState lastSession = sessions.first();
    QString restoreText;
    
    // 构建恢复命令
    if (!lastSession.workingDirectory.isEmpty()) {
        restoreText = QString("cd \"%1\"").arg(lastSession.workingDirectory);
    }
    
    if (!lastSession.lastSuccessfulCommand.isEmpty()) {
        if (!restoreText.isEmpty()) restoreText += " && ";
        restoreText += lastSession.lastSuccessfulCommand;
    }
    
    if (restoreText.isEmpty()) {
        qDebug() << "[TerminalHistoryPlugin] Nothing to restore";
        return;
    }
    
    // 发送到终端输入行（不执行）
    injectTextToTerminal(restoreText);
    qDebug() << "[TerminalHistoryPlugin] Restored session:" << lastSession.sessionId << "Command:" << restoreText;
}

void TerminalHistoryPlugin::restoreSessionById(const QString& sessionId) {
    SessionState state = m_sessionManager.getSession(sessionId);
    if (state.sessionId.isEmpty()) {
        qDebug() << "[TerminalHistoryPlugin] Session not found:" << sessionId;
        return;
    }
    
    QString restoreText;
    if (!state.workingDirectory.isEmpty()) {
        restoreText = QString("cd \"%1\"").arg(state.workingDirectory);
    }
    
    if (!state.lastSuccessfulCommand.isEmpty()) {
        if (!restoreText.isEmpty()) restoreText += " && ";
        restoreText += state.lastSuccessfulCommand;
    }
    
    if (restoreText.isEmpty()) return;
    
    injectTextToTerminal(restoreText);
    qDebug() << "[TerminalHistoryPlugin] Restored session by ID:" << sessionId;
}

void TerminalHistoryPlugin::fillLastCommand() {
    if (m_currentSessionId.isEmpty()) {
        qDebug() << "[TerminalHistoryPlugin] No active session";
        return;
    }
    
    QString cmd = m_sessionManager.getSession(m_currentSessionId).lastSuccessfulCommand;
    if (cmd.isEmpty()) {
        qDebug() << "[TerminalHistoryPlugin] No successful command in current session";
        return;
    }
    
    injectTextToTerminal(cmd);
    qDebug() << "[TerminalHistoryPlugin] Filled last command:" << cmd;
}

void TerminalHistoryPlugin::saveCommand(const QString& command) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO command_history (command, working_directory, hostname, session_id) VALUES (?, ?, ?, ?)");
    query.addBindValue(command);
    query.addBindValue(m_currentWorkingDir);
    query.addBindValue(QHostInfo::localHostName());
    query.addBindValue(m_currentSessionId);
    if (!query.exec()) qCritical() << "Failed to save:" << query.lastError();
}

QString TerminalHistoryPlugin::queryHistoryByOffset(int offset) {
    if (!m_db.isOpen()) return QString();
    QSqlQuery query(m_db);
    query.prepare("SELECT command FROM command_history WHERE working_directory = ? ORDER BY timestamp DESC LIMIT 1 OFFSET ?");
    query.addBindValue(m_currentWorkingDir);
    query.addBindValue(qAbs(offset));
    if (query.exec() && query.next()) return query.value(0).toString();
    return QString();
}

QString TerminalHistoryPlugin::getLastSuccessfulCommand(const QString& sessionId) {
    SessionInfo info = m_sessionManager.getSession(sessionId);
    return info.lastSuccessfulCommand;
}
