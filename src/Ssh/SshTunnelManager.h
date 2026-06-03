#ifndef SSH_TUNNEL_MANAGER_H
#define SSH_TUNNEL_MANAGER_H

#include <QObject>
#include <QMap>
#include <QTcpServer>

struct SshTunnelConfig {
    QString id;
    QString name;
    QString sshHost;
    int sshPort;
    QString sshUsername;
    QString sshPassword;  // Or key path
    QString privateKeyPath;
    
    // Local forward: Local -> Remote
    bool localForwardEnabled;
    int localPort;
    QString remoteHost;
    int remotePort;
    
    // Remote forward: Remote -> Local
    bool remoteForwardEnabled;
    int remotePortForward;
    int localPortForward;
    
    // Dynamic forward (SOCKS proxy)
    bool dynamicForwardEnabled;
    int dynamicPort;
    
    SshTunnelConfig() 
        : sshPort(22)
        , localForwardEnabled(false)
        , localPort(8080)
        , remotePort(80)
        , remoteForwardEnabled(false)
        , remotePortForward(8080)
        , localPortForward(80)
        , dynamicForwardEnabled(false)
        , dynamicPort(1080) {}
};

struct SshTunnel {
    SshTunnelConfig config;
    bool isActive;
    QString connectionId;
    QTcpServer* localServer;
    QString errorMessage;
    qint64 startTime;
    
    SshTunnel() : isActive(false), localServer(nullptr), startTime(0) {}
};

class SshTunnelManager : public QObject {
    Q_OBJECT
public:
    explicit SshTunnelManager(QObject* parent = nullptr);
    
    static SshTunnelManager* instance();
    
    // 隧道配置管理
    QString createTunnel(const SshTunnelConfig& config);
    void deleteTunnel(const QString& id);
    void updateTunnel(const QString& id, const SshTunnelConfig& config);
    
    // 隧道控制
    bool startTunnel(const QString& id);
    void stopTunnel(const QString& id);
    void stopAllTunnels();
    
    // 隧道状态
    SshTunnel getTunnel(const QString& id) const;
    QList<SshTunnel> getAllTunnels() const;
    QList<SshTunnel> getActiveTunnels() const;
    bool isTunnelActive(const QString& id) const;
    
    // 端口检查
    static bool isPortAvailable(int port);
    static QList<int> getListeningPorts();
    
    // 持久化
    void saveTunnels();
    void loadTunnels();
    
    // 导入导出
    void exportTunnels(const QString& filePath);
    void importTunnels(const QString& filePath);
    
signals:
    void tunnelStarted(const QString& id);
    void tunnelStopped(const QString& id);
    void tunnelError(const QString& id, const QString& error);
    void connectionRequested(const SshTunnelConfig& config, QString* connectionId);
    void disconnectionRequested(const QString& connectionId);

private slots:
    void onLocalConnection();
    
private:
    static SshTunnelManager* s_instance;
    
    QMap<QString, SshTunnel> m_tunnels;
    QString m_tunnelsFile;
    
    bool setupLocalForward(SshTunnel& tunnel);
    bool setupRemoteForward(const SshTunnel& tunnel);
    bool setupDynamicForward(SshTunnel& tunnel);
    void cleanupTunnel(SshTunnel& tunnel);
};

#endif
