#include "WebTerminalGateway.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QUuid>
#include <QTimer>
#include <QCryptographicHash>
#include <QDebug>

WebTerminalGateway* WebTerminalGateway::s_instance = nullptr;

WebTerminalGateway::WebTerminalGateway(QObject* parent) : QObject(parent) {
    m_webSocketServer = new QWebSocketServer("WindTerm Web Gateway", QWebSocketServer::NonSecureMode, this);
    connect(m_webSocketServer, &QWebSocketServer::newConnection, this, &WebTerminalGateway::onNewConnection);
    connect(m_webSocketServer, &QWebSocketServer::serverError, this, [this](QWebSocketProtocol::CloseCode code) {
        Q_UNUSED(code)
        emit errorOccurred(m_webSocketServer->errorString());
    });
    // 定期清理过期会话
    QTimer* cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, &WebTerminalGateway::cleanupExpiredSessions);
    cleanupTimer->start(60000);  // 每分钟检查
}

WebTerminalGateway::~WebTerminalGateway() {
    stop();
}

WebTerminalGateway* WebTerminalGateway::instance() {
    if (!s_instance) s_instance = new WebTerminalGateway();
    return s_instance;
}

bool WebTerminalGateway::start(quint16 port, bool secure) {
    // 如果需要 WSS，需要设置 SSL 配置
    if (secure) {
        m_webSocketServer->deleteLater();
        m_webSocketServer = new QWebSocketServer("WindTerm Web Gateway (Secure)", QWebSocketServer::SecureMode, this);
        // SSL 配置需要在应用层面设置
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        m_webSocketServer->setSslConfiguration(sslConfig);
        connect(m_webSocketServer, &QWebSocketServer::newConnection, this, &WebTerminalGateway::onNewConnection);
    }
    
    if (m_webSocketServer->listen(QHostAddress::Any, port)) {
        m_secure = secure;
        emit serverStarted(port);
        return true;
    }
    emit errorOccurred(m_webSocketServer->errorString());
    return false;
}

void WebTerminalGateway::stop() {
    // 关闭所有客户端连接
    for (auto socket : m_clients) {
        socket->close();
    }
    m_clients.clear();
    
    // 销毁所有会话
    for (auto session : m_sessions) {
        delete session;
    }
    m_sessions.clear();
    m_clientSessions.clear();
    
    m_webSocketServer->close();
    emit serverStopped();
}

bool WebTerminalGateway::isRunning() const {
    return m_webSocketServer->isListening();
}

quint16 WebTerminalGateway::port() const {
    return m_webSocketServer->serverPort();
}

bool WebTerminalGateway::isSecure() const {
    return m_secure;
}

QString WebTerminalGateway::createSession(const QString& host, int sshPort, const QString& username) {
    QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    TerminalSession* session = new TerminalSession(this);
    session->connectToHost(host, sshPort, username);
    
    m_sessions[sessionId] = session;
    connect(session, &TerminalSession::dataReceived, this, [this, sessionId](const QByteArray& data) {
        onSessionData(sessionId, data);
    });
    connect(session, &TerminalSession::closed, this, [this, sessionId]() {
        onSessionClosed(sessionId);
    });
    
    emit sessionCreated(sessionId);
    return sessionId;
}

bool WebTerminalGateway::destroySession(const QString& sessionId) {
    if (!m_sessions.contains(sessionId)) {
        return false;
    }
    
    cleanupSession(sessionId);
    return true;
}

QStringList WebTerminalGateway::listSessions() const {
    return m_sessions.keys();
}

QJsonObject WebTerminalGateway::getSessionInfo(const QString& sessionId) const {
    if (!m_sessions.contains(sessionId)) {
        return QJsonObject();
    }
    
    QJsonObject info;
    info["sessionId"] = sessionId;
    info["connected"] = m_sessions[sessionId]->isConnected();
    // 其他会话信息...
    return info;
}

int WebTerminalGateway::getSessionCount() const {
    return m_sessions.count();
}

void WebTerminalGateway::setAuthToken(const QString& token) {
    m_authToken = token;
}

bool WebTerminalGateway::validateToken(const QString& token) const {
    if (!m_authenticationEnabled) return true;
    return token == m_authToken;
}

QString WebTerminalGateway::generateToken() const {
    QByteArray random = QUuid::createUuid().toRfc4122();
    return QCryptographicHash::hash(random, QCryptographicHash::Sha256).toHex();
}

void WebTerminalGateway::setMaxConnections(int max) {
    m_maxConnections = max;
}

void WebTerminalGateway::setSessionTimeout(int minutes) {
    m_sessionTimeout = minutes;
}

void WebTerminalGateway::enableAuthentication(bool enable) {
    m_authenticationEnabled = enable;
}

void WebTerminalGateway::onNewConnection() {
    if (m_clients.size() >= m_maxConnections) {
        qDebug() << "Max connections reached";
        return;
    }
    
    QWebSocket* socket = m_webSocketServer->nextPendingConnection();
    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    m_clients[clientId] = socket;
    
    connect(socket, &QWebSocket::textMessageReceived, this, &WebTerminalGateway::onTextMessageReceived);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &WebTerminalGateway::onBinaryMessageReceived);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &WebTerminalGateway::onSocketError);
    connect(socket, &QWebSocket::disconnected, this, [this, clientId, socket]() {
        m_clients.remove(clientId);
        m_clientSessions.remove(clientId);
        socket->deleteLater();
        emit clientDisconnected(clientId);
    });
    
    emit clientConnected(clientId);
}

void WebTerminalGateway::onTextMessageReceived(const QString& message) {
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;
    
    QString clientId = m_clients.key(socket);
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull()) {
        sendError(socket, "Invalid JSON", 400);
        return;
    }
    
    QJsonObject request = doc.object();
    QString action = request["action"].toString();
    
    if (action == "handshake") {
        handleHandshake(socket, request);
    } else if (action == "create") {
        handleCreate(socket, request);
    } else if (action == "attach") {
        handleAttach(socket, request);
    } else if (action == "destroy") {
        handleDestroy(socket, request);
    } else if (action == "list") {
        handleList(socket);
    } else if (action == "resize") {
        handleResize(socket, request);
    } else if (action == "input") {
        handleInput(socket, request);
    } else if (action == "ping") {
        // 心跳响应
        QJsonObject pong;
        pong["type"] = "pong";
        sendResponse(socket, pong);
    } else {
        sendError(socket, "Unknown action: " + action, 400);
    }
}

void WebTerminalGateway::onBinaryMessageReceived(const QByteArray& message) {
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;
    
    QString clientId = m_clients.key(socket);
    QString sessionId = m_clientSessions.value(clientId);
    
    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) {
        sendError(socket, "No session attached", 400);
        return;
    }
    
    // 转发到终端会话
    m_sessions[sessionId]->sendInput(message);
}

void WebTerminalGateway::onSocketError(QAbstractSocket::SocketError error) {
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;
    
    qDebug() << "WebSocket error:" << error;
    emit errorOccurred(socket->errorString());
}

void WebTerminalGateway::onSessionData(const QString& sessionId, const QByteArray& data) {
    broadcastToSession(sessionId, data);
}

void WebTerminalGateway::onSessionClosed(const QString& sessionId) {
    // 通知所有连接的客户端
    for (auto it = m_clientSessions.begin(); it != m_clientSessions.end(); ) {
        if (it.value() == sessionId) {
            QWebSocket* socket = m_clients.value(it.key());
            if (socket) {
                QJsonObject response;
                response["type"] = "session_closed";
                response["sessionId"] = sessionId;
                sendResponse(socket, response);
            }
            it = m_clientSessions.erase(it);
        } else {
            ++it;
        }
    }
    
    cleanupSession(sessionId);
}

void WebTerminalGateway::handleHandshake(QWebSocket* socket, const QJsonObject& request) {
    QString token = request["token"].toString();
    
    if (!validateToken(token)) {
        sendError(socket, "Authentication failed", 401);
        return;
    }
    
    QJsonObject response;
    response["type"] = "handshake_ok";
    response["version"] = "1.0";
    response["capabilities"] = QJsonArray{"create", "attach", "destroy", "list", "resize", "input"};
    sendResponse(socket, response);
}

void WebTerminalGateway::handleCreate(QWebSocket* socket, const QJsonObject& request) {
    QString host = request["host"].toString();
    int port = request["port"].toInt(22);
    QString username = request["username"].toString();
    
    if (host.isEmpty()) {
        sendError(socket, "Host is required", 400);
        return;
    }
    
    QString sessionId = createSession(host, port, username);
    
    QJsonObject response;
    response["type"] = "session_created";
    response["sessionId"] = sessionId;
    response["host"] = host;
    sendResponse(socket, response);
    
    // 绑定客户端到会话
    QString clientId = m_clients.key(socket);
    m_clientSessions[clientId] = sessionId;
}

void WebTerminalGateway::handleAttach(QWebSocket* socket, const QJsonObject& request) {
    QString sessionId = request["sessionId"].toString();
    
    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) {
        sendError(socket, "Session not found", 404);
        return;
    }
    
    QString clientId = m_clients.key(socket);
    m_clientSessions[clientId] = sessionId;
    
    QJsonObject response;
    response["type"] = "attached";
    response["sessionId"] = sessionId;
    sendResponse(socket, response);
}

void WebTerminalGateway::handleDestroy(QWebSocket* socket, const QJsonObject& request) {
    QString sessionId = request["sessionId"].toString();
    
    if (sessionId.isEmpty()) {
        sendError(socket, "Session ID required", 400);
        return;
    }
    
    if (!m_sessions.contains(sessionId)) {
        sendError(socket, "Session not found", 404);
        return;
    }
    
    destroySession(sessionId);
    
    QJsonObject response;
    response["type"] = "session_destroyed";
    response["sessionId"] = sessionId;
    sendResponse(socket, response);
}

void WebTerminalGateway::handleList(QWebSocket* socket) {
    QJsonObject response;
    response["type"] = "session_list";
    
    QJsonArray sessions;
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        QJsonObject sessionInfo = getSessionInfo(it.key());
        sessions.append(sessionInfo);
    }
    response["sessions"] = sessions;
    
    sendResponse(socket, response);
}

void WebTerminalGateway::handleResize(QWebSocket* socket, const QJsonObject& request) {
    QString sessionId = request["sessionId"].toString();
    int cols = request["cols"].toInt(80);
    int rows = request["rows"].toInt(24);
    
    if (!m_sessions.contains(sessionId)) {
        sendError(socket, "Session not found", 404);
        return;
    }
    
    m_sessions[sessionId]->setTerminalSize(cols, rows);
    
    QJsonObject response;
    response["type"] = "resized";
    response["sessionId"] = sessionId;
    response["cols"] = cols;
    response["rows"] = rows;
    sendResponse(socket, response);
}

void WebTerminalGateway::handleInput(QWebSocket* socket, const QJsonObject& request) {
    QString sessionId = request["sessionId"].toString();
    QString input = request["input"].toString();
    
    if (!m_sessions.contains(sessionId)) {
        sendError(socket, "Session not found", 404);
        return;
    }
    
    m_sessions[sessionId]->sendInput(input.toUtf8());
    
    QJsonObject response;
    response["type"] = "input_sent";
    response["sessionId"] = sessionId;
    sendResponse(socket, response);
}

void WebTerminalGateway::sendResponse(QWebSocket* socket, const QJsonObject& response) {
    QJsonDocument doc(response);
    socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void WebTerminalGateway::sendError(QWebSocket* socket, const QString& error, int code) {
    QJsonObject response;
    response["type"] = "error";
    response["error"] = error;
    response["code"] = code;
    sendResponse(socket, response);
}

void WebTerminalGateway::broadcastToSession(const QString& sessionId, const QByteArray& data) {
    // 转换为 Base64 以便 JSON 传输
    QString base64Data = data.toBase64();
    
    for (auto it = m_clientSessions.begin(); it != m_clientSessions.end(); ++it) {
        if (it.value() == sessionId) {
            QWebSocket* socket = m_clients.value(it.key());
            if (socket && socket->isValid()) {
                QJsonObject response;
                response["type"] = "data";
                response["sessionId"] = sessionId;
                response["data"] = base64Data;
                sendResponse(socket, response);
            }
        }
    }
}

void WebTerminalGateway::cleanupSession(const QString& sessionId) {
    // 断开所有连接的客户端
    for (auto it = m_clientSessions.begin(); it != m_clientSessions.end(); ) {
        if (it.value() == sessionId) {
            QWebSocket* socket = m_clients.value(it.key());
            if (socket) {
                QJsonObject response;
                response["type"] = "session_closed";
                response["sessionId"] = sessionId;
                sendResponse(socket, response);
                socket->close();
            }
            it = m_clientSessions.erase(it);
        } else {
            ++it;
        }
    }
    
    // 删除会话
    TerminalSession* session = m_sessions.take(sessionId);
    if (session) {
        session->deleteLater();
    }
    
    emit sessionDestroyed(sessionId);
}

void WebTerminalGateway::cleanupExpiredSessions() {
    // 实现会话超时清理逻辑
    // 这里简化处理，实际需要根据最后活动时间判断
}

#include "WebTerminalGateway.moc"
