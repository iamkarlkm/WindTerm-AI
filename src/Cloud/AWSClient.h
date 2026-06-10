#ifndef AWSCLIENT_H
#define AWSCLIENT_H

#include <QObject>
#include <QMap>
#include <QJsonArray>

/**
 * @brief EC2 实例信息
 */
struct EC2Instance {
    QString instanceId;
    QString instanceType;
    QString state;      // running, stopped, terminated, etc.
    QString publicIp;
    QString privateIp;
    QString launchTime;
    QString availabilityZone;
    QString vpcId;
    QStringList tags;
    QString imageName;
};

/**
 * @brief S3 存储桶信息
 */
struct S3Bucket {
    QString name;
    QString region;
    QString creationDate;
    qint64 size;
    int objectCount;
};

/**
 * @brief AWS 客户端 - EC2/S3 管理
 * 
 * 功能:
 * - EC2 实例管理 (列表/启动/停止/终止)
 * - S3 存储桶管理
 * - 区域查询
 * - 密钥对管理
 */
class AWSClient : public QObject {
    Q_OBJECT

public:
    explicit AWSClient(QObject* parent = nullptr);
    ~AWSClient();

    // 认证配置
    void setCredentials(const QString& accessKey, const QString& secretKey);
    void setRegion(const QString& region);
    QString getRegion() const;

    // EC2 操作
    QList<EC2Instance> describeInstances(const QString& filter = "") const;
    bool startInstance(const QString& instanceId);
    bool stopInstance(const QString& instanceId);
    bool terminateInstance(const QString& instanceId);
    bool rebootInstance(const QString& instanceId);
    
    // EC2 连接
    QString getInstanceConnectCommand(const QString& instanceId, const QString& username = "ec2-user") const;
    
    // S3 操作
    QList<S3Bucket> listBuckets() const;
    QJsonObject listObjects(const QString& bucketName, const QString& prefix = "") const;
    bool uploadFile(const QString& bucketName, const QString& key, const QString& filePath);
    bool downloadFile(const QString& bucketName, const QString& key, const QString& localPath);
    bool deleteObject(const QString& bucketName, const QString& key);
    
    // 区域管理
    QStringList getAvailableRegions() const;
    QString getCurrentRegion() const;

    // 密钥对
    QStringList listKeyPairs() const;
    bool createKeyPair(const QString& keyName);
    bool deleteKeyPair(const QString& keyName);
    QString getKeyPairMaterial(const QString& keyName) const;

signals:
    void credentialsUpdated();
    void regionChanged(const QString& region);
    void instancesUpdated(const QList<EC2Instance>& instances);
    void bucketsUpdated(const QList<S3Bucket>& buckets);
    void operationCompleted(const QString& operation, bool success);
    void errorOccurred(const QString& message, int code);

private:
    // AWS API 调用辅助
    QString signRequest(const QString& service, const QString& method, const QString& path, 
                        const QString& payload = "") const;
    QByteArray makeRequest(const QString& service, const QString& action, 
                          const QMap<QString, QString>& params) const;
    QJsonArray parseJsonResponse(const QByteArray& response) const;
    
    // EC2 辅助方法
    EC2Instance parseInstance(const QJsonObject& json) const;
    
    // S3 辅助方法
    S3Bucket parseBucket(const QJsonObject& json) const;
    
    QString m_accessKey;
    QString m_secretKey;
    QString m_region = "us-east-1";
    
    // 缓存
    mutable QList<EC2Instance> m_instancesCache;
    mutable QList<S3Bucket> m_bucketsCache;
    mutable qint64 m_cacheTime = 0;
    static const int CACHE_TTL_SECONDS = 300;  // 5 分钟缓存
    
    static AWSClient* s_instance;

public:
    static AWSClient* instance();
};

#endif // AWSCLIENT_H
