#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMap>
#include <QString>
#include <QDateTime>

struct SessionInfo {
    int id;
    QString sessionId;
    QString host;
    int port;
    QString protocol;
    QString workingDirectory;
    QString lastCommand;
    QString lastSuccessfulCommand;
    QMap<QString, QString> environment;
    QDateTime createdAt;
    QDateTime lastActiveAt;
    qint64 processId;
};

class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject* parent = nullptr);
    bool initialize();
    void shutdown();
    
    void saveSession(const QString& sessionId, const QString& host, int port, const QString& protocol);
    void updateSession(const QString& sessionId, const QString& workingDir, const QString& command);
    void markCommandSuccess(const QString& sessionId, const QString& command);
    void setEnvironment(const QString& sessionId, const QMap<QString, QString>& env);
    void closeSession(const QString& sessionId);
    
    QList<SessionInfo> getAllSessions();
    SessionInfo getSession(const QString& sessionId);
    QList<SessionInfo> getRecentSessions(int limit = 20);
    void deleteSession(const QString& sessionId);
    void cleanupOldSessions(int days = 7);
    
private:
    void createTables();
    QString generateSessionId();
    
    QSqlDatabase m_db;
};

#endif
