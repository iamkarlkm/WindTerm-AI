#include "NotificationManager.h"
#include <QDateTime>
#include <QUuid>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSystemTrayIcon>
#include <QApplication>
#include <QIcon>
#include <QDebug>

NotificationManager* NotificationManager::s_instance = nullptr;

NotificationManager::NotificationManager(QObject* parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_enabled(true)
    , m_soundEnabled(true)
    , m_trayEnabled(true)
    , m_maxNotifications(100)
    , m_bellCooldown(1000)
    , m_lastBellTime(0) {
    
    // 初始化托盘图标
    m_trayIcon->setIcon(QIcon::fromTheme("utilities-terminal", QApplication::windowIcon()));
    m_trayIcon->setToolTip("WindTerm Extensions");
    m_trayIcon->show();
    
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &NotificationManager::onTrayMessageClicked);
}

NotificationManager* NotificationManager::instance() {
    if (!s_instance) {
        s_instance = new NotificationManager();
    }
    return s_instance;
}

QString NotificationManager::sendNotification(const QString& title, const QString& message,
                                              NotificationType type, int duration) {
    if (!m_enabled) return QString();
    
    Notification notification;
    notification.id = generateId();
    notification.title = title;
    notification.message = message;
    notification.type = type;
    notification.duration = duration;
    notification.timestamp = QDateTime::currentMSecsSinceEpoch();
    notification.read = false;
    
    m_notifications[notification.id] = notification;
    m_notificationQueue.enqueue(notification.id);
    
    // 清理旧通知
    while (m_notificationQueue.size() > m_maxNotifications) {
        QString oldId = m_notificationQueue.dequeue();
        m_notifications.remove(oldId);
    }
    
    // 播放声音
    if (m_soundEnabled) {
        playSound(type);
    }
    
    // 显示托盘通知
    if (m_trayEnabled && type != NotificationType::Bell) {
        QSystemTrayIcon::MessageIcon trayIcon;
        switch (type) {
            case NotificationType::Success:
                trayIcon = QSystemTrayIcon::Information;
                break;
            case NotificationType::Warning:
                trayIcon = QSystemTrayIcon::Warning;
                break;
            case NotificationType::Error:
                trayIcon = QSystemTrayIcon::Critical;
                break;
            default:
                trayIcon = QSystemTrayIcon::Information;
        }
        showTrayNotification(title, message, trayIcon);
    }
    
    emit notificationSent(notification);
    emit unreadCountChanged(unreadCount());
    
    qDebug() << "[NotificationManager] Sent notification:" << notification.id << title;
    
    return notification.id;
}

void NotificationManager::showTrayNotification(const QString& title, const QString& message,
                                               QSystemTrayIcon::MessageIcon icon) {
    if (m_trayIcon->isSystemTrayAvailable()) {
        m_trayIcon->showMessage(title, message, icon, 5000);
    }
}

void NotificationManager::triggerBellNotification(const QString& terminalTitle) {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // 冷却时间检查
    if (currentTime - m_lastBellTime < m_bellCooldown) {
        return;
    }
    m_lastBellTime = currentTime;
    
    if (!m_enabled) return;
    
    // 播放 Bell 声音
    if (m_soundEnabled) {
        playSound(NotificationType::Bell);
    }
    
    // 发送通知
    QString title = terminalTitle.isEmpty() ? "Terminal Bell" : QString("Bell: %1").arg(terminalTitle);
    sendNotification(title, "Terminal bell triggered", NotificationType::Bell, 2000);
    
    emit bellTriggered(terminalTitle);
}

void NotificationManager::markAsRead(const QString& id) {
    if (m_notifications.contains(id)) {
        m_notifications[id].read = true;
        emit unreadCountChanged(unreadCount());
    }
}

void NotificationManager::markAllAsRead() {
    for (auto it = m_notifications.begin(); it != m_notifications.end(); ++it) {
        it->read = true;
    }
    emit unreadCountChanged(0);
}

void NotificationManager::clearNotification(const QString& id) {
    m_notifications.remove(id);
    emit notificationCleared(id);
    emit unreadCountChanged(unreadCount());
}

void NotificationManager::clearAll() {
    m_notifications.clear();
    m_notificationQueue.clear();
    emit unreadCountChanged(0);
}

void NotificationManager::clearOldNotifications(int maxAgeSeconds) {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 maxAgeMs = maxAgeSeconds * 1000;
    
    QList<QString> toRemove;
    for (auto it = m_notifications.begin(); it != m_notifications.end(); ++it) {
        if (currentTime - it->timestamp > maxAgeMs) {
            toRemove.append(it.key());
        }
    }
    
    for (const QString& id : toRemove) {
        m_notifications.remove(id);
    }
    
    if (!toRemove.isEmpty()) {
        emit unreadCountChanged(unreadCount());
    }
}

Notification NotificationManager::getNotification(const QString& id) const {
    return m_notifications.value(id);
}

QList<Notification> NotificationManager::getAllNotifications() const {
    return m_notifications.values();
}

QList<Notification> NotificationManager::getUnreadNotifications() const {
    QList<Notification> unread;
    for (auto it = m_notifications.begin(); it != m_notifications.end(); ++it) {
        if (!it->read) {
            unread.append(it.value());
        }
    }
    return unread;
}

int NotificationManager::unreadCount() const {
    int count = 0;
    for (auto it = m_notifications.begin(); it != m_notifications.end(); ++it) {
        if (!it->read) {
            count++;
        }
    }
    return count;
}

void NotificationManager::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void NotificationManager::setSoundEnabled(bool enabled) {
    m_soundEnabled = enabled;
}

void NotificationManager::setTrayEnabled(bool enabled) {
    m_trayEnabled = enabled;
}

QIcon NotificationManager::getIconForType(NotificationType type) {
    switch (type) {
        case NotificationType::Info:
            return QIcon::fromTheme("dialog-information");
        case NotificationType::Success:
            return QIcon::fromTheme("dialog-ok-apply");
        case NotificationType::Warning:
            return QIcon::fromTheme("dialog-warning");
        case NotificationType::Error:
            return QIcon::fromTheme("dialog-error");
        case NotificationType::Bell:
            return QIcon::fromTheme("audio-volume-high");
        default:
            return QIcon::fromTheme("dialog-information");
    }
}

void NotificationManager::onTrayMessageClicked() {
    // 用户点击托盘通知时的处理
    markAllAsRead();
}

void NotificationManager::playSound(NotificationType type) {
    // 使用 QtMultimedia 播放系统声音
    QString soundFile;
    switch (type) {
        case NotificationType::Bell:
            soundFile = "/usr/share/sounds/freedesktop/stereo/bell.oga";
            break;
        case NotificationType::Error:
            soundFile = "/usr/share/sounds/freedesktop/stereo/alarm-clock-elapsed.oga";
            break;
        case NotificationType::Warning:
            soundFile = "/usr/share/sounds/freedesktop/stereo/message.oga";
            break;
        case NotificationType::Success:
            soundFile = "/usr/share/sounds/freedesktop/stereo/complete.oga";
            break;
        default:
            soundFile = "/usr/share/sounds/freedesktop/stereo/message.oga";
    }
    
    // 简单的蜂鸣声备用方案
    QApplication::beep();
}

QString NotificationManager::generateId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

#include "NotificationManager.moc"
