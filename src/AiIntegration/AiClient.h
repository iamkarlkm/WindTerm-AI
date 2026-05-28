#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "AiConfig.h"

class AiClient : public QObject {
    Q_OBJECT
public:
    explicit AiClient(QObject* parent = nullptr);
    ~AiClient() override;
    
    void setConfig(const AiConfig& config) { m_config = config; }
    AiConfig config() const { return m_config; }
    
    void sendPrompt(const QString& prompt, const QString& context = QString());
    void sendContext(const QString& workingDir, const QStringList& recentCommands);
    void cancel();
    
    bool isProcessing() const { return m_isProcessing; }
    
signals:
    void responseReceived(const QString& response);
    void responseChunk(const QString& chunk);
    void responseFinished();
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

private slots:
    void onWsConnected();
    void onWsDisconnected();
    void onWsReadyRead();
    void onWsError(QAbstractSocket::SocketError error);
    void onHttpReplyFinished();
    void onTimeout();

private:
    void sendViaWebSocket(const QString& prompt, const QString& context);
    void sendViaHttp(const QString& prompt, const QString& context);
    QString buildSystemPrompt(const QString& context);
    QJsonArray buildMessages(const QString& prompt, const QString& context);
    void processStreamResponse(const QByteArray& data);
    
    QTcpSocket* m_wsSocket;
    QNetworkAccessManager* m_httpManager;
    QNetworkReply* m_currentReply = nullptr;
    QByteArray m_wsBuffer;
    QByteArray m_httpBuffer;
    AiConfig m_config;
    bool m_isProcessing = false;
    QTimer* m_timeoutTimer;
    static constexpr int TIMEOUT_MS = 30000;
};

#endif
