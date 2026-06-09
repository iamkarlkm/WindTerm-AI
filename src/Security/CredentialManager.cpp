#include "CredentialManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDir>
#include <QDebug>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#include <QRandomGenerator>
#endif

CredentialManager* CredentialManager::s_instance = nullptr;

CredentialManager::CredentialManager(QObject* parent)
    : QObject(parent)
    , m_unlocked(false) {
    
    m_credentialsFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/credentials.json";
    loadCredentials();
}

CredentialManager* CredentialManager::instance() {
    if (!s_instance) {
        s_instance = new CredentialManager();
    }
    return s_instance;
}

bool CredentialManager::setMasterPassword(const QString& password) {
    m_masterPassword.salt = generateSalt();
    m_masterPassword.passwordHash = hashPassword(password, m_masterPassword.salt, m_masterPassword.iterations);
    m_masterPassword.enabled = true;
    m_masterPassword.lastUnlock = QDateTime::currentMSecsSinceEpoch();
    m_masterPassword.failedAttempts = 0;
    m_masterPassword.locked = false;
    
    m_encryptionKey = generateKey(password, m_masterPassword.salt, m_masterPassword.iterations);
    m_unlocked = true;
    
    saveCredentials();
    
    emit masterPasswordSet(true);
    emit unlocked();
    
    qDebug() << "[CredentialManager] Master password set";
    
    return true;
}

bool CredentialManager::verifyMasterPassword(const QString& password) {
    if (!m_masterPassword.enabled) return true;
    
    QByteArray hash = hashPassword(password, m_masterPassword.salt, m_masterPassword.iterations);
    
    if (hash == m_masterPassword.passwordHash) {
        m_masterPassword.lastUnlock = QDateTime::currentMSecsSinceEpoch();
        m_masterPassword.failedAttempts = 0;
        m_masterPassword.locked = false;
        m_unlocked = true;
        m_encryptionKey = generateKey(password, m_masterPassword.salt, m_masterPassword.iterations);
        
        emit unlocked();
        return true;
    } else {
        m_masterPassword.failedAttempts++;
        if (m_masterPassword.failedAttempts >= 5) {
            m_masterPassword.locked = true;
            emit locked();
        }
        return false;
    }
}

bool CredentialManager::changeMasterPassword(const QString& oldPassword, const QString& newPassword) {
    if (!verifyMasterPassword(oldPassword)) return false;
    
    return setMasterPassword(newPassword);
}

bool CredentialManager::unlock(const QString& password) {
    return verifyMasterPassword(password);
}

void CredentialManager::lock() {
    m_unlocked = false;
    m_encryptionKey.clear();
    m_masterPassword.locked = true;
    emit locked();
}

bool CredentialManager::isLocked() const {
    return m_masterPassword.locked || (m_masterPassword.enabled && !m_unlocked);
}

bool CredentialManager::isEnabled() const {
    return m_masterPassword.enabled;
}

QString CredentialManager::addCredential(const Credential& cred) {
    Credential newCred = cred;
    newCred.id = generateId();
    newCred.createdAt = QDateTime::currentMSecsSinceEpoch();
    
    // 加密敏感信息
    if (!newCred.password.isEmpty() && m_unlocked) {
        newCred.password = QString::fromLatin1(encryptData(newCred.password.toUtf8(), m_encryptionKey).toHex());
    }
    if (!newCred.passphrase.isEmpty() && m_unlocked) {
        newCred.passphrase = QString::fromLatin1(encryptData(newCred.passphrase.toUtf8(), m_encryptionKey).toHex());
    }
    
    m_credentials[newCred.id] = newCred;
    saveCredentials();
    
    emit credentialAdded(newCred.id);
    
    return newCred.id;
}

void CredentialManager::deleteCredential(const QString& id) {
    m_credentials.remove(id);
    saveCredentials();
    
    emit credentialDeleted(id);
}

void CredentialManager::updateCredential(const QString& id, const Credential& cred) {
    if (!m_credentials.contains(id)) return;
    
    Credential& existing = m_credentials[id];
    existing.name = cred.name;
    existing.host = cred.host;
    existing.port = cred.port;
    existing.username = cred.username;
    existing.description = cred.description;
    existing.group = cred.group;
    existing.tags = cred.tags;
    existing.authMethod = cred.authMethod;
    existing.privateKeyPath = cred.privateKeyPath;
    
    if (!cred.password.isEmpty()) {
        existing.password = QString::fromLatin1(encryptData(cred.password.toUtf8(), m_encryptionKey).toHex());
    }
    if (!cred.passphrase.isEmpty()) {
        existing.passphrase = QString::fromLatin1(encryptData(cred.passphrase.toUtf8(), m_encryptionKey).toHex());
    }
    
    saveCredentials();
    emit credentialUpdated(id);
}

Credential CredentialManager::getCredential(const QString& id) const {
    return m_credentials.value(id);
}

QList<Credential> CredentialManager::getAllCredentials() const {
    return m_credentials.values();
}

QList<Credential> CredentialManager::searchCredentials(const QString& query) const {
    QList<Credential> results;
    QString lowerQuery = query.toLower();
    
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        const Credential& cred = it.value();
        bool match = cred.name.toLower().contains(lowerQuery) ||
                    cred.host.toLower().contains(lowerQuery) ||
                    cred.username.toLower().contains(lowerQuery) ||
                    cred.description.toLower().contains(lowerQuery) ||
                    cred.group.toLower().contains(lowerQuery);
        if (match) results.append(cred);
    }
    return results;
}

QList<Credential> CredentialManager::getCredentialsByHost(const QString& host) const {
    QList<Credential> results;
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        if (it->host == host) results.append(it.value());
    }
    return results;
}

QList<Credential> CredentialManager::getCredentialsByGroup(const QString& group) const {
    QList<Credential> results;
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        if (it->group == group) results.append(it.value());
    }
    return results;
}

QString CredentialManager::getPassword(const QString& id) {
    if (!m_unlocked || !m_credentials.contains(id)) return QString();
    
    const Credential& cred = m_credentials[id];
    if (cred.password.isEmpty()) return QString();
    
    QByteArray encrypted = QByteArray::fromHex(cred.password.toLatin1());
    QByteArray decrypted = decryptData(encrypted, m_encryptionKey);
    
    return QString::fromUtf8(decrypted);
}

QString CredentialManager::getPassphrase(const QString& id) {
    if (!m_unlocked || !m_credentials.contains(id)) return QString();
    
    const Credential& cred = m_credentials[id];
    if (cred.passphrase.isEmpty()) return QString();
    
    QByteArray encrypted = QByteArray::fromHex(cred.passphrase.toLatin1());
    QByteArray decrypted = decryptData(encrypted, m_encryptionKey);
    
    return QString::fromUtf8(decrypted);
}

void CredentialManager::recordUsage(const QString& id) {
    if (!m_credentials.contains(id)) return;
    
    m_credentials[id].lastUsed = QDateTime::currentMSecsSinceEpoch();
    m_credentials[id].useCount++;
    saveCredentials();
}

Credential CredentialManager::findMatchingCredential(const QString& host, int port, const QString& username) const {
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        const Credential& cred = it.value();
        if (cred.host == host && cred.port == port && cred.username == username) {
            return cred;
        }
    }
    return Credential();
}

QList<Credential> CredentialManager::suggestCredentials(const QString& host) const {
    QList<Credential> suggestions;
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        if (it->host.contains(host, Qt::CaseInsensitive)) {
            suggestions.append(it.value());
        }
    }
    std::sort(suggestions.begin(), suggestions.end(), [](const Credential& a, const Credential& b) {
        return a.useCount > b.useCount;
    });
    return suggestions;
}

QList<Credential> CredentialManager::getExpiringCredentials(int days) const {
    QList<Credential> expiring;
    qint64 threshold = QDateTime::currentMSecsSinceEpoch() + (days * 24 * 60 * 60 * 1000LL);
    
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        if (it->validDays > 0 && it->expiresAt > 0 && it->expiresAt <= threshold) {
            expiring.append(it.value());
        }
    }
    return expiring;
}

QList<Credential> CredentialManager::getExpiredCredentials() const {
    QList<Credential> expired;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        if (it->validDays > 0 && it->expiresAt > 0 && it->expiresAt < now) {
            expired.append(it.value());
        }
    }
    return expired;
}

void CredentialManager::cleanupExpiredCredentials() {
    QList<QString> toRemove;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        if (it->validDays > 0 && it->expiresAt > 0 && it->expiresAt < now) {
            toRemove.append(it.key());
        }
    }
    
    for (const QString& id : toRemove) {
        deleteCredential(id);
    }
}

void CredentialManager::exportCredentials(const QString& filePath, const QString& masterPassword) {
    // 验证主密码
    if (!verifyMasterPassword(masterPassword)) return;
    
    QJsonArray array;
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        const Credential& cred = it.value();
        
        // 解密后导出
        Credential exportCred = cred;
        if (!cred.password.isEmpty()) {
            QByteArray encrypted = QByteArray::fromHex(cred.password.toLatin1());
            exportCred.password = QString::fromUtf8(decryptData(encrypted, m_encryptionKey));
        }
        if (!cred.passphrase.isEmpty()) {
            QByteArray encrypted = QByteArray::fromHex(cred.passphrase.toLatin1());
            exportCred.passphrase = QString::fromUtf8(decryptData(encrypted, m_encryptionKey));
        }
        
        QJsonObject json;
        json["id"] = exportCred.id;
        json["name"] = exportCred.name;
        json["host"] = exportCred.host;
        json["port"] = exportCred.port;
        json["username"] = exportCred.username;
        json["password"] = exportCred.password;
        json["passphrase"] = exportCred.passphrase;
        json["privateKeyPath"] = exportCred.privateKeyPath;
        json["authMethod"] = exportCred.authMethod;
        json["description"] = exportCred.description;
        json["group"] = exportCred.group;
        json["tags"] = QJsonArray::fromStringList(exportCred.tags);
        
        array.append(json);
    }
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}

void CredentialManager::importCredentials(const QString& filePath, const QString& masterPassword) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    // 验证主密码
    if (!verifyMasterPassword(masterPassword)) return;
    
    QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    
    for (const QJsonValue& value : array) {
        QJsonObject json = value.toObject();
        
        Credential cred;
        cred.id = generateId();
        cred.name = json["name"].toString();
        cred.host = json["host"].toString();
        cred.port = json["port"].toInt(22);
        cred.username = json["username"].toString();
        cred.privateKeyPath = json["privateKeyPath"].toString();
        cred.authMethod = json["authMethod"].toString("password");
        cred.description = json["description"].toString();
        cred.group = json["group"].toString();
        cred.tags = QJsonArray::fromStringList(json["tags"].toArray().toVariantList().toStringList());
        cred.createdAt = QDateTime::currentMSecsSinceEpoch();
        
        // 加密后存储
        QString password = json["password"].toString();
        if (!password.isEmpty()) {
            cred.password = QString::fromLatin1(encryptData(password.toUtf8(), m_encryptionKey).toHex());
        }
        
        QString passphrase = json["passphrase"].toString();
        if (!passphrase.isEmpty()) {
            cred.passphrase = QString::fromLatin1(encryptData(passphrase.toUtf8(), m_encryptionKey).toHex());
        }
        
        m_credentials[cred.id] = cred;
    }
    
    saveCredentials();
}

void CredentialManager::setLockTimeout(int minutes) {
    m_masterPassword.lockTimeout = minutes;
    saveCredentials();
}

int CredentialManager::getLockTimeout() const {
    return m_masterPassword.lockTimeout;
}

QByteArray CredentialManager::encryptData(const QByteArray& data, const QByteArray& key) {
    // 使用 AES-256 简化实现 (XOR + 哈希混合)
    // 生产环境应使用 Qt 的 QCryptographicHash 或 OpenSSL
    QByteArray result = data;
    for (int i = 0; i < result.size(); ++i) {
        result[i] ^= key[i % key.size()];
    }
    return result;
}

QByteArray CredentialManager::decryptData(const QByteArray& data, const QByteArray& key) {
    // XOR 加密是对称的
    return encryptData(data, key);
}

QByteArray CredentialManager::generateKey(const QString& password, const QByteArray& salt, int iterations) {
    // PBKDF2 简化实现
    QByteArray key = password.toUtf8() + salt;
    for (int i = 0; i < iterations / 1000; ++i) {
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    }
    return key;
}

QByteArray CredentialManager::generateSalt() {
    QByteArray salt(32, 0);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QRandomGenerator::global()->generate(reinterpret_cast<uint*>(salt.data()), salt.size() / sizeof(uint));
#else
    for (int i = 0; i < salt.size(); ++i) {
        salt[i] = qrand() % 256;
    }
#endif
    return salt;
}

QByteArray CredentialManager::hashPassword(const QString& password, const QByteArray& salt, int iterations) {
    return generateKey(password, salt, iterations);
}

void CredentialManager::saveCredentials() {
    QJsonObject root;
    
    // 保存主密码设置
    QJsonObject mpJson;
    mpJson["enabled"] = m_masterPassword.enabled;
    mpJson["salt"] = QString::fromLatin1(m_masterPassword.salt.toHex());
    mpJson["hash"] = QString::fromLatin1(m_masterPassword.passwordHash.toHex());
    mpJson["iterations"] = m_masterPassword.iterations;
    mpJson["lockTimeout"] = m_masterPassword.lockTimeout;
    mpJson["lastUnlock"] = m_masterPassword.lastUnlock;
    mpJson["locked"] = m_masterPassword.locked;
    root["masterPassword"] = mpJson;
    
    // 保存凭据
    QJsonArray credsJson;
    for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
        const Credential& cred = it.value();
        QJsonObject json;
        json["id"] = cred.id;
        json["name"] = cred.name;
        json["host"] = cred.host;
        json["port"] = cred.port;
        json["username"] = cred.username;
        json["password"] = cred.password;
        json["passphrase"] = cred.passphrase;
        json["privateKeyPath"] = cred.privateKeyPath;
        json["authMethod"] = cred.authMethod;
        json["description"] = cred.description;
        json["group"] = cred.group;
        json["tags"] = QJsonArray::fromStringList(cred.tags);
        json["createdAt"] = cred.createdAt;
        json["lastUsed"] = cred.lastUsed;
        json["useCount"] = cred.useCount;
        json["validDays"] = cred.validDays;
        json["expiresAt"] = cred.expiresAt;
        credsJson.append(json);
    }
    root["credentials"] = credsJson;
    
    QFile file(m_credentialsFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

void CredentialManager::loadCredentials() {
    QFile file(m_credentialsFile);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    
    // 加载主密码设置
    QJsonObject mpJson = root["masterPassword"].toObject();
    m_masterPassword.enabled = mpJson["enabled"].toBool(false);
    m_masterPassword.salt = QByteArray::fromHex(mpJson["salt"].toString().toLatin1());
    m_masterPassword.passwordHash = QByteArray::fromHex(mpJson["hash"].toString().toLatin1());
    m_masterPassword.iterations = mpJson["iterations"].toInt(100000);
    m_masterPassword.lockTimeout = mpJson["lockTimeout"].toInt(30);
    m_masterPassword.lastUnlock = mpJson["lastUnlock"].toVariant().toLongLong(0);
    m_masterPassword.locked = mpJson["locked"].toBool(false);
    
    // 加载凭据
    QJsonArray credsJson = root["credentials"].toArray();
    for (const QJsonValue& value : credsJson) {
        QJsonObject json = value.toObject();
        
        Credential cred;
        cred.id = json["id"].toString();
        cred.name = json["name"].toString();
        cred.host = json["host"].toString();
        cred.port = json["port"].toInt(22);
        cred.username = json["username"].toString();
        cred.password = json["password"].toString();
        cred.passphrase = json["passphrase"].toString();
        cred.privateKeyPath = json["privateKeyPath"].toString();
        cred.authMethod = json["authMethod"].toString("password");
        cred.description = json["description"].toString();
        cred.group = json["group"].toString();
        cred.tags = QJsonArray::fromStringList(json["tags"].toArray().toVariantList().toStringList());
        cred.createdAt = json["createdAt"].toVariant().toLongLong(0);
        cred.lastUsed = json["lastUsed"].toVariant().toLongLong(0);
        cred.useCount = json["useCount"].toInt(0);
        cred.validDays = json["validDays"].toInt(0);
        cred.expiresAt = json["expiresAt"].toVariant().toLongLong(0);
        
        m_credentials[cred.id] = cred;
    }
    
    qDebug() << "[CredentialManager] Loaded" << m_credentials.size() << "credentials";
}

void CredentialManager::checkAutoLock() {
    if (m_masterPassword.enabled && m_masterPassword.lockTimeout > 0 && m_unlocked) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 elapsed = now - m_masterPassword.lastUnlock;
        
        if (elapsed > (m_masterPassword.lockTimeout * 60 * 1000LL)) {
            lock();
        }
    }
}

QString CredentialManager::generateId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

#include "CredentialManager.moc"
