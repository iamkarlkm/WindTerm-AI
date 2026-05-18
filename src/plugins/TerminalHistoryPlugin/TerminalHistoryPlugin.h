#ifndef TERMINAL_HISTORY_PLUGIN_H
#define TERMINAL_HISTORY_PLUGIN_H

#include "TerminalEventHook.h"
#include "SessionManager.h"
#include <QSqlDatabase>
#include <QProcessEnvironment>

class TerminalHistoryPlugin : public TerminalEventHook {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.windterm.plugins.terminalhistory" FILE "terminal_history.json")
public:
    explicit TerminalHistoryPlugin(QObject* parent = nullptr);
    ~TerminalHistoryPlugin() override;
    bool initialize();
    void shutdown();
    bool interceptKeyEvent(int key, int modifiers, const QString& text) override;
    void onCommandExecuted(const QString& command) override;
    void onWorkingDirectoryChanged(const QString& path) override;
    QString getCommandHistory(int offset) override;
    void onSessionStart(const QString& sessionId, const QString& host, int port, const QString& protocol);
    void onSessionEnd(const QString& sessionId);
    void captureEnvironment(const QString& sessionId);
    
private:
    void initDatabase();
    void saveCommand(const QString& command);
    QString queryHistoryByOffset(int offset);
    QString getLastSuccessfulCommand(const QString& sessionId);
    
    QSqlDatabase m_db;
    SessionManager m_sessionManager;
    QString m_currentWorkingDir;
    QString m_currentSessionId;
    int m_currentIndex = -1;
    QProcessEnvironment m_lastEnvironment;
};

#endif
