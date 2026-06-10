#ifndef COMMAND_COMPLETION_H
#define COMMAND_COMPLETION_H

#include <QObject>
#include <QMap>
#include <QSet>

struct CommandInfo {
    QString name;
    QString description;
    QString category;
    QStringList aliases;
    QStringList options;
    QMap<QString, QString> optionDescriptions;
    QString syntax;
    QStringList examples;
    int usageCount;
    QString lastUsed;
    QString manPage;
};

struct CompletionResult {
    QString text;
    QString type;  // command, option, argument, file, directory
    QString description;
    QString icon;
    int relevance;
    
    CompletionResult() : relevance(0) {}
};

class CommandCompletionEngine : public QObject {
    Q_OBJECT
public:
    explicit CommandCompletionEngine(QObject* parent = nullptr);
    
    static CommandCompletionEngine* instance();
    
    // 初始化
    void initialize();
    void loadCommandDatabase();
    void scanSystemCommands();
    
    // 补全查询
    QList<CompletionResult> getCompletions(const QString& input, int cursorPosition) const;
    QList<CompletionResult> getCommandCompletions(const QString& prefix, int limit = 20) const;
    QList<CompletionResult> getOptionCompletions(const QString& command, const QString& prefix) const;
    QList<CompletionResult> getArgumentCompletions(const QString& command, int argIndex, const QString& prefix) const;
    
    // 文件路径补全
    QList<CompletionResult> completeFilePath(const QString& path, bool directoriesOnly = false) const;
    
    // 智能推荐
    QString suggestNextCommand(const QString& currentCommand, const QString& host = "") const;
    QStringList suggestRelatedCommands(const QString& command) const;
    QString explainCommand(const QString& command) const;
    QString generateCommand(const QString& naturalLanguage) const;
    
    // 命令信息
    CommandInfo getCommandInfo(const QString& command) const;
    bool isCommandExists(const QString& command) const;
    QString getCommandHelp(const QString& command) const;
    QString getCommandSyntax(const QString& command) const;
    
    // 学习功能
    void recordUsage(const QString& command);
    void setHostContext(const QString& host);
    QMap<QString, int> getHostSpecificCommands(const QString& host, int limit = 20) const;
    
    // 语法高亮
    QMap<int, QPair<int, QString>> getSyntaxHighlights(const QString& command) const;
    
    // 参数提示
    struct ArgumentHint {
        int position;
        QString name;
        QString type;
        bool required;
        QString description;
    };
    QList<ArgumentHint> getArgumentHints(const QString& command) const;
    
    // 错误诊断
    struct Diagnostic {
        int position;
        int length;
        QString message;
        QString severity;  // error, warning, info
        QString suggestion;
    };
    QList<Diagnostic> diagnoseCommand(const QString& command) const;
    
signals:
    void databaseLoaded();
    void usageRecorded(const QString& command);

private:
    static CommandCompletionEngine* s_instance;
    
    mutable QMap<QString, CommandInfo> m_commands;
    mutable QMap<QString, int> m_commandFrequency;
    mutable QMap<QString, QMap<QString, int>> m_hostCommandFrequency;
    QSet<QString> m_executables;
    QString m_currentHost;
    
    void loadBuiltInCommands();
    void parseManPage(const QString& command);
    QStringList executeAndCapture(const QString& command) const;
    
    int calculateRelevance(const QString& completion, const QString& input) const;
};

#endif
