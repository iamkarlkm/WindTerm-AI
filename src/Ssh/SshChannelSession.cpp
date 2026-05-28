#include "SshChannelSession.h"
#include <libssh/libssh.h>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>

SshChannelSession::SshChannelSession(QObject* parent)
    : QObject(parent), m_sshSession(nullptr), m_channel(nullptr),
      m_notifier(nullptr), m_readTimer(nullptr),
      m_connected(false), m_socketFd(-1) {
    
    m_readTimer = new QTimer(this);
    m_readTimer->setInterval(100);
    QObject::connect(m_readTimer, &QTimer::timeout, this, &SshChannelSession::onReadTimeout);
}

SshChannelSession::~SshChannelSession() {
    disconnect();
}

bool SshChannelSession::connectToServer(const SshConfig& config) {
    if (m_connected) {
        disconnect();
    }
    
    m_config = config;
    
    if (!m_config.isValid()) {
        emit error(QStringLiteral("SSH 配置无效"));
        return false;
    }
    
    if (!initializeSession()) {
        return false;
    }
    
    if (!authenticate()) {
        cleanup();
        return false;
    }
    
    if (!openChannel()) {
        cleanup();
        return false;
    }
    
    m_connected = true;
    emit connected();
    m_readTimer->start();
    
    qDebug() << "[SshChannelSession] Connected to" << m_config.host << ":" << m_config.port;
    return true;
}

void SshChannelSession::disconnect() {
    if (!m_connected) return;
    
    m_readTimer->stop();
    
    if (m_channel) {
        ssh_channel_close(m_channel);
        ssh_channel_free(m_channel);
        m_channel = nullptr;
    }
    
    if (m_sshSession) {
        ssh_disconnect(m_sshSession);
        ssh_free(m_sshSession);
        m_sshSession = nullptr;
    }
    
    if (m_notifier) {
        delete m_notifier;
        m_notifier = nullptr;
    }
    
    m_connected = false;
    m_socketFd = -1;
    
    emit disconnected();
    qDebug() << "[SshChannelSession] Disconnected from" << m_config.host;
}

void SshChannelSession::write(const QByteArray& data) {
    if (!m_connected || !m_channel) return;
    
    int rc = ssh_channel_write(m_channel, data.constData(), data.size());
    if (rc < 0) {
        emit error(QStringLiteral("写入 SSH 通道失败"));
    }
}

bool SshChannelSession::initializeSession() {
    m_sshSession = ssh_new();
    if (!m_sshSession) {
        emit error(QStringLiteral("创建 SSH 会话失败"));
        return false;
    }
    
    ssh_options_set(m_sshSession, SSH_OPTIONS_HOST, m_config.host.toUtf8().constData());
    ssh_options_set(m_sshSession, SSH_OPTIONS_PORT, &m_config.port);
    ssh_options_set(m_sshSession, SSH_OPTIONS_USER, m_config.username.toUtf8().constData());
    int timeout = 10;
    ssh_options_set(m_sshSession, SSH_OPTIONS_TIMEOUT, &timeout);
    
    int rc = ssh_connect(m_sshSession);
    if (rc != SSH_OK) {
        emit error(QStringLiteral("连接 SSH 服务器失败: ") + QString(ssh_get_error(m_sshSession)));
        ssh_free(m_sshSession);
        m_sshSession = nullptr;
        return false;
    }
    
    return true;
}

bool SshChannelSession::authenticate() {
    int rc;
    
    if (m_config.authMethod == SshAuthMethod::Password) {
        if (m_config.password.isEmpty()) {
            emit error(QStringLiteral("密码不能为空"));
            return false;
        }
        
        rc = ssh_userauth_password(m_sshSession, nullptr, m_config.password.toUtf8().constData());
        if (rc != SSH_AUTH_SUCCESS) {
            emit error(QStringLiteral("密码认证失败: ") + QString(ssh_get_error(m_sshSession)));
            return false;
        }
    } else if (m_config.authMethod == SshAuthMethod::PublicKey) {
        if (m_config.privateKeyPath.isEmpty()) {
            emit error(QStringLiteral("私钥路径不能为空"));
            return false;
        }
        
        rc = ssh_userauth_publickey_auto(m_sshSession, nullptr, nullptr);
        if (rc != SSH_AUTH_SUCCESS) {
            emit error(QStringLiteral("公钥认证失败: ") + QString(ssh_get_error(m_sshSession)));
            return false;
        }
    }
    
    return true;
}

bool SshChannelSession::openChannel() {
    m_channel = ssh_channel_new(m_sshSession);
    if (!m_channel) {
        emit error(QStringLiteral("创建 SSH 通道失败"));
        return false;
    }
    
    int rc = ssh_channel_open_session(m_channel);
    if (rc != SSH_OK) {
        emit error(QStringLiteral("打开 SSH 会话失败: ") + QString(ssh_get_error(m_sshSession)));
        return false;
    }
    
    rc = ssh_channel_request_pty(m_channel);
    if (rc != SSH_OK) {
        emit error(QStringLiteral("请求 PTY 失败"));
        return false;
    }
    
    rc = ssh_channel_change_pty_size(m_channel, 80, 24);
    if (rc != SSH_OK) {
        qWarning() << "[SshChannelSession] Failed to set PTY size";
    }
    
    rc = ssh_channel_request_shell(m_channel);
    if (rc != SSH_OK) {
        emit error(QStringLiteral("请求 shell 失败"));
        return false;
    }
    
    m_socketFd = ssh_get_fd(m_sshSession);
    if (m_socketFd >= 0) {
        m_notifier = new QSocketNotifier(m_socketFd, QSocketNotifier::Read, this);
        QObject::connect(m_notifier, &QSocketNotifier::activated, this, &SshChannelSession::onSocketReadyRead);
    }
    
    return true;
}

void SshChannelSession::onSocketReadyRead() {
    readData();
}

void SshChannelSession::onReadTimeout() {
    readData();
}

void SshChannelSession::readData() {
    if (!m_connected || !m_channel) return;
    
    char buffer[4096];
    int nbytes;
    
    while ((nbytes = ssh_channel_read_nonblocking(m_channel, buffer, sizeof(buffer), 0)) > 0) {
        emit dataReceived(QByteArray(buffer, nbytes));
    }
    
    while ((nbytes = ssh_channel_read_nonblocking(m_channel, buffer, sizeof(buffer), 1)) > 0) {
        emit dataReceived(QByteArray(buffer, nbytes));
    }
    
    if (ssh_channel_is_closed(m_channel) || !ssh_channel_is_open(m_channel)) {
        disconnect();
    }
}

void SshChannelSession::cleanup() {
    if (m_channel) {
        ssh_channel_free(m_channel);
        m_channel = nullptr;
    }
    
    if (m_sshSession) {
        ssh_disconnect(m_sshSession);
        ssh_free(m_sshSession);
        m_sshSession = nullptr;
    }
    
    if (m_notifier) {
        delete m_notifier;
        m_notifier = nullptr;
    }
}
