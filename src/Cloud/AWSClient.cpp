#include "AWSClient.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QUrl>
#include <QFile>
#include <QDebug>
#include <QEventLoop>

AWSClient* AWSClient::s_instance = nullptr;

AWSClient::AWSClient(QObject* parent) : QObject(parent) {
}

AWSClient::~AWSClient() {
}

AWSClient* AWSClient::instance() {
    if (!s_instance) s_instance = new AWSClient();
    return s_instance;
}

void AWSClient::setCredentials(const QString& accessKey, const QString& secretKey) {
    m_accessKey = accessKey;
    m_secretKey = secretKey;
    emit credentialsUpdated();
}

void AWSClient::setRegion(const QString& region) {
    if (m_region != region) {
        m_region = region;
        emit regionChanged(region);
    }
}

QString AWSClient::getRegion() const {
    return m_region;
}

QList<EC2Instance> AWSClient::describeInstances(const QString& filter) const {
    Q_UNUSED(filter)
    // 模拟返回数据 (实际实现需要调用 AWS EC2 API)
    QList<EC2Instance> instances;
    
    EC2Instance inst1;
    inst1.instanceId = "i-0123456789abcdef0";
    inst1.instanceType = "t3.micro";
    inst1.state = "running";
    inst1.publicIp = "54.123.45.67";
    inst1.privateIp = "10.0.1.100";
    inst1.launchTime = QDateTime::currentDateTime().addDays(-30).toString(Qt::ISODate);
    inst1.availabilityZone = m_region + "a";
    inst1.vpcId = "vpc-0123456789abcdef0";
    inst1.tags << "Name=WebServer" << "Environment=Production";
    inst1.imageName = "ami-0123456789abcdef0";
    instances.append(inst1);
    
    EC2Instance inst2;
    inst2.instanceId = "i-0fedcba9876543210";
    inst2.instanceType = "t3.small";
    inst2.state = "stopped";
    inst2.publicIp = "";
    inst2.privateIp = "10.0.1.101";
    inst2.launchTime = QDateTime::currentDateTime().addDays(-15).toString(Qt::ISODate);
    inst2.availabilityZone = m_region + "b";
    inst2.vpcId = "vpc-0123456789abcdef0";
    inst2.tags << "Name=DBServer" << "Environment=Production";
    inst2.imageName = "ami-0fedcba9876543210";
    instances.append(inst2);
    
    m_instancesCache = instances;
    m_cacheTime = QDateTime::currentSecsSinceEpoch();
    
    emit instancesUpdated(instances);
    return instances;
}

bool AWSClient::startInstance(const QString& instanceId) {
    // 实际实现需要调用 AWS StartInstances API
    qDebug() << "Starting instance:" << instanceId;
    emit operationCompleted("startInstance", true);
    return true;
}

bool AWSClient::stopInstance(const QString& instanceId) {
    qDebug() << "Stopping instance:" << instanceId;
    emit operationCompleted("stopInstance", true);
    return true;
}

bool AWSClient::terminateInstance(const QString& instanceId) {
    qDebug() << "Terminating instance:" << instanceId;
    emit operationCompleted("terminateInstance", true);
    return true;
}

bool AWSClient::rebootInstance(const QString& instanceId) {
    qDebug() << "Rebooting instance:" << instanceId;
    emit operationCompleted("rebootInstance", true);
    return true;
}

QString AWSClient::getInstanceConnectCommand(const QString& instanceId, const QString& username) const {
    for (const auto& inst : m_instancesCache) {
        if (inst.instanceId == instanceId && !inst.publicIp.isEmpty()) {
            return QString("ssh -i ~/.ssh/%1.pem %2@%3")
                .arg(inst.instanceId, username, inst.publicIp);
        }
    }
    return QString();
}

QList<S3Bucket> AWSClient::listBuckets() const {
    // 模拟返回数据
    QList<S3Bucket> buckets;
    
    S3Bucket bucket1;
    bucket1.name = "my-application-logs";
    bucket1.region = m_region;
    bucket1.creationDate = QDateTime::currentDateTime().addMonths(-6).toString(Qt::ISODate);
    bucket1.size = 1024 * 1024 * 500;  // 500 MB
    bucket1.objectCount = 1234;
    buckets.append(bucket1);
    
    S3Bucket bucket2;
    bucket2.name = "backup-data";
    bucket2.region = m_region;
    bucket2.creationDate = QDateTime::currentDateTime().addMonths(-12).toString(Qt::ISODate);
    bucket2.size = 1024 * 1024 * 1024 * 5;  // 5 GB
    bucket2.objectCount = 567;
    buckets.append(bucket2);
    
    m_bucketsCache = buckets;
    emit bucketsUpdated(buckets);
    return buckets;
}

QJsonObject AWSClient::listObjects(const QString& bucketName, const QString& prefix) const {
    Q_UNUSED(bucketName)
    Q_UNUSED(prefix)
    QJsonObject result;
    result["objects"] = QJsonArray();
    result["prefix"] = prefix;
    return result;
}

bool AWSClient::uploadFile(const QString& bucketName, const QString& key, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred("Cannot open file: " + filePath, 400);
        return false;
    }
    
    // 实际实现需要调用 S3 PutObject API
    qDebug() << "Uploading" << filePath << "to" << bucketName << "/" << key;
    emit operationCompleted("uploadFile", true);
    return true;
}

bool AWSClient::downloadFile(const QString& bucketName, const QString& key, const QString& localPath) {
    Q_UNUSED(bucketName)
    Q_UNUSED(key)
    Q_UNUSED(localPath)
    // 实际实现需要调用 S3 GetObject API
    emit operationCompleted("downloadFile", true);
    return true;
}

bool AWSClient::deleteObject(const QString& bucketName, const QString& key) {
    Q_UNUSED(bucketName)
    Q_UNUSED(key)
    // 实际实现需要调用 S3 DeleteObject API
    emit operationCompleted("deleteObject", true);
    return true;
}

QStringList AWSClient::getAvailableRegions() const {
    return {
        "us-east-1",      // US East (N. Virginia)
        "us-east-2",      // US East (Ohio)
        "us-west-1",      // US West (N. California)
        "us-west-2",      // US West (Oregon)
        "eu-west-1",      // Europe (Ireland)
        "eu-west-2",      // Europe (London)
        "eu-central-1",   // Europe (Frankfurt)
        "ap-southeast-1", // Asia Pacific (Singapore)
        "ap-southeast-2", // Asia Pacific (Sydney)
        "ap-northeast-1", // Asia Pacific (Tokyo)
        "ap-northeast-2", // Asia Pacific (Seoul)
        "sa-east-1",      // South America (São Paulo)
        "ca-central-1"    // Canada (Montreal)
    };
}

QString AWSClient::getCurrentRegion() const {
    return m_region;
}

QStringList AWSClient::listKeyPairs() const {
    // 模拟返回
    return {"production-key", "development-key", "backup-key"};
}

bool AWSClient::createKeyPair(const QString& keyName) {
    Q_UNUSED(keyName)
    // 实际实现需要调用 EC2 CreateKeyPair API
    emit operationCompleted("createKeyPair", true);
    return true;
}

bool AWSClient::deleteKeyPair(const QString& keyName) {
    Q_UNUSED(keyName)
    // 实际实现需要调用 EC2 DeleteKeyPair API
    emit operationCompleted("deleteKeyPair", true);
    return true;
}

QString AWSClient::getKeyPairMaterial(const QString& keyName) const {
    Q_UNUSED(keyName)
    // 实际实现需要获取私钥内容
    return "-----BEGIN RSA PRIVATE KEY-----\n...\n-----END RSA PRIVATE KEY-----";
}

QString AWSClient::signRequest(const QString& service, const QString& method, const QString& path,
                                const QString& payload) const {
    Q_UNUSED(service)
    Q_UNUSED(method)
    Q_UNUSED(path)
    Q_UNUSED(payload)
    // AWS Signature Version 4 实现
    return QString();
}

QByteArray AWSClient::makeRequest(const QString& service, const QString& action,
                                   const QMap<QString, QString>& params) const {
    Q_UNUSED(service)
    Q_UNUSED(action)
    Q_UNUSED(params)
    // 实际 AWS API 调用实现
    return QByteArray();
}

QJsonArray AWSClient::parseJsonResponse(const QByteArray& response) const {
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isObject()) {
        return doc.object()["Reservations"].toArray();
    }
    return QJsonArray();
}

EC2Instance AWSClient::parseInstance(const QJsonObject& json) const {
    EC2Instance instance;
    instance.instanceId = json["InstanceId"].toString();
    instance.instanceType = json["InstanceType"].toString();
    instance.state = json["State"].toObject()["Name"].toString();
    instance.publicIp = json["PublicIpAddress"].toString();
    instance.privateIp = json["PrivateIpAddress"].toString();
    instance.launchTime = json["LaunchTime"].toString();
    // 解析其他字段...
    return instance;
}

S3Bucket AWSClient::parseBucket(const QJsonObject& json) const {
    S3Bucket bucket;
    bucket.name = json["Name"].toString();
    bucket.creationDate = json["CreationDate"].toString();
    // 解析其他字段...
    return bucket;
}

#include "AWSClient.moc"
