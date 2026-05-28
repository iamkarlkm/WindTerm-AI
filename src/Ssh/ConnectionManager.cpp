#include "ConnectionManager.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

ConnectionManager* ConnectionManager::s_instance = nullptr;

ConnectionManager::ConnectionManager(QObject* parent)
    : QObject(parent), m_initialized(false) {}

ConnectionManager::~ConnectionManager() {
    if (m_database.isOpen()) {
        m_database.close();
    }
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

ConnectionManager* ConnectionManager::instance(QObject* parent) {
    if (!s_instance) {
        s_instance = new ConnectionManager(parent);
    }
    return s_instance;
}

bool ConnectionManager::initialize(const QString& dbPath) {
    if (m_initialized) return true;
    
    QString path = dbPath.isEmpty() ? defaultDbPath() : dbPath;
    
    QDir dbDir = QFileInfo(path).absoluteDir();
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(".")) {
            emit databaseError(QStringLiteral("无法创建数据库目录: ") + dbDir.absolutePath());
            return false;
        }
    }
    
    m_database = QSqlDatabase::addDatabase("QSQLITE", "SshProfiles");
    m_database.setDatabaseName(path);
    
    if (!m_database.open()) {
        emit databaseError(m_database.lastError().text());
        return false;
    }
    
    if (!createTables()) {
        return false;
    }
    
    m_initialized = true;
    qDebug() << "[ConnectionManager] Initialized:" << path;
    return true;
}

bool ConnectionManager::createTables() {
    QSqlQuery query(m_database);
    
    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS ssh_profiles (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            host TEXT NOT NULL,
            port INTEGER DEFAULT 22,
            username TEXT NOT NULL,
            auth_method TEXT DEFAULT 'password',
            private_key_path TEXT DEFAULT '',
            last_connected TEXT,
            created_at TEXT DEFAULT (datetime('now')),
            updated_at TEXT DEFAULT (datetime('now'))
        )
    )";
    
    if (!query.exec(createTable)) {
        emit databaseError(query.lastError().text());
        return false;
    }
    
    return true;
}

QString ConnectionManager::defaultDbPath() const {
#ifdef Q_OS_WIN
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#endif
    return dataDir + "/ssh_profiles.db";
}

qint64 ConnectionManager::saveProfile(const ConnectionProfile& profile) {
    if (!m_initialized) return -1;
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO ssh_profiles (
            name, host, port, username, auth_method, private_key_path
        ) VALUES (
            :name, :host, :port, :username, :auth_method, :private_key_path
        )
    )");
    
    query.bindValue(":name", profile.name);
    query.bindValue(":host", profile.host);
    query.bindValue(":port", profile.port);
    query.bindValue(":username", profile.username);
    query.bindValue(":auth_method", profile.authMethod == SshAuthMethod::PublicKey ? "publickey" : "password");
    query.bindValue(":private_key_path", profile.privateKeyPath);
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return -1;
    }
    
    qint64 id = query.lastInsertId().toLongLong();
    emit profileSaved(id);
    return id;
}

bool ConnectionManager::updateProfile(const ConnectionProfile& profile) {
    if (!m_initialized || profile.id <= 0) return false;
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        UPDATE ssh_profiles SET
            name = :name,
            host = :host,
            port = :port,
            username = :username,
            auth_method = :auth_method,
            private_key_path = :private_key_path,
            updated_at = datetime('now')
        WHERE id = :id
    )");
    
    query.bindValue(":id", profile.id);
    query.bindValue(":name", profile.name);
    query.bindValue(":host", profile.host);
    query.bindValue(":port", profile.port);
    query.bindValue(":username", profile.username);
    query.bindValue(":auth_method", profile.authMethod == SshAuthMethod::PublicKey ? "publickey" : "password");
    query.bindValue(":private_key_path", profile.privateKeyPath);
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    
    return true;
}

bool ConnectionManager::deleteProfile(qint64 id) {
    if (!m_initialized || id <= 0) return false;
    
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM ssh_profiles WHERE id = :id");
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    
    emit profileDeleted(id);
    return true;
}

QList<ConnectionProfile> ConnectionManager::loadProfiles() {
    QList<ConnectionProfile> profiles;
    
    if (!m_initialized) return profiles;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM ssh_profiles ORDER BY name ASC");
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return profiles;
    }
    
    while (query.next()) {
        ConnectionProfile profile;
        profile.id = query.value("id").toLongLong();
        profile.name = query.value("name").toString();
        profile.host = query.value("host").toString();
        profile.port = query.value("port").toInt();
        profile.username = query.value("username").toString();
        profile.authMethod = query.value("auth_method").toString() == "publickey" 
            ? SshAuthMethod::PublicKey : SshAuthMethod::Password;
        profile.privateKeyPath = query.value("private_key_path").toString();
        profile.lastConnected = query.value("last_connected").toString();
        
        profiles.append(profile);
    }
    
    return profiles;
}

ConnectionProfile ConnectionManager::getProfile(qint64 id) {
    ConnectionProfile profile;
    
    if (!m_initialized || id <= 0) return profile;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM ssh_profiles WHERE id = :id");
    query.bindValue(":id", id);
    
    if (query.exec() && query.next()) {
        profile.id = query.value("id").toLongLong();
        profile.name = query.value("name").toString();
        profile.host = query.value("host").toString();
        profile.port = query.value("port").toInt();
        profile.username = query.value("username").toString();
        profile.authMethod = query.value("auth_method").toString() == "publickey" 
            ? SshAuthMethod::PublicKey : SshAuthMethod::Password;
        profile.privateKeyPath = query.value("private_key_path").toString();
        profile.lastConnected = query.value("last_connected").toString();
    }
    
    return profile;
}

SshChannelSession* ConnectionManager::createSession() {
    return new SshChannelSession(this);
}
