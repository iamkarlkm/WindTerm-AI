#include "WorkspaceManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QStandardPaths>
#include <QUuid>
#include <QDebug>

WorkspaceManager* WorkspaceManager::s_instance = nullptr;

WorkspaceManager::WorkspaceManager(QObject* parent)
    : QObject(parent)
    , m_autoSaveEnabled(true) {
    
    // 设置工作区目录
    m_workspaceDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/workspaces";
    ensureWorkspaceDir();
}

WorkspaceManager* WorkspaceManager::instance() {
    if (!s_instance) {
        s_instance = new WorkspaceManager();
    }
    return s_instance;
}

void WorkspaceManager::ensureWorkspaceDir() {
    QDir dir;
    if (!dir.exists(m_workspaceDir)) {
        dir.mkpath(m_workspaceDir);
    }
}

QString WorkspaceManager::createWorkspace(const QString& name, const QString& description) {
    WorkspaceInfo workspace;
    workspace.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    workspace.name = name;
    workspace.description = description;
    workspace.createdAt = QDateTime::currentMSecsSinceEpoch();
    workspace.modifiedAt = workspace.createdAt;
    
    m_workspaces[workspace.id] = workspace;
    
    if (m_currentWorkspaceId.isEmpty()) {
        m_currentWorkspaceId = workspace.id;
    }
    
    if (m_autoSaveEnabled) {
        saveWorkspace(workspace.id);
    }
    
    emit workspaceCreated(workspace.id);
    
    qDebug() << "[WorkspaceManager] Created workspace:" << workspace.id << name;
    
    return workspace.id;
}

void WorkspaceManager::deleteWorkspace(const QString& id) {
    if (!m_workspaces.contains(id)) return;
    
    // 删除文件
    QString filePath = workspaceFilePath(id);
    QFile::remove(filePath);
    
    m_workspaces.remove(id);
    
    if (m_currentWorkspaceId == id) {
        m_currentWorkspaceId.clear();
    }
    
    m_recentWorkspaces.removeAll(id);
    
    emit workspaceDeleted(id);
    
    qDebug() << "[WorkspaceManager] Deleted workspace:" << id;
}

void WorkspaceManager::renameWorkspace(const QString& id, const QString& newName) {
    if (!m_workspaces.contains(id)) return;
    
    m_workspaces[id].name = newName;
    m_workspaces[id].modifiedAt = QDateTime::currentMSecsSinceEpoch();
    
    if (m_autoSaveEnabled) {
        saveWorkspace(id);
    }
}

void WorkspaceManager::setDescription(const QString& id, const QString& description) {
    if (!m_workspaces.contains(id)) return;
    
    m_workspaces[id].description = description;
    m_workspaces[id].modifiedAt = QDateTime::currentMSecsSinceEpoch();
    
    if (m_autoSaveEnabled) {
        saveWorkspace(id);
    }
}

WorkspaceInfo WorkspaceManager::getWorkspace(const QString& id) const {
    return m_workspaces.value(id);
}

QList<WorkspaceInfo> WorkspaceManager::getAllWorkspaces() const {
    return m_workspaces.values();
}

WorkspaceInfo WorkspaceManager::getCurrentWorkspace() const {
    return m_workspaces.value(m_currentWorkspaceId);
}

void WorkspaceManager::switchWorkspace(const QString& id) {
    if (!m_workspaces.contains(id)) return;
    
    m_currentWorkspaceId = id;
    addToRecent(id);
    
    emit workspaceSwitched(id);
    
    qDebug() << "[WorkspaceManager] Switched to workspace:" << id;
}

void WorkspaceManager::closeCurrentWorkspace() {
    if (!m_currentWorkspaceId.isEmpty()) {
        QString id = m_currentWorkspaceId;
        m_currentWorkspaceId.clear();
        emit workspaceSwitched(QString());
        qDebug() << "[WorkspaceManager] Closed workspace:" << id;
    }
}

void WorkspaceManager::addSessionToWorkspace(const QString& workspaceId, const QString& sessionId) {
    if (!m_workspaces.contains(workspaceId)) return;
    
    if (!m_workspaces[workspaceId].sessionIds.contains(sessionId)) {
        m_workspaces[workspaceId].sessionIds.append(sessionId);
        m_workspaces[workspaceId].modifiedAt = QDateTime::currentMSecsSinceEpoch();
        
        if (m_autoSaveEnabled) {
            saveWorkspace(workspaceId);
        }
        
        emit sessionAdded(workspaceId, sessionId);
    }
}

void WorkspaceManager::removeSessionFromWorkspace(const QString& workspaceId, const QString& sessionId) {
    if (!m_workspaces.contains(workspaceId)) return;
    
    m_workspaces[workspaceId].sessionIds.removeAll(sessionId);
    m_workspaces[workspaceId].modifiedAt = QDateTime::currentMSecsSinceEpoch();
    
    if (m_autoSaveEnabled) {
        saveWorkspace(workspaceId);
    }
    
    emit sessionRemoved(workspaceId, sessionId);
}

void WorkspaceManager::updateSession(const QString& workspaceId, const WorkspaceSession& session) {
    // Session state update - can be extended to save to workspace file
    Q_UNUSED(workspaceId)
    Q_UNUSED(session)
}

void WorkspaceManager::saveLayout(const QString& workspaceId, const QString& layout) {
    if (!m_workspaces.contains(workspaceId)) return;
    
    m_workspaces[workspaceId].layout = layout;
    m_workspaces[workspaceId].modifiedAt = QDateTime::currentMSecsSinceEpoch();
    
    if (m_autoSaveEnabled) {
        saveWorkspace(workspaceId);
    }
}

QString WorkspaceManager::loadLayout(const QString& workspaceId) const {
    if (!m_workspaces.contains(workspaceId)) return QString();
    
    return m_workspaces[workspaceId].layout;
}

void WorkspaceManager::saveWorkspace(const QString& id) {
    if (!m_workspaces.contains(id)) return;
    
    const WorkspaceInfo& workspace = m_workspaces[id];
    
    QJsonObject json;
    json["id"] = workspace.id;
    json["name"] = workspace.name;
    json["description"] = workspace.description;
    json["createdAt"] = workspace.createdAt;
    json["modifiedAt"] = workspace.modifiedAt;
    json["autoSave"] = workspace.autoSave;
    
    QJsonArray sessionsJson;
    for (const QString& sessionId : workspace.sessionIds) {
        sessionsJson.append(sessionId);
    }
    json["sessions"] = sessionsJson;
    
    json["layout"] = workspace.layout;
    
    QString filePath = workspaceFilePath(id);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
    
    emit workspaceSaved(id);
    
    qDebug() << "[WorkspaceManager] Saved workspace:" << id;
}

void WorkspaceManager::saveAllWorkspaces() {
    for (auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it) {
        saveWorkspace(it.key());
    }
}

void WorkspaceManager::loadWorkspace(const QString& id) {
    QString filePath = workspaceFilePath(id);
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject json = doc.object();
    
    WorkspaceInfo workspace;
    workspace.id = json["id"].toString();
    workspace.name = json["name"].toString();
    workspace.description = json["description"].toString();
    workspace.createdAt = json["createdAt"].toVariant().toLongLong();
    workspace.modifiedAt = json["modifiedAt"].toVariant().toLongLong();
    workspace.autoSave = json["autoSave"].toBool(true);
    workspace.layout = json["layout"].toString();
    
    QJsonArray sessionsJson = json["sessions"].toArray();
    for (const QJsonValue& sessionValue : sessionsJson) {
        workspace.sessionIds.append(sessionValue.toString());
    }
    
    m_workspaces[id] = workspace;
    
    emit workspaceLoaded(id);
    
    qDebug() << "[WorkspaceManager] Loaded workspace:" << id;
}

void WorkspaceManager::loadAllWorkspaces() {
    ensureWorkspaceDir();
    
    QDir dir(m_workspaceDir);
    QFileInfoList files = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
    
    for (const QFileInfo& fileInfo : files) {
        QString id = fileInfo.baseName();
        loadWorkspace(id);
    }
    
    qDebug() << "[WorkspaceManager] Loaded" << m_workspaces.size() << "workspaces";
}

void WorkspaceManager::setAutoSaveEnabled(bool enabled) {
    m_autoSaveEnabled = enabled;
}

bool WorkspaceManager::isAutoSaveEnabled() const {
    return m_autoSaveEnabled;
}

QStringList WorkspaceManager::getRecentWorkspaces(int limit) const {
    QStringList recent;
    for (const QString& id : m_recentWorkspaces) {
        if (m_workspaces.contains(id)) {
            recent.append(id);
            if (recent.size() >= limit) break;
        }
    }
    return recent;
}

void WorkspaceManager::addToRecent(const QString& id) {
    m_recentWorkspaces.removeAll(id);
    m_recentWorkspaces.prepend(id);
    
    // 限制最近使用列表大小
    while (m_recentWorkspaces.size() > 20) {
        m_recentWorkspaces.removeLast();
    }
}

void WorkspaceManager::exportWorkspace(const QString& id, const QString& filePath) {
    if (!m_workspaces.contains(id)) return;
    
    const WorkspaceInfo& workspace = m_workspaces[id];
    
    QJsonObject json;
    json["id"] = workspace.id;
    json["name"] = workspace.name;
    json["description"] = workspace.description;
    json["sessions"] = QJsonArray::fromStringList(workspace.sessionIds);
    json["layout"] = workspace.layout;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
}

void WorkspaceManager::importWorkspace(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject json = doc.object();
    
    WorkspaceInfo workspace;
    workspace.id = json["id"].toString();
    workspace.name = json["name"].toString();
    workspace.description = json["description"].toString();
    workspace.layout = json["layout"].toString();
    
    QJsonArray sessionsJson = json["sessions"].toArray();
    for (const QJsonValue& sessionValue : sessionsJson) {
        workspace.sessionIds.append(sessionValue.toString());
    }
    
    workspace.createdAt = QDateTime::currentMSecsSinceEpoch();
    workspace.modifiedAt = workspace.createdAt;
    
    // 如果 ID 冲突，生成新 ID
    if (m_workspaces.contains(workspace.id)) {
        workspace.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    m_workspaces[workspace.id] = workspace;
    
    if (m_autoSaveEnabled) {
        saveWorkspace(workspace.id);
    }
    
    emit workspaceCreated(workspace.id);
}

QString WorkspaceManager::workspaceFilePath(const QString& id) const {
    return m_workspaceDir + "/" + id + ".json";
}

#include "WorkspaceManager.moc"
