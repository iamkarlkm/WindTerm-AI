#include "ScpClient.h"
#include <libssh/libssh.h>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QThread>

ScpClient::ScpClient(QObject* parent) : QObject(parent) {}

ScpClient::~ScpClient() { disconnect(); }

bool ScpClient::connect(const SshConfig& config) {
    if (m_connected) return true;
    
    m_config = config;
    m_sshSession = ssh_new();
    if (!m_sshSession) {
        emit error("Failed to create SSH session");
        return false;
    }
    
    ssh_options_set(m_sshSession, SSH_OPTIONS_HOST, config.host.toUtf8().constData());
    ssh_options_set(m_sshSession, SSH_OPTIONS_PORT, &config.port);
    ssh_options_set(m_sshSession, SSH_OPTIONS_USER, config.username.toUtf8().constData());
    
    if (ssh_connect(m_sshSession) != SSH_OK) {
        emit error(QString("SSH connection failed: %1").arg(ssh_get_error(m_sshSession)));
        ssh_free(m_sshSession);
        m_sshSession = nullptr;
        return false;
    }
    
    int auth = ssh_userauth_password(m_sshSession, nullptr, config.password.toUtf8().constData());
    if (auth != SSH_AUTH_SUCCESS) {
        if (!config.privateKeyPath.isEmpty()) {
            auth = ssh_userauth_publickey_auto(m_sshSession, nullptr, nullptr);
            if (auth != SSH_AUTH_SUCCESS) {
                emit error("Authentication failed");
                ssh_disconnect(m_sshSession);
                ssh_free(m_sshSession);
                m_sshSession = nullptr;
                return false;
            }
        } else {
            emit error("Authentication failed");
            ssh_disconnect(m_sshSession);
            ssh_free(m_sshSession);
            m_sshSession = nullptr;
            return false;
        }
    }
    
    m_connected = true;
    emit connected();
    return true;
}

void ScpClient::disconnect() {
    if (m_sshSession) {
        ssh_disconnect(m_sshSession);
        ssh_free(m_sshSession);
        m_sshSession = nullptr;
    }
    m_connected = false;
    emit disconnected();
}

bool ScpClient::uploadFile(const QString& localPath, const QString& remotePath) {
    if (!m_connected) {
        emit error("Not connected");
        return false;
    }
    
    QFileInfo info(localPath);
    if (!info.exists()) {
        emit error(QString("Local file not found: %1").arg(localPath));
        return false;
    }
    
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error(QString("Cannot open local file: %1").arg(localPath));
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    return pushFile(localPath, info.fileName(), data.size());
}

bool ScpClient::downloadFile(const QString& remotePath, const QString& localPath) {
    if (!m_connected) {
        emit error("Not connected");
        return false;
    }
    
    return pullFile(remotePath, localPath);
}

bool ScpClient::uploadDirectory(const QString& localPath, const QString& remotePath) {
    QDir dir(localPath);
    if (!dir.exists()) return false;
    
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& entry : entries) {
        if (entry.isDir()) {
            QString remoteDir = remotePath + "/" + entry.fileName();
            uploadDirectory(entry.absoluteFilePath(), remoteDir);
        } else {
            uploadFile(entry.absoluteFilePath(), remotePath + "/" + entry.fileName());
        }
    }
    return true;
}

bool ScpClient::downloadDirectory(const QString& remotePath, const QString& localPath) {
    QDir dir(localPath);
    if (!dir.exists()) {
        dir.mkpath(localPath);
    }
    
    QList<ScpFileInfo> files = listRemoteDirectory(remotePath);
    for (const auto& file : files) {
        QString localFilePath = localPath + "/" + file.name;
        if (file.isDirectory) {
            downloadDirectory(remotePath + "/" + file.name, localFilePath);
        } else {
            downloadFile(remotePath + "/" + file.name, localFilePath);
        }
    }
    return true;
}

QList<ScpFileInfo> ScpClient::listRemoteDirectory(const QString& remotePath) {
    QList<ScpFileInfo> result;
    
    if (!m_connected) return result;
    
    ssh_channel channel = ssh_channel_new(m_sshSession);
    if (!channel) return result;
    
    QString cmd = QString("ls -la \"%1\"").arg(remotePath);
    if (ssh_channel_open_session(channel) != SSH_OK) {
        ssh_channel_free(channel);
        return result;
    }
    
    if (ssh_channel_request_exec(channel, cmd.toUtf8().constData()) != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return result;
    }
    
    QByteArray output;
    char buffer[4096];
    int nbytes;
    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
        output.append(buffer, nbytes);
    }
    
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    
    QStringList lines = QString::fromUtf8(output).split('\n', Qt::SkipEmptyParts);
    for (const auto& line : lines) {
        if (line.startsWith("total") || line.startsWith(".")) continue;
        
        ScpFileInfo info;
        QStringList parts = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 9) {
            info.name = parts.mid(8).join(" ");
            info.size = parts[4].toLongLong();
            info.isDirectory = parts[0].startsWith('d');
        }
        result.append(info);
    }
    
    return result;
}

bool ScpClient::remoteFileExists(const QString& remotePath) {
    if (!m_connected) return false;
    
    ssh_channel channel = ssh_channel_new(m_sshSession);
    if (!channel) return false;
    
    QString cmd = QString("test -e \"%1\" && echo exists").arg(remotePath);
    if (ssh_channel_open_session(channel) != SSH_OK) {
        ssh_channel_free(channel);
        return false;
    }
    
    if (ssh_channel_request_exec(channel, cmd.toUtf8().constData()) != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return false;
    }
    
    char buffer[16];
    int nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    bool exists = (nbytes > 0 && QString::fromUtf8(buffer, nbytes).contains("exists"));
    
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    
    return exists;
}

qint64 ScpClient::remoteFileSize(const QString& remotePath) {
    if (!m_connected) return 0;
    
    ssh_channel channel = ssh_channel_new(m_sshSession);
    if (!channel) return 0;
    
    QString cmd = QString("stat -c%%s \"%1\" 2>/dev/null || wc -c < \"%1\"").arg(remotePath);
    if (ssh_channel_open_session(channel) != SSH_OK) {
        ssh_channel_free(channel);
        return 0;
    }
    
    if (ssh_channel_request_exec(channel, cmd.toUtf8().constData()) != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return 0;
    }
    
    QByteArray output;
    char buffer[4096];
    int nbytes;
    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
        output.append(buffer, nbytes);
    }
    
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    
    return QString::fromUtf8(output).trimmed().toLongLong();
}

bool ScpClient::pushFile(const QString& localPath, const QString& remoteName, qint64 size) {
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QString remotePath = "/tmp/" + remoteName;
    ssh_scp scp = ssh_scp_new(m_sshSession, SSH_SCP_WRITE, remotePath.toUtf8().constData());
    if (!scp) {
        emit error("Failed to create SCP session");
        return false;
    }
    
    if (ssh_scp_init(scp) != SSH_OK) {
        emit error(QString("SCP init failed: %1").arg(ssh_get_error(m_sshSession)));
        ssh_scp_free(scp);
        return false;
    }
    
    int permissions = 0644;
    if (ssh_scp_push_file(scp, remoteName.toUtf8().constData(), size, permissions) != SSH_OK) {
        emit error(QString("SCP push file failed: %1").arg(ssh_get_error(m_sshSession)));
        ssh_scp_free(scp);
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    if (ssh_scp_write(scp, data.constData(), data.size()) != SSH_OK) {
        emit error(QString("SCP write failed: %1").arg(ssh_get_error(m_sshSession)));
        ssh_scp_free(scp);
        return false;
    }
    
    emit transferProgress(size, size);
    ssh_scp_close(scp);
    ssh_scp_free(scp);
    
    return true;
}

bool ScpClient::pullFile(const QString& remoteName, const QString& localPath) {
    ssh_scp scp = ssh_scp_new(m_sshSession, SSH_SCP_READ, remoteName.toUtf8().constData());
    if (!scp) {
        emit error("Failed to create SCP session");
        return false;
    }
    
    if (ssh_scp_init(scp) != SSH_OK) {
        emit error(QString("SCP init failed: %1").arg(ssh_get_error(m_sshSession)));
        ssh_scp_free(scp);
        return false;
    }
    
    if (ssh_scp_accept_request(scp) != SSH_OK) {
        emit error(QString("SCP accept request failed: %1").arg(ssh_get_error(m_sshSession)));
        ssh_scp_free(scp);
        return false;
    }
    
    int fileSize = ssh_scp_request_get_size(scp);
    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error(QString("Cannot open local file: %1").arg(localPath));
        ssh_scp_free(scp);
        return false;
    }
    
    QByteArray buffer;
    buffer.resize(fileSize);
    int totalRead = 0;
    
    while (totalRead < fileSize) {
        int toRead = qMin(4096, fileSize - totalRead);
        int read = ssh_scp_read(scp, buffer.data() + totalRead, toRead);
        if (read == SSH_ERROR) break;
        totalRead += read;
        emit transferProgress(totalRead, fileSize);
    }
    
    file.write(buffer.data(), totalRead);
    file.close();
    
    ssh_scp_close(scp);
    ssh_scp_free(scp);
    
    return (totalRead == fileSize);
}

QString ScpClient::receiveRemoteFileInfo(qint64* size, QString* name) {
    Q_UNUSED(size);
    Q_UNUSED(name);
    return QString();
}
