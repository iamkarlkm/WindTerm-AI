#ifndef WEBTERMINALGATEWAY_H
#define WEBTERMINALGATEWAY_H

#include <QObject>
#include <QTcpServer>
#include <QMap>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

class TerminalSession;
class CredentialManager;

/**
 * @brief Web 终端网关 - 通过 WebSocket 提供浏览器终端访问
 * 
 * 功能:
 * - WebSocket 服务器 (默认端口 8080)
 * - 会话管理 (创建/销毁/列表)
 * - 认证机制 (Token 验证)
 * - 多客户端支持
 * - 会话广播
 */
class WebTerminalGateway : public QObject {
    Q_OBJECT

public:
    explicit WebTerminalGateway(QObject* parent = nullptr);
    ~WebTerminalGateway();

    // 服务器控制
    bool start(quint16 port = 8080, bool secure = false);
    void stop();
    bool isRunning() const;
    quint16 port() const;
    bool isSecure() const;

    // 会话管理
    QString createSession(const QString& host, int sshPort = 22, const QString& username = "");
    bool destroySession(const QString& sessionId);
    QStringList listSessions() const;
    
    // 会话信息
    QJsonObject getSessionInfo(const QString& sessionId) const;
    int getSessionCount() const;

    // 认证
    void setAuthToken(const QString& token);
    bool validateToken(const QString& token) const;
    QString generateToken() const;

    // 配置
    void setMaxConnections(int max);
    void setSessionTimeout(int minutes);
    void enableAuthentication(bool enable);

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void clientConnected(const QString& clientId);
    void clientDisconnected(const QString& clientId);
    void sessionCreated(const QString& sessionId);
    void sessionDestroyed(const QString& sessionId);
    void errorOccurred(const QString& message);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& message);
    void onSocketError(QAbstractSocket::SocketError error);
    void onSessionData(const QString& sessionId, const QByteArray& data);
    void onSessionClosed(const QString& sessionId);

private:
    // 消息处理
    void handleHandshake(QWebSocket* socket, const QJsonObject& request);
    void handleAttach(QWebSocket* socket, const QJsonObject& request);
    void handleCreate(QWebSocket* socket, const QJsonObject& request);
    void handleDestroy(QWebSocket* socket, const QJsonObject& request);
    void handleList(QWebSocket* socket);
    void handleResize(QWebSocket* socket, const QJsonObject& request);
    void handleInput(QWebSocket* socket, const QJsonObject& request);
    
    // 响应发送
    void sendResponse(QWebSocket* socket, const QJsonObject& response);
    void sendError(QWebSocket* socket, const QString& error, int code = 500);
    void broadcastToSession(const QString& sessionId, const QByteArray& data);

    // 清理
    void cleanupSession(const QString& sessionId);
    void cleanupExpiredSessions();

    QWebSocketServer* m_webSocketServer = nullptr;
    QMap<QString, QWebSocket*> m_clients;          // clientId -> socket
    QMap<QString, QString> m_clientSessions;        // clientId -> sessionId
    QMap<QString, TerminalSession*> m_sessions;     // sessionId -> session
    QString m_authToken;
    int m_maxConnections = 100;
    int m_sessionTimeout = 60;  // minutes
    bool m_authenticationEnabled = true;
    bool m_secure = false;
    
    // 静态实例
    static WebTerminalGateway* s_instance;

public:
    static WebTerminalGateway* instance();
};

#endif // WEBTERMINALGATEWAY_H
