#ifndef SCP_CLIENT_H
#define SCP_CLIENT_H

#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QByteArray>
#include "Ssh/SshConfig.h"

struct ssh_session_struct;
struct ssh_channel_struct;

typedef struct ssh_session_struct* ssh_session;
typedef struct ssh_channel_struct* ssh_channel;

enum class ScpDirection {
    Upload,
    Download
};

struct ScpFileInfo {
    QString name;
    QString path;
    qint64 size = 0;
    qint64 transferred = 0;
    bool isDirectory = false;
};

class ScpClient : public QObject {
    Q_OBJECT
public:
    explicit ScpClient(QObject* parent = nullptr);
    ~ScpClient() override;
    
    bool connect(const SshConfig& config);
    void disconnect();
    bool isConnected() const { return m_connected; }
    
    bool uploadFile(const QString& localPath, const QString& remotePath);
    bool downloadFile(const QString& remotePath, const QString& localPath);
    bool uploadDirectory(const QString& localPath, const QString& remotePath);
    bool downloadDirectory(const QString& remotePath, const QString& localPath);
    
    QList<ScpFileInfo> listRemoteDirectory(const QString& remotePath);
    bool remoteFileExists(const QString& remotePath);
    qint64 remoteFileSize(const QString& remotePath);

signals:
    void transferProgress(qint64 transferred, qint64 total);
    void transferStarted(const QString& fileName);
    void transferFinished(const QString& fileName, bool success);
    void error(const QString& message);
    void connected();
    void disconnected();

private:
    bool pushFile(const QString& localPath, const QString& remoteName, qint64 size);
    bool pullFile(const QString& remoteName, const QString& localPath);
    QString receiveRemoteFileInfo(qint64* size, QString* name);
    
    ssh_session m_sshSession = nullptr;
    bool m_connected = false;
    SshConfig m_config;
    bool m_cancelRequested = false;
};

#endif
