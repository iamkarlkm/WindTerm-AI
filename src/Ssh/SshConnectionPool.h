#ifndef SSH_CONNECTION_POOL_H
#define SSH_CONNECTION_POOL_H

#include <QObject>
#include <QMap>
#include <QDateTime>
#include <QQueue>
#include <QMutex>
#include <memory>

class SshConnection;
struct SshConfig;

class SshConnectionPool : public QObject {
    Q_OBJECT
public:
    explicit SshConnectionPool(QObject* parent = nullptr);
    ~SshConnectionPool() override;
    
    static SshConnectionPool* instance();
    
    void initialize();
    void shutdown();
    
    // 获取连接（从池中获取或创建新连接）
    void* acquireConnection(const QString& host, int port = 22, 
                                     const QString& username = "", 
                                     const QString& password = "");
    
    // 释放连接回池中
    void releaseConnection(void* connection);
    
    // 关闭指定主机的所有连接
    void closeConnections(const QString& host);
    
    // 关闭所有连接
    void closeAllConnections();
    
    // 连接统计
    int activeConnectionCount();
    int idleConnectionCount();
    int totalConnectionCount();
    
    // 配置
    void setMaxConnectionsPerHost(int max);
    void setConnectionTimeout(int timeoutMs);
    void setIdleTimeout(int timeoutMs);
    
signals:
    void connectionCreated(const QString& host);
    void connectionClosed(const QString& host);
    void poolShutdown();

private:
    struct ConnectionInfo {
        void* connection;
        QString host;
        int port;
        QString username;
        QDateTime lastUsed;
        bool inUse;
    };
    
    void* createConnection(const SshConfig& config);
    void cleanupIdleConnections();
    QString connectionKey(const QString& host, int port, const QString& username) const;
    
    static SshConnectionPool* s_instance;
    
    QMap<QString, QList<ConnectionInfo>> m_connectionPools;
    QMap<QString, int> m_maxConnections;  // 每个主机的最大连接数
    QRecursiveMutex m_mutex;
    
    int m_defaultMaxConnections;
    int m_connectionTimeout;
    int m_idleTimeout;
    
    void* m_cleanupTimer;
};

#endif
