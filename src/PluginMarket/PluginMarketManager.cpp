#include "PluginMarketManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QUuid>

PluginMarketManager* PluginMarketManager::s_instance = nullptr;

PluginMarketManager::PluginMarketManager(QObject* parent) : QObject(parent) {
    m_networkManager = new QNetworkAccessManager(this);
    m_connected = false;
    m_marketplaceUrl = "https://plugins.windterm-ai.com/api/v1";
    
    loadPluginCache();
}

PluginMarketManager::~PluginMarketManager() {
    savePluginCache();
}

PluginMarketManager* PluginMarketManager::instance() {
    if (!s_instance) s_instance = new PluginMarketManager();
    return s_instance;
}

void PluginMarketManager::setMarketplaceUrl(const QString& url) {
    m_marketplaceUrl = url;
}

bool PluginMarketManager::connectToMarketplace() {
    QNetworkRequest request(QUrl(m_marketplaceUrl + "/health"));
    QNetworkReply* reply = m_networkManager->get(request);
    
    // 简单检查，实际应该异步等待
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_connected = true;
            emit marketplaceConnected();
            loadPluginCache();
        } else {
            m_connected = false;
            emit errorOccurred("Failed to connect to marketplace: " + reply->errorString());
        }
        reply->deleteLater();
    });
    
    return true;
}

bool PluginMarketManager::isConnected() const {
    return m_connected;
}

QList<PluginInfo> PluginMarketManager::browsePlugins(int page, int pageSize) const {
    Q_UNUSED(page)
    Q_UNUSED(pageSize)
    // 返回缓存的插件列表
    return m_pluginCache.values();
}

QList<PluginInfo> PluginMarketManager::searchPlugins(const QString& query, const QStringList& tags) const {
    QList<PluginInfo> results;
    
    for (auto it = m_pluginCache.begin(); it != m_pluginCache.end(); ++it) {
        const PluginInfo& plugin = it.value();
        
        // 搜索名称、描述、标签
        bool match = plugin.name.contains(query, Qt::CaseInsensitive) ||
                     plugin.description.contains(query, Qt::CaseInsensitive);
        
        // 标签匹配
        if (!tags.isEmpty()) {
            for (const QString& tag : tags) {
                if (plugin.tags.contains(tag)) {
                    match = true;
                    break;
                }
            }
        }
        
        if (match) {
            results.append(plugin);
        }
    }
    
    return results;
}

QList<PluginInfo> PluginMarketManager::getFeaturedPlugins() const {
    return m_featuredPlugins;
}

QList<PluginInfo> PluginMarketManager::getPopularPlugins() const {
    return m_popularPlugins;
}

QList<PluginInfo> PluginMarketManager::getNewPlugins() const {
    QList<PluginInfo> newPlugins;
    
    QDateTime thirtyDaysAgo = QDateTime::currentDateTime().addDays(-30);
    
    for (auto it = m_pluginCache.begin(); it != m_pluginCache.end(); ++it) {
        if (it->lastUpdated > thirtyDaysAgo) {
            newPlugins.append(it.value());
        }
    }
    
    return newPlugins;
}

QList<PluginInfo> PluginMarketManager::getPluginsByCategory(const QString& category) const {
    QList<PluginInfo> categoryPlugins;
    
    for (auto it = m_pluginCache.begin(); it != m_pluginCache.end(); ++it) {
        if (it->tags.contains(category)) {
            categoryPlugins.append(it.value());
        }
    }
    
    return categoryPlugins;
}

PluginInfo PluginMarketManager::getPluginDetails(const QString& pluginId) const {
    return m_pluginCache.value(pluginId);
}

QByteArray PluginMarketManager::downloadPlugin(const QString& pluginId) {
    if (!m_pluginCache.contains(pluginId)) {
        emit errorOccurred("Plugin not found: " + pluginId);
        return QByteArray();
    }
    
    const PluginInfo& plugin = m_pluginCache[pluginId];
    QNetworkRequest request(QUrl(plugin.downloadUrl));
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, 
        [this, pluginId](qint64 received, qint64 total) {
            emit downloadProgress(pluginId, received, total);
        });
    
    // 存储下载中的插件
    m_downloadingPlugins[pluginId] = QByteArray();
    
    connect(reply, &QNetworkReply::finished, this, [this, pluginId, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            m_downloadingPlugins[pluginId] = data;
            emit pluginDownloaded(pluginId);
        } else {
            emit errorOccurred("Download failed: " + reply->errorString());
        }
        reply->deleteLater();
    });
    
    return QByteArray();  // 异步返回，实际数据通过信号传递
}

bool PluginMarketManager::installPlugin(const QString& pluginId, const QByteArray& package) {
    if (!m_pluginCache.contains(pluginId) && package.isEmpty()) {
        return false;
    }
    
    // 模拟安装过程
    QString version = m_pluginCache.value(pluginId).version;
    m_installedPlugins[pluginId] = version;
    
    emit pluginInstalled(pluginId);
    savePluginCache();
    return true;
}

bool PluginMarketManager::uninstallPlugin(const QString& pluginId) {
    if (!m_installedPlugins.contains(pluginId)) {
        return false;
    }
    
    m_installedPlugins.remove(pluginId);
    emit pluginUninstalled(pluginId);
    savePluginCache();
    return true;
}

bool PluginMarketManager::updatePlugin(const QString& pluginId) {
    if (!m_installedPlugins.contains(pluginId)) {
        return false;
    }
    
    if (!m_pluginCache.contains(pluginId)) {
        return false;
    }
    
    const PluginInfo& plugin = m_pluginCache[pluginId];
    m_installedPlugins[pluginId] = plugin.version;
    
    emit pluginUpdated(pluginId);
    return true;
}

QList<QString> PluginMarketManager::getInstalledPlugins() const {
    return m_installedPlugins.keys();
}

QList<QString> PluginMarketManager::getAvailableUpdates() const {
    QList<QString> updates;
    
    for (auto it = m_installedPlugins.begin(); it != m_installedPlugins.end(); ++it) {
        QString pluginId = it.key();
        QString installedVersion = it.value();
        
        if (m_pluginCache.contains(pluginId)) {
            const PluginInfo& plugin = m_pluginCache[pluginId];
            if (plugin.version != installedVersion) {
                updates.append(pluginId);
            }
        }
    }
    
    return updates;
}

void PluginMarketManager::ratePlugin(const QString& pluginId, int rating, const QString& comment) {
    Q_UNUSED(pluginId)
    Q_UNUSED(rating)
    Q_UNUSED(comment)
    // 实际实现需要调用 API 提交评分
}

QList<QJsonObject> PluginMarketManager::getPluginReviews(const QString& pluginId) const {
    Q_UNUSED(pluginId)
    // 返回插件评论
    return QList<QJsonObject>();
}

bool PluginMarketManager::checkDependencies(const QString& pluginId) const {
    Q_UNUSED(pluginId)
    // 检查依赖
    return true;
}

QList<QString> PluginMarketManager::getMissingDependencies(const QString& pluginId) const {
    Q_UNUSED(pluginId)
    // 返回缺失的依赖
    return QList<QString>();
}

bool PluginMarketManager::installDependencies(const QString& pluginId) {
    Q_UNUSED(pluginId)
    // 安装依赖
    return true;
}

bool PluginMarketManager::verifyPluginSignature(const QString& pluginId) {
    Q_UNUSED(pluginId)
    // 验证插件签名
    return true;
}

bool PluginMarketManager::verifyPluginCompatibility(const QString& pluginId) const {
    if (!m_pluginCache.contains(pluginId)) return false;
    
    const PluginInfo& plugin = m_pluginCache[pluginId];
    // 简单版本检查
    QString appVersion = "0.2.0";
    return appVersion >= plugin.minAppVersion && appVersion <= plugin.maxAppVersion;
}

void PluginMarketManager::onDownloadFinished(QNetworkReply* reply) {
    Q_UNUSED(reply)
    // 下载完成处理
}

void PluginMarketManager::onDownloadProgress(qint64 received, qint64 total) {
    Q_UNUSED(received)
    Q_UNUSED(total)
    // 下载进度处理
}

void PluginMarketManager::loadPluginCache() {
    QString cacheFile = getPluginDataDir() + "/plugin_cache.json";
    QFile file(cacheFile);
    
    if (!file.exists()) {
        // 加载示例插件
        PluginInfo plugin1;
        plugin1.id = "com.example.theme-dracula";
        plugin1.name = "Dracula Theme";
        plugin1.version = "1.2.0";
        plugin1.description = "Dracula color theme for WindTerm";
        plugin1.author = "Dracula Team";
        plugin1.downloads = 15420;
        plugin1.rating = 4.8;
        plugin1.ratingCount = 234;
        plugin1.license = "MIT";
        plugin1.minAppVersion = "0.1.0";
        plugin1.maxAppVersion = "1.0.0";
        plugin1.tags << "theme" << "dark" << "popular";
        plugin1.verified = true;
        plugin1.lastUpdated = QDateTime::currentDateTime();
        m_pluginCache[plugin1.id] = plugin1;
        m_featuredPlugins.append(plugin1);
        m_popularPlugins.append(plugin1);
        
        PluginInfo plugin2;
        plugin2.id = "com.example.plugin-k8s";
        plugin2.name = "Kubernetes Manager";
        plugin2.version = "2.0.1";
        plugin2.description = "Manage Kubernetes clusters from WindTerm";
        plugin2.author = "CloudNative Team";
        plugin2.downloads = 8932;
        plugin2.rating = 4.6;
        plugin2.ratingCount = 156;
        plugin2.license = "Apache-2.0";
        plugin2.minAppVersion = "0.2.0";
        plugin2.maxAppVersion = "1.0.0";
        plugin2.tags << "kubernetes" << "cloud" << "devops";
        plugin2.verified = true;
        plugin2.lastUpdated = QDateTime::currentDateTime().addDays(-5);
        m_pluginCache[plugin2.id] = plugin2;
        m_popularPlugins.append(plugin2);
        
        PluginInfo plugin3;
        plugin3.id = "com.example.plugin-snippets";
        plugin3.name = "Code Snippets";
        plugin3.version = "1.0.0";
        plugin3.description = "Quick access to common code snippets";
        plugin3.author = "DevTools Inc";
        plugin3.downloads = 5234;
        plugin3.rating = 4.3;
        plugin3.ratingCount = 89;
        plugin3.license = "MIT";
        plugin3.minAppVersion = "0.1.5";
        plugin3.maxAppVersion = "1.0.0";
        plugin3.tags << "productivity" << "snippets" << "new";
        plugin3.verified = false;
        plugin3.lastUpdated = QDateTime::currentDateTime().addDays(-2);
        m_pluginCache[plugin3.id] = plugin3;
        
        savePluginCache();
        emit pluginsLoaded(m_pluginCache.size());
        return;
    }
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        QJsonObject root = doc.object();
        QJsonArray plugins = root["plugins"].toArray();
        
        for (const QJsonValue& val : plugins) {
            QJsonObject obj = val.toObject();
            PluginInfo info;
            info.id = obj["id"].toString();
            info.name = obj["name"].toString();
            info.version = obj["version"].toString();
            info.description = obj["description"].toString();
            info.author = obj["author"].toString();
            info.downloads = obj["downloads"].toInt();
            info.rating = obj["rating"].toDouble();
            
            m_pluginCache[info.id] = info;
        }
        
        emit pluginsLoaded(m_pluginCache.size());
    }
}

void PluginMarketManager::savePluginCache() {
    QString cacheDir = getPluginDataDir();
    QDir().mkpath(cacheDir);
    
    QString cacheFile = cacheDir + "/plugin_cache.json";
    QFile file(cacheFile);
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject root;
        QJsonArray plugins;
        
        for (auto it = m_pluginCache.begin(); it != m_pluginCache.end(); ++it) {
            QJsonObject obj;
            obj["id"] = it->id;
            obj["name"] = it->name;
            obj["version"] = it->version;
            obj["description"] = it->description;
            obj["author"] = it->author;
            obj["downloads"] = it->downloads;
            obj["rating"] = it->rating;
            plugins.append(obj);
        }
        
        root["plugins"] = plugins;
        root["lastUpdated"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
    
    // 保存已安装插件列表
    QString installedFile = cacheDir + "/installed_plugins.json";
    QFile installed(installedFile);
    
    if (installed.open(QIODevice::WriteOnly)) {
        QJsonObject root;
        for (auto it = m_installedPlugins.begin(); it != m_installedPlugins.end(); ++it) {
            root[it.key()] = it.value();
        }
        installed.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        installed.close();
    }
}

QString PluginMarketManager::getPluginDataDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/plugins";
}

#include "PluginMarketManager.moc"
