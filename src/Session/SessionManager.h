#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <QTimer>
#include <QObject>
#include <QMap>
#include <QJsonDocument>
#include <QDateTime>

struct SessionData {
    QString id;
    QString name;
    QString type;
    QString host;
    int port;
    QString username;
    QString workingDirectory;
    QString shell;
    QString lastCommand;
    QStringList commandHistory;
    QByteArray bufferContent;
    int bufferRows;
    int bufferCols;
    QDateTime createdAt;
    QDateTime lastActiveAt;
    bool isActive;
    
    SessionData() : port(22), bufferRows(24), bufferCols(80), isActive(false) {}
    
    QJsonObject toJson() const;
    static SessionData fromJson(const QJsonObject& json);
};

class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject* parent = nullptr);
    
    static SessionManager* instance();
    
    QString createSession(const QString& name, const QString& type = "local");
    void saveSession(const QString& sessionId);
    void loadSession(const QString& sessionId);
    void closeSession(const QString& sessionId);
    void switchToSession(const QString& sessionId);
    
    SessionData getSession(const QString& sessionId) const;
    QStringList listSessions() const;
    QStringList listActiveSessions() const;
    QString getCurrentSession() const { return m_currentSessionId; }
    
    void setAutoSaveEnabled(bool enabled);
    void setAutoSaveInterval(int seconds);
    
    void exportSessions(const QString& filePath);
    void importSessions(const QString& filePath);
    
    void cleanupOldSessions(int daysToKeep);
    void clearAllSessions();
    
signals:
    void sessionCreated(const QString& sessionId);
    void sessionLoaded(const QString& sessionId);
    void sessionClosed(const QString& sessionId);
    void sessionSwitched(const QString& sessionId);
    void autoSaveTriggered();

private:
    QString generateSessionId();
    void saveToDisk(const QString& sessionId);
    void loadFromDisk(const QString& sessionId);
    
    static SessionManager* s_instance;
    
    QMap<QString, SessionData> m_sessions;
    QString m_currentSessionId;
    QString m_sessionDir;
    
    bool m_autoSaveEnabled;
    int m_autoSaveInterval;
    QTimer* m_autoSaveTimer;
};

#endif
