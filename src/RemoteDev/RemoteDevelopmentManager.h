#ifndef REMOTEDEVELOPMENTMANAGER_H
#define REMOTEDEVELOPMENTMANAGER_H

#include <QObject>
#include <QMap>
#include <QProcess>

/**
 * @brief 远程工作区配置
 */
struct RemoteWorkspace {
    QString id;
    QString name;
    QString host;
    int port;
    QString username;
    QString remotePath;
    QString localSyncPath;
    bool syncEnabled = true;
    QStringList excludePatterns;
    QString sshConfig;
};

/**
 * @brief 文件同步状态
 */
struct SyncStatus {
    QString filePath;
    QString status;  // synced, modified, new, deleted, conflict
    QDateTime lastSync;
    QString direction;  // upload, download
};

/**
 * @brief 远程开发管理器 - VS Code Remote 风格
 * 
 * 功能:
 * - 远程工作区管理
 * - 双向文件同步
 * - 远程终端连接
 * - 端口转发
 * - 远程调试支持
 */
class RemoteDevelopmentManager : public QObject {
    Q_OBJECT

public:
    explicit RemoteDevelopmentManager(QObject* parent = nullptr);
    ~RemoteDevelopmentManager();

    // 工作区管理
    QString createWorkspace(const RemoteWorkspace& config);
    bool updateWorkspace(const QString& workspaceId, const RemoteWorkspace& config);
    bool deleteWorkspace(const QString& workspaceId);
    RemoteWorkspace getWorkspace(const QString& workspaceId) const;
    QList<RemoteWorkspace> listWorkspaces() const;
    
    // 连接管理
    bool connectToWorkspace(const QString& workspaceId);
    bool disconnectFromWorkspace(const QString& workspaceId);
    bool isConnected(const QString& workspaceId) const;
    
    // 文件同步
    void syncFiles(const QString& workspaceId, const QStringList& paths = QStringList());
    void startAutoSync(const QString& workspaceId);
    void stopAutoSync(const QString& workspaceId);
    QList<SyncStatus> getSyncStatus(const QString& workspaceId) const;
    QList<SyncStatus> getConflicts(const QString& workspaceId) const;
    bool resolveConflict(const QString& workspaceId, const QString& filePath, bool keepRemote);
    
    // 端口转发
    bool addPortForward(const QString& workspaceId, int localPort, int remotePort);
    bool removePortForward(const QString& workspaceId, int localPort);
    QList<QPair<int, int>> getPortForwards(const QString& workspaceId) const;
    
    // 远程命令执行
    QString executeRemoteCommand(const QString& workspaceId, const QString& command);
    QProcess* createRemoteProcess(const QString& workspaceId);
    
    // 远程文件系统
    QStringList listRemoteDirectory(const QString& workspaceId, const QString& path);
    QByteArray readRemoteFile(const QString& workspaceId, const QString& path);
    bool writeRemoteFile(const QString& workspaceId, const QString& path, const QByteArray& content);
    bool deleteRemoteFile(const QString& workspaceId, const QString& path);
    bool createRemoteDirectory(const QString& workspaceId, const QString& path);

signals:
    void workspaceConnected(const QString& workspaceId);
    void workspaceDisconnected(const QString& workspaceId);
    void syncStarted(const QString& workspaceId);
    void syncCompleted(const QString& workspaceId, int synced, int failed);
    void syncConflict(const QString& workspaceId, const QString& filePath);
    void fileChanged(const QString& workspaceId, const QString& filePath);
    void portForwardAdded(const QString& workspaceId, int localPort, int remotePort);
    void portForwardRemoved(const QString& workspaceId, int localPort);
    void remoteOutput(const QString& workspaceId, const QByteArray& output);
    void errorOccurred(const QString& workspaceId, const QString& message);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupFileWatcher(const QString& workspaceId);
    void syncFile(const QString& workspaceId, const QString& filePath, const QString& direction);
    QString generateWorkspaceId() const;
    
    QMap<QString, RemoteWorkspace> m_workspaces;
    QMap<QString, bool> m_connections;         // workspaceId -> connected
    QMap<QString, QList<SyncStatus>> m_syncStatus;
    QMap<QString, QList<QPair<int, int>>> m_portForwards;
    QMap<QString, QFileSystemWatcher*> m_fileWatchers;
    QMap<QString, QProcess*> m_processes;
    
    bool m_autoSyncEnabled = true;
    int m_syncIntervalMs = 5000;
    
    static RemoteDevelopmentManager* s_instance;

public:
    static RemoteDevelopmentManager* instance();
};

#endif // REMOTEDEVELOPMENTMANAGER_H
