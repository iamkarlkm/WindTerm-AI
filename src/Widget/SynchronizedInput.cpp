#include "SynchronizedInput.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QDebug>

SynchronizedInputManager* SynchronizedInputManager::s_instance = nullptr;

SynchronizedInputManager::SynchronizedInputManager(QObject* parent)
    : QObject(parent) {
    
    m_groupsFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sync_groups.json";
    loadGroups();
}

SynchronizedInputManager* SynchronizedInputManager::instance() {
    if (!s_instance) {
        s_instance = new SynchronizedInputManager();
    }
    return s_instance;
}

QString SynchronizedInputManager::createGroup(const QString& name) {
    SyncGroup group;
    group.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    group.name = name.isEmpty() ? "Sync Group" : name;
    group.enabled = true;
    
    m_groups[group.id] = group;
    
    saveGroups();
    
    emit groupCreated(group.id);
    
    qDebug() << "[SynchronizedInput] Created group:" << group.id << group.name;
    
    return group.id;
}

void SynchronizedInputManager::deleteGroup(const QString& groupId) {
    if (!m_groups.contains(groupId)) return;
    
    m_groups.remove(groupId);
    
    saveGroups();
    
    emit groupDeleted(groupId);
    
    qDebug() << "[SynchronizedInput] Deleted group:" << groupId;
}

void SynchronizedInputManager::renameGroup(const QString& groupId, const QString& newName) {
    if (!m_groups.contains(groupId)) return;
    
    m_groups[groupId].name = newName;
    
    saveGroups();
}

void SynchronizedInputManager::addSessionToGroup(const QString& groupId, const QString& sessionId) {
    if (!m_groups.contains(groupId)) return;
    
    m_groups[groupId].sessionIds.insert(sessionId);
    
    // 如果没有 master，设置第一个会话为 master
    if (m_groups[groupId].masterSession.isEmpty()) {
        m_groups[groupId].masterSession = sessionId;
    }
    
    saveGroups();
    
    emit sessionAdded(groupId, sessionId);
    
    qDebug() << "[SynchronizedInput] Added session to group:" << groupId << sessionId;
}

void SynchronizedInputManager::removeSessionFromGroup(const QString& groupId, const QString& sessionId) {
    if (!m_groups.contains(groupId)) return;
    
    m_groups[groupId].sessionIds.remove(sessionId);
    
    // 如果移除的是 master，重新选择 master
    if (m_groups[groupId].masterSession == sessionId) {
        if (!m_groups[groupId].sessionIds.isEmpty()) {
            m_groups[groupId].masterSession = *m_groups[groupId].sessionIds.begin();
        } else {
            m_groups[groupId].masterSession.clear();
        }
    }
    
    saveGroups();
    
    emit sessionRemoved(groupId, sessionId);
    
    qDebug() << "[SynchronizedInput] Removed session from group:" << groupId << sessionId;
}

void SynchronizedInputManager::setMasterSession(const QString& groupId, const QString& sessionId) {
    if (!m_groups.contains(groupId)) return;
    if (!m_groups[groupId].sessionIds.contains(sessionId)) return;
    
    m_groups[groupId].masterSession = sessionId;
    
    saveGroups();
}

void SynchronizedInputManager::enableGroup(const QString& groupId) {
    if (!m_groups.contains(groupId)) return;
    
    m_groups[groupId].enabled = true;
    
    saveGroups();
    
    emit groupToggled(groupId, true);
}

void SynchronizedInputManager::disableGroup(const QString& groupId) {
    if (!m_groups.contains(groupId)) return;
    
    m_groups[groupId].enabled = false;
    
    saveGroups();
    
    emit groupToggled(groupId, false);
}

bool SynchronizedInputManager::isGroupEnabled(const QString& groupId) const {
    if (!m_groups.contains(groupId)) return false;
    return m_groups[groupId].enabled;
}

void SynchronizedInputManager::enableAllGroups() {
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        it->enabled = true;
    }
    saveGroups();
}

void SynchronizedInputManager::disableAllGroups() {
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        it->enabled = false;
    }
    saveGroups();
}

SyncGroup SynchronizedInputManager::getGroup(const QString& groupId) const {
    return m_groups.value(groupId);
}

QList<SyncGroup> SynchronizedInputManager::getAllGroups() const {
    return m_groups.values();
}

QSet<QString> SynchronizedInputManager::getSessionGroups(const QString& sessionId) const {
    QSet<QString> groups;
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        if (it->sessionIds.contains(sessionId)) {
            groups.insert(it.key());
        }
    }
    return groups;
}

QStringList SynchronizedInputManager::getGroupSessions(const QString& groupId) const {
    if (!m_groups.contains(groupId)) return QStringList();
    return QStringList(m_groups[groupId].sessionIds.values());
}

void SynchronizedInputManager::broadcastText(const QString& groupId, const QString& text) {
    if (!m_groups.contains(groupId)) return;
    if (!m_groups[groupId].enabled) return;
    
    const SyncGroup& group = m_groups[groupId];
    
    // 发送到组内所有会话（除了 master，因为 master 已经输入了）
    for (const QString& sessionId : group.sessionIds) {
        if (sessionId != group.masterSession) {
            emit textBroadcasted(groupId, sessionId, text);
        }
    }
}

void SynchronizedInputManager::broadcastKey(const QString& groupId, int key, Qt::KeyboardModifiers modifiers) {
    if (!m_groups.contains(groupId)) return;
    if (!m_groups[groupId].enabled) return;
    
    const SyncGroup& group = m_groups[groupId];
    
    // 发送到组内所有会话（除了 master）
    for (const QString& sessionId : group.sessionIds) {
        if (sessionId != group.masterSession) {
            emit keyBroadcasted(groupId, sessionId, key);
        }
    }
}

void SynchronizedInputManager::broadcastCommand(const QString& groupId, const QString& command) {
    // 发送完整命令（包括回车）
    broadcastText(groupId, command + "\n");
}

void SynchronizedInputManager::saveGroups() {
    QJsonArray groupsJson;
    
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        QJsonObject json;
        json["id"] = it.key();
        json["name"] = it->name;
        json["enabled"] = it->enabled;
        json["masterSession"] = it->masterSession;
        
        QJsonArray sessionsJson;
        for (const QString& sessionId : it->sessionIds) {
            sessionsJson.append(sessionId);
        }
        json["sessions"] = sessionsJson;
        
        groupsJson.append(json);
    }
    
    QFile file(m_groupsFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(groupsJson).toJson(QJsonDocument::Indented));
    }
}

void SynchronizedInputManager::loadGroups() {
    QFile file(m_groupsFile);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonArray groupsJson = QJsonDocument::fromJson(file.readAll()).array();
    
    for (const QJsonValue& value : groupsJson) {
        QJsonObject json = value.toObject();
        
        SyncGroup group;
        group.id = json["id"].toString();
        group.name = json["name"].toString();
        group.enabled = json["enabled"].toBool(true);
        group.masterSession = json["masterSession"].toString();
        
        QJsonArray sessionsJson = json["sessions"].toArray();
        for (const QJsonValue& sessionValue : sessionsJson) {
            group.sessionIds.insert(sessionValue.toString());
        }
        
        m_groups[group.id] = group;
    }
    
    qDebug() << "[SynchronizedInput] Loaded" << m_groups.size() << "sync groups";
}

#include "SynchronizedInput.moc"
