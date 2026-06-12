#ifndef PLUGINMARKETMANAGER_H
#define PLUGINMARKETMANAGER_H

#include <QObject>
#include <QMap>
#include <QNetworkAccessManager>

/**
 * @brief 插件信息
 */
struct PluginInfo {
    QString id;
    QString name;
    QString version;
    QString description;
    QString author;
    QString repository;
    QString downloadUrl;
    QString homepage;
    QStringList tags;
    int downloads;
    double rating;
    int ratingCount;
    QString license;
    QString minAppVersion;
    QString maxAppVersion;
    QDateTime lastUpdated;
    QStringList screenshots;
    bool verified;
};

/**
 * @brief 插件市场管理器 - 第三方插件生态
 * 
 * 功能:
 * - 插件浏览/搜索
 * - 插件安装/卸载/更新
 * - 评分和评论
 * - 依赖管理
 * - 插件验证
 */
class PluginMarketManager : public QObject {
    Q_OBJECT

public:
    explicit PluginMarketManager(QObject* parent = nullptr);
    ~PluginMarketManager();

    // 市场连接
    void setMarketplaceUrl(const QString& url);
    bool connectToMarketplace();
    bool isConnected() const;

    // 插件浏览
    QList<PluginInfo> browsePlugins(int page = 1, int pageSize = 20) const;
    QList<PluginInfo> searchPlugins(const QString& query, const QStringList& tags = QStringList()) const;
    QList<PluginInfo> getFeaturedPlugins() const;
    QList<PluginInfo> getPopularPlugins() const;
    QList<PluginInfo> getNewPlugins() const;
    QList<PluginInfo> getPluginsByCategory(const QString& category) const;

    // 插件详情
    PluginInfo getPluginDetails(const QString& pluginId) const;
    QByteArray downloadPlugin(const QString& pluginId);
    
    // 插件管理
    bool installPlugin(const QString& pluginId, const QByteArray& package = QByteArray());
    bool uninstallPlugin(const QString& pluginId);
    bool updatePlugin(const QString& pluginId);
    QList<QString> getInstalledPlugins() const;
    QList<QString> getAvailableUpdates() const;
    
    // 评分和评论
    void ratePlugin(const QString& pluginId, int rating, const QString& comment = "");
    QList<QJsonObject> getPluginReviews(const QString& pluginId) const;
    
    // 依赖管理
    bool checkDependencies(const QString& pluginId) const;
    QList<QString> getMissingDependencies(const QString& pluginId) const;
    bool installDependencies(const QString& pluginId);
    
    // 插件验证
    bool verifyPluginSignature(const QString& pluginId);
    bool verifyPluginCompatibility(const QString& pluginId) const;

signals:
    void marketplaceConnected();
    void marketplaceDisconnected();
    void pluginsLoaded(int count);
    void pluginDownloaded(const QString& pluginId);
    void pluginInstalled(const QString& pluginId);
    void pluginUninstalled(const QString& pluginId);
    void pluginUpdated(const QString& pluginId);
    void downloadProgress(const QString& pluginId, qint64 received, qint64 total);
    void errorOccurred(const QString& message);

private slots:
    void onDownloadFinished(QNetworkReply* reply);
    void onDownloadProgress(qint64 received, qint64 total);

private:
    QNetworkAccessManager* m_networkManager;
    QString m_marketplaceUrl;
    bool m_connected;
    
    QMap<QString, PluginInfo> m_pluginCache;
    QList<PluginInfo> m_featuredPlugins;
    QList<PluginInfo> m_popularPlugins;
    QMap<QString, QString> m_installedPlugins;  // pluginId -> version
    QMap<QString, QByteArray> m_downloadingPlugins;
    
    void loadPluginCache();
    void savePluginCache();
    QString getPluginDataDir() const;
    
    static PluginMarketManager* s_instance;

public:
    static PluginMarketManager* instance();
};

#endif // PLUGINMARKETMANAGER_H
