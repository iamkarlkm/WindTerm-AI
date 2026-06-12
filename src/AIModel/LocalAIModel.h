#ifndef LOCALAIMODEL_H
#define LOCALAIMODEL_H

#include <QObject>
#include <QMap>
#include <QProcess>

/**
 * @brief AI 模型配置
 */
struct AIModelConfig {
    QString id;
    QString name;
    QString type;         // llm, embedding, multimodal
    QString provider;     // ollama, llama.cpp, vllm, etc.
    QString modelPath;
    QString modelName;
    int contextSize = 4096;
    int temperature = 7;  // x10 (7 = 0.7)
    int maxTokens = 2048;
    int gpuLayers = 0;    // 0 = CPU only
    bool streaming = true;
};

/**
 * @brief 对话消息
 */
struct ChatMessage {
    QString role;         // system, user, assistant
    QString content;
    QDateTime timestamp;
};

/**
 * @brief 本地 AI 模型管理器 - 本地 LLM 支持
 * 
 * 功能:
 * - 多模型后端支持 (Ollama/llama.cpp/vLLM)
 * - 模型加载/卸载
 * - 流式对话
 * - 上下文管理
 * - GPU 加速支持
 * - 模型量化
 */
class LocalAIModel : public QObject {
    Q_OBJECT

public:
    explicit LocalAIModel(QObject* parent = nullptr);
    ~LocalAIModel();

    // 模型管理
    bool loadModel(const AIModelConfig& config);
    bool unloadModel(const QString& modelId);
    bool isModelLoaded(const QString& modelId) const;
    QList<QString> getLoadedModels() const;
    QList<AIModelConfig> getAvailableModels() const;
    
    // 对话
    QString chat(const QString& modelId, const QString& prompt, const QList<ChatMessage>& history = QList<ChatMessage>());
    void chatAsync(const QString& modelId, const QString& prompt, const QList<ChatMessage>& history = QList<ChatMessage>());
    void stopGeneration();
    
    // 流式输出
    void enableStreaming(bool enable);
    bool isStreaming() const;
    
    // 模型信息
    QJsonObject getModelInfo(const QString& modelId) const;
    qint64 getModelMemoryUsage(const QString& modelId) const;
    
    // 后端管理
    bool startBackend(const QString& backend, int port = 8080);
    bool stopBackend(const QString& backend);
    bool isBackendRunning(const QString& backend) const;
    
    // 性能优化
    void setGpuLayers(int layers);
    void setContextSize(int size);
    void setTemperature(float temp);
    
    // 模型下载
    bool downloadModel(const QString& modelId, const QString& source);
    qint64 getDownloadProgress(const QString& modelId) const;

signals:
    void modelLoaded(const QString& modelId);
    void modelUnloaded(const QString& modelId);
    void responseStarted(const QString& modelId);
    void responseToken(const QString& modelId, const QString& token);
    void responseCompleted(const QString& modelId, const QString& fullResponse);
    void downloadProgress(const QString& modelId, qint64 received, qint64 total);
    void errorOccurred(const QString& modelId, const QString& error);

private slots:
    void onBackendOutput();
    void onBackendFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QMap<QString, AIModelConfig> m_models;
    QMap<QString, QProcess*> m_backendProcesses;
    QMap<QString, QList<ChatMessage>> m_chatHistory;
    QMap<QString, qint64> m_downloadProgress;
    
    QString m_defaultBackend;
    int m_defaultPort;
    bool m_streamingEnabled;
    
    QString executeBackendCommand(const QString& backend, const QStringList& args) const;
    QString parseLlamaCppOutput(const QByteArray& output) const;
    QString parseOllamaOutput(const QByteArray& output) const;
    
    static LocalAIModel* s_instance;

public:
    static LocalAIModel* instance();
};

#endif // LOCALAIMODEL_H
