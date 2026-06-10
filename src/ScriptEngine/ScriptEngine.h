#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include <QObject>
#include <QMap>
#include <QProcess>
#include <QThread>

enum class ScriptLanguage {
    Python,
    JavaScript,
    Shell,
    Bash,
    PowerShell
};

struct ScriptConfig {
    QString id;
    QString name;
    QString description;
    ScriptLanguage language;
    QString code;
    QString filePath;
    QStringList arguments;
    QMap<QString, QString> environment;
    QString workingDirectory;
    int timeout;  // seconds, 0 = no timeout
    bool captureOutput;
    bool showInTerminal;
    qint64 createdAt;
    qint64 modifiedAt;
    int runCount;
    int lastRunDuration;  // ms
    QString author;
    QString version;
    
    ScriptConfig() 
        : language(ScriptLanguage::Shell), timeout(300), captureOutput(true), 
          showInTerminal(false), createdAt(0), modifiedAt(0), runCount(0), lastRunDuration(0) {}
};

struct ScriptExecutionResult {
    QString scriptId;
    QString scriptName;
    int exitCode;
    QString output;
    QString error;
    int duration;  // ms
    qint64 timestamp;
    bool success;
    QString errorMessage;
    
    ScriptExecutionResult() : exitCode(-1), timestamp(0), success(false) {}
};

class ScriptExecutionThread : public QThread {
    Q_OBJECT
public:
    ScriptExecutionThread(const ScriptConfig& config, const QString& sessionId);
    
    void run() override;
    void stop();
    
signals:
    void started();
    void outputReceived(const QString& output);
    void errorReceived(const QString& error);
    void finished(const ScriptExecutionResult& result);
    void progress(int percent);

private:
    ScriptConfig m_config;
    QString m_sessionId;
    QProcess* m_process;
    bool m_stopped;
};

class ScriptEngineManager : public QObject {
    Q_OBJECT
public:
    explicit ScriptEngineManager(QObject* parent = nullptr);
    
    static ScriptEngineManager* instance();
    
    // 脚本管理
    QString createScript(const ScriptConfig& config);
    void deleteScript(const QString& id);
    void updateScript(const QString& id, const ScriptConfig& config);
    
    // 脚本查询
    ScriptConfig getScript(const QString& id) const;
    QList<ScriptConfig> getAllScripts() const;
    QList<ScriptConfig> getScriptsByLanguage(ScriptLanguage lang) const;
    QList<ScriptConfig> searchScripts(const QString& query) const;
    
    // 脚本执行
    QString executeScript(const QString& scriptId, const QStringList& args = QStringList(), const QString& sessionId = "");
    ScriptExecutionResult executeCode(ScriptLanguage lang, const QString& code, const QString& sessionId = "");
    void stopExecution(const QString& executionId);
    bool isRunning(const QString& executionId) const;
    
    // 执行状态
    ScriptExecutionResult getExecutionResult(const QString& executionId) const;
    QList<QString> getRunningExecutions() const;
    QList<ScriptExecutionResult> getExecutionHistory(const QString& scriptId, int limit = 20) const;
    
    // 脚本模板
    ScriptConfig createPythonTemplate(const QString& name);
    ScriptConfig createJavaScriptTemplate(const QString& name);
    ScriptConfig createShellTemplate(const QString& name);
    
    // 导入导出
    void exportScript(const QString& id, const QString& filePath);
    void importScript(const QString& filePath);
    void exportAllScripts(const QString& dirPath);
    void importAllScripts(const QString& dirPath);
    
    // 运行时检测
    static bool isPythonAvailable();
    static bool isNodeAvailable();
    static QString getPythonVersion();
    static QString getNodeVersion();
    static QStringList getAvailableLanguages();
    
    // 代码补全
    QStringList suggestKeywords(ScriptLanguage lang, const QString& prefix) const;
    QString getSyntaxTemplate(ScriptLanguage lang, const QString& keyword) const;
    
signals:
    void scriptAdded(const QString& id);
    void scriptDeleted(const QString& id);
    void scriptUpdated(const QString& id);
    void executionStarted(const QString& executionId, const QString& scriptId);
    void executionOutput(const QString& executionId, const QString& output);
    void executionError(const QString& executionId, const QString& error);
    void executionFinished(const QString& executionId, const ScriptExecutionResult& result);

private:
    static ScriptEngineManager* s_instance;
    
    QMap<QString, ScriptConfig> m_scripts;
    QMap<QString, ScriptExecutionThread*> m_runningExecutions;
    QMap<QString, QList<ScriptExecutionResult>> m_executionHistory;
    
    QString m_scriptsDir;
    QString m_historyFile;
    
    QString generateId();
    void saveScripts();
    void loadScripts();
    void saveHistory();
    void loadHistory();
    
    QString detectInterpreter(ScriptLanguage lang) const;
    QStringList buildCommand(const ScriptConfig& config) const;
};

#endif
