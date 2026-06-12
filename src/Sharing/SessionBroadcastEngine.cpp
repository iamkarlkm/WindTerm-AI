#include "SessionBroadcastEngine.h"
#include <QUuid>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>

SessionBroadcastEngine* SessionBroadcastEngine::s_instance = nullptr;

SessionBroadcastEngine::SessionBroadcastEngine(QObject* parent) : QObject(parent) {
    // 定期清理不活跃的广播
    QTimer* cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, &SessionBroadcastEngine::cleanupInactiveBroadcasts);
    cleanupTimer->start(300000);  // 5 分钟
}

SessionBroadcastEngine::~SessionBroadcastEngine() {
}

SessionBroadcastEngine* SessionBroadcastEngine::instance() {
    if (!s_instance) s_instance = new SessionBroadcastEngine();
    return s_instance;
}

QString SessionBroadcastEngine::startBroadcast(const QString& sessionId, const BroadcastConfig& config) {
    if (m_broadcasts.size() >= m_maxConcurrentBroadcasts) {
        qDebug() << "Max concurrent broadcasts reached";
        return QString();
    }
    
    QString broadcastId = generateBroadcastId();
    
    BroadcastConfig broadcastConfig = config;
    broadcastConfig.sessionId = sessionId;
    m_broadcasts[broadcastId] = broadcastConfig;
    
    m_broadcastViewers[broadcastId] = QSet<QString>();
    m_viewerLists[broadcastId] = QList<Viewer>();
    m_chatHistories[broadcastId] = QList<ChatMessage>();
    
    // 添加系统消息
    ChatMessage sysMsg;
    sysMsg.isSystem = true;
    sysMsg.message = "Broadcast started: " + config.streamTitle;
    sysMsg.timestamp = QDateTime::currentDateTime();
    m_chatHistories[broadcastId].append(sysMsg);
    
    emit broadcastStarted(broadcastId);
    return broadcastId;
}

bool SessionBroadcastEngine::stopBroadcast(const QString& broadcastId) {
    if (!m_broadcasts.contains(broadcastId)) return false;
    
    // 清理所有数据
    m_broadcasts.remove(broadcastId);
    m_broadcastViewers.remove(broadcastId);
    m_viewerLists.remove(broadcastId);
    m_chatHistories.remove(broadcastId);
    m_outputBuffers.remove(broadcastId);
    
    emit broadcastStopped(broadcastId);
    return true;
}

bool SessionBroadcastEngine::pauseBroadcast(const QString& broadcastId) {
    Q_UNUSED(broadcastId)
    // 实现暂停逻辑
    emit broadcastPaused(broadcastId);
    return true;
}

bool SessionBroadcastEngine::resumeBroadcast(const QString& broadcastId) {
    Q_UNUSED(broadcastId)
    // 实现恢复逻辑
    emit broadcastResumed(broadcastId);
    return true;
}

bool SessionBroadcastEngine::joinViewer(const QString& broadcastId, const QString& viewerId) {
    if (!m_broadcasts.contains(broadcastId)) return false;
    
    const BroadcastConfig& config = m_broadcasts[broadcastId];
    
    // 检查人数限制
    if (m_broadcastViewers[broadcastId].size() >= config.maxViewers) {
        emit maxViewersReached(broadcastId);
        return false;
    }
    
    // 添加观看者
    m_broadcastViewers[broadcastId].insert(viewerId);
    
    Viewer viewer;
    viewer.id = viewerId;
    viewer.sessionId = broadcastId;
    viewer.joinedAt = QDateTime::currentDateTime();
    m_viewerLists[broadcastId].append(viewer);
    
    // 发送欢迎消息
    ChatMessage welcomeMsg;
    welcomeMsg.isSystem = true;
    welcomeMsg.message = "Viewer " + viewerId.left(8) + " joined";
    welcomeMsg.timestamp = QDateTime::currentDateTime();
    m_chatHistories[broadcastId].append(welcomeMsg);
    
    emit viewerJoined(broadcastId, viewerId);
    return true;
}

bool SessionBroadcastEngine::leaveViewer(const QString& broadcastId, const QString& viewerId) {
    if (!m_broadcasts.contains(broadcastId)) return false;
    
    m_broadcastViewers[broadcastId].remove(viewerId);
    
    // 从列表移除
    auto& viewers = m_viewerLists[broadcastId];
    viewers.erase(std::remove_if(viewers.begin(), viewers.end(),
        [viewerId](const Viewer& v) { return v.id == viewerId; }),
        viewers.end());
    
    // 发送离开消息
    ChatMessage leaveMsg;
    leaveMsg.isSystem = true;
    leaveMsg.message = "Viewer " + viewerId.left(8) + " left";
    leaveMsg.timestamp = QDateTime::currentDateTime();
    m_chatHistories[broadcastId].append(leaveMsg);
    
    emit viewerLeft(broadcastId, viewerId);
    return true;
}

int SessionBroadcastEngine::getViewerCount(const QString& broadcastId) const {
    return m_broadcastViewers.value(broadcastId).size();
}

QList<Viewer> SessionBroadcastEngine::getViewerList(const QString& broadcastId) const {
    return m_viewerLists.value(broadcastId);
}

void SessionBroadcastEngine::broadcastTerminalOutput(const QString& broadcastId, const QByteArray& data) {
    if (!m_broadcasts.contains(broadcastId)) return;
    
    // 添加到缓冲 (用于新观看者回放)
    m_outputBuffers[broadcastId].append(data);
    
    // 限制缓冲大小 (1MB)
    if (m_outputBuffers[broadcastId].size() > 1024 * 1024) {
        m_outputBuffers[broadcastId] = m_outputBuffers[broadcastId].right(512 * 1024);
    }
    
    emit dataBroadcasted(broadcastId, data);
}

void SessionBroadcastEngine::broadcastCursorMove(const QString& broadcastId, int row, int col) {
    Q_UNUSED(row)
    Q_UNUSED(col)
    // 实现光标位置广播
    QJsonObject cursorData;
    cursorData["type"] = "cursor";
    cursorData["row"] = row;
    cursorData["col"] = col;
    
    QByteArray data = QJsonDocument(cursorData).toJson(QJsonDocument::Compact);
    emit dataBroadcasted(broadcastId, data);
}

void SessionBroadcastEngine::broadcastResize(const QString& broadcastId, int cols, int rows) {
    QJsonObject resizeData;
    resizeData["type"] = "resize";
    resizeData["cols"] = cols;
    resizeData["rows"] = rows;
    
    QByteArray data = QJsonDocument(resizeData).toJson(QJsonDocument::Compact);
    emit dataBroadcasted(broadcastId, data);
}

void SessionBroadcastEngine::sendChatMessage(const QString& broadcastId, const QString& from, const QString& message) {
    if (!m_broadcasts.contains(broadcastId)) return;
    
    if (!m_broadcasts[broadcastId].enableChat) {
        return;  // 聊天已禁用
    }
    
    ChatMessage chatMsg;
    chatMsg.from = from;
    chatMsg.message = message;
    chatMsg.timestamp = QDateTime::currentDateTime();
    chatMsg.isSystem = false;
    
    m_chatHistories[broadcastId].append(chatMsg);
    
    // 限制历史记录
    if (m_chatHistories[broadcastId].size() > m_chatHistoryLimit) {
        m_chatHistories[broadcastId].removeFirst();
    }
    
    emit chatMessageReceived(broadcastId, chatMsg);
}

QList<ChatMessage> SessionBroadcastEngine::getChatHistory(const QString& broadcastId, int limit) const {
    QList<ChatMessage> history = m_chatHistories.value(broadcastId);
    if (history.size() > limit) {
        return history.right(limit);
    }
    return history;
}

void SessionBroadcastEngine::clearChat(const QString& broadcastId) {
    if (!m_broadcasts.contains(broadcastId)) return;
    m_chatHistories[broadcastId].clear();
}

void SessionBroadcastEngine::updateConfig(const QString& broadcastId, const BroadcastConfig& config) {
    if (!m_broadcasts.contains(broadcastId)) return;
    m_broadcasts[broadcastId] = config;
}

BroadcastConfig SessionBroadcastEngine::getConfig(const QString& broadcastId) const {
    return m_broadcasts.value(broadcastId);
}

QJsonObject SessionBroadcastEngine::getBroadcastStats(const QString& broadcastId) const {
    if (!m_broadcasts.contains(broadcastId)) return QJsonObject();
    
    const BroadcastConfig& config = m_broadcasts[broadcastId];
    int viewerCount = m_broadcastViewers.value(broadcastId).size();
    
    QJsonObject stats;
    stats["broadcastId"] = broadcastId;
    stats["sessionId"] = config.sessionId;
    stats["viewerCount"] = viewerCount;
    stats["maxViewers"] = config.maxViewers;
    stats["title"] = config.streamTitle;
    stats["isPublic"] = config.isPublic;
    stats["chatEnabled"] = config.enableChat;
    stats["recordingEnabled"] = config.enableRecording;
    stats["chatMessageCount"] = m_chatHistories.value(broadcastId).size();
    stats["bufferSize"] = m_outputBuffers.value(broadcastId).size();
    
    return stats;
}

QList<QString> SessionBroadcastEngine::getActiveBroadcasts() const {
    return m_broadcasts.keys();
}

void SessionBroadcastEngine::cleanupInactiveBroadcasts() {
    // 清理超过 1 小时无活动的广播
    QDateTime now = QDateTime::currentDateTime();
    QList<QString> toRemove;
    
    for (auto it = m_broadcasts.begin(); it != m_broadcasts.end(); ++it) {
        // 检查是否有观看者
        if (m_broadcastViewers[it.key()].isEmpty()) {
            // 无观看者且超过 1 小时
            // TODO: 记录最后活动时间
            toRemove.append(it.key());
        }
    }
    
    for (const QString& id : toRemove) {
        stopBroadcast(id);
    }
}

QString SessionBroadcastEngine::generateBroadcastId() const {
    return "broadcast_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

#include "SessionBroadcastEngine.moc"
