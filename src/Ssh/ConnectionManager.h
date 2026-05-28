#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>

#include "Ssh/SshChannelSession.h"
#include "Ssh/ConnectionProfile.h"

class ConnectionManager : public QObject {
    Q_OBJECT
public:
    explicit ConnectionManager(QObject* parent = nullptr);
    ~ConnectionManager() override;
    
    static ConnectionManager* instance(QObject* parent = nullptr);
    
    bool initialize(const QString& dbPath = QString());
    bool isInitialized() const { return m_initialized; }
    
    qint64 saveProfile(const ConnectionProfile& profile);
    bool updateProfile(const ConnectionProfile& profile);
    bool deleteProfile(qint64 id);
    
    QList<ConnectionProfile> loadProfiles();
    ConnectionProfile getProfile(qint64 id);
    
    SshChannelSession* createSession();
    
signals:
    void profileSaved(qint64 id);
    void profileDeleted(qint64 id);
    void databaseError(const QString& error);
    
private:
    bool createTables();
    QString defaultDbPath() const;
    
    QSqlDatabase m_database;
    bool m_initialized;
    
    static ConnectionManager* s_instance;
};

#endif
