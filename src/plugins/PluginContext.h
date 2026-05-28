#ifndef PLUGIN_CONTEXT_H
#define PLUGIN_CONTEXT_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <functional>

class PluginContext : public QObject {
    Q_OBJECT
public:
    explicit PluginContext(QObject* parent = nullptr) : QObject(parent) {}
    
    void setTerminalWriter(std::function<void(const QString&)> writer) {
        m_terminalWriter = std::move(writer);
    }
    void setDirectoryOpener(std::function<void(const QString&)> opener) {
        m_directoryOpener = std::move(opener);
    }
    void setSettingsReader(std::function<QVariant(const QString&)> reader) {
        m_settingsReader = std::move(reader);
    }
    void setSettingsWriter(std::function<void(const QString&, const QVariant&)> writer) {
        m_settingsWriter = std::move(writer);
    }
    void setNotificationSender(std::function<void(const QString&, const QString&)> sender) {
        m_notificationSender = std::move(sender);
    }
    
    void writeTerminal(const QString& text) {
        if (m_terminalWriter) m_terminalWriter(text);
    }
    void openDirectory(const QString& path) {
        if (m_directoryOpener) m_directoryOpener(path);
    }
    QVariant readSetting(const QString& key) {
        if (m_settingsReader) return m_settingsReader(key);
        return QVariant();
    }
    void writeSetting(const QString& key, const QVariant& value) {
        if (m_settingsWriter) m_settingsWriter(key, value);
    }
    void sendNotification(const QString& title, const QString& message) {
        if (m_notificationSender) m_notificationSender(title, message);
    }
    QString applicationVersion() const { return m_appVersion; }
    void setApplicationVersion(const QString& version) { m_appVersion = version; }

private:
    std::function<void(const QString&)> m_terminalWriter;
    std::function<void(const QString&)> m_directoryOpener;
    std::function<QVariant(const QString&)> m_settingsReader;
    std::function<void(const QString&, const QVariant&)> m_settingsWriter;
    std::function<void(const QString&, const QString&)> m_notificationSender;
    QString m_appVersion;
};

#endif
