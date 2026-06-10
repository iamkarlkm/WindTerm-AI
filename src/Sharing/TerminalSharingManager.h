#ifndef TERMINALSHARINGMANAGER_H
#define TERMINALSHARINGMANAGER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QDateTime>
#include <QUuid>

/**
 * @brief 终端共享会话信息
 */
struct SharingSession {
    QString id;
    QString ownerId;
    QString name;
    bool readOnly = false;
    int maxParticipants = 10;
    QDateTime createdAt;
    QDateTime expiresAt;
    QSet<QString> participants;
    QString description;
    bool active = true;
};

/**
 * @brief 参与者信息
 */
struct Participant {
    QString id;
    QString sessionId;
    QString name;
    bool isOwner = false;
    bool canWrite = false;
    QDateTime joinedAt;
    QString role;  // viewer, editor, admin
};

/**
 * @brief 终端共享管理器 - 实时协作功能
 * 
 * 功能:
 * - 创建/加入共享会话
 * - 角色权限管理 (owner/viewer/editor)
 * - 实时同步终端输出
 * - 输入冲突处理
 * - 会话录制与回放
 */
class TerminalSharingManager : public QObject {
    Q_OBJECT

public:
    explicit TerminalSharingManager(QObject* parent = nullptr);
    ~TerminalSharingManager();

    // 会话管理
    QString createSharingSession(const QString& ownerId, const QString& name = "", bool readOnly = false);
    bool joinSession(const QString& sessionId, const QString& participantId);
    bool leaveSession(const QString& sessionId, const QString& participantId);
    bool destroySession(const QString& sessionId);
    
    // 会话信息
    SharingSession getSessionInfo(const QString& sessionId) const;
    QList<SharingSession> getActiveSessions() const;
    QList<Participant> getParticipants(const QString& sessionId) const;
    
    // 权限管理
    void setParticipantRole(const QString& sessionId, const QString& participantId, const QString& role);
    void setSessionReadOnly(const QString& sessionId, bool readOnly);
    void setMaxParticipants(const QString& sessionId, int max);
    
    // 数据同步
    void broadcastOutput(const QString& sessionId, const QByteArray& data);
    void forwardInput(const QString& sessionId, const QString& fromParticipant, const QByteArray& data);
    
    // 邀请管理
    QString generateInviteLink(const QString& sessionId);
    QString parseInviteLink(const QString& link) const;
    bool revokeInvite(const QString& sessionId);
    
    // 录制功能
    void startRecording(const QString& sessionId);
    void stopRecording(const QString& sessionId);
    QString getRecordingPath(const QString& sessionId) const;
    
    // 统计信息
    int getActiveSessionCount() const;
    int getTotalParticipantCount() const;
    QJsonObject getSessionStats(const QString& sessionId) const;

signals:
    void sessionCreated(const QString& sessionId);
    void sessionDestroyed(const QString& sessionId);
    void participantJoined(const QString& sessionId, const QString& participantId);
    void participantLeft(const QString& sessionId, const QString& participantId);
    void roleChanged(const QString& sessionId, const QString& participantId, const QString& newRole);
    void outputBroadcasted(const QString& sessionId, const QByteArray& data);
    void inputReceived(const QString& sessionId, const QString& fromParticipant, const QByteArray& data);
    void recordingStarted(const QString& sessionId);
    void recordingStopped(const QString& sessionId);
    void sessionExpired(const QString& sessionId);

private:
    void cleanupExpiredSessions();
    QString generateSessionId() const;
    bool validateSession(const QString& sessionId) const;
    
    QMap<QString, SharingSession> m_sessions;
    QMap<QString, QList<Participant>> m_sessionParticipants;
    QMap<QString, QString> m_participantSessions;  // participantId -> sessionId
    QMap<QString, bool> m_recordingSessions;
    
    int m_defaultMaxParticipants = 10;
    int m_sessionExpirationMinutes = 120;
    
    static TerminalSharingManager* s_instance;

public:
    static TerminalSharingManager* instance();
};

#endif // TERMINALSHARINGMANAGER_H
