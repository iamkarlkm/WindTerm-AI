#ifndef TERMINALSESSION_H
#define TERMINALSESSION_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QSshSocket>

class TerminalSession : public QObject {
    Q_OBJECT

public:
    explicit TerminalSession(QObject* parent = nullptr);
    ~TerminalSession();

    // 连接管理
    void connectToHost(const QString& host, int port = 22, const QString& username = "");
    void connectToLocal(const QString& command);
    void disconnect();
    bool isConnected() const;

    // 终端控制
    void sendInput(const QByteArray& data);
    void setTerminalSize(int cols, int rows);
    void sendSignal(int signal);  // SIGINT, SIGTERM, etc.

    // 会话信息
    QString sessionId() const;
    QString host() const;
    int port() const;
    QString username() const;
    qint64 connectedTime() const;
    qint64 lastActivityTime() const;

signals:
    void connected();
    void closed();
    void dataReceived(const QByteArray& data);
    void error(const QString& message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

private:
    void initializeSession();
    
    QTcpSocket* m_socket = nullptr;
    QString m_host;
    int m_port = 22;
    QString m_username;
    qint64 m_connectedTime = 0;
    qint64 m_lastActivityTime = 0;
    int m_cols = 80;
    int m_rows = 24;
    bool m_connected = false;
};

#endif // TERMINALSESSION_H
