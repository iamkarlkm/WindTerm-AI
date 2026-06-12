#include "RemoteDevelopmentManager.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QDebug>
#include <QUuid>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

RemoteDevelopmentManager* RemoteDevelopmentManager::s_instance = nullptr;

RemoteDevelopmentManager::RemoteDevelopmentManager(QObject* parent) : QObject(parent) {
}

RemoteDevelopmentManager::~RemoteDevelopmentManager() {
}

RemoteDevelopmentManager* RemoteDevelopmentManager::instance() {
    if (!s_instance) s_instance = new RemoteDevelopmentManager();
    return s_instance;
}

QString RemoteDevelopmentManager::createWorkspace(const RemoteWorkspace& config) {
    QString workspaceId = generateWorkspaceId();
    
    RemoteWorkspace ws = config;
    ws.id = workspaceId;
    
    m_workspaces[workspaceId] = ws;
    m_syncStatus[workspaceId] = QList<SyncStatus>();
    m_portForwards[workspaceId] = QList<QPair<int, int>>();
    m_connections[workspaceId] = false;
    
    return workspaceId;
}

bool RemoteDevelopmentManager::updateWorkspace(const QString& workspaceId, const RemoteWorkspace& config) {
    if (!m_workspaces.contains(workspaceId)) return false;
    
    RemoteWorkspace ws = config;
    ws.id = workspaceId;
    m_workspaces[workspaceId] = ws;
    
    return true;
}

bool RemoteDevelopmentManager::deleteWorkspace(const QString& workspaceId) {
    if (!m_workspaces.contains(workspaceId)) return false;
    
    disconnectFromWorkspace(workspaceId);
    
    m_workspaces.remove(workspaceId);
    m_syncStatus.remove(workspaceId);
    m_portForwards.remove(workspaceId);
    m_connections.remove(workspaceId);
    
    return true;
}

RemoteWorkspace RemoteDevelopmentManager::getWorkspace(const QString& workspaceId) const {
    return m_workspaces.value(workspaceId);
}

QList<RemoteWorkspace> RemoteDevelopmentManager::listWorkspaces() const {
    return m_workspaces.values();
}

bool RemoteDevelopmentManager::connectToWorkspace(const QString& workspaceId) {
    if (!m_workspaces.contains(workspaceId)) return false;
    
    const RemoteWorkspace& ws = m_workspaces[workspaceId];
    
    // 建立 SSH 连接
    // 实际实现需要启动 SSH 进程
    qDebug() << "Connecting to" << ws.username << "@" << ws.host << ":" << ws.port;
    
    m_connections[workspaceId] = true;
    
    // 启动文件监视
    if (ws.syncEnabled && !ws.localSyncPath.isEmpty()) {
        setupFileWatcher(workspaceId);
    }
    
    emit workspaceConnected(workspaceId);
    return true;
}

bool RemoteDevelopmentManager::disconnectFromWorkspace(const QString& workspaceId) {
    if (!m_workspaces.contains(workspaceId)) return false;
    
    // 清理资源
    if (m_fileWatchers.contains(workspaceId)) {
        delete m_fileWatchers[workspaceId];
        m_fileWatchers.remove(workspaceId);
    }
    
    m_connections[workspaceId] = false;
    emit workspaceDisconnected(workspaceId);
    return true;
}

bool RemoteDevelopmentManager::isConnected(const QString& workspaceId) const {
    return m_connections.value(workspaceId, false);
}

void RemoteDevelopmentManager::syncFiles(const QString& workspaceId, const QStringList& paths) {
    if (!m_workspaces.contains(workspaceId)) return;
    if (!m_connections.value(workspaceId, false)) return;
    
    emit syncStarted(workspaceId);
    
    const RemoteWorkspace& ws = m_workspaces[workspaceId];
    
    // 实际实现需要使用 rsync 或 SFTP
    int synced = 0;
    int failed = 0;
    
    // 模拟同步
    if (paths.isEmpty()) {
        // 全量同步
        synced = 10;
    } else {
        // 增量同步
        for (const QString& path : paths) {
            syncFile(workspaceId, path, "both");
            synced++;
        }
    }
    
    emit syncCompleted(workspaceId, synced, failed);
}

void RemoteDevelopmentManager::startAutoSync(const QString& workspaceId) {
    Q_UNUSED(workspaceId)
    // 启动定时同步
}

void RemoteDevelopmentManager::stopAutoSync(const QString& workspaceId) {
    Q_UNUSED(workspaceId)
    // 停止定时同步
}

QList<SyncStatus> RemoteDevelopmentManager::getSyncStatus(const QString& workspaceId) const {
    return m_syncStatus.value(workspaceId);
}

QList<SyncStatus> RemoteDevelopmentManager::getConflicts(const QString& workspaceId) const {
    QList<SyncStatus> all = m_syncStatus.value(workspaceId);
    QList<SyncStatus> conflicts;
    
    for (const auto& status : all) {
        if (status.status == "conflict") {
            conflicts.append(status);
        }
    }
    
    return conflicts;
}

bool RemoteDevelopmentManager::resolveConflict(const QString& workspaceId, const QString& filePath, bool keepRemote) {
    Q_UNUSED(workspaceId)
    Q_UNUSED(filePath)
    Q_UNUSED(keepRemote)
    // 解决冲突
    return true;
}

bool RemoteDevelopmentManager::addPortForward(const QString& workspaceId, int localPort, int remotePort) {
    if (!m_workspaces.contains(workspaceId)) return false;
    
    m_portForwards[workspaceId].append(qMakePair(localPort, remotePort));
    
    // 实际实现需要建立 SSH 隧道
    // ssh -L localPort:localhost:remotePort user@host
    
    emit portForwardAdded(workspaceId, localPort, remotePort);
    return true;
}

bool RemoteDevelopmentManager::removePortForward(const QString& workspaceId, int localPort) {
    if (!m_portForwards.contains(workspaceId)) return false;
    
    auto& forwards = m_portForwards[workspaceId];
    forwards.erase(std::remove_if(forwards.begin(), forwards.end(),
        [localPort](const QPair<int, int>& p) { return p.first == localPort; }),
        forwards.end());
    
    emit portForwardRemoved(workspaceId, localPort);
    return true;
}

QList<QPair<int, int>> RemoteDevelopmentManager::getPortForwards(const QString& workspaceId) const {
    return m_portForwards.value(workspaceId);
}

QString RemoteDevelopmentManager::executeRemoteCommand(const QString& workspaceId, const QString& command) {
    if (!m_workspaces.contains(workspaceId)) return QString();
    if (!m_connections.value(workspaceId, false)) return QString();
    
    const RemoteWorkspace& ws = m_workspaces[workspaceId];
    
    // 通过 SSH 执行远程命令
    // ssh user@host "command"
    
    qDebug() << "Executing remote command:" << command;
    
    // 模拟返回
    return "Command executed successfully";
}

QProcess* RemoteDevelopmentManager::createRemoteProcess(const QString& workspaceId) {
    if (!m_workspaces.contains(workspaceId)) return nullptr;
    
    QProcess* process = new QProcess(this);
    m_processes[workspaceId] = process;
    
    connect(process, &QProcess::readyReadStandardOutput, this, &RemoteDevelopmentManager::onProcessReadyRead);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RemoteDevelopmentManager::onProcessFinished);
    
    return process;
}

QStringList RemoteDevelopmentManager::listRemoteDirectory(const QString& workspaceId, const QString& path) {
    Q_UNUSED(workspaceId)
    Q_UNUSED(path)
    // 通过 SFTP 列出远程目录
    return QStringList();
}

QByteArray RemoteDevelopmentManager::readRemoteFile(const QString& workspaceId, const QString& path) {
    Q_UNUSED(workspaceId)
    Q_UNUSED(path)
    // 通过 SFTP 读取远程文件
    return QByteArray();
}

bool RemoteDevelopmentManager::writeRemoteFile(const QString& workspaceId, const QString& path, const QByteArray& content) {
    Q_UNUSED(workspaceId)
    Q_UNUSED(path)
    Q_UNUSED(content)
    // 通过 SFTP 写入远程文件
    return true;
}

bool RemoteDevelopmentManager::deleteRemoteFile(const QString& workspaceId, const QString& path) {
    Q_UNUSED(workspaceId)
    Q_UNUSED(path)
    // 删除远程文件
    return true;
}

bool RemoteDevelopmentManager::createRemoteDirectory(const QString& workspaceId, const QString& path) {
    Q_UNUSED(workspaceId)
    Q_UNUSED(path)
    // 创建远程目录
    return true;
}

void RemoteDevelopmentManager::onProcessReadyRead() {
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;
    
    // 查找对应的 workspaceId
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it.value() == process) {
            emit remoteOutput(it.key(), process->readAllStandardOutput());
            return;
        }
    }
}

void RemoteDevelopmentManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    // 清理完成的进程
}

void RemoteDevelopmentManager::setupFileWatcher(const QString& workspaceId) {
    if (!m_workspaces.contains(workspaceId)) return;
    
    const RemoteWorkspace& ws = m_workspaces[workspaceId];
    
    if (m_fileWatchers.contains(workspaceId)) {
        delete m_fileWatchers[workspaceId];
    }
    
    QFileSystemWatcher* watcher = new QFileSystemWatcher(this);
    watcher->addPath(ws.localSyncPath);
    
    connect(watcher, &QFileSystemWatcher::fileChanged, this, [this, workspaceId](const QString& path) {
        emit fileChanged(workspaceId, path);
        if (m_autoSyncEnabled) {
            syncFiles(workspaceId, QStringList() << path);
        }
    });
    
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, [this, workspaceId](const QString& path) {
        emit fileChanged(workspaceId, path);
    });
    
    m_fileWatchers[workspaceId] = watcher;
}

void RemoteDevelopmentManager::syncFile(const QString& workspaceId, const QString& filePath, const QString& direction) {
    Q_UNUSED(workspaceId)
    Q_UNUSED(filePath)
    Q_UNUSED(direction)
    // 同步单个文件
}

QString RemoteDevelopmentManager::generateWorkspaceId() const {
    return "ws_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

#include "RemoteDevelopmentManager.moc"
