#ifndef AI_CONFIG_H
#define AI_CONFIG_H

#include <QString>
#include <QSettings>

enum class AiProvider {
    OpenAI,
    Claude,
    Ollama,
    Custom
};

struct AiConfig {
    AiProvider provider = AiProvider::OpenAI;
    QString apiKey;
    QString model = "gpt-3.5-turbo";
    QString apiUrl = "https://api.openai.com/v1/chat/completions";
    double temperature = 0.7;
    int maxTokens = 1024;
    bool enabled = false;
    bool streamResponse = true;
    
    static AiConfig load(QSettings* settings) {
        AiConfig config;
        config.provider = static_cast<AiProvider>(settings->value("ai/provider", 0).toInt());
        config.apiKey = settings->value("ai/apiKey", "").toString();
        config.model = settings->value("ai/model", "gpt-3.5-turbo").toString();
        config.apiUrl = settings->value("ai/apiUrl", "https://api.openai.com/v1/chat/completions").toString();
        config.temperature = settings->value("ai/temperature", 0.7).toDouble();
        config.maxTokens = settings->value("ai/maxTokens", 1024).toInt();
        config.enabled = settings->value("ai/enabled", false).toBool();
        config.streamResponse = settings->value("ai/streamResponse", true).toBool();
        return config;
    }
    
    void save(QSettings* settings) const {
        settings->setValue("ai/provider", static_cast<int>(provider));
        settings->setValue("ai/apiKey", apiKey);
        settings->setValue("ai/model", model);
        settings->setValue("ai/apiUrl", apiUrl);
        settings->setValue("ai/temperature", temperature);
        settings->setValue("ai/maxTokens", maxTokens);
        settings->setValue("ai/enabled", enabled);
        settings->setValue("ai/streamResponse", streamResponse);
    }
};

#endif
