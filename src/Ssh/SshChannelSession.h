#ifndef SSH_CHANNEL_SESSION_H
#define SSH_CHANNEL_SESSION_H

#include <QObject>
#include <QByteArray>
#include <QTimer>
#include <QSocketNotifier>

#include "Ssh/SshConfig.h"

struct ssh_session_struct;
struct ssh_channel_struct;

typedef struct ssh_session_struct* ssh_session;
typedef struct ssh_channel_struct* ssh_channel;

class SshChannelSession : public QObject {
    Q_OBJECT
public:
    explicit SshChannelSession(QObject* parent = nullptr);
    ~SshChannelSession() override;
    
    bool connectToServer(const SshConfig& config);
    void disconnect();
    void write(const QByteArray& data);
    
    bool isConnected() const { return m_connected; }
    QString host() const { return m_config.host; }
    int port() const { return m_config.port; }
    QString username() const { return m_config.username; }
    
signals:
    void dataReceived(const QByteArray& data);
    void connected();
    void disconnected();
    void error(const QString& message);
    
private slots:
    void onSocketReadyRead();
    void onReadTimeout();
    
private:
    bool initializeSession();
    bool authenticate();
    bool openChannel();
    void readData();
    void cleanup();
    
    SshConfig m_config;
    ssh_session m_sshSession;
    ssh_channel m_channel;
    
    QSocketNotifier* m_notifier;
    QTimer* m_readTimer;
    
    bool m_connected;
    int m_socketFd;
};

#endif
