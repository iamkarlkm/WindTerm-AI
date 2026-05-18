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
    
    // 发送文本到终端输入行
    void sendTextToInput(const QString& text) override;
    void clearInput() override;
    QString getCurrentInput() override { return m_currentInput; }
    
    // 恢复上次会话状态
    Q_INVOKABLE void restoreLastSession();
    Q_INVOKABLE void restoreSessionById(const QString& sessionId);
    Q_INVOKABLE void fillLastCommand();
    
private:
    void initDatabase();
    void saveCommand(const QString& command);
    QString queryHistoryByOffset(int offset);
    QString getLastSuccessfulCommand(const QString& sessionId);
    void injectTextToTerminal(const QString& text);
    
    QSqlDatabase m_db;
    SessionManager m_sessionManager;
    QString m_currentWorkingDir;
    QString m_currentSessionId;
    QString m_currentInput;
    int m_currentIndex = -1;
    QProcessEnvironment m_lastEnvironment;
};

#endif
