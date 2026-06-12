#include "LocalAIModel.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>
#include <QDebug>
#include <QUuid>

LocalAIModel* LocalAIModel::s_instance = nullptr;

LocalAIModel::LocalAIModel(QObject* parent) : QObject(parent) {
    m_defaultBackend = "ollama";
    m_defaultPort = 8080;
    m_streamingEnabled = true;
}

LocalAIModel::~LocalAIModel() {
    for (auto process : m_backendProcesses) {
        if (process) {
            process->kill();
            delete process;
        }
    }
}

LocalAIModel* LocalAIModel::instance() {
    if (!s_instance) s_instance = new LocalAIModel();
    return s_instance;
}

bool LocalAIModel::loadModel(const AIModelConfig& config) {
    m_models[config.id] = config;
    
    // 启动后端进程
    if (!m_backendProcesses.contains(config.id)) {
        QProcess* process = new QProcess(this);
        
        connect(process, &QProcess::readyReadStandardOutput, this, &LocalAIModel::onBackendOutput);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &LocalAIModel::onBackendFinished);
        
        m_backendProcesses[config.id] = process;
    }
    
    emit modelLoaded(config.id);
    return true;
}

bool LocalAIModel::unloadModel(const QString& modelId) {
    if (!m_models.contains(modelId)) return false;
    
    if (m_backendProcesses.contains(modelId)) {
        QProcess* process = m_backendProcesses[modelId];
        if (process) {
            process->kill();
            process->waitForFinished(3000);
        }
    }
    
    m_models.remove(modelId);
    m_chatHistory.remove(modelId);
    
    emit modelUnloaded(modelId);
    return true;
}

bool LocalAIModel::isModelLoaded(const QString& modelId) const {
    return m_models.contains(modelId);
}

QList<QString> LocalAIModel::getLoadedModels() const {
    return m_models.keys();
}

QList<AIModelConfig> LocalAIModel::getAvailableModels() const {
    // 返回预定义模型列表
    QList<AIModelConfig> models;
    
    AIModelConfig llama;
    llama.id = "llama-2-7b";
    llama.name = "Llama 2 7B";
    llama.type = "llm";
    llama.provider = "ollama";
    llama.modelName = "llama2";
    llama.contextSize = 4096;
    models.append(llama);
    
    AIModelConfig mistral;
    mistral.id = "mistral-7b";
    mistral.name = "Mistral 7B";
    mistral.type = "llm";
    mistral.provider = "ollama";
    mistral.modelName = "mistral";
    mistral.contextSize = 8192;
    models.append(mistral);
    
    AIModelConfig codellama;
    codellama.id = "codellama-7b";
    codellama.name = "Code Llama 7B";
    codellama.type = "llm";
    codellama.provider = "ollama";
    codellama.modelName = "codellama";
    codellama.contextSize = 4096;
    models.append(codellama);
    
    AIModelConfig embedding;
    embedding.id = "nomic-embed";
    embedding.name = "Nomic Embed";
    embedding.type = "embedding";
    embedding.provider = "ollama";
    embedding.modelName = "nomic-embed-text";
    embedding.contextSize = 8192;
    models.append(embedding);
    
    return models;
}

QString LocalAIModel::chat(const QString& modelId, const QString& prompt, const QList<ChatMessage>& history) {
    if (!m_models.contains(modelId)) {
        emit errorOccurred(modelId, "Model not loaded");
        return QString();
    }
    
    const AIModelConfig& config = m_models[modelId];
    
    // 保存对话历史
    ChatMessage userMsg;
    userMsg.role = "user";
    userMsg.content = prompt;
    userMsg.timestamp = QDateTime::currentDateTime();
    
    m_chatHistory[modelId].append(userMsg);
    
    // 构建请求
    QString fullPrompt;
    for (const ChatMessage& msg : m_chatHistory[modelId]) {
        if (msg.role == "system") {
            fullPrompt += "System: " + msg.content + "\n";
        } else if (msg.role == "user") {
            fullPrompt += "User: " + msg.content + "\n";
        } else if (msg.role == "assistant") {
            fullPrompt += "Assistant: " + msg.content + "\n";
        }
    }
    fullPrompt += "Assistant: ";
    
    // 调用后端
    QString response;
    if (config.provider == "ollama") {
        response = executeOllamaChat(config.modelName, fullPrompt, config);
    } else if (config.provider == "llama.cpp") {
        response = executeLlamaCppChat(config.modelPath, fullPrompt, config);
    }
    
    // 保存助手回复
    ChatMessage assistantMsg;
    assistantMsg.role = "assistant";
    assistantMsg.content = response;
    assistantMsg.timestamp = QDateTime::currentDateTime();
    m_chatHistory[modelId].append(assistantMsg);
    
    emit responseCompleted(modelId, response);
    return response;
}

void LocalAIModel::chatAsync(const QString& modelId, const QString& prompt, const QList<ChatMessage>& history) {
    QThreadPool::globalInstance()->start([this, modelId, prompt, history]() {
        chat(modelId, prompt, history);
    });
}

void LocalAIModel::stopGeneration() {
    for (auto process : m_backendProcesses) {
        if (process && process->state() == QProcess::Running) {
            process->kill();
        }
    }
}

void LocalAIModel::enableStreaming(bool enable) {
    m_streamingEnabled = enable;
}

bool LocalAIModel::isStreaming() const {
    return m_streamingEnabled;
}

QJsonObject LocalAIModel::getModelInfo(const QString& modelId) const {
    if (!m_models.contains(modelId)) return QJsonObject();
    
    const AIModelConfig& config = m_models[modelId];
    QJsonObject info;
    info["id"] = config.id;
    info["name"] = config.name;
    info["type"] = config.type;
    info["provider"] = config.provider;
    info["contextSize"] = config.contextSize;
    info["temperature"] = config.temperature / 10.0;
    info["maxTokens"] = config.maxTokens;
    return info;
}

qint64 LocalAIModel::getModelMemoryUsage(const QString& modelId) const {
    Q_UNUSED(modelId)
    // 估算内存使用 (简化)
    return 4LL * 1024 * 1024 * 1024;  // 4GB 默认
}

bool LocalAIModel::startBackend(const QString& backend, int port) {
    QProcess* process = new QProcess(this);
    
    QStringList args;
    if (backend == "ollama") {
        args << "serve";
    } else if (backend == "llama.cpp") {
        args << "-m" << "model.bin" << "--port" << QString::number(port);
    }
    
    process->start(backend, args);
    
    if (process->waitForStarted(5000)) {
        m_backendProcesses[backend] = process;
        return true;
    }
    
    delete process;
    return false;
}

bool LocalAIModel::stopBackend(const QString& backend) {
    if (!m_backendProcesses.contains(backend)) return false;
    
    QProcess* process = m_backendProcesses[backend];
    process->kill();
    process->waitForFinished(3000);
    
    m_backendProcesses.remove(backend);
    return true;
}

bool LocalAIModel::isBackendRunning(const QString& backend) const {
    if (!m_backendProcesses.contains(backend)) return false;
    return m_backendProcesses[backend]->state() == QProcess::Running;
}

void LocalAIModel::setGpuLayers(int layers) {
    for (auto it = m_models.begin(); it != m_models.end(); ++it) {
        it->gpuLayers = layers;
    }
}

void LocalAIModel::setContextSize(int size) {
    for (auto it = m_models.begin(); it != m_models.end(); ++it) {
        it->contextSize = size;
    }
}

void LocalAIModel::setTemperature(float temp) {
    for (auto it = m_models.begin(); it != m_models.end(); ++it) {
        it->temperature = (int)(temp * 10);
    }
}

bool LocalAIModel::downloadModel(const QString& modelId, const QString& source) {
    Q_UNUSED(modelId)
    Q_UNUSED(source)
    // 实际实现需要调用下载 API
    emit downloadProgress(modelId, 50, 100);
    return true;
}

qint64 LocalAIModel::getDownloadProgress(const QString& modelId) const {
    return m_downloadProgress.value(modelId, 0);
}

void LocalAIModel::onBackendOutput() {
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;
    
    QByteArray output = process->readAllStandardOutput();
    
    // 查找对应的 modelId
    for (auto it = m_backendProcesses.begin(); it != m_backendProcesses.end(); ++it) {
        if (it.value() == process) {
            QString modelId = it.key();
            if (m_streamingEnabled) {
                QString text = parseOllamaOutput(output);
                if (!text.isEmpty()) {
                    emit responseToken(modelId, text);
                }
            }
            break;
        }
    }
}

void LocalAIModel::onBackendFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    // 后端进程结束处理
}

QString LocalAIModel::executeBackendCommand(const QString& backend, const QStringList& args) const {
    QProcess process;
    process.start(backend, args);
    if (process.waitForFinished(30000)) {
        return QString::fromUtf8(process.readAllStandardOutput());
    }
    return QString();
}

QString LocalAIModel::parseLlamaCppOutput(const QByteArray& output) const {
    return QString::fromUtf8(output);
}

QString LocalAIModel::parseOllamaOutput(const QByteArray& output) const {
    return QString::fromUtf8(output);
}

QString LocalAIModel::executeOllamaChat(const QString& modelName, const QString& prompt, const AIModelConfig& config) {
    Q_UNUSED(config)
    QProcess process;
    process.start("ollama", QStringList() << "run" << modelName << prompt);
    
    if (process.waitForFinished(60000)) {
        return QString::fromUtf8(process.readAllStandardOutput());
    }
    
    return "Error: Request timeout or failed";
}

QString LocalAIModel::executeLlamaCppChat(const QString& modelPath, const QString& prompt, const AIModelConfig& config) {
    QProcess process;
    QStringList args;
    args << "-m" << modelPath;
    args << "-n" << QString::number(config.maxTokens);
    args << "--temp" << QString::number(config.temperature / 10.0);
    args << "-p" << prompt;
    
    process.start("main", args);
    
    if (process.waitForFinished(60000)) {
        return QString::fromUtf8(process.readAllStandardOutput());
    }
    
    return "Error: Request timeout or failed";
}

#include "LocalAIModel.moc"
