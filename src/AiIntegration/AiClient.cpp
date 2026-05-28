#include "AiClient.h"
#include <QDebug>
#include <QTimer>

AiClient::AiClient(QObject* parent)
    : QObject(parent)
    , m_wsSocket(new QTcpSocket(this))
    , m_httpManager(new QNetworkAccessManager(this))
    , m_timeoutTimer(new QTimer(this)) {
    
    connect(m_wsSocket, &QTcpSocket::connected, this, &AiClient::onWsConnected);
    connect(m_wsSocket, &QTcpSocket::disconnected, this, &AiClient::onWsDisconnected);
    connect(m_wsSocket, &QTcpSocket::readyRead, this, &AiClient::onWsReadyRead);
    connect(m_wsSocket, &QTcpSocket::errorOccurred, this, &AiClient::onWsError);
    
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(TIMEOUT_MS);
    connect(m_timeoutTimer, &QTimer::timeout, this, &AiClient::onTimeout);
}

AiClient::~AiClient() {
    cancel();
}

void AiClient::sendPrompt(const QString& prompt, const QString& context) {
    if (m_isProcessing) return;
    m_isProcessing = true;
    m_timeoutTimer->start();
    
    if (m_config.provider == AiProvider::Ollama) {
        sendViaHttp(prompt, context);
    } else {
        sendViaWebSocket(prompt, context);
    }
}

void AiClient::sendContext(const QString& workingDir, const QStringList& recentCommands) {
    if (m_wsSocket->state() != QTcpSocket::ConnectedState) return;
    
    QJsonObject obj;
    obj["type"] = "context";
    obj["working_dir"] = workingDir;
    QJsonArray cmdArray;
    for (const auto& cmd : recentCommands) {
        cmdArray.append(cmd);
    }
    obj["commands"] = cmdArray;
    
    m_wsSocket->write(QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n");
}

void AiClient::cancel() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply = nullptr;
    }
    m_timeoutTimer->stop();
    m_isProcessing = false;
    m_httpBuffer.clear();
}

void AiClient::sendViaWebSocket(const QString& prompt, const QString& context) {
    if (m_wsSocket->state() != QTcpSocket::ConnectedState) {
        m_wsSocket->connectToHost("127.0.0.1", 8766);
        if (!m_wsSocket->waitForConnected(3000)) {
            emit errorOccurred("WebSocket connection failed");
            m_isProcessing = false;
            return;
        }
    }
    
    QJsonObject obj;
    obj["type"] = "prompt";
    obj["text"] = prompt;
    if (!context.isEmpty()) {
        obj["context"] = context;
    }
    
    m_wsSocket->write(QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n");
}

void AiClient::sendViaHttp(const QString& prompt, const QString& context) {
    QNetworkRequest request(QUrl(m_config.apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_config.apiKey.isEmpty()) {
        request.setRawHeader("Authorization", 
            QString("Bearer %1").arg(m_config.apiKey).toUtf8());
    }
    
    QJsonObject body;
    body["model"] = m_config.model;
    body["messages"] = buildMessages(prompt, context);
    body["temperature"] = m_config.temperature;
    body["max_tokens"] = m_config.maxTokens;
    body["stream"] = m_config.streamResponse;
    
    m_currentReply = m_httpManager->post(request, QJsonDocument(body).toJson());
    connect(m_currentReply, &QNetworkReply::finished, this, &AiClient::onHttpReplyFinished);
    connect(m_currentReply, &QNetworkReply::readyRead, [this]() {
        m_httpBuffer.append(m_currentReply->readAll());
        if (m_config.streamResponse) {
            processStreamResponse(m_httpBuffer);
        }
    });
}

void AiClient::onWsConnected() {
    emit connected();
    qDebug() << "[AiClient] WebSocket connected";
}

void AiClient::onWsDisconnected() {
    emit disconnected();
    qDebug() << "[AiClient] WebSocket disconnected";
}

void AiClient::onWsError(QAbstractSocket::SocketError error) {
    emit errorOccurred(m_wsSocket->errorString());
    m_isProcessing = false;
    m_timeoutTimer->stop();
}

void AiClient::onWsReadyRead() {
    m_wsBuffer.append(m_wsSocket->readAll());
    
    while (true) {
        int nl = m_wsBuffer.indexOf('\n');
        if (nl == -1) break;
        
        QByteArray line = m_wsBuffer.left(nl);
        m_wsBuffer = m_wsBuffer.mid(nl + 1);
        
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("response")) {
                emit responseReceived(obj["response"].toString());
                emit responseFinished();
                m_isProcessing = false;
                m_timeoutTimer->stop();
            } else if (obj.contains("chunk")) {
                emit responseChunk(obj["chunk"].toString());
            } else if (obj.contains("status") && obj["status"] == "done") {
                emit responseFinished();
                m_isProcessing = false;
                m_timeoutTimer->stop();
            }
        }
    }
}

void AiClient::onHttpReplyFinished() {
    if (!m_currentReply) return;
    
    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit errorOccurred(m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        m_isProcessing = false;
        m_timeoutTimer->stop();
        return;
    }
    
    if (!m_config.streamResponse) {
        QByteArray data = m_currentReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("choices")) {
                QJsonArray choices = obj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QString response = choices[0].toObject()["message"].toObject()["content"].toString();
                    emit responseReceived(response);
                }
            }
        }
        emit responseFinished();
    }
    
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    m_isProcessing = false;
    m_timeoutTimer->stop();
}

void AiClient::onTimeout() {
    cancel();
    emit errorOccurred("Request timeout");
}

QString AiClient::buildSystemPrompt(const QString& context) {
    QString prompt = "You are a helpful terminal assistant. Provide concise, accurate answers about command-line usage, system administration, and programming.";
    if (!context.isEmpty()) {
        prompt += "\n\nContext: " + context;
    }
    return prompt;
}

QJsonArray AiClient::buildMessages(const QString& prompt, const QString& context) {
    QJsonArray messages;
    
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = buildSystemPrompt(context);
    messages.append(systemMsg);
    
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = prompt;
    messages.append(userMsg);
    
    return messages;
}

void AiClient::processStreamResponse(const QByteArray& data) {
    QString text = QString::fromUtf8(data);
    int pos = 0;
    
    while (true) {
        int start = text.indexOf("data: ", pos);
        if (start == -1) break;
        start += 6;
        
        int end = text.indexOf("\n", start);
        if (end == -1) break;
        
        QString line = text.mid(start, end - start).trimmed();
        if (line == "[DONE]") {
            emit responseFinished();
            m_isProcessing = false;
            m_timeoutTimer->stop();
            break;
        }
        
        pos = end + 1;
        
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("choices")) {
                QJsonArray choices = obj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject choice = choices[0].toObject();
                    QJsonObject delta = choice["delta"].toObject();
                    if (delta.contains("content")) {
                        emit responseChunk(delta["content"].toString());
                    }
                }
            }
        }
    }
}
