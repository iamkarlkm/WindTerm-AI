#ifndef AI_ASSIST_MANAGER_H
#define AI_ASSIST_MANAGER_H

#include <QObject>
#include <QMap>

struct AIDiagnostic {
    int position;
    int length;
    QString message;
    QString severity;  // error, warning, info, suggestion
    QString suggestion;
    QString relatedDoc;
};

struct AIRecommendation {
    QString type;  // command, option, workflow, optimization
    QString title;
    QString description;
    QString example;
    int confidence;  // 0-100
    QStringList tags;
};

struct CommandExplanation {
    QString command;
    QString summary;
    QString detailedExplanation;
    QStringList breakdown;
    QStringList examples;
    QStringList warnings;
    QStringList relatedCommands;
};

struct NaturalLanguageParse {
    QString intent;
    QString action;
    QStringList targets;
    QMap<QString, QString> parameters;
    QString generatedCommand;
    int confidence;
    QStringList alternatives;
};

class AIAssistManager : public QObject {
    Q_OBJECT
public:
    explicit AIAssistManager(QObject* parent = nullptr);
    
    static AIAssistManager* instance();
    
    // 错误诊断
    QList<AIDiagnostic> diagnoseCommand(const QString& command, const QString& output = "") const;
    QList<AIDiagnostic> diagnoseError(const QString& errorMessage) const;
    QString suggestFix(const QString& error, const QString& command = "") const;
    
    // 智能推荐
    QList<AIRecommendation> getRecommendations(const QString& context, int limit = 5) const;
    QStringList suggestNextCommands(const QString& commandHistory, int limit = 3) const;
    QString suggestOptimization(const QString& command) const;
    
    // 命令解释
    CommandExplanation explainCommand(const QString& command) const;
    QString explainError(const QString& error) const;
    QString explainOutput(const QString& command, const QString& output) const;
    
    // 自然语言处理
    NaturalLanguageParse parseNaturalLanguage(const QString& text) const;
    QString generateCommand(const QString& naturalLanguage) const;
    QStringList generateAlternatives(const QString& naturalLanguage) const;
    
    // 上下文学习
    void recordContext(const QString& host, const QString& command, const QString& result);
    void recordPattern(const QString& pattern, const QString& command);
    QMap<QString, int> getContextPatterns(const QString& host) const;
    
    // 知识库
    void loadKnowledgeBase();
    void addKnowledge(const QString& category, const QString& key, const QVariant& value);
    QVariant getKnowledge(const QString& category, const QString& key) const;
    
    // 代码生成
    QString generateScript(const QString& description, const QString& language = "bash") const;
    QString generateDockerfile(const QString& appType, const QString& language = "python") const;
    QString generateK8sManifest(const QString& appName, int replicas = 3) const;
    
    // 安全建议
    struct SecurityWarning {
        QString issue;
        QString severity;  // critical, high, medium, low
        QString recommendation;
        QString cve;
    };
    QList<SecurityWarning> checkCommandSecurity(const QString& command) const;
    
    // 性能分析
    struct PerformanceAnalysis {
        QString bottleneck;
        QString suggestion;
        int estimatedImprovement;  // percent
    };
    PerformanceAnalysis analyzeCommandPerformance(const QString& command) const;
    
signals:
    void diagnosticCompleted(const QList<AIDiagnostic>& diagnostics);
    void recommendationUpdated(const QList<AIRecommendation>& recommendations);
    void commandGenerated(const QString& command);
    void contextLearned(const QString& pattern);

private:
    static AIAssistManager* s_instance;
    
    QMap<QString, QVariant> m_knowledgeBase;
    QMap<QString, QMap<QString, int>> m_contextPatterns;
    QMap<QString, QList<QString>> m_errorPatterns;
    QMap<QString, QString> m_nlpPatterns;
    
    void loadErrorPatterns();
    void loadNLPPatterns();
    void loadSecurityRules();
    
    QString matchPattern(const QString& text, const QMap<QString, QString>& patterns) const;
    int calculateConfidence(const QString& command, const QString& intent) const;
};

#endif
