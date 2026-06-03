#include "SshTunnelManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QDateTime>
#include <QDebug>

SshTunnelManager* SshTunnelManager::s_instance = nullptr;

SshTunnelManager::SshTunnelManager(QObject* parent)
    : QObject(parent) {
    
    m_tunnelsFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ssh_tunnels.json";
    loadTunnels();
}

SshTunnelManager* SshTunnelManager::instance() {
    if (!s_instance) {
        s_instance = new SshTunnelManager();
    }
    return s_instance;
}

QString SshTunnelManager::createTunnel(const SshTunnelConfig& config) {
    SshTunnel tunnel;
    tunnel.config = config;
    tunnel.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    tunnel.isActive = false;
    tunnel.localServer = nullptr;
    tunnel.startTime = 0;
    
    m_tunnels[tunnel.id] = tunnel;
    
    saveTunnels();
    
    qDebug() << "[SshTunnelManager] Created tunnel:" << tunnel.id << config.name;
    
    return tunnel.id;
}

void SshTunnelManager::deleteTunnel(const QString& id) {
    if (!m_tunnels.contains(id)) return;
    
    stopTunnel(id);
    m_tunnels.remove(id);
    
    saveTunnels();
}

void SshTunnelManager::updateTunnel(const QString& id, const SshTunnelConfig& config) {
    if (!m_tunnels.contains(id)) return;
    
    bool wasActive = m_tunnels[id].isActive;
    
    if (wasActive) {
        stopTunnel(id);
    }
    
    m_tunnels[id].config = config;
    
    if (wasActive) {
        startTunnel(id);
    }
    
    saveTunnels();
}

bool SshTunnelManager::startTunnel(const QString& id) {
    if (!m_tunnels.contains(id)) return false;
    
    SshTunnel& tunnel = m_tunnels[id];
    
    if (tunnel.isActive) {
        return true;
    }
    
    // 检查端口可用性
    if (tunnel.config.localForwardEnabled && !isPortAvailable(tunnel.config.localPort)) {
        tunnel.errorMessage = QString("Local port %1 is already in use").arg(tunnel.config.localPort);
        emit tunnelError(id, tunnel.errorMessage);
        return false;
    }
    
    if (tunnel.config.dynamicForwardEnabled && !isPortAvailable(tunnel.config.dynamicPort)) {
        tunnel.errorMessage = QString("Dynamic port %1 is already in use").arg(tunnel.config.dynamicPort);
        emit tunnelError(id, tunnel.errorMessage);
        return false;
    }
    
    // 请求建立 SSH 连接
    QString connectionId;
    emit connectionRequested(tunnel.config, &connectionId);
    
    if (connectionId.isEmpty()) {
        tunnel.errorMessage = "Failed to establish SSH connection";
        emit tunnelError(id, tunnel.errorMessage);
        return false;
    }
    
    tunnel.connectionId = connectionId;
    
    // 设置本地转发
    if (tunnel.config.localForwardEnabled) {
        if (!setupLocalForward(tunnel)) {
            return false;
        }
    }
    
    // 设置动态转发（SOCKS）
    if (tunnel.config.dynamicForwardEnabled) {
        if (!setupDynamicForward(tunnel)) {
            return false;
        }
    }
    
    tunnel.isActive = true;
    tunnel.startTime = QDateTime::currentMSecsSinceEpoch();
    
    emit tunnelStarted(id);
    
    qDebug() << "[SshTunnelManager] Started tunnel:" << id;
    
    return true;
}

void SshTunnelManager::stopTunnel(const QString& id) {
    if (!m_tunnels.contains(id)) return;
    
    SshTunnel& tunnel = m_tunnels[id];
    
    if (!tunnel.isActive) {
        return;
    }
    
    cleanupTunnel(tunnel);
    
    // 断开 SSH 连接
    if (!tunnel.connectionId.isEmpty()) {
        emit disconnectionRequested(tunnel.connectionId);
    }
    
    tunnel.isActive = false;
    tunnel.connectionId.clear();
    tunnel.startTime = 0;
    
    emit tunnelStopped(id);
    
    qDebug() << "[SshTunnelManager] Stopped tunnel:" << id;
}

void SshTunnelManager::stopAllTunnels() {
    for (auto it = m_tunnels.begin(); it != m_tunnels.end(); ++it) {
        stopTunnel(it.key());
    }
}

SshTunnel SshTunnelManager::getTunnel(const QString& id) const {
    return m_tunnels.value(id);
}

QList<SshTunnel> SshTunnelManager::getAllTunnels() const {
    return m_tunnels.values();
}

QList<SshTunnel> SshTunnelManager::getActiveTunnels() const {
    QList<SshTunnel> active;
    for (auto it = m_tunnels.begin(); it != m_tunnels.end(); ++it) {
        if (it->isActive) {
            active.append(it.value());
        }
    }
    return active;
}

bool SshTunnelManager::isTunnelActive(const QString& id) const {
    if (!m_tunnels.contains(id)) return false;
    return m_tunnels[id].isActive;
}

bool SshTunnelManager::isPortAvailable(int port) {
    QTcpServer server;
    return server.listen(QHostAddress::Any, port);
}

QList<int> SshTunnelManager::getListeningPorts() {
    QList<int> ports;
    
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            // In a real implementation, we would parse /proc/net/tcp
            // This is a simplified version
            Q_UNUSED(entry)
        }
    }
    
    return ports;
}

void SshTunnelManager::saveTunnels() {
    QJsonArray tunnelsJson;
    
    for (auto it = m_tunnels.begin(); it != m_tunnels.end(); ++it) {
        const SshTunnel& tunnel = it.value();
        
        QJsonObject json;
        json["id"] = tunnel.id;
        json["name"] = tunnel.config.name;
        json["sshHost"] = tunnel.config.sshHost;
        json["sshPort"] = tunnel.config.sshPort;
        json["sshUsername"] = tunnel.config.sshUsername;
        json["privateKeyPath"] = tunnel.config.privateKeyPath;
        
        json["localForwardEnabled"] = tunnel.config.localForwardEnabled;
        json["localPort"] = tunnel.config.localPort;
        json["remoteHost"] = tunnel.config.remoteHost;
        json["remotePort"] = tunnel.config.remotePort;
        
        json["remoteForwardEnabled"] = tunnel.config.remoteForwardEnabled;
        json["remotePortForward"] = tunnel.config.remotePortForward;
        json["localPortForward"] = tunnel.config.localPortForward;
        
        json["dynamicForwardEnabled"] = tunnel.config.dynamicForwardEnabled;
        json["dynamicPort"] = tunnel.config.dynamicPort;
        
        tunnelsJson.append(json);
    }
    
    QFile file(m_tunnelsFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(tunnelsJson).toJson(QJsonDocument::Indented));
    }
}

void SshTunnelManager::loadTunnels() {
    QFile file(m_tunnelsFile);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonArray tunnelsJson = QJsonDocument::fromJson(file.readAll()).array();
    
    for (const QJsonValue& value : tunnelsJson) {
        QJsonObject json = value.toObject();
        
        SshTunnelConfig config;
        config.name = json["name"].toString();
        config.sshHost = json["sshHost"].toString();
        config.sshPort = json["sshPort"].toInt(22);
        config.sshUsername = json["sshUsername"].toString();
        config.privateKeyPath = json["privateKeyPath"].toString();
        
        config.localForwardEnabled = json["localForwardEnabled"].toBool(false);
        config.localPort = json["localPort"].toInt(8080);
        config.remoteHost = json["remoteHost"].toString();
        config.remotePort = json["remotePort"].toInt(80);
        
        config.remoteForwardEnabled = json["remoteForwardEnabled"].toBool(false);
        config.remotePortForward = json["remotePortForward"].toInt(8080);
        config.localPortForward = json["localPortForward"].toInt(80);
        
        config.dynamicForwardEnabled = json["dynamicForwardEnabled"].toBool(false);
        config.dynamicPort = json["dynamicPort"].toInt(1080);
        
        SshTunnel tunnel;
        tunnel.id = json["id"].toString();
        tunnel.config = config;
        tunnel.isActive = false;
        tunnel.localServer = nullptr;
        
        m_tunnels[tunnel.id] = tunnel;
    }
    
    qDebug() << "[SshTunnelManager] Loaded" << m_tunnels.size() << "tunnels";
}

void SshTunnelManager::exportTunnels(const QString& filePath) {
    QJsonArray tunnelsJson;
    
    for (auto it = m_tunnels.begin(); it != m_tunnels.end(); ++it) {
        QJsonObject json;
        json["id"] = it->id;
        json["name"] = it->config.name;
        json["sshHost"] = it->config.sshHost;
        json["sshPort"] = it->config.sshPort;
        json["sshUsername"] = it->config.sshUsername;
        json["privateKeyPath"] = it->config.privateKeyPath;
        
        json["localForwardEnabled"] = it->config.localForwardEnabled;
        json["localPort"] = it->config.localPort;
        json["remoteHost"] = it->config.remoteHost;
        json["remotePort"] = it->config.remotePort;
        
        json["dynamicForwardEnabled"] = it->config.dynamicForwardEnabled;
        json["dynamicPort"] = it->config.dynamicPort;
        
        tunnelsJson.append(json);
    }
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(tunnelsJson).toJson(QJsonDocument::Indented));
    }
}

void SshTunnelManager::importTunnels(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonArray tunnelsJson = QJsonDocument::fromJson(file.readAll()).array();
    
    for (const QJsonValue& value : tunnelsJson) {
        QJsonObject json = value.toObject();
        
        SshTunnelConfig config;
        config.name = json["name"].toString();
        config.sshHost = json["sshHost"].toString();
        config.sshPort = json["sshPort"].toInt(22);
        config.sshUsername = json["sshUsername"].toString();
        config.privateKeyPath = json["privateKeyPath"].toString();
        
        config.localForwardEnabled = json["localForwardEnabled"].toBool(false);
        config.localPort = json["localPort"].toInt(8080);
        config.remoteHost = json["remoteHost"].toString();
        config.remotePort = json["remotePort"].toInt(80);
        
        config.dynamicForwardEnabled = json["dynamicForwardEnabled"].toBool(false);
        config.dynamicPort = json["dynamicPort"].toInt(1080);
        
        createTunnel(config);
    }
}

bool SshTunnelManager::setupLocalForward(SshTunnel& tunnel) {
    tunnel.localServer = new QTcpServer(this);
    
    if (!tunnel.localServer->listen(QHostAddress::LocalHost, tunnel.config.localPort)) {
        tunnel.errorMessage = QString("Failed to listen on port %1: %2")
            .arg(tunnel.config.localPort)
            .arg(tunnel.localServer->errorString());
        emit tunnelError(tunnel.id, tunnel.errorMessage);
        return false;
    }
    
    connect(tunnel.localServer, &QTcpServer::newConnection, this, &SshTunnelManager::onLocalConnection);
    
    qDebug() << "[SshTunnelManager] Local forward:" << tunnel.config.localPort 
             << "->" << tunnel.config.remoteHost << ":" << tunnel.config.remotePort;
    
    return true;
}

bool SshTunnelManager::setupDynamicForward(SshTunnel& tunnel) {
    // SOCKS proxy setup would go here
    // For now, just log the configuration
    qDebug() << "[SshTunnelManager] Dynamic forward (SOCKS):" << tunnel.config.dynamicPort;
    
    return true;
}

void SshTunnelManager::cleanupTunnel(SshTunnel& tunnel) {
    if (tunnel.localServer) {
        tunnel.localServer->close();
        tunnel.localServer->deleteLater();
        tunnel.localServer = nullptr;
    }
}

void SshTunnelManager::onLocalConnection() {
    QTcpServer* server = qobject_cast<QTcpServer*>(sender());
    if (!server) return;
    
    QTcpSocket* clientSocket = server->nextPendingConnection();
    
    // In a real implementation, we would forward this connection through SSH
    qDebug() << "[SshTunnelManager] New local connection from" << clientSocket->peerAddress();
    
    // Cleanup when done
    clientSocket->deleteLater();
}

#include "SshTunnelManager.moc"
