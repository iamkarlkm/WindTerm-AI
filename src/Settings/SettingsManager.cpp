#include "SettingsManager.h"
#include "Theme/ThemeManager.h"
#include "Bookmarks/BookmarksStore.h"
#include "Ssh/ConnectionManager.h"
#include "CommandHistory/CommandHistoryStore.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSqlQuery>
#include <QDebug>

SettingsManager* SettingsManager::s_instance = nullptr;

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent) {
}

SettingsManager* SettingsManager::instance(QObject* parent) {
    if (!s_instance) {
        s_instance = new SettingsManager(parent);
    }
    return s_instance;
}

bool SettingsManager::exportSettings(const QString& filePath, bool themes, bool bookmarks,
                                      bool connections, bool history) {
    QJsonObject root;
    root["version"] = "0.2.0";
    root["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (themes) {
        root["themes"] = collectThemes();
    }
    if (bookmarks) {
        root["bookmarks"] = collectBookmarks();
    }
    if (connections) {
        root["connections"] = collectConnections();
    }
    if (history) {
        root["commandHistory"] = collectCommandHistory();
    }
    
    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[SettingsManager] Failed to open file for export:" << filePath;
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool SettingsManager::importSettings(const QString& filePath, bool themes, bool bookmarks,
                                      bool connections, bool history) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[SettingsManager] Failed to open file for import:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[SettingsManager] Invalid JSON format";
        return false;
    }
    
    return importData(doc.toJson(), themes, bookmarks, connections, history);
}

QString SettingsManager::getExportData(bool themes, bool bookmarks, bool connections, bool history) const {
    QJsonObject root;
    root["version"] = "0.2.0";
    root["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (themes) root["themes"] = const_cast<SettingsManager*>(this)->collectThemes();
    if (bookmarks) root["bookmarks"] = const_cast<SettingsManager*>(this)->collectBookmarks();
    if (connections) root["connections"] = const_cast<SettingsManager*>(this)->collectConnections();
    if (history) root["commandHistory"] = const_cast<SettingsManager*>(this)->collectCommandHistory();
    
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool SettingsManager::importData(const QString& jsonData, bool themes, bool bookmarks,
                                  bool connections, bool history) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
    if (doc.isNull() || !doc.isObject()) return false;
    
    QJsonObject root = doc.object();
    bool success = true;
    
    if (themes && root.contains("themes")) {
        success &= restoreThemes(root["themes"].toObject());
    }
    if (bookmarks && root.contains("bookmarks")) {
        success &= restoreBookmarks(root["bookmarks"].toObject());
    }
    if (connections && root.contains("connections")) {
        success &= restoreConnections(root["connections"].toObject());
    }
    if (history && root.contains("commandHistory")) {
        success &= restoreCommandHistory(root["commandHistory"].toObject());
    }
    
    return success;
}

QJsonObject SettingsManager::collectThemes() const {
    QJsonObject themesObj;
    ThemeManager* manager = ThemeManager::instance();
    QJsonArray themesArray;
    
    for (const auto& theme : manager->availableThemes()) {
        themesArray.append(theme.toJson());
    }
    
    themesObj["themes"] = themesArray;
    return themesObj;
}

QJsonObject SettingsManager::collectBookmarks() const {
    QJsonObject bookmarksObj;
    BookmarksStore* store = BookmarksStore::instance();
    QJsonArray bookmarksArray;
    
    for (const auto& bookmark : store->getAllBookmarks()) {
        QJsonObject obj;
        obj["name"] = bookmark.name;
        obj["path"] = bookmark.path;
        obj["category"] = bookmark.category;
        obj["description"] = bookmark.description;
        obj["created_at"] = bookmark.createdAt.toString(Qt::ISODate);
        obj["usage_count"] = bookmark.usageCount;
        bookmarksArray.append(obj);
    }
    
    bookmarksObj["bookmarks"] = bookmarksArray;
    return bookmarksObj;
}

QJsonObject SettingsManager::collectConnections() const {
    QJsonObject connectionsObj;
    ConnectionManager* manager = ConnectionManager::instance();
    QJsonArray connectionsArray;
    
    auto profiles = manager->loadProfiles();
    for (const auto& profile : profiles) {
        QJsonObject obj;
        obj["name"] = profile.name;
        obj["host"] = profile.host;
        obj["port"] = profile.port;
        obj["username"] = profile.username;
        obj["auth_method"] = static_cast<int>(profile.authMethod);
        obj["private_key_path"] = profile.privateKeyPath;
        obj["last_connected"] = profile.lastConnected;
        obj["created_at"] = profile.createdAt.toString(Qt::ISODate);
        connectionsArray.append(obj);
    }
    
    connectionsObj["connections"] = connectionsArray;
    return connectionsObj;
}

QJsonObject SettingsManager::collectCommandHistory() const {
    QJsonObject historyObj;
    CommandHistoryStore* store = CommandHistoryStore::instance();
    QJsonArray historyArray;
    
    auto entries = store->recent(10000);
    for (const auto& entry : entries) {
        QJsonObject obj;
        obj["command"] = entry.command;
        obj["working_directory"] = entry.workingDirectory;
        obj["session_type"] = entry.sessionType;
        obj["created_at"] = entry.timestamp.toString(Qt::ISODate);
        obj["last_used"] = entry.timestamp.toString(Qt::ISODate);
        obj["usage_count"] = entry.usageCount;
        historyArray.append(obj);
    }
    
    historyObj["commandHistory"] = historyArray;
    return historyObj;
}

bool SettingsManager::restoreThemes(const QJsonObject& data) {
    if (!data.contains("themes")) return false;
    
    ThemeManager* manager = ThemeManager::instance();
    QJsonArray themesArray = data["themes"].toArray();
    
    for (const auto& val : themesArray) {
        ThemeConfig theme = ThemeConfig::fromJson(val.toObject());
        if (theme.isValid()) {
            manager->saveTheme(theme);
        }
    }
    
    return true;
}

bool SettingsManager::restoreBookmarks(const QJsonObject& data) {
    if (!data.contains("bookmarks")) return false;
    
    BookmarksStore* store = BookmarksStore::instance();
    QJsonArray bookmarksArray = data["bookmarks"].toArray();
    
    for (const auto& val : bookmarksArray) {
        QJsonObject obj = val.toObject();
        store->addBookmark(
            obj["name"].toString(),
            obj["path"].toString(),
            obj["category"].toString(),
            obj["description"].toString()
        );
    }
    
    return true;
}

bool SettingsManager::restoreConnections(const QJsonObject& data) {
    if (!data.contains("connections")) return false;
    
    ConnectionManager* manager = ConnectionManager::instance();
    QJsonArray connectionsArray = data["connections"].toArray();
    
    for (const auto& val : connectionsArray) {
        QJsonObject obj = val.toObject();
        ConnectionProfile profile;
        profile.name = obj["name"].toString();
        profile.host = obj["host"].toString();
        profile.port = obj["port"].toInt(22);
        profile.username = obj["username"].toString();
        profile.authMethod = static_cast<SshAuthMethod>(obj["auth_method"].toInt(0));
        profile.privateKeyPath = obj["private_key_path"].toString();
        
        manager->saveProfile(profile);
    }
    
    return true;
}

bool SettingsManager::restoreCommandHistory(const QJsonObject& data) {
    if (!data.contains("commandHistory")) return false;
    
    CommandHistoryStore* store = CommandHistoryStore::instance();
    QJsonArray historyArray = data["commandHistory"].toArray();
    
    for (const auto& val : historyArray) {
        QJsonObject obj = val.toObject();
        store->addCommand(
            obj["command"].toString(),
            obj["working_directory"].toString(),
            obj["session_type"].toString()
        );
    }
    
    return true;
}
