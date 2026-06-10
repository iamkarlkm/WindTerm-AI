#include "TerminalSession.h"
#include <QUuid>
#include <QDateTime>
#include <QDebug>
#include <QProcess>

TerminalSession::TerminalSession(QObject* parent) : QObject(parent) {
    m_socket = new QTcpSocket(this);
    
    connect(m_socket, &QTcpSocket::connected, this, &TerminalSession::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TerminalSession::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TerminalSession::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TerminalSession::onError);
}

TerminalSession::~TerminalSession() {
    disconnect();
}

void TerminalSession::connectToHost(const QString& host, int port, const QString& username) {
    m_host = host;
    m_port = port;
    m_username = username;
    
    m_socket->connectToHost(host, port);
}

void TerminalSession::connectToLocal(const QString& command) {
    // 本地命令执行（通过 QProcess）
    QProcess* process = new QProcess(this);
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        emit dataReceived(process->readAllStandardOutput());
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        emit dataReceived(process->readAllStandardError());
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this]() {
        emit closed();
    });
    
    process->start("bash", QStringList() << "-c" << command);
    m_connected = true;
    m_connectedTime = QDateTime::currentMSecsSinceEpoch();
    emit connected();
}

void TerminalSession::disconnect() {
    if (m_socket) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }
    m_connected = false;
}

bool TerminalSession::isConnected() const {
    return m_connected && m_socket->state() == QAbstractSocket::ConnectedState;
}

void TerminalSession::sendInput(const QByteArray& data) {
    if (!m_connected) return;
    
    m_socket->write(data);
    m_lastActivityTime = QDateTime::currentMSecsSinceEpoch();
}

void TerminalSession::setTerminalSize(int cols, int rows) {
    m_cols = cols;
    m_rows = rows;
    // TODO: 发送 SSH 终端大小调整请求
}

void TerminalSession::sendSignal(int signal) {
    Q_UNUSED(signal)
    // TODO: 实现信号发送
}

QString TerminalSession::sessionId() const {
    return QString::number(quintptr(this));
}

QString TerminalSession::host() const {
    return m_host;
}

int TerminalSession::port() const {
    return m_port;
}

QString TerminalSession::username() const {
    return m_username;
}

qint64 TerminalSession::connectedTime() const {
    return m_connectedTime;
}

qint64 TerminalSession::lastActivityTime() const {
    return m_lastActivityTime;
}

void TerminalSession::onConnected() {
    m_connected = true;
    m_connectedTime = QDateTime::currentMSecsSinceEpoch();
    m_lastActivityTime = m_connectedTime;
    emit connected();
}

void TerminalSession::onDisconnected() {
    m_connected = false;
    emit closed();
}

void TerminalSession::onReadyRead() {
    QByteArray data = m_socket->readAll();
    m_lastActivityTime = QDateTime::currentMSecsSinceEpoch();
    emit dataReceived(data);
}

void TerminalSession::onError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    emit error(m_socket->errorString());
}

#include "TerminalSession.moc"
