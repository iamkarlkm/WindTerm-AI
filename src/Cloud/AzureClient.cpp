#include "AzureClient.h"
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
#include <QUuid>

AzureClient* AzureClient::s_instance = nullptr;

AzureClient::AzureClient(QObject* parent) : QObject(parent) {
}

AzureClient::~AzureClient() {
}

AzureClient* AzureClient::instance() {
    if (!s_instance) s_instance = new AzureClient();
    return s_instance;
}

void AzureClient::setCredentials(const QString& tenantId, const QString& clientId, const QString& clientSecret) {
    m_tenantId = tenantId;
    m_clientId = clientId;
    m_clientSecret = clientSecret;
    emit credentialsUpdated();
}

void AzureClient::setSubscriptionId(const QString& subscriptionId) {
    m_subscriptionId = subscriptionId;
    emit subscriptionChanged(subscriptionId);
}

QString AzureClient::getSubscriptionId() const {
    return m_subscriptionId;
}

QList<AzureVM> AzureClient::listVirtualMachines(const QString& resourceGroup) const {
    // 模拟返回数据
    QList<AzureVM> vms;
    
    AzureVM vm1;
    vm1.vmId = "/subscriptions/xxx/resourceGroups/rg1/providers/Microsoft.Compute/virtualMachines/web-vm-1";
    vm1.name = "web-vm-1";
    vm1.size = "Standard_B2s";
    vm1.powerState = "running";
    vm1.location = m_region;
    vm1.resourceGroup = resourceGroup.isEmpty() ? "production-rg" : resourceGroup;
    vm1.publicIp = "20.123.45.67";
    vm1.privateIp = "10.0.1.4";
    vm1.osType = "Linux";
    vm1.createdTime = QDateTime::currentDateTime().addDays(-45);
    vms.append(vm1);
    
    AzureVM vm2;
    vm2.vmId = "/subscriptions/xxx/resourceGroups/rg1/providers/Microsoft.Compute/virtualMachines/db-vm-1";
    vm2.name = "db-vm-1";
    vm2.size = "Standard_D4s_v3";
    vm2.powerState = "running";
    vm2.location = m_region;
    vm2.resourceGroup = resourceGroup.isEmpty() ? "production-rg" : resourceGroup;
    vm2.publicIp = "";
    vm2.privateIp = "10.0.1.5";
    vm2.osType = "Linux";
    vm2.createdTime = QDateTime::currentDateTime().addDays(-30);
    vms.append(vm2);
    
    AzureVM vm3;
    vm3.vmId = "/subscriptions/xxx/resourceGroups/rg2/providers/Microsoft.Compute/virtualMachines/dev-vm-1";
    vm3.name = "dev-vm-1";
    vm3.size = "Standard_B1s";
    vm3.powerState = "deallocated";
    vm3.location = "westus";
    vm3.resourceGroup = "development-rg";
    vm3.publicIp = "";
    vm3.privateIp = "10.0.2.4";
    vm3.osType = "Windows";
    vm3.createdTime = QDateTime::currentDateTime().addDays(-10);
    vms.append(vm3);
    
    m_vmsCache = vms;
    m_cacheTime = QDateTime::currentSecsSinceEpoch();
    
    emit vmsUpdated(vms);
    return vms;
}

bool AzureClient::startVM(const QString& resourceGroup, const QString& vmName) {
    qDebug() << "Starting VM:" << vmName << "in" << resourceGroup;
    emit operationCompleted("startVM", true);
    return true;
}

bool AzureClient::stopVM(const QString& resourceGroup, const QString& vmName) {
    qDebug() << "Stopping VM:" << vmName << "in" << resourceGroup;
    emit operationCompleted("stopVM", true);
    return true;
}

bool AzureClient::deallocateVM(const QString& resourceGroup, const QString& vmName) {
    qDebug() << "Deallocating VM:" << vmName << "in" << resourceGroup;
    emit operationCompleted("deallocateVM", true);
    return true;
}

bool AzureClient::deleteVM(const QString& resourceGroup, const QString& vmName) {
    qDebug() << "Deleting VM:" << vmName << "in" << resourceGroup;
    emit operationCompleted("deleteVM", true);
    return true;
}

bool AzureClient::restartVM(const QString& resourceGroup, const QString& vmName) {
    qDebug() << "Restarting VM:" << vmName << "in" << resourceGroup;
    emit operationCompleted("restartVM", true);
    return true;
}

QString AzureClient::getVMConnectCommand(const QString& resourceGroup, const QString& vmName, const QString& username) const {
    for (const auto& vm : m_vmsCache) {
        if (vm.name == vmName && vm.resourceGroup == resourceGroup && !vm.publicIp.isEmpty()) {
            if (vm.osType == "Linux") {
                return QString("ssh %1@%2").arg(username, vm.publicIp);
            } else {
                return QString("mstsc /v:%1").arg(vm.publicIp);
            }
        }
    }
    return QString();
}

QList<AzureStorageAccount> AzureClient::listStorageAccounts(const QString& resourceGroup) const {
    // 模拟返回
    QList<AzureStorageAccount> accounts;
    
    AzureStorageAccount acc1;
    acc1.id = "/subscriptions/xxx/resourceGroups/rg1/providers/Microsoft.Storage/storageAccounts/prodstorage";
    acc1.name = "prodstorage";
    acc1.resourceGroup = resourceGroup.isEmpty() ? "production-rg" : resourceGroup;
    acc1.location = m_region;
    acc1.sku = "Standard_LRS";
    acc1.kind = "StorageV2";
    acc1.primaryEndpoint = "https://prodstorage.blob.core.windows.net/";
    accounts.append(acc1);
    
    AzureStorageAccount acc2;
    acc2.id = "/subscriptions/xxx/resourceGroups/rg2/providers/Microsoft.Storage/storageAccounts/devstorage";
    acc2.name = "devstorage";
    acc2.resourceGroup = "development-rg";
    acc2.location = "westus";
    acc2.sku = "Standard_GRS";
    acc2.kind = "BlobStorage";
    acc2.primaryEndpoint = "https://devstorage.blob.core.windows.net/";
    accounts.append(acc2);
    
    m_storageCache = accounts;
    emit storageAccountsUpdated(accounts);
    return accounts;
}

QJsonObject AzureClient::listBlobs(const QString& accountName, const QString& container, const QString& prefix) const {
    Q_UNUSED(accountName)
    Q_UNUSED(container)
    Q_UNUSED(prefix)
    
    QJsonObject result;
    result["container"] = container;
    result["prefix"] = prefix;
    result["blobs"] = QJsonArray();
    return result;
}

bool AzureClient::uploadBlob(const QString& accountName, const QString& container, const QString& blobName, const QString& filePath) {
    qDebug() << "Uploading" << filePath << "to" << accountName << "/" << container << "/" << blobName;
    emit operationCompleted("uploadBlob", true);
    return true;
}

bool AzureClient::downloadBlob(const QString& accountName, const QString& container, const QString& blobName, const QString& localPath) {
    qDebug() << "Downloading" << accountName << "/" << container << "/" << blobName << "to" << localPath;
    emit operationCompleted("downloadBlob", true);
    return true;
}

bool AzureClient::deleteBlob(const QString& accountName, const QString& container, const QString& blobName) {
    qDebug() << "Deleting" << accountName << "/" << container << "/" << blobName;
    emit operationCompleted("deleteBlob", true);
    return true;
}

QStringList AzureClient::listResourceGroups() const {
    return {"production-rg", "development-rg", "staging-rg", "testing-rg"};
}

bool AzureClient::createResourceGroup(const QString& name, const QString& location) {
    qDebug() << "Creating resource group:" << name << "in" << location;
    emit operationCompleted("createResourceGroup", true);
    return true;
}

bool AzureClient::deleteResourceGroup(const QString& name) {
    qDebug() << "Deleting resource group:" << name;
    emit operationCompleted("deleteResourceGroup", true);
    return true;
}

QStringList AzureClient::getAvailableRegions() const {
    return {
        "eastus",           // US East
        "eastus2",          // US East 2
        "westus",           // US West
        "westus2",          // US West 2
        "centralus",        // US Central
        "northcentralus",   // US North Central
        "southcentralus",   // US South Central
        "westcentralus",    // US West Central
        "northeurope",      // Europe North
        "westeurope",       // Europe West
        "eastasia",         // Asia East
        "southeastasia",    // Asia Southeast
        "japaneast",        // Japan East
        "japanwest",        // Japan West
        "australiaeast",    // Australia East
        "australiasoutheast", // Australia Southeast
        "brazilsouth",      // Brazil South
        "canadacentral",    // Canada Central
        "canadaeast",       // Canada East
        "indiacentral",     // India Central
        "indiasouth",       // India South
        "koreacentral",     // Korea Central
        "koreasouth",       // Korea South
        "germanywestcentral", // Germany West Central
        "francecentral",    // France Central
        "uksouth",          // UK South
        "ukwest",           // UK West
        "uaenorth",         // UAE North
        "switzerlandnorth", // Switzerland North
        "norwayeast",       // Norway East
        "southafricanorth"  // South Africa North
    };
}

QString AzureClient::getCurrentRegion() const {
    return m_region;
}

void AzureClient::setRegion(const QString& region) {
    if (m_region != region) {
        m_region = region;
    }
}

QString AzureClient::getAccessToken() const {
    // 实际实现需要调用 Azure AD OAuth2 API 获取访问令牌
    return QString();
}

QByteArray AzureClient::makeRequest(const QString& method, const QString& url, const QJsonObject& body) const {
    Q_UNUSED(method)
    Q_UNUSED(url)
    Q_UNUSED(body)
    // 实际 Azure REST API 调用实现
    return QByteArray();
}

AzureVM AzureClient::parseVM(const QJsonObject& json) const {
    AzureVM vm;
    vm.vmId = json["id"].toString();
    vm.name = json["name"].toString();
    vm.location = json["location"].toString();
    // 解析其他字段...
    return vm;
}

AzureStorageAccount AzureClient::parseStorageAccount(const QJsonObject& json) const {
    AzureStorageAccount account;
    account.id = json["id"].toString();
    account.name = json["name"].toString();
    account.location = json["location"].toString();
    // 解析其他字段...
    return account;
}

#include "AzureClient.moc"
