#ifndef CONNECTION_PROFILE_H
#define CONNECTION_PROFILE_H

#include <QString>
#include <QDateTime>
#include <QMetaType>

#include "Ssh/SshConfig.h"

struct ConnectionProfile {
    qint64 id;
    QString name;
    QString host;
    int port;
    QString username;
    QString password;
    SshAuthMethod authMethod;
    QString privateKeyPath;
    QString lastConnected;
    QDateTime createdAt;
    QDateTime updatedAt;
    
    ConnectionProfile() 
        : id(0), port(22), authMethod(SshAuthMethod::Password) {
        createdAt = QDateTime::currentDateTime();
        updatedAt = createdAt;
    }
    
    bool isValid() const {
        return !name.isEmpty() && !host.isEmpty() && !username.isEmpty();
    }
    
    QString authMethodLabel() const {
        return authMethod == SshAuthMethod::PublicKey ? QStringLiteral("公钥") : QStringLiteral("密码");
    }
    
    SshConfig toSshConfig() const {
        SshConfig config;
        config.host = host;
        config.port = port;
        config.username = username;
        config.password = password;
        config.authMethod = authMethod;
        config.privateKeyPath = privateKeyPath;
        return config;
    }
};

Q_DECLARE_METATYPE(ConnectionProfile)

#endif
