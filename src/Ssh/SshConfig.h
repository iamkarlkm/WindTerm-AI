#ifndef SSH_CONFIG_H
#define SSH_CONFIG_H

#include <QString>

enum class SshAuthMethod {
    Password,
    PublicKey
};

struct SshConfig {
    QString host;
    int port = 22;
    QString username;
    QString password;
    QString privateKeyPath;
    QString passphrase;
    SshAuthMethod authMethod = SshAuthMethod::Password;
    
    bool isValid() const {
        return !host.isEmpty() && port > 0 && port < 65536 && !username.isEmpty();
    }
    
    QString authMethodLabel() const {
        return authMethod == SshAuthMethod::PublicKey ? QStringLiteral("公钥") : QStringLiteral("密码");
    }
};

#endif
