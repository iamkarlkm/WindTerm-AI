#ifndef AI_CLIENT_H
#define AI_CLIENT_H
#include <QObject>
#include <QTcpSocket>
#include <QString>

class AiClient : public QObject {
    Q_OBJECT
public:
    explicit AiClient(QObject* parent = nullptr);
    void connectToServer(const QString& host = "127.0.0.1", quint16 port = 8766);
    void sendPrompt(const QString& prompt);
    void sendContext(const QString& workingDir, const QString& recentCommands);
signals:
    void responseReceived(const QString& response);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
private:
    QTcpSocket* m_socket;
    QByteArray m_buffer;
};
#endif
