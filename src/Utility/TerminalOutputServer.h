#ifndef WINDTERM_OUTPUT_SERVER_H
#define WINDTERM_OUTPUT_SERVER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QRegularExpression>
#include <QMutex>
#include <QMutexLocker>
#include <QMap>
#include <QJsonObject>

class QWebSocketServer;
class QWebSocket;

struct ServerSubscription {
    QString name;
    QRegularExpression regex;

    ServerSubscription() = default;
    ServerSubscription(const QString &n, const QRegularExpression &r)
        : name(n), regex(r) {}
};

struct ClientSession {
    QWebSocket *socket = nullptr;
    QVector<ServerSubscription> subscriptions;
    QString remoteId;

    ClientSession() = default;
    explicit ClientSession(QWebSocket *s) : socket(s) {}
};

class TerminalOutputServer : public QObject {
    Q_OBJECT
public:
    static TerminalOutputServer &instance();

    bool start(quint16 port);
    void stop();
    bool isRunning() const;

    void loadMacros(const QJsonObject &macrosObj);
    void registerFunction(const QString &name, const QByteArray &code);

    Q_SLOT void feedLine(const QString &line);

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void clientConnected(const QString &remoteId);
    void clientDisconnected(const QString &remoteId);
    void subscriptionAdded(const QString &remoteId, const QString &name);
    void subscriptionRemoved(const QString &remoteId, const QString &name);
    void pushMessage(const QString &remoteId, const QString &name, const QString &text);
    void commandRequested(const QString &command);
    void rawBytesRequested(const QByteArray &data);
    void signalRequested(int signal);

private:
    explicit TerminalOutputServer(QObject *parent = nullptr);
    ~TerminalOutputServer() override;

    TerminalOutputServer(const TerminalOutputServer &) = delete;
    TerminalOutputServer &operator=(const TerminalOutputServer &) = delete;

    void onNewConnection();
    void onTextMessageReceived(QWebSocket *socket, const QString &message);
    void onClientDisconnected(QWebSocket *socket);

    ClientSession *findSession(QWebSocket *socket);
    void sendJson(QWebSocket *socket, const QJsonObject &obj);
    void pushToClient(ClientSession &session, const ServerSubscription &sub,
                      const QString &matchedText);
    void handleExec(QWebSocket *socket, const QJsonObject &obj);
    void handleMacro(QWebSocket *socket, const QJsonObject &obj);
    void handleCall(QWebSocket *socket, const QJsonObject &obj);
    void registerBuiltinFunctions();

    QWebSocketServer *m_server;
    QVector<ClientSession> m_sessions;
    mutable QMutex m_mutex;
    bool m_running;

    QMap<QString, QString> m_macros;
    QMap<QString, QByteArray> m_functions;
    QMap<QString, QString> m_commandFunctions;
    bool m_builtinsRegistered;
};

#endif // WINDTERM_OUTPUT_SERVER_H
