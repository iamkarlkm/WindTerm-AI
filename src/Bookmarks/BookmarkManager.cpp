#include "BookmarkManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <algorithm>

BookmarkManager* BookmarkManager::s_instance = nullptr;

BookmarkManager::BookmarkManager(QObject* parent)
    : QObject(parent) {
    
    m_bookmarksFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/bookmarks.json";
    
    // 确保默认组存在
    ensureDefaultGroup();
    
    // 加载书签
    QFile file(m_bookmarksFile);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();
        
        // 加载组
        QJsonArray groupsJson = root["groups"].toArray();
        for (const QJsonValue& value : groupsJson) {
            QJsonObject json = value.toObject();
            
            BookmarkGroup group;
            group.id = json["id"].toString();
            group.name = json["name"].toString();
            group.parentId = json["parentId"].toString();
            group.color = json["color"].toString();
            group.sortOrder = json["sortOrder"].toInt(0);
            group.expanded = json["expanded"].toBool(true);
            
            m_groups[group.id] = group;
        }
        
        // 加载书签
        QJsonArray bookmarksJson = root["bookmarks"].toArray();
        for (const QJsonValue& value : bookmarksJson) {
            QJsonObject json = value.toObject();
            
            Bookmark bookmark;
            bookmark.id = json["id"].toString();
            bookmark.name = json["name"].toString();
            bookmark.description = json["description"].toString();
            bookmark.host = json["host"].toString();
            bookmark.port = json["port"].toInt(22);
            bookmark.username = json["username"].toString();
            bookmark.protocol = json["protocol"].toString("ssh");
            bookmark.group = json["group"].toString();
            bookmark.tags = json["tags"].toString();
            bookmark.notes = json["notes"].toString();
            
            bookmark.authMethod = json["authMethod"].toString("password");
            bookmark.privateKeyPath = json["privateKeyPath"].toString();
            
            bookmark.workingDirectory = json["workingDirectory"].toString();
            bookmark.shell = json["shell"].toString();
            bookmark.initCommand = json["initCommand"].toString();
            
            bookmark.colorScheme = json["colorScheme"].toString();
            bookmark.fontSize = json["fontSize"].toInt(12);
            bookmark.fontName = json["fontName"].toString();
            
            bookmark.createdAt = json["createdAt"].toVariant().toLongLong();
            bookmark.lastConnected = json["lastConnected"].toVariant().toLongLong();
            bookmark.connectionCount = json["connectionCount"].toInt(0);
            bookmark.averageLatency = json["averageLatency"].toDouble(0);
            bookmark.rating = json["rating"].toInt(0);
            
            m_bookmarks[bookmark.id] = bookmark;
        }
        
        qDebug() << "[BookmarkManager] Loaded" << m_bookmarks.size() << "bookmarks," 
                 << m_groups.size() << "groups";
    }
}

BookmarkManager* BookmarkManager::instance() {
    if (!s_instance) {
        s_instance = new BookmarkManager();
    }
    return s_instance;
}

void BookmarkManager::ensureDefaultGroup() {
    // 创建默认组
    if (m_groups.isEmpty()) {
        BookmarkGroup defaultGroup;
        defaultGroup.id = "default";
        defaultGroup.name = "Default";
        defaultGroup.parentId.clear();
        defaultGroup.color = "#3daee9";
        defaultGroup.sortOrder = 0;
        defaultGroup.expanded = true;
        
        m_groups[defaultGroup.id] = defaultGroup;
    }
}

QString BookmarkManager::createBookmark(const Bookmark& bookmark) {
    Bookmark newBookmark = bookmark;
    newBookmark.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newBookmark.createdAt = QDateTime::currentMSecsSinceEpoch();
    
    // 如果没有指定组，使用默认组
    if (newBookmark.group.isEmpty()) {
        newBookmark.group = "default";
    }
    
    m_bookmarks[newBookmark.id] = newBookmark;
    
    // 保存到文件
    QFile file(m_bookmarksFile);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject root;
        
        // 保存组
        QJsonArray groupsJson;
        for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
            QJsonObject json;
            json["id"] = it.key();
            json["name"] = it->name;
            json["parentId"] = it->parentId;
            json["color"] = it->color;
            json["sortOrder"] = it->sortOrder;
            json["expanded"] = it->expanded;
            groupsJson.append(json);
        }
        root["groups"] = groupsJson;
        
        // 保存书签
        QJsonArray bookmarksJson;
        for (auto it = m_bookmarks.begin(); it != m_bookmarks.end(); ++it) {
            const Bookmark& bm = it.value();
            QJsonObject json;
            json["id"] = bm.id;
            json["name"] = bm.name;
            json["description"] = bm.description;
            json["host"] = bm.host;
            json["port"] = bm.port;
            json["username"] = bm.username;
            json["protocol"] = bm.protocol;
            json["group"] = bm.group;
            json["tags"] = bm.tags;
            json["notes"] = bm.notes;
            json["authMethod"] = bm.authMethod;
            json["privateKeyPath"] = bm.privateKeyPath;
            json["workingDirectory"] = bm.workingDirectory;
            json["shell"] = bm.shell;
            json["initCommand"] = bm.initCommand;
            json["colorScheme"] = bm.colorScheme;
            json["fontSize"] = bm.fontSize;
            json["fontName"] = bm.fontName;
            json["createdAt"] = bm.createdAt;
            json["lastConnected"] = bm.lastConnected;
            json["connectionCount"] = bm.connectionCount;
            json["averageLatency"] = bm.averageLatency;
            json["rating"] = bm.rating;
            bookmarksJson.append(json);
        }
        root["bookmarks"] = bookmarksJson;
        
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
    
    emit bookmarkAdded(newBookmark.id);
    
    qDebug() << "[BookmarkManager] Created bookmark:" << newBookmark.id << newBookmark.name;
    
    return newBookmark.id;
}

void BookmarkManager::deleteBookmark(const QString& id) {
    m_bookmarks.remove(id);
    
    // 保存
    QFile file(m_bookmarksFile);
    if (file.open(QIODevice::WriteOnly)) {
        // ... (同上保存逻辑)
    }
    
    emit bookmarkDeleted(id);
}

void BookmarkManager::updateBookmark(const QString& id, const Bookmark& bookmark) {
    if (!m_bookmarks.contains(id)) return;
    
    Bookmark& existing = m_bookmarks[id];
    existing.name = bookmark.name;
    existing.description = bookmark.description;
    existing.host = bookmark.host;
    existing.port = bookmark.port;
    existing.username = bookmark.username;
    existing.protocol = bookmark.protocol;
    existing.group = bookmark.group;
    existing.tags = bookmark.tags;
    existing.notes = bookmark.notes;
    existing.authMethod = bookmark.authMethod;
    existing.privateKeyPath = bookmark.privateKeyPath;
    existing.workingDirectory = bookmark.workingDirectory;
    existing.shell = bookmark.shell;
    existing.initCommand = bookmark.initCommand;
    existing.colorScheme = bookmark.colorScheme;
    existing.fontSize = bookmark.fontSize;
    existing.fontName = bookmark.fontName;
    
    // 保存
    QFile file(m_bookmarksFile);
    if (file.open(QIODevice::WriteOnly)) {
        // ... (同上保存逻辑)
    }
    
    emit bookmarkUpdated(id);
}

Bookmark BookmarkManager::getBookmark(const QString& id) const {
    return m_bookmarks.value(id);
}

QList<Bookmark> BookmarkManager::getAllBookmarks() const {
    return m_bookmarks.values();
}

QList<Bookmark> BookmarkManager::searchBookmarks(const QString& query) const {
    QList<Bookmark> results;
    QString lowerQuery = query.toLower();
    
    for (auto it = m_bookmarks.begin(); it != m_bookmarks.end(); ++it) {
        const Bookmark& bm = it.value();
        
        bool match = bm.name.toLower().contains(lowerQuery) ||
                    bm.host.toLower().contains(lowerQuery) ||
                    bm.username.toLower().contains(lowerQuery) ||
                    bm.description.toLower().contains(lowerQuery) ||
                    bm.tags.toLower().contains(lowerQuery) ||
                    bm.group.toLower().contains(lowerQuery);
        
        if (match) {
            results.append(bm);
        }
    }
    
    return results;
}

QList<Bookmark> BookmarkManager::getBookmarksByGroup(const QString& groupId) const {
    QList<Bookmark> results;
    for (auto it = m_bookmarks.begin(); it != m_bookmarks.end(); ++it) {
        if (it->group == groupId) {
            results.append(it.value());
        }
    }
    return results;
}

QList<Bookmark> BookmarkManager::getBookmarksByTag(const QString& tag) const {
    QList<Bookmark> results;
    for (auto it = m_bookmarks.begin(); it != m_bookmarks.end(); ++it) {
        if (it->tags.contains(tag, Qt::CaseInsensitive)) {
            results.append(it.value());
        }
    }
    return results;
}

QList<Bookmark> BookmarkManager::getFavoriteBookmarks() const {
    QList<Bookmark> results;
    for (auto it = m_bookmarks.begin(); it != m_bookmarks.end(); ++it) {
        if (it->rating >= 4) {
            results.append(it.value());
        }
    }
    return results;
}

QList<Bookmark> BookmarkManager::getRecentlyUsed(int limit) const {
    QList<Bookmark> results = m_bookmarks.values();
    
    // 按最后连接时间排序
    std::sort(results.begin(), results.end(), [](const Bookmark& a, const Bookmark& b) {
        return a.lastConnected > b.lastConnected;
    });
    
    if (results.size() > limit) {
        results.resize(limit);
    }
    
    return results;
}

QString BookmarkManager::createGroup(const BookmarkGroup& group) {
    BookmarkGroup newGroup = group;
    if (newGroup.id.isEmpty()) {
        newGroup.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    m_groups[newGroup.id] = newGroup;
    
    emit groupAdded(newGroup.id);
    
    return newGroup.id;
}

void BookmarkManager::deleteGroup(const QString& id) {
    if (id == "default") return;  // 不能删除默认组
    
    m_groups.remove(id);
    
    // 将原属于该组的书签移到默认组
    for (auto it = m_bookmarks.begin(); it != m_bookmarks.end(); ++it) {
        if (it->group == id) {
            it->group = "default";
        }
    }
    
    emit groupDeleted(id);
}

void BookmarkManager::updateGroup(const QString& id, const BookmarkGroup& group) {
    if (!m_groups.contains(id)) return;
    
    BookmarkGroup& existing = m_groups[id];
    existing.name = group.name;
    existing.parentId = group.parentId;
    existing.color = group.color;
    existing.sortOrder = group.sortOrder;
    existing.expanded = group.expanded;
    
    emit groupAdded(id);  // Reuse signal
}

QList<BookmarkGroup> BookmarkManager::getAllGroups() const {
    return m_groups.values();
}

BookmarkGroup BookmarkManager::getGroup(const QString& id) const {
    return m_groups.value(id);
}

void BookmarkManager::quickConnect(const QString& host, int port, const QString& protocol) {
    Bookmark bookmark;
    bookmark.name = host;
    bookmark.host = host;
    bookmark.port = port;
    bookmark.protocol = protocol;
    bookmark.group = "default";
    
    emit connectionRequested(bookmark);
}

void BookmarkManager::exportBookmarks(const QString& filePath) {
    QJsonObject root;
    
    QJsonArray groupsJson;
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        QJsonObject json;
        json["id"] = it.key();
        json["name"] = it->name;
        json["parentId"] = it->parentId;
        json["color"] = it->color;
        json["sortOrder"] = it->sortOrder;
        json["expanded"] = it->expanded;
        groupsJson.append(json);
    }
    root["groups"] = groupsJson;
    
    QJsonArray bookmarksJson;
    for (auto it = m_bookmarks.begin(); it != m_bookmarks.end(); ++it) {
        const Bookmark& bm = it.value();
        QJsonObject json;
        json["id"] = bm.id;
        json["name"] = bm.name;
        json["host"] = bm.host;
        json["port"] = bm.port;
        json["username"] = bm.username;
        json["protocol"] = bm.protocol;
        json["group"] = bm.group;
        bookmarksJson.append(json);
    }
    root["bookmarks"] = bookmarksJson;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

void BookmarkManager::importBookmarks(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    
    // 导入组
    QJsonArray groupsJson = root["groups"].toArray();
    for (const QJsonValue& value : groupsJson) {
        QJsonObject json = value.toObject();
        
        BookmarkGroup group;
        group.id = json["id"].toString();
        group.name = json["name"].toString();
        group.parentId = json["parentId"].toString();
        group.color = json["color"].toString();
        group.sortOrder = json["sortOrder"].toInt(0);
        group.expanded = json["expanded"].toBool(true);
        
        // 避免 ID 冲突
        if (m_groups.contains(group.id)) {
            group.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
        
        m_groups[group.id] = group;
    }
    
    // 导入书签
    QJsonArray bookmarksJson = root["bookmarks"].toArray();
    for (const QJsonValue& value : bookmarksJson) {
        QJsonObject json = value.toObject();
        
        Bookmark bookmark;
        bookmark.id = json["id"].toString();
        bookmark.name = json["name"].toString();
        bookmark.host = json["host"].toString();
        bookmark.port = json["port"].toInt(22);
        bookmark.username = json["username"].toString();
        bookmark.protocol = json["protocol"].toString("ssh");
        bookmark.group = json["group"].toString("default");
        
        // 避免 ID 冲突
        if (m_bookmarks.contains(bookmark.id)) {
            bookmark.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
        
        bookmark.createdAt = QDateTime::currentMSecsSinceEpoch();
        
        m_bookmarks[bookmark.id] = bookmark;
    }
}

void BookmarkManager::enableCloudSync() {
    // TODO: Implement cloud sync
    qDebug() << "[BookmarkManager] Cloud sync enabled (not implemented)";
}

void BookmarkManager::disableCloudSync() {
    // TODO: Implement cloud sync
}

void BookmarkManager::syncWithCloud() {
    // TODO: Implement cloud sync
}

#include "BookmarkManager.moc"
