#ifndef TERMINAL_INFO_PLUGIN_H
#define TERMINAL_INFO_PLUGIN_H

#include "../PluginInterface.h"
#include "../PluginContext.h"

class TerminalInfoPlugin : public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "terminal_info.json")
    
public:
    explicit TerminalInfoPlugin(QObject* parent = nullptr);
    ~TerminalInfoPlugin() override;
    
    PluginMetadata metadata() const override;
    bool initialize(PluginContext* context) override;
    void shutdown() override;
    
    void onSessionStarted(const QString& sessionId) override;
    void onCommandExecuted(const QString& command) override;
    void onWorkingDirectoryChanged(const QString& path) override;
    
private:
    PluginContext* m_context = nullptr;
    QString m_currentSession;
    int m_commandCount = 0;
    QTime m_sessionStartTime;
};

#endif
