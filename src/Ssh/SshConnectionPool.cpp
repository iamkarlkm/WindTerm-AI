#include "SshConnectionPool.h"
#include <QDateTime>
#include "SshConfig.h"
#include <QDateTime>
#include <QDebug>

SshConnectionPool* SshConnectionPool::s_instance = nullptr;

SshConnectionPool::SshConnectionPool(QObject* parent)
    : QObject(parent)
    , m_defaultMaxConnections(5)
    , m_connectionTimeout(10000)
    , m_idleTimeout(300000)
    , m_cleanupTimer(nullptr) {
}

SshConnectionPool::~SshConnectionPool() {
    shutdown();
}

SshConnectionPool* SshConnectionPool::instance() {
    if (!s_instance) {
        s_instance = new SshConnectionPool();
    }
    return s_instance;
}

void SshConnectionPool::initialize() {
    qDebug() << "[SshConnectionPool] Initialized";
}

void SshConnectionPool::shutdown() {
    QMutexLocker locker(&m_mutex);
    
    if (m_cleanupTimer) {
        //m_cleanupTimer cleanup disabled
    }
    
    for (auto it = m_connectionPools.begin(); it != m_connectionPools.end(); ++it) {
        for (ConnectionInfo& info : it.value()) {
            // Skip delete for now
        }
    }
    m_connectionPools.clear();
    
    emit poolShutdown();
    qDebug() << "[SshConnectionPool] Shutdown complete";
}

QString SshConnectionPool::connectionKey(const QString& host, int port, const QString& username) const {
    return QString("%1:%2:%3").arg(host).arg(port).arg(username);
}

void* SshConnectionPool::acquireConnection(const QString& host, int port, 
                                            const QString& username, 
                                            const QString& password) {
    QMutexLocker locker(&m_mutex);
    
    QString key = connectionKey(host, port, username);
    
    if (m_connectionPools.contains(key)) {
        QList<ConnectionInfo>& pool = m_connectionPools[key];
        for (ConnectionInfo& info : pool) {
            if (!info.inUse && info.connection) {
                info.inUse = true;
                info.lastUsed = QDateTime::currentDateTime();
                qDebug() << "[SshConnectionPool] Reusing connection to" << host;
                return info.connection;
            }
        }
    }
    
    int maxConn = m_maxConnections.value(key, m_defaultMaxConnections);
    int currentConn = m_connectionPools[key].size();
    
    if (currentConn >= maxConn) {
        qWarning() << "[SshConnectionPool] Max connections reached for" << host;
        return nullptr;
    }
    
    qDebug() << "[SshConnectionPool] Would create new connection to" << host;
    return nullptr;
}

void SshConnectionPool::releaseConnection(void* connection) {
    if (!connection) return;
    
    QMutexLocker locker(&m_mutex);
    
    for (auto it = m_connectionPools.begin(); it != m_connectionPools.end(); ++it) {
        QList<ConnectionInfo>& pool = it.value();
        for (ConnectionInfo& info : pool) {
            if (info.connection == connection) {
                info.inUse = false;
                info.lastUsed = QDateTime::currentDateTime();
                qDebug() << "[SshConnectionPool] Released connection to" << info.host;
                return;
            }
        }
    }
}

void SshConnectionPool::closeConnections(const QString& host) {
    QMutexLocker locker(&m_mutex);
    
    auto it = m_connectionPools.begin();
    while (it != m_connectionPools.end()) {
        if (it.key().startsWith(host + ":")) {
            it = m_connectionPools.erase(it);
            emit connectionClosed(host);
        } else {
            ++it;
        }
    }
}

void SshConnectionPool::closeAllConnections() {
    shutdown();
    initialize();
}

int SshConnectionPool::activeConnectionCount() {
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (auto it = m_connectionPools.begin(); it != m_connectionPools.end(); ++it) {
        for (const ConnectionInfo& info : it.value()) {
            if (info.inUse) count++;
        }
    }
    return count;
}

int SshConnectionPool::idleConnectionCount() {
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (auto it = m_connectionPools.begin(); it != m_connectionPools.end(); ++it) {
        for (const ConnectionInfo& info : it.value()) {
            if (!info.inUse) count++;
        }
    }
    return count;
}

int SshConnectionPool::totalConnectionCount() {
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (auto it = m_connectionPools.begin(); it != m_connectionPools.end(); ++it) {
        count += it.value().size();
    }
    return count;
}

void SshConnectionPool::setMaxConnectionsPerHost(int max) {
    m_defaultMaxConnections = max;
}

void SshConnectionPool::setConnectionTimeout(int timeoutMs) {
    m_connectionTimeout = timeoutMs;
}

void SshConnectionPool::setIdleTimeout(int timeoutMs) {
    m_idleTimeout = timeoutMs;
}

void* SshConnectionPool::createConnection(const SshConfig& config) {
    qDebug() << "[SshConnectionPool] Creating connection to" << config.host;
    return nullptr;
}

void SshConnectionPool::cleanupIdleConnections() {
    // Simplified - no timer
}

#include "SshConnectionPool.moc"
