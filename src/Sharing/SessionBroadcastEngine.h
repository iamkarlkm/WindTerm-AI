#ifndef SESSIONBROADCASTENGINE_H
#define SESSIONBROADCASTENGINE_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QQueue>
#include <QByteArray>

/**
 * @brief 广播会话配置
 */
struct BroadcastConfig {
    QString sessionId;
    int maxViewers = 1000;
    int latencyMs = 500;
    bool enableChat = true;
    bool enableRecording = true;
    QString streamTitle;
    QString streamDescription;
    bool isPublic = false;
};

/**
 * @brief 观看者信息
 */
struct Viewer {
    QString id;
    QString sessionId;
    QDateTime joinedAt;
    QString location;
    bool isMuted = false;
};

/**
 * @brief 聊天消息
 */
struct ChatMessage {
    QString from;
    QString message;
    QDateTime timestamp;
    bool isSystem = false;
};

/**
 * @brief 会话广播引擎 - 大规模终端直播
 * 
 * 功能:
 * - 大规模观看者支持 (1000+)
 * - 低延迟广播
 * - 实时聊天室
 * - 观看者统计
 * - 弹幕支持
 */
class SessionBroadcastEngine : public QObject {
    Q_OBJECT

public:
    explicit SessionBroadcastEngine(QObject* parent = nullptr);
    ~SessionBroadcastEngine();

    // 广播控制
    QString startBroadcast(const QString& sessionId, const BroadcastConfig& config);
    bool stopBroadcast(const QString& broadcastId);
    bool pauseBroadcast(const QString& broadcastId);
    bool resumeBroadcast(const QString& broadcastId);
    
    // 观看者管理
    bool joinViewer(const QString& broadcastId, const QString& viewerId);
    bool leaveViewer(const QString& broadcastId, const QString& viewerId);
    int getViewerCount(const QString& broadcastId) const;
    QList<Viewer> getViewerList(const QString& broadcastId) const;
    
    // 数据广播
    void broadcastTerminalOutput(const QString& broadcastId, const QByteArray& data);
    void broadcastCursorMove(const QString& broadcastId, int row, int col);
    void broadcastResize(const QString& broadcastId, int cols, int rows);
    
    // 聊天功能
    void sendChatMessage(const QString& broadcastId, const QString& from, const QString& message);
    QList<ChatMessage> getChatHistory(const QString& broadcastId, int limit = 50) const;
    void clearChat(const QString& broadcastId);
    
    // 配置管理
    void updateConfig(const QString& broadcastId, const BroadcastConfig& config);
    BroadcastConfig getConfig(const QString& broadcastId) const;
    
    // 统计信息
    QJsonObject getBroadcastStats(const QString& broadcastId) const;
    QList<QString> getActiveBroadcasts() const;

signals:
    void broadcastStarted(const QString& broadcastId);
    void broadcastStopped(const QString& broadcastId);
    void broadcastPaused(const QString& broadcastId);
    void broadcastResumed(const QString& broadcastId);
    void viewerJoined(const QString& broadcastId, const QString& viewerId);
    void viewerLeft(const QString& broadcastId, const QString& viewerId);
    void dataBroadcasted(const QString& broadcastId, const QByteArray& data);
    void chatMessageReceived(const QString& broadcastId, const ChatMessage& message);
    void maxViewersReached(const QString& broadcastId);

private:
    void cleanupInactiveBroadcasts();
    QString generateBroadcastId() const;
    
    QMap<QString, BroadcastConfig> m_broadcasts;
    QMap<QString, QSet<QString>> m_broadcastViewers;  // broadcastId -> viewerIds
    QMap<QString, QList<Viewer>> m_viewerLists;
    QMap<QString, QList<ChatMessage>> m_chatHistories;
    QMap<QString, QByteArray> m_outputBuffers;
    
    int m_maxConcurrentBroadcasts = 10;
    int m_chatHistoryLimit = 100;
    
    static SessionBroadcastEngine* s_instance;

public:
    static SessionBroadcastEngine* instance();
};

#endif // SESSIONBROADCASTENGINE_H
