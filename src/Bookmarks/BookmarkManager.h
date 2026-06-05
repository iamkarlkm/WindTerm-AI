#ifndef BOOKMARK_MANAGER_H
#define BOOKMARK_MANAGER_H

#include <QObject>
#include <QMap>
#include <QList>

struct Bookmark {
    QString id;
    QString name;
    QString description;
    QString host;
    int port;
    QString username;
    QString protocol;  // ssh, telnet, serial, ftp, sftp
    QString group;
    QString tags;
    QString notes;
    
    // 认证信息
    QString authMethod;  // password, key, agent
    QString password;  // Encrypted in production
    QString privateKeyPath;
    QString passphrase;
    
    // 会话设置
    QString workingDirectory;
    QString shell;
    QString initCommand;
    QString environment;  // JSON string
    
    // 外观设置
    QString colorScheme;
    int fontSize;
    QString fontName;
    
    // 元数据
    qint64 createdAt;
    qint64 lastConnected;
    int connectionCount;
    double averageLatency;  // ms
    int rating;  // 0-5
    
    Bookmark() 
        : port(22)
        , protocol("ssh")
        , authMethod("password")
        , fontSize(12)
        , createdAt(0)
        , lastConnected(0)
        , connectionCount(0)
        , averageLatency(0)
        , rating(0) {}
};

struct BookmarkGroup {
    QString id;
    QString name;
    QString parentId;
    QString color;  // Icon color
    int sortOrder;
    bool expanded;
    
    BookmarkGroup() : sortOrder(0), expanded(true) {}
};

class BookmarkManager : public QObject {
    Q_OBJECT
public:
    explicit BookmarkManager(QObject* parent = nullptr);
    
    static BookmarkManager* instance();
    
    // 书签管理
    QString createBookmark(const Bookmark& bookmark);
    void deleteBookmark(const QString& id);
    void updateBookmark(const QString& id, const Bookmark& bookmark);
    
    // 书签查询
    Bookmark getBookmark(const QString& id) const;
    QList<Bookmark> getAllBookmarks() const;
    QList<Bookmark> searchBookmarks(const QString& query) const;
    QList<Bookmark> getBookmarksByGroup(const QString& groupId) const;
    QList<Bookmark> getBookmarksByTag(const QString& tag) const;
    QList<Bookmark> getFavoriteBookmarks() const;
    QList<Bookmark> getRecentlyUsed(int limit = 10) const;
    
    // 书签组管理
    QString createGroup(const BookmarkGroup& group);
    void deleteGroup(const QString& id);
    void updateGroup(const QString& id, const BookmarkGroup& group);
    QList<BookmarkGroup> getAllGroups() const;
    BookmarkGroup getGroup(const QString& id) const;
    
    // 快速连接
    void quickConnect(const QString& host, int port = 22, const QString& protocol = "ssh");
    
    // 导入导出
    void exportBookmarks(const QString& filePath);
    void importBookmarks(const QString& filePath);
    
    // 云同步（预留接口）
    void enableCloudSync();
    void disableCloudSync();
    void syncWithCloud();
    
signals:
    void bookmarkAdded(const QString& id);
    void bookmarkDeleted(const QString& id);
    void bookmarkUpdated(const QString& id);
    void groupAdded(const QString& id);
    void groupDeleted(const QString& id);
    void connectionRequested(const Bookmark& bookmark);

private:
    static BookmarkManager* s_instance;
    
    QMap<QString, Bookmark> m_bookmarks;
    QMap<QString, BookmarkGroup> m_groups;
    QString m_bookmarksFile;
    
    void ensureDefaultGroup();
};

#endif
