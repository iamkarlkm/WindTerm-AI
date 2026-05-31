#include "SessionManager.h"
#include <QDir>
#include <QJsonObject>
#include <QFile>
#include <QJsonArray>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QDebug>

SessionManager* SessionManager::s_instance = nullptr;

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
    , m_autoSaveEnabled(true)
    , m_autoSaveInterval(300)
    , m_autoSaveTimer(new QTimer(this)) {
    
    m_sessionDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/sessions";
    QDir().mkpath(m_sessionDir);
    
    connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() {
        if (m_autoSaveEnabled && !m_currentSessionId.isEmpty()) {
            saveSession(m_currentSessionId);
            emit autoSaveTriggered();
        }
    });
    m_autoSaveTimer->start(m_autoSaveInterval * 1000);
}

SessionManager* SessionManager::instance() {
    if (!s_instance) {
        s_instance = new SessionManager();
    }
    return s_instance;
}

QString SessionManager::generateSessionId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString SessionManager::createSession(const QString& name, const QString& type) {
    SessionData session;
    session.id = generateSessionId();
    session.name = name;
    session.type = type;
    session.createdAt = QDateTime::currentDateTime();
    session.lastActiveAt = session.createdAt;
    session.isActive = true;
    
    m_sessions[session.id] = session;
    m_currentSessionId = session.id;
    
    saveToDisk(session.id);
    emit sessionCreated(session.id);
    
    qDebug() << "[SessionManager] Created session:" << session.id;
    return session.id;
}

void SessionManager::saveSession(const QString& sessionId) {
    if (!m_sessions.contains(sessionId)) return;
    
    m_sessions[sessionId].lastActiveAt = QDateTime::currentDateTime();
    saveToDisk(sessionId);
}

void SessionManager::loadSession(const QString& sessionId) {
    if (!m_sessions.contains(sessionId)) {
        loadFromDisk(sessionId);
    }
    
    if (m_sessions.contains(sessionId)) {
        switchToSession(sessionId);
        emit sessionLoaded(sessionId);
    }
}

void SessionManager::closeSession(const QString& sessionId) {
    if (!m_sessions.contains(sessionId)) return;
    
    saveSession(sessionId);
    m_sessions[sessionId].isActive = false;
    
    if (m_currentSessionId == sessionId) {
        m_currentSessionId.clear();
    }
    
    emit sessionClosed(sessionId);
    qDebug() << "[SessionManager] Closed session:" << sessionId;
}

void SessionManager::switchToSession(const QString& sessionId) {
    if (!m_sessions.contains(sessionId)) return;
    
    m_currentSessionId = sessionId;
    m_sessions[sessionId].isActive = true;
    m_sessions[sessionId].lastActiveAt = QDateTime::currentDateTime();
    
    emit sessionSwitched(sessionId);
}

SessionData SessionManager::getSession(const QString& sessionId) const {
    return m_sessions.value(sessionId);
}

QStringList SessionManager::listSessions() const {
    return m_sessions.keys();
}

QStringList SessionManager::listActiveSessions() const {
    QStringList active;
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.value().isActive) {
            active.append(it.key());
        }
    }
    return active;
}

void SessionManager::setAutoSaveEnabled(bool enabled) {
    m_autoSaveEnabled = enabled;
    if (enabled) {
        m_autoSaveTimer->start();
    } else {
        m_autoSaveTimer->stop();
    }
}

void SessionManager::setAutoSaveInterval(int seconds) {
    m_autoSaveInterval = seconds;
    m_autoSaveTimer->setInterval(seconds * 1000);
}

void SessionManager::exportSessions(const QString& filePath) {
    QJsonArray array;
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        array.append(it.value().toJson());
    }
    
    QJsonDocument doc(array);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

void SessionManager::importSessions(const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray array = doc.array();
        
        for (const QJsonValue& value : array) {
            SessionData session = SessionData::fromJson(value.toObject());
            m_sessions[session.id] = session;
        }
    }
}

void SessionManager::cleanupOldSessions(int daysToKeep) {
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-daysToKeep);
    
    auto it = m_sessions.begin();
    while (it != m_sessions.end()) {
        if (it.value().lastActiveAt < cutoff && !it.value().isActive) {
            it = m_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

void SessionManager::clearAllSessions() {
    m_sessions.clear();
    m_currentSessionId.clear();
}

void SessionManager::saveToDisk(const QString& sessionId) {
    if (!m_sessions.contains(sessionId)) return;
    
    QString filePath = m_sessionDir + "/" + sessionId + ".json";
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_sessions[sessionId].toJson()).toJson());
    }
}

void SessionManager::loadFromDisk(const QString& sessionId) {
    QString filePath = m_sessionDir + "/" + sessionId + ".json";
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        SessionData session = SessionData::fromJson(doc.object());
        m_sessions[sessionId] = session;
    }
}

QJsonObject SessionData::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["type"] = type;
    json["host"] = host;
    json["port"] = port;
    json["username"] = username;
    json["workingDirectory"] = workingDirectory;
    json["shell"] = shell;
    json["lastCommand"] = lastCommand;
    json["commandHistory"] = QJsonArray::fromStringList(commandHistory);
    json["bufferRows"] = bufferRows;
    json["bufferCols"] = bufferCols;
    json["createdAt"] = createdAt.toString(Qt::ISODate);
    json["lastActiveAt"] = lastActiveAt.toString(Qt::ISODate);
    json["isActive"] = isActive;
    return json;
}

SessionData SessionData::fromJson(const QJsonObject& json) {
    SessionData session;
    session.id = json.value("id").toString();
    session.name = json.value("name").toString();
    session.type = json.value("type").toString();
    session.host = json.value("host").toString();
    session.port = json.value("port").toInt(22);
    session.username = json.value("username").toString();
    session.workingDirectory = json.value("workingDirectory").toString();
    session.shell = json.value("shell").toString();
    session.lastCommand = json.value("lastCommand").toString();
    {
        QJsonArray arr = json.value("commandHistory").toArray();
        for (const QJsonValue& v : arr) session.commandHistory << v.toString();
    }
    session.bufferRows = json.value("bufferRows").toInt(24);
    session.bufferCols = json.value("bufferCols").toInt(80);
    session.createdAt = QDateTime::fromString(json.value("createdAt").toString(), Qt::ISODate);
    session.lastActiveAt = QDateTime::fromString(json.value("lastActiveAt").toString(), Qt::ISODate);
    session.isActive = json.value("isActive").toBool(false);
    return session;
}
