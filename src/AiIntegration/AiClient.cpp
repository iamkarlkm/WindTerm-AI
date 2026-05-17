#include "AiClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

AiClient::AiClient(QObject* parent) : QObject(parent), m_socket(new QTcpSocket(this)) {
    connect(m_socket, &QTcpSocket::connected, this, &AiClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &AiClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &AiClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, &AiClient::onError);
}
void AiClient::connectToServer(const QString& host, quint16 port) { m_socket->connectToHost(host, port); }
void AiClient::sendPrompt(const QString& prompt) {
    if (m_socket->state() != QTcpSocket::ConnectedState) return;
    QJsonObject obj{{"type", "prompt"}, {"text", prompt}};
    m_socket->write(QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n");
}
void AiClient::sendContext(const QString& workingDir, const QString& recentCommands) {
    if (m_socket->state() != QTcpSocket::ConnectedState) return;
    QJsonObject obj{{"type", "context"}, {"working_dir", workingDir}, {"commands", recentCommands}};
    m_socket->write(QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n");
}
void AiClient::onConnected() { emit connected(); qDebug() << "AI Server connected"; }
void AiClient::onDisconnected() { emit disconnected(); qDebug() << "AI Server disconnected"; }
void AiClient::onError(QAbstractSocket::SocketError error) { emit errorOccurred(m_socket->errorString()); Q_UNUSED(error); }
void AiClient::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    while (true) {
        int nl = m_buffer.indexOf('\n');
        if (nl == -1) break;
        QByteArray line = m_buffer.left(nl); m_buffer = m_buffer.mid(nl + 1);
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            emit responseReceived(doc.object()["response"].toString());
        }
    }
}
