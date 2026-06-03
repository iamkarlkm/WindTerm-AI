#ifndef WORKSPACE_MANAGER_H
#define WORKSPACE_MANAGER_H

#include <QObject>
#include <QMap>
#include <QList>

struct WorkspaceInfo {
    QString id;
    QString name;
    QString description;
    QStringList sessionIds;
    QString layout;  // JSON layout string
    qint64 createdAt;
    qint64 modifiedAt;
    bool autoSave;
    
    WorkspaceInfo() : createdAt(0), modifiedAt(0), autoSave(true) {}
};

struct WorkspaceSession {
    QString id;
    QString title;
    QString workingDirectory;
    QString command;
    QByteArray geometry;
    int scrollPosition;
};

class WorkspaceManager : public QObject {
    Q_OBJECT
public:
    explicit WorkspaceManager(QObject* parent = nullptr);
    
    static WorkspaceManager* instance();
    
    // 工作区管理
    QString createWorkspace(const QString& name, const QString& description = "");
    void deleteWorkspace(const QString& id);
    void renameWorkspace(const QString& id, const QString& newName);
    void setDescription(const QString& id, const QString& description);
    
    // 工作区信息
    WorkspaceInfo getWorkspace(const QString& id) const;
    QList<WorkspaceInfo> getAllWorkspaces() const;
    WorkspaceInfo getCurrentWorkspace() const;
    QString getCurrentWorkspaceId() const { return m_currentWorkspaceId; }
    
    // 工作区切换
    void switchWorkspace(const QString& id);
    void closeCurrentWorkspace();
    
    // 会话管理
    void addSessionToWorkspace(const QString& workspaceId, const QString& sessionId);
    void removeSessionFromWorkspace(const QString& workspaceId, const QString& sessionId);
    void updateSession(const QString& workspaceId, const WorkspaceSession& session);
    
    // 布局管理
    void saveLayout(const QString& workspaceId, const QString& layout);
    QString loadLayout(const QString& workspaceId) const;
    
    // 持久化
    void saveWorkspace(const QString& id);
    void saveAllWorkspaces();
    void loadWorkspace(const QString& id);
    void loadAllWorkspaces();
    
    // 自动保存
    void setAutoSaveEnabled(bool enabled);
    bool isAutoSaveEnabled() const;
    
    // 最近使用
    QStringList getRecentWorkspaces(int limit = 10) const;
    void addToRecent(const QString& id);
    
    // 导入导出
    void exportWorkspace(const QString& id, const QString& filePath);
    void importWorkspace(const QString& filePath);
    
signals:
    void workspaceCreated(const QString& id);
    void workspaceDeleted(const QString& id);
    void workspaceSwitched(const QString& id);
    void workspaceSaved(const QString& id);
    void workspaceLoaded(const QString& id);
    void sessionAdded(const QString& workspaceId, const QString& sessionId);
    void sessionRemoved(const QString& workspaceId, const QString& sessionId);

private:
    static WorkspaceManager* s_instance;
    
    QMap<QString, WorkspaceInfo> m_workspaces;
    QString m_currentWorkspaceId;
    QString m_workspaceDir;
    
    bool m_autoSaveEnabled;
    QStringList m_recentWorkspaces;
    
    void ensureWorkspaceDir();
    QString workspaceFilePath(const QString& id) const;
};

#endif
