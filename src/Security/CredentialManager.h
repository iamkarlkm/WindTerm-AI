#ifndef CREDENTIAL_MANAGER_H
#define CREDENTIAL_MANAGER_H

#include <QObject>
#include <QMap>
#include <QByteArray>

struct Credential {
    QString id;
    QString name;
    QString host;
    int port;
    QString username;
    QString password;
    QString privateKeyPath;
    QString passphrase;
    QString authMethod;
    QString description;
    QString group;
    QStringList tags;
    qint64 createdAt;
    qint64 lastUsed;
    int useCount;
    bool autoUnlock;
    int validDays;
    qint64 expiresAt;
    
    Credential() : port(22), authMethod("password"), createdAt(0), lastUsed(0), useCount(0), autoUnlock(false), validDays(0), expiresAt(0) {}
};

struct MasterPasswordSettings {
    bool enabled;
    QByteArray passwordHash;
    QByteArray salt;
    int iterations;
    int lockTimeout;
    qint64 lastUnlock;
    int failedAttempts;
    bool locked;
    
    MasterPasswordSettings() : enabled(false), iterations(100000), lockTimeout(30), lastUnlock(0), failedAttempts(0), locked(false) {}
};

class CredentialManager : public QObject {
    Q_OBJECT
public:
    explicit CredentialManager(QObject* parent = nullptr);
    static CredentialManager* instance();
    
    bool setMasterPassword(const QString& password);
    bool verifyMasterPassword(const QString& password);
    bool changeMasterPassword(const QString& oldPassword, const QString& newPassword);
    bool unlock(const QString& password);
    void lock();
    bool isLocked() const;
    bool isEnabled() const;
    
    QString addCredential(const Credential& cred);
    void deleteCredential(const QString& id);
    void updateCredential(const QString& id, const Credential& cred);
    Credential getCredential(const QString& id) const;
    QList<Credential> getAllCredentials() const;
    QList<Credential> searchCredentials(const QString& query) const;
    QList<Credential> getCredentialsByHost(const QString& host) const;
    QList<Credential> getCredentialsByGroup(const QString& group) const;
    
    QString getPassword(const QString& id);
    QString getPassphrase(const QString& id);
    void recordUsage(const QString& id);
    
    Credential findMatchingCredential(const QString& host, int port, const QString& username) const;
    QList<Credential> suggestCredentials(const QString& host) const;
    
    QList<Credential> getExpiringCredentials(int days = 7) const;
    QList<Credential> getExpiredCredentials() const;
    void cleanupExpiredCredentials();
    
    void exportCredentials(const QString& filePath, const QString& masterPassword);
    void importCredentials(const QString& filePath, const QString& masterPassword);
    
    void setLockTimeout(int minutes);
    int getLockTimeout() const;
    
    static QByteArray encryptData(const QByteArray& data, const QByteArray& key);
    static QByteArray decryptData(const QByteArray& data, const QByteArray& key);
    static QByteArray generateKey(const QString& password, const QByteArray& salt, int iterations);
    static QByteArray generateSalt();
    static QByteArray hashPassword(const QString& password, const QByteArray& salt, int iterations);

signals:
    void credentialAdded(const QString& id);
    void credentialDeleted(const QString& id);
    void credentialUpdated(const QString& id);
    void masterPasswordSet(bool enabled);
    void locked();
    void unlocked();
    void credentialsExpiring(const QString& id, int daysLeft);

private:
    static CredentialManager* s_instance;
    QMap<QString, Credential> m_credentials;
    MasterPasswordSettings m_masterPassword;
    bool m_unlocked;
    QByteArray m_encryptionKey;
    QString m_credentialsFile;
    
    void saveCredentials();
    void loadCredentials();
    void checkAutoLock();
    QString generateId();
};

#endif
