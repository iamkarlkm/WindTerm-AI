#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMap>
#include <QString>
#include <QDateTime>
#include <QJsonObject>

enum class ConnectionType { SSH = 0, Telnet, Rlogin, Serial, TCP, UDP, LocalShell };

struct SessionState {
    int id;
    QString sessionId;
    QString sessionName;
    ConnectionType connectionType;
    QString host;
    int port;
    QString username;
    QString workingDirectory;
    QString lastCommand;
    QString lastSuccessfulCommand;
    QStringList commandHistory;
    QMap<QString, QString> environment;
    QString shellType;
    qint64 processId;
    QDateTime createdAt;
    QDateTime lastActiveAt;
    QString autoRestoreCommand;
};

class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject* parent = nullptr);
    bool initialize();
    void shutdown();
    
    QString createSession(const QString& name, ConnectionType type, const QString& host, int port, const QString& username);
    void updateSessionState(const QString& sessionId, const QString& workingDir, const QString& lastCmd);
    void markCommandSuccess(const QString& sessionId, const QString& command);
    void appendCommand(const QString& sessionId, const QString& command);
    void setEnvironment(const QString& sessionId, const QMap<QString, QString>& env);
    void setShellType(const QString& sessionId, const QString& shell);
    void closeSession(const QString& sessionId);
    
    QList<SessionState> getAllSessions();
    SessionState getSession(const QString& sessionId);
    QList<SessionState> getRecentSessions(int limit = 20);
    QList<SessionState> getActiveSessions();
    void deleteSession(const QString& sessionId);
    void cleanupOldSessions(int days = 7);
    
    QJsonObject getRestoreData(const QString& sessionId);
    QString generateRestoreScript(const QString& sessionId);
    
signals:
    void sessionReadyToRestore(const QString& sessionId, const QString& workingDir, const QString& command);
    
private:
    void createTables();
    QString generateSessionId();
    QString connectionTypeToString(ConnectionType type);
    ConnectionType stringToConnectionType(const QString& str);
    
    QSqlDatabase m_db;
};

#endif
