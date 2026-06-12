#ifndef AZURECLIENT_H
#define AZURECLIENT_H

#include <QObject>
#include <QMap>
#include <QList>

/**
 * @brief Azure VM 实例信息
 */
struct AzureVM {
    QString vmId;
    QString name;
    QString size;
    QString powerState;    // running, stopped, deallocated
    QString location;
    QString resourceGroup;
    QString publicIp;
    QString privateIp;
    QString osType;        // Linux, Windows
    QDateTime createdTime;
};

/**
 * @brief Azure 存储账户信息
 */
struct AzureStorageAccount {
    QString id;
    QString name;
    QString resourceGroup;
    QString location;
    QString sku;           // Standard_LRS, Premium_LRS, etc.
    QString kind;          // Storage, BlobStorage, etc.
    QString primaryEndpoint;
};

/**
 * @brief Azure 客户端 - VM/存储管理
 * 
 * 功能:
 * - 虚拟机管理 (列表/启动/停止/删除)
 * - 存储账户管理
 * - 资源组管理
 * - 区域查询
 */
class AzureClient : public QObject {
    Q_OBJECT

public:
    explicit AzureClient(QObject* parent = nullptr);
    ~AzureClient();

    // 认证配置
    void setCredentials(const QString& tenantId, const QString& clientId, const QString& clientSecret);
    void setSubscriptionId(const QString& subscriptionId);
    QString getSubscriptionId() const;

    // VM 操作
    QList<AzureVM> listVirtualMachines(const QString& resourceGroup = "") const;
    bool startVM(const QString& resourceGroup, const QString& vmName);
    bool stopVM(const QString& resourceGroup, const QString& vmName);
    bool deallocateVM(const QString& resourceGroup, const QString& vmName);
    bool deleteVM(const QString& resourceGroup, const QString& vmName);
    bool restartVM(const QString& resourceGroup, const QString& vmName);
    
    // VM 连接
    QString getVMConnectCommand(const QString& resourceGroup, const QString& vmName, const QString& username = "azureuser") const;
    
    // 存储账户
    QList<AzureStorageAccount> listStorageAccounts(const QString& resourceGroup = "") const;
    QJsonObject listBlobs(const QString& accountName, const QString& container, const QString& prefix = "") const;
    bool uploadBlob(const QString& accountName, const QString& container, const QString& blobName, const QString& filePath);
    bool downloadBlob(const QString& accountName, const QString& container, const QString& blobName, const QString& localPath);
    bool deleteBlob(const QString& accountName, const QString& container, const QString& blobName);
    
    // 资源组
    QStringList listResourceGroups() const;
    bool createResourceGroup(const QString& name, const QString& location);
    bool deleteResourceGroup(const QString& name);
    
    // 区域
    QStringList getAvailableRegions() const;
    QString getCurrentRegion() const;
    void setRegion(const QString& region);

signals:
    void credentialsUpdated();
    void subscriptionChanged(const QString& subscriptionId);
    void vmsUpdated(const QList<AzureVM>& vms);
    void storageAccountsUpdated(const QList<AzureStorageAccount>& accounts);
    void operationCompleted(const QString& operation, bool success);
    void errorOccurred(const QString& message, int code);

private:
    // Azure API 辅助
    QString getAccessToken() const;
    QByteArray makeRequest(const QString& method, const QString& url, const QJsonObject& body = QJsonObject()) const;
    
    // 解析辅助
    AzureVM parseVM(const QJsonObject& json) const;
    AzureStorageAccount parseStorageAccount(const QJsonObject& json) const;
    
    QString m_tenantId;
    QString m_clientId;
    QString m_clientSecret;
    QString m_subscriptionId;
    QString m_region = "eastus";
    
    // 缓存
    mutable QList<AzureVM> m_vmsCache;
    mutable QList<AzureStorageAccount> m_storageCache;
    mutable qint64 m_cacheTime = 0;
    static const int CACHE_TTL_SECONDS = 300;
    
    static AzureClient* s_instance;

public:
    static AzureClient* instance();
};

#endif // AZURECLIENT_H
