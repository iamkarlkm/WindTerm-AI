#ifndef TERMINAL_HISTORY_PLUGIN_H
#define TERMINAL_HISTORY_PLUGIN_H

#include "TerminalEventHook.h"
#include <QSqlDatabase>

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
private:
    void initDatabase();
    void saveCommand(const QString& command);
    QString queryHistoryByOffset(int offset);
    QSqlDatabase m_db;
    QString m_currentWorkingDir;
    int m_currentIndex = -1;
};

#endif
