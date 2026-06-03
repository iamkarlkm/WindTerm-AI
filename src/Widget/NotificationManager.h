#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include <QObject>
#include <QMap>
#include <QSystemTrayIcon>
#include <QQueue>

enum class NotificationType {
    Info,
    Success,
    Warning,
    Error,
    Bell
};

struct Notification {
    QString id;
    QString title;
    QString message;
    NotificationType type;
    int duration;  // milliseconds, 0 = persistent
    qint64 timestamp;
    bool read;
    
    Notification() : type(NotificationType::Info), duration(5000), timestamp(0), read(false) {}
};

class NotificationManager : public QObject {
    Q_OBJECT
public:
    explicit NotificationManager(QObject* parent = nullptr);
    
    static NotificationManager* instance();
    
    // 发送通知
    QString sendNotification(const QString& title, const QString& message,
                            NotificationType type = NotificationType::Info,
                            int duration = 5000);
    
    // 系统托盘通知
    void showTrayNotification(const QString& title, const QString& message,
                             QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);
    
    // Bell 通知
    void triggerBellNotification(const QString& terminalTitle = "");
    
    // 通知管理
    void markAsRead(const QString& id);
    void markAllAsRead();
    void clearNotification(const QString& id);
    void clearAll();
    void clearOldNotifications(int maxAgeSeconds = 3600);
    
    // 查询
    Notification getNotification(const QString& id) const;
    QList<Notification> getAllNotifications() const;
    QList<Notification> getUnreadNotifications() const;
    int unreadCount() const;
    
    // 配置
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    void setSoundEnabled(bool enabled);
    void setTrayEnabled(bool enabled);
    
    // 获取图标
    static QIcon getIconForType(NotificationType type);
    
signals:
    void notificationSent(const Notification& notification);
    void notificationCleared(const QString& id);
    void unreadCountChanged(int count);
    void bellTriggered(const QString& terminalTitle);

private slots:
    void onTrayMessageClicked();
    
private:
    void playSound(NotificationType type);
    QString generateId();
    
    static NotificationManager* s_instance;
    
    QMap<QString, Notification> m_notifications;
    QQueue<QString> m_notificationQueue;
    QSystemTrayIcon* m_trayIcon;
    
    bool m_enabled;
    bool m_soundEnabled;
    bool m_trayEnabled;
    
    int m_maxNotifications;
    int m_bellCooldown;  // milliseconds
    qint64 m_lastBellTime;
};

#endif
