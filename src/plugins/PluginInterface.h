#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QJsonObject>

enum class PluginType {
    TerminalHook,
    ThemeProvider,
    CommandExtension,
    UISupplement,
    ProtocolHandler
};

enum class PluginState {
    Unloaded,
    Loading,
    Loaded,
    Initialized,
    Running,
    Stopped,
    Error
};

struct PluginMetadata {
    QString id;
    QString name;
    QString version;
    QString description;
    QString author;
    PluginType type = PluginType::TerminalHook;
    QStringList dependencies;
    QJsonObject extraData;
};

class PluginContext;

class PluginInterface : public QObject {
    Q_OBJECT
public:
    explicit PluginInterface(QObject* parent = nullptr) : QObject(parent) {}
    ~PluginInterface() override = default;

    virtual PluginMetadata metadata() const = 0;
    virtual bool initialize(PluginContext* context) { Q_UNUSED(context); return true; }
    virtual void shutdown() {}
    virtual PluginState state() const { return m_state; }
    
    virtual bool interceptKeyEvent(int key, int modifiers, const QString& text) {
        Q_UNUSED(key); Q_UNUSED(modifiers); Q_UNUSED(text);
        return false;
    }
    virtual void onTerminalOutput(const QString& text) { Q_UNUSED(text); }
    virtual void onCommandExecuted(const QString& command) { Q_UNUSED(command); }
    virtual void onWorkingDirectoryChanged(const QString& path) { Q_UNUSED(path); }
    virtual void onSessionStarted(const QString& sessionId) { Q_UNUSED(sessionId); }
    virtual void onSessionEnded(const QString& sessionId) { Q_UNUSED(sessionId); }
    virtual void onTabCreated(int tabIndex) { Q_UNUSED(tabIndex); }
    virtual void onTabClosed(int tabIndex) { Q_UNUSED(tabIndex); }

signals:
    void stateChanged(PluginState newState);
    void sendTextToTerminal(const QString& text);
    void requestNotification(const QString& title, const QString& message);
    void requestMenuAction(const QString& actionId);

protected:
    void setState(PluginState state) {
        m_state = state;
        emit stateChanged(state);
    }
    PluginState m_state = PluginState::Unloaded;
};

#define PluginInterface_iid "com.windterm.extensions.PluginInterface/1.0"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif
