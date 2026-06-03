#ifndef SYNCHRONIZED_INPUT_H
#define SYNCHRONIZED_INPUT_H

#include <QObject>
#include <QMap>
#include <QSet>

struct SyncGroup {
    QString id;
    QString name;
    QSet<QString> sessionIds;
    bool enabled;
    QString masterSession;  // The session that broadcasts input
    
    SyncGroup() : enabled(true) {}
};

class SynchronizedInputManager : public QObject {
    Q_OBJECT
public:
    explicit SynchronizedInputManager(QObject* parent = nullptr);
    
    static SynchronizedInputManager* instance();
    
    // 同步组管理
    QString createGroup(const QString& name = "Sync Group");
    void deleteGroup(const QString& groupId);
    void renameGroup(const QString& groupId, const QString& newName);
    
    // 会话管理
    void addSessionToGroup(const QString& groupId, const QString& sessionId);
    void removeSessionFromGroup(const QString& groupId, const QString& sessionId);
    void setMasterSession(const QString& groupId, const QString& sessionId);
    
    // 同步控制
    void enableGroup(const QString& groupId);
    void disableGroup(const QString& groupId);
    bool isGroupEnabled(const QString& groupId) const;
    
    void enableAllGroups();
    void disableAllGroups();
    
    // 状态查询
    SyncGroup getGroup(const QString& groupId) const;
    QList<SyncGroup> getAllGroups() const;
    QSet<QString> getSessionGroups(const QString& sessionId) const;
    QStringList getGroupSessions(const QString& groupId) const;
    
    // 输入广播
    void broadcastText(const QString& groupId, const QString& text);
    void broadcastKey(const QString& groupId, int key, Qt::KeyboardModifiers modifiers);
    void broadcastCommand(const QString& groupId, const QString& command);
    
    // 持久化
    void saveGroups();
    void loadGroups();
    
signals:
    void groupCreated(const QString& groupId);
    void groupDeleted(const QString& groupId);
    void sessionAdded(const QString& groupId, const QString& sessionId);
    void sessionRemoved(const QString& groupId, const QString& sessionId);
    void groupToggled(const QString& groupId, bool enabled);
    void textBroadcasted(const QString& groupId, const QString& sessionId, const QString& text);
    void keyBroadcasted(const QString& groupId, const QString& sessionId, int key);

private:
    static SynchronizedInputManager* s_instance;
    
    QMap<QString, SyncGroup> m_groups;
    QString m_groupsFile;
};

#endif
