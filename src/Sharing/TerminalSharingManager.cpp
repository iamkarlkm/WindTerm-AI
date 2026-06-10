#include "TerminalSharingManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDebug>
#include <QTimer>

TerminalSharingManager* TerminalSharingManager::s_instance = nullptr;

TerminalSharingManager::TerminalSharingManager(QObject* parent) : QObject(parent) {
    // 定期清理过期会话
    QTimer* cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, &TerminalSharingManager::cleanupExpiredSessions);
    cleanupTimer->start(60000);  // 每分钟检查
}

TerminalSharingManager::~TerminalSharingManager() {
}

TerminalSharingManager* TerminalSharingManager::instance() {
    if (!s_instance) s_instance = new TerminalSharingManager();
    return s_instance;
}

QString TerminalSharingManager::createSharingSession(const QString& ownerId, const QString& name, bool readOnly) {
    QString sessionId = generateSessionId();
    
    SharingSession session;
    session.id = sessionId;
    session.ownerId = ownerId;
    session.name = name.isEmpty() ? "Session " + sessionId.left(8) : name;
    session.readOnly = readOnly;
    session.maxParticipants = m_defaultMaxParticipants;
    session.createdAt = QDateTime::currentDateTime();
    session.expiresAt = session.createdAt.addMinutes(m_sessionExpirationMinutes);
    session.active = true;
    
    m_sessions[sessionId] = session;
    
    // 添加所有者为第一个参与者
    Participant owner;
    owner.id = ownerId;
    owner.sessionId = sessionId;
    owner.name = "Owner";
    owner.isOwner = true;
    owner.canWrite = !readOnly;
    owner.joinedAt = QDateTime::currentDateTime();
    owner.role = "admin";
    
    m_sessionParticipants[sessionId].append(owner);
    m_participantSessions[ownerId] = sessionId;
    
    emit sessionCreated(sessionId);
    return sessionId;
}

bool TerminalSharingManager::joinSession(const QString& sessionId, const QString& participantId) {
    if (!validateSession(sessionId)) {
        return false;
    }
    
    SharingSession& session = m_sessions[sessionId];
    
    // 检查会话是否已满
    if (session.participants.size() >= session.maxParticipants) {
        qDebug() << "Session is full";
        return false;
    }
    
    // 检查参与者是否已加入
    if (session.participants.contains(participantId)) {
        return true;  // 已在会话中
    }
    
    // 添加参与者
    Participant participant;
    participant.id = participantId;
    participant.sessionId = sessionId;
    participant.name = "User " + participantId.left(6);
    participant.isOwner = false;
    participant.canWrite = !session.readOnly;
    participant.joinedAt = QDateTime::currentDateTime();
    participant.role = session.readOnly ? "viewer" : "editor";
    
    m_sessionParticipants[sessionId].append(participant);
    m_participantSessions[participantId] = sessionId;
    session.participants.insert(participantId);
    
    emit participantJoined(sessionId, participantId);
    return true;
}

bool TerminalSharingManager::leaveSession(const QString& sessionId, const QString& participantId) {
    if (!m_sessions.contains(sessionId)) {
        return false;
    }
    
    // 从参与者列表移除
    auto& participants = m_sessionParticipants[sessionId];
    participants.erase(std::remove_if(participants.begin(), participants.end(),
        [participantId](const Participant& p) { return p.id == participantId; }),
        participants.end());
    
    m_sessions[sessionId].participants.remove(participantId);
    m_participantSessions.remove(participantId);
    
    // 如果是所有者，销毁会话
    if (m_sessions[sessionId].ownerId == participantId) {
        destroySession(sessionId);
    } else {
        emit participantLeft(sessionId, participantId);
    }
    
    return true;
}

bool TerminalSharingManager::destroySession(const QString& sessionId) {
    if (!m_sessions.contains(sessionId)) {
        return false;
    }
    
    // 通知所有参与者
    for (const auto& participant : m_sessionParticipants[sessionId]) {
        m_participantSessions.remove(participant.id);
        emit participantLeft(sessionId, participant.id);
    }
    
    m_sessionParticipants.remove(sessionId);
    m_sessions.remove(sessionId);
    m_recordingSessions.remove(sessionId);
    
    emit sessionDestroyed(sessionId);
    return true;
}

SharingSession TerminalSharingManager::getSessionInfo(const QString& sessionId) const {
    return m_sessions.value(sessionId);
}

QList<SharingSession> TerminalSharingManager::getActiveSessions() const {
    QList<SharingSession> active;
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it->active) {
            active.append(it.value());
        }
    }
    return active;
}

QList<Participant> TerminalSharingManager::getParticipants(const QString& sessionId) const {
    return m_sessionParticipants.value(sessionId);
}

void TerminalSharingManager::setParticipantRole(const QString& sessionId, const QString& participantId, const QString& role) {
    if (!validateSession(sessionId)) return;
    
    for (auto& participant : m_sessionParticipants[sessionId]) {
        if (participant.id == participantId) {
            participant.role = role;
            participant.canWrite = (role == "editor" || role == "admin");
            emit roleChanged(sessionId, participantId, role);
            return;
        }
    }
}

void TerminalSharingManager::setSessionReadOnly(const QString& sessionId, bool readOnly) {
    if (!validateSession(sessionId)) return;
    
    m_sessions[sessionId].readOnly = readOnly;
    
    // 更新所有参与者的写权限
    for (auto& participant : m_sessionParticipants[sessionId]) {
        if (!participant.isOwner) {
            participant.canWrite = !readOnly && (participant.role == "editor" || participant.role == "admin");
        }
    }
}

void TerminalSharingManager::setMaxParticipants(const QString& sessionId, int max) {
    if (!validateSession(sessionId)) return;
    m_sessions[sessionId].maxParticipants = max;
}

void TerminalSharingManager::broadcastOutput(const QString& sessionId, const QByteArray& data) {
    if (!validateSession(sessionId)) return;
    
    emit outputBroadcasted(sessionId, data);
    
    // 如果正在录制，写入文件
    if (m_recordingSessions.contains(sessionId)) {
        QString path = getRecordingPath(sessionId);
        QFile file(path);
        if (file.open(QIODevice::Append)) {
            file.write(data);
            file.close();
        }
    }
}

void TerminalSharingManager::forwardInput(const QString& sessionId, const QString& fromParticipant, const QByteArray& data) {
    if (!validateSession(sessionId)) return;
    
    // 检查权限
    bool canWrite = false;
    for (const auto& p : m_sessionParticipants[sessionId]) {
        if (p.id == fromParticipant && p.canWrite) {
            canWrite = true;
            break;
        }
    }
    
    if (!canWrite) {
        qDebug() << "Participant" << fromParticipant << "cannot write to session" << sessionId;
        return;
    }
    
    emit inputReceived(sessionId, fromParticipant, data);
}

QString TerminalSharingManager::generateInviteLink(const QString& sessionId) {
    if (!validateSession(sessionId)) return QString();
    
    // 生成邀请码
    QString inviteCode = QCryptographicHash::hash(
        (sessionId + QDateTime::currentDateTime().toString()).toUtf8(),
        QCryptographicHash::Sha256
    ).toHex().left(16);
    
    // 返回邀请链接格式
    return "windterm://share/" + sessionId + "/" + inviteCode;
}

QString TerminalSharingManager::parseInviteLink(const QString& link) const {
    if (!link.startsWith("windterm://share/")) return QString();
    
    QStringList parts = link.mid(17).split("/");
    if (parts.size() >= 2) {
        return parts[0];  // sessionId
    }
    return QString();
}

bool TerminalSharingManager::revokeInvite(const QString& sessionId) {
    Q_UNUSED(sessionId)
    // TODO: 实现邀请撤销逻辑
    return true;
}

void TerminalSharingManager::startRecording(const QString& sessionId) {
    if (!validateSession(sessionId)) return;
    
    m_recordingSessions[sessionId] = true;
    
    // 创建录制目录
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/recordings";
    QDir().mkpath(dir);
    
    emit recordingStarted(sessionId);
}

void TerminalSharingManager::stopRecording(const QString& sessionId) {
    if (!m_recordingSessions.contains(sessionId)) return;
    
    m_recordingSessions.remove(sessionId);
    emit recordingStopped(sessionId);
}

QString TerminalSharingManager::getRecordingPath(const QString& sessionId) const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/recordings";
    return dir + "/session_" + sessionId + "_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".log";
}

int TerminalSharingManager::getActiveSessionCount() const {
    int count = 0;
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it->active) count++;
    }
    return count;
}

int TerminalSharingManager::getTotalParticipantCount() const {
    int count = 0;
    for (auto it = m_sessionParticipants.begin(); it != m_sessionParticipants.end(); ++it) {
        count += it.value().size();
    }
    return count;
}

QJsonObject TerminalSharingManager::getSessionStats(const QString& sessionId) const {
    if (!m_sessions.contains(sessionId)) return QJsonObject();
    
    QJsonObject stats;
    stats["sessionId"] = sessionId;
    stats["participantCount"] = m_sessionParticipants[sessionId].size();
    stats["maxParticipants"] = m_sessions[sessionId].maxParticipants;
    stats["readOnly"] = m_sessions[sessionId].readOnly;
    stats["recording"] = m_recordingSessions.contains(sessionId);
    stats["createdAt"] = m_sessions[sessionId].createdAt.toString(Qt::ISODate);
    stats["uptime"] = m_sessions[sessionId].createdAt.secsTo(QDateTime::currentDateTime());
    
    return stats;
}

void TerminalSharingManager::cleanupExpiredSessions() {
    QDateTime now = QDateTime::currentDateTime();
    
    QList<QString> expired;
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it->expiresAt < now) {
            expired.append(it.key());
        }
    }
    
    for (const QString& sessionId : expired) {
        emit sessionExpired(sessionId);
        destroySession(sessionId);
    }
}

QString TerminalSharingManager::generateSessionId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool TerminalSharingManager::validateSession(const QString& sessionId) const {
    if (!m_sessions.contains(sessionId)) {
        qDebug() << "Session not found:" << sessionId;
        return false;
    }
    
    if (!m_sessions[sessionId].active) {
        qDebug() << "Session not active:" << sessionId;
        return false;
    }
    
    if (m_sessions[sessionId].expiresAt < QDateTime::currentDateTime()) {
        qDebug() << "Session expired:" << sessionId;
        return false;
    }
    
    return true;
}

#include "TerminalSharingManager.moc"
