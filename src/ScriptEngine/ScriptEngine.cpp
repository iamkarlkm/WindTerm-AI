#include "ScriptEngine.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QThread>
#include <algorithm>

ScriptEngineManager* ScriptEngineManager::s_instance = nullptr;

// ScriptExecutionThread implementation
ScriptExecutionThread::ScriptExecutionThread(const ScriptConfig& config, const QString& sessionId)
    : m_config(config)
    , m_sessionId(sessionId)
    , m_process(nullptr)
    , m_stopped(false) {
}

void ScriptExecutionThread::run() {
    emit started();
    
    m_process = new QProcess();
    ScriptExecutionResult result;
    result.scriptId = m_config.id;
    result.scriptName = m_config.name;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    QElapsedTimer timer;
    timer.start();
    
    // 设置工作目录
    if (!m_config.workingDirectory.isEmpty()) {
        m_process->setWorkingDirectory(m_config.workingDirectory);
    }
    
    // 设置环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (auto it = m_config.environment.begin(); it != m_config.environment.end(); ++it) {
        env.insert(it.key(), it.value());
    }
    m_process->setProcessEnvironment(env);
    
    // 构建命令
    QString interpreter;
    QStringList args = m_config.arguments;
    
    switch (m_config.language) {
        case ScriptLanguage::Python:
            interpreter = "python3";
            #ifdef Q_OS_WIN
            if (QStandardPaths::findExecutable("python").isEmpty()) {
                interpreter = "python";
            }
            #endif
            break;
        case ScriptLanguage::JavaScript:
            interpreter = "node";
            break;
        case ScriptLanguage::Shell:
        case ScriptLanguage::Bash:
            interpreter = "bash";
            args.prepend("-c");
            break;
        case ScriptLanguage::PowerShell:
            interpreter = "powershell";
            args.prepend("-Command");
            break;
    }
    
    // 如果是文件，添加到参数
    if (!m_config.filePath.isEmpty() && m_config.language != ScriptLanguage::Shell) {
        args.append(m_config.filePath);
    }
    
    qDebug() << "[ScriptEngine] Executing:" << interpreter << args;
    
    m_process->start(interpreter, args);
    
    // 等待输出
    while (!m_process->waitForFinished(100)) {
        if (m_stopped) {
            m_process->kill();
            result.exitCode = -1;
            result.errorMessage = "Execution stopped by user";
            result.success = false;
            break;
        }
        
        // 读取输出
        if (m_config.captureOutput) {
            QString output = QString::fromUtf8(m_process->readAllStandardOutput());
            if (!output.isEmpty()) {
                emit outputReceived(output);
                result.output += output;
            }
            
            QString error = QString::fromUtf8(m_process->readAllStandardError());
            if (!error.isEmpty()) {
                emit errorReceived(error);
                result.error += error;
            }
        }
    }
    
    if (!m_stopped) {
        // 读取剩余输出
        if (m_config.captureOutput) {
            QString output = QString::fromUtf8(m_process->readAllStandardOutput());
            if (!output.isEmpty()) {
                emit outputReceived(output);
                result.output += output;
            }
            
            QString error = QString::fromUtf8(m_process->readAllStandardError());
            if (!error.isEmpty()) {
                emit errorReceived(error);
                result.error += error;
            }
        }
        
        result.exitCode = m_process->exitCode();
        result.success = (result.exitCode == 0);
    }
    
    result.duration = timer.elapsed();
    result.errorMessage = result.error;
    
    emit finished(result);
    
    m_process->deleteLater();
    deleteLater();
}

void ScriptExecutionThread::stop() {
    m_stopped = true;
    if (m_process) {
        m_process->kill();
    }
}

// ScriptEngineManager implementation
ScriptEngineManager::ScriptEngineManager(QObject* parent)
    : QObject(parent) {
    
    m_scriptsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/scripts";
    m_historyFile = m_scriptsDir + "/execution_history.json";
    
    QDir().mkpath(m_scriptsDir);
    
    loadScripts();
    loadHistory();
}

ScriptEngineManager* ScriptEngineManager::instance() {
    if (!s_instance) {
        s_instance = new ScriptEngineManager();
    }
    return s_instance;
}

QString ScriptEngineManager::createScript(const ScriptConfig& config) {
    ScriptConfig newConfig = config;
    newConfig.id = generateId();
    newConfig.createdAt = QDateTime::currentMSecsSinceEpoch();
    newConfig.modifiedAt = newConfig.createdAt;
    
    // 保存脚本文件
    if (!newConfig.filePath.isEmpty()) {
        QString fullPath = m_scriptsDir + "/" + newConfig.filePath;
        QFile file(fullPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(newConfig.code.toUtf8());
        }
    }
    
    m_scripts[newConfig.id] = newConfig;
    saveScripts();
    
    emit scriptAdded(newConfig.id);
    
    qDebug() << "[ScriptEngine] Created script:" << newConfig.id << newConfig.name;
    
    return newConfig.id;
}

void ScriptEngineManager::deleteScript(const QString& id) {
    if (!m_scripts.contains(id)) return;
    
    // 删除文件
    ScriptConfig& config = m_scripts[id];
    if (!config.filePath.isEmpty()) {
        QString fullPath = m_scriptsDir + "/" + config.filePath;
        QFile::remove(fullPath);
    }
    
    m_scripts.remove(id);
    saveScripts();
    
    emit scriptDeleted(id);
}

void ScriptEngineManager::updateScript(const QString& id, const ScriptConfig& config) {
    if (!m_scripts.contains(id)) return;
    
    ScriptConfig& existing = m_scripts[id];
    existing.name = config.name;
    existing.description = config.description;
    existing.language = config.language;
    existing.code = config.code;
    existing.arguments = config.arguments;
    existing.environment = config.environment;
    existing.workingDirectory = config.workingDirectory;
    existing.timeout = config.timeout;
    existing.captureOutput = config.captureOutput;
    existing.showInTerminal = config.showInTerminal;
    existing.modifiedAt = QDateTime::currentMSecsSinceEpoch();
    
    // 更新文件
    if (!existing.filePath.isEmpty()) {
        QString fullPath = m_scriptsDir + "/" + existing.filePath;
        QFile file(fullPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(existing.code.toUtf8());
        }
    }
    
    saveScripts();
    emit scriptUpdated(id);
}

ScriptConfig ScriptEngineManager::getScript(const QString& id) const {
    return m_scripts.value(id);
}

QList<ScriptConfig> ScriptEngineManager::getAllScripts() const {
    return m_scripts.values();
}

QList<ScriptConfig> ScriptEngineManager::getScriptsByLanguage(ScriptLanguage lang) const {
    QList<ScriptConfig> result;
    for (auto it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        if (it->language == lang) {
            result.append(it.value());
        }
    }
    return result;
}

QList<ScriptConfig> ScriptEngineManager::searchScripts(const QString& query) const {
    QList<ScriptConfig> results;
    QString lowerQuery = query.toLower();
    
    for (auto it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        const ScriptConfig& config = it.value();
        bool match = config.name.toLower().contains(lowerQuery) ||
                    config.description.toLower().contains(lowerQuery) ||
                    config.author.toLower().contains(lowerQuery) ||
                    config.code.toLower().contains(lowerQuery);
        if (match) results.append(config);
    }
    return results;
}

QString ScriptEngineManager::executeScript(const QString& scriptId, const QStringList& args, const QString& sessionId) {
    if (!m_scripts.contains(scriptId)) return QString();
    
    ScriptConfig config = m_scripts[scriptId];
    config.arguments = args;
    
    QString executionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    ScriptExecutionThread* thread = new ScriptExecutionThread(config, sessionId);
    m_runningExecutions[executionId] = thread;
    
    connect(thread, &ScriptExecutionThread::started, this, [this, executionId, scriptId]() {
        emit executionStarted(executionId, scriptId);
    });
    
    connect(thread, &ScriptExecutionThread::outputReceived, this, [this, executionId](const QString& output) {
        emit executionOutput(executionId, output);
    });
    
    connect(thread, &ScriptExecutionThread::errorReceived, this, [this, executionId](const QString& error) {
        emit executionError(executionId, error);
    });
    
    connect(thread, &ScriptExecutionThread::finished, this, [this, executionId, scriptId](const ScriptExecutionResult& result) {
        m_runningExecutions.remove(executionId);
        m_executionHistory[scriptId].append(result);
        m_executionHistory[scriptId].resize(100);  // Keep last 100
        
        // 更新运行统计
        if (m_scripts.contains(scriptId)) {
            m_scripts[scriptId].runCount++;
            m_scripts[scriptId].lastRunDuration = result.duration;
            saveScripts();
        }
        
        saveHistory();
        emit executionFinished(executionId, result);
    });
    
    thread->start();
    
    return executionId;
}

ScriptExecutionResult ScriptEngineManager::executeCode(ScriptLanguage lang, const QString& code, const QString& sessionId) {
    ScriptConfig config;
    config.id = generateId();
    config.name = "Inline Script";
    config.language = lang;
    config.code = code;
    config.captureOutput = true;
    
    ScriptExecutionThread* thread = new ScriptExecutionThread(config, sessionId);
    
    ScriptExecutionResult result;
    
    QEventLoop loop;
    connect(thread, &ScriptExecutionThread::finished, &loop, &QEventLoop::quit);
    connect(thread, &ScriptExecutionThread::finished, [&result](const ScriptExecutionResult& r) {
        result = r;
    });
    
    thread->start();
    loop.exec();
    
    return result;
}

void ScriptEngineManager::stopExecution(const QString& executionId) {
    if (m_runningExecutions.contains(executionId)) {
        m_runningExecutions[executionId]->stop();
    }
}

bool ScriptEngineManager::isRunning(const QString& executionId) const {
    return m_runningExecutions.contains(executionId);
}

ScriptExecutionResult ScriptEngineManager::getExecutionResult(const QString& executionId) const {
    // For completed executions, search history
    for (auto it = m_executionHistory.begin(); it != m_executionHistory.end(); ++it) {
        for (const ScriptExecutionResult& result : it.value()) {
            if (result.scriptId == executionId) {
                return result;
            }
        }
    }
    return ScriptExecutionResult();
}

QList<QString> ScriptEngineManager::getRunningExecutions() const {
    return m_runningExecutions.keys();
}

QList<ScriptExecutionResult> ScriptEngineManager::getExecutionHistory(const QString& scriptId, int limit) const {
    QList<ScriptExecutionResult> history = m_executionHistory.value(scriptId);
    
    std::sort(history.begin(), history.end(), [](const ScriptExecutionResult& a, const ScriptExecutionResult& b) {
        return a.timestamp > b.timestamp;
    });
    
    if (history.size() > limit) {
        history.resize(limit);
    }
    
    return history;
}

ScriptConfig ScriptEngineManager::createPythonTemplate(const QString& name) {
    ScriptConfig config;
    config.name = name;
    config.description = "Python script template";
    config.language = ScriptLanguage::Python;
    config.code = R"(#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
)'" + name + R"(
"""

import sys
import os

def main():
    print("Hello from Python script!")
    print(f"Arguments: {sys.argv[1:]}")
    
    # Your code here
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
)";
    config.timeout = 300;
    config.captureOutput = true;
    
    return config;
}

ScriptConfig ScriptEngineManager::createJavaScriptTemplate(const QString& name) {
    ScriptConfig config;
    config.name = name;
    config.description = "Node.js script template";
    config.language = ScriptLanguage::JavaScript;
    config.code = R"(#!/usr/bin/env node
/**
 * )" + name + R"(
 */

const args = process.argv.slice(2);

console.log("Hello from Node.js script!");
console.log("Arguments:", args);

// Your code here

process.exit(0);
)";
    config.timeout = 300;
    config.captureOutput = true;
    
    return config;
}

ScriptConfig ScriptEngineManager::createShellTemplate(const QString& name) {
    ScriptConfig config;
    config.name = name;
    config.description = "Bash script template";
    config.language = ScriptLanguage::Bash;
    config.code = R"(#!/bin/bash
#
# )" + name + R"(
#

set -e

echo "Hello from Bash script!"
echo "Arguments: $@"

# Your code here

exit 0
)";
    config.timeout = 300;
    config.captureOutput = true;
    
    return config;
}

void ScriptEngineManager::exportScript(const QString& id, const QString& filePath) {
    if (!m_scripts.contains(id)) return;
    
    const ScriptConfig& config = m_scripts[id];
    
    QJsonObject json;
    json["id"] = config.id;
    json["name"] = config.name;
    json["description"] = config.description;
    json["language"] = static_cast<int>(config.language);
    json["code"] = config.code;
    json["arguments"] = QJsonArray::fromStringList(config.arguments);
    json["workingDirectory"] = config.workingDirectory;
    json["timeout"] = config.timeout;
    json["author"] = config.author;
    json["version"] = config.version;
    
    // 环境变量
    QJsonObject envJson;
    for (auto it = config.environment.begin(); it != config.environment.end(); ++it) {
        envJson[it.key()] = it.value();
    }
    json["environment"] = envJson;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
}

void ScriptEngineManager::importScript(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject json = doc.object();
    
    ScriptConfig config;
    config.id = generateId();
    config.name = json["name"].toString();
    config.description = json["description"].toString();
    config.language = static_cast<ScriptLanguage>(json["language"].toInt(0));
    config.code = json["code"].toString();
    config.arguments = QJsonArray::fromStringList(json["arguments"].toArray().toVariantList().toStringList());
    config.workingDirectory = json["workingDirectory"].toString();
    config.timeout = json["timeout"].toInt(300);
    config.author = json["author"].toString();
    config.version = json["version"].toString();
    
    QJsonObject envJson = json["environment"].toObject();
    for (auto it = envJson.begin(); it != envJson.end(); ++it) {
        config.environment[it.key()] = it.value().toString();
    }
    
    config.createdAt = QDateTime::currentMSecsSinceEpoch();
    config.modifiedAt = config.createdAt;
    
    m_scripts[config.id] = config;
    saveScripts();
    
    emit scriptAdded(config.id);
}

void ScriptEngineManager::exportAllScripts(const QString& dirPath) {
    QDir().mkpath(dirPath);
    
    for (auto it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        QString filePath = dirPath + "/" + it->name.replace(" ", "_") + ".json";
        exportScript(it.key(), filePath);
    }
}

void ScriptEngineManager::importAllScripts(const QString& dirPath) {
    QDir dir(dirPath);
    QFileInfoList files = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
    
    for (const QFileInfo& fileInfo : files) {
        importScript(fileInfo.absoluteFilePath());
    }
}

bool ScriptEngineManager::isPythonAvailable() {
    return !QStandardPaths::findExecutable("python3").isEmpty() || 
           !QStandardPaths::findExecutable("python").isEmpty();
}

bool ScriptEngineManager::isNodeAvailable() {
    return !QStandardPaths::findExecutable("node").isEmpty();
}

QString ScriptEngineManager::getPythonVersion() {
    QProcess process;
    process.start("python3", QStringList() << "--version");
    process.waitForFinished(5000);
    QString output = QString::fromUtf8(process.readAllStandardError());
    return output.trimmed();
}

QString ScriptEngineManager::getNodeVersion() {
    QProcess process;
    process.start("node", QStringList() << "--version");
    process.waitForFinished(5000);
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    return output.trimmed();
}

QStringList ScriptEngineManager::getAvailableLanguages() {
    QStringList languages;
    
    if (isPythonAvailable()) languages << "Python";
    if (isNodeAvailable()) languages << "JavaScript (Node.js)";
    languages << "Bash" << "Shell" << "PowerShell";
    
    return languages;
}

QStringList ScriptEngineManager::suggestKeywords(ScriptLanguage lang, const QString& prefix) const {
    QStringList keywords;
    
    switch (lang) {
        case ScriptLanguage::Python: {
            keywords << "def" << "class" << "import" << "from" << "return" << "if" << "else" 
                    << "elif" << "for" << "while" << "try" << "except" << "finally" 
                    << "with" << "as" << "lambda" << "yield" << "raise" << "assert";
            break;
        }
        case ScriptLanguage::JavaScript: {
            keywords << "function" << "const" << "let" << "var" << "return" << "if" << "else"
                    << "for" << "while" << "try" << "catch" << "finally" << "class" << "import"
                    << "export" << "async" << "await" << "new" << "this";
            break;
        }
        case ScriptLanguage::Bash:
        case ScriptLanguage::Shell: {
            keywords << "if" << "then" << "else" << "fi" << "for" << "do" << "done" << "while"
                    << "case" << "esac" << "function" << "return" << "exit" << "export";
            break;
        }
        default:
            break;
    }
    
    if (!prefix.isEmpty()) {
        keywords.filter(prefix);
    }
    
    return keywords;
}

QString ScriptEngineManager::getSyntaxTemplate(ScriptLanguage lang, const QString& keyword) const {
    if (lang == ScriptLanguage::Python) {
        if (keyword == "def") return "def ${1:function_name}(${2:args}):\n    ${3:pass}";
        if (keyword == "class") return "class ${1:ClassName}:\n    def __init__(self):\n        ${2:pass}";
        if (keyword == "for") return "for ${1:item} in ${2:iterable}:\n    ${3:pass}";
        if (keyword == "if") return "if ${1:condition}:\n    ${2:pass}";
        if (keyword == "try") return "try:\n    ${1:pass}\nexcept ${2:Exception} as ${3:e}:\n    ${4:pass}";
    }
    
    if (lang == ScriptLanguage::JavaScript) {
        if (keyword == "function") return "function ${1:name}(${2:args}) {\n    ${3:// code}\n}";
        if (keyword == "class") return "class ${1:ClassName} {\n    constructor(${2:args}) {\n        ${3:}\n    }\n}";
        if (keyword == "async") return "async function ${1:name}(${2:args}) {\n    ${3:await }\n}";
    }
    
    if (lang == ScriptLanguage::Bash) {
        if (keyword == "if") return "if [[ ${1:condition} ]]; then\n    ${2:}\nfi";
        if (keyword == "for") return "for ${1:var} in ${2:list}; do\n    ${3:}\ndone";
        if (keyword == "function") return "${1:name}() {\n    ${2:}\n}";
    }
    
    return QString();
}

QString ScriptEngineManager::generateId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void ScriptEngineManager::saveScripts() {
    QJsonObject root;
    
    QJsonArray scriptsJson;
    for (auto it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        const ScriptConfig& config = it.value();
        QJsonObject json;
        json["id"] = config.id;
        json["name"] = config.name;
        json["description"] = config.description;
        json["language"] = static_cast<int>(config.language);
        json["code"] = config.code;
        json["filePath"] = config.filePath;
        json["arguments"] = QJsonArray::fromStringList(config.arguments);
        json["workingDirectory"] = config.workingDirectory;
        json["timeout"] = config.timeout;
        json["captureOutput"] = config.captureOutput;
        json["showInTerminal"] = config.showInTerminal;
        json["createdAt"] = config.createdAt;
        json["modifiedAt"] = config.modifiedAt;
        json["runCount"] = config.runCount;
        json["lastRunDuration"] = config.lastRunDuration;
        json["author"] = config.author;
        json["version"] = config.version;
        
        QJsonObject envJson;
        for (auto it2 = config.environment.begin(); it2 != config.environment.end(); ++it2) {
            envJson[it2.key()] = it2.value();
        }
        json["environment"] = envJson;
        
        scriptsJson.append(json);
    }
    root["scripts"] = scriptsJson;
    
    QFile file(m_scriptsDir + "/scripts.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

void ScriptEngineManager::loadScripts() {
    QFile file(m_scriptsDir + "/scripts.json");
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonArray scriptsJson = root["scripts"].toArray();
    
    for (const QJsonValue& value : scriptsJson) {
        QJsonObject json = value.toObject();
        
        ScriptConfig config;
        config.id = json["id"].toString();
        config.name = json["name"].toString();
        config.description = json["description"].toString();
        config.language = static_cast<ScriptLanguage>(json["language"].toInt(0));
        config.code = json["code"].toString();
        config.filePath = json["filePath"].toString();
        config.arguments = QJsonArray::fromStringList(json["arguments"].toArray().toVariantList().toStringList());
        config.workingDirectory = json["workingDirectory"].toString();
        config.timeout = json["timeout"].toInt(300);
        config.captureOutput = json["captureOutput"].toBool(true);
        config.showInTerminal = json["showInTerminal"].toBool(false);
        config.createdAt = json["createdAt"].toVariant().toLongLong(0);
        config.modifiedAt = json["modifiedAt"].toVariant().toLongLong(0);
        config.runCount = json["runCount"].toInt(0);
        config.lastRunDuration = json["lastRunDuration"].toInt(0);
        config.author = json["author"].toString();
        config.version = json["version"].toString();
        
        QJsonObject envJson = json["environment"].toObject();
        for (auto it = envJson.begin(); it != envJson.end(); ++it) {
            config.environment[it.key()] = it.value().toString();
        }
        
        m_scripts[config.id] = config;
    }
    
    qDebug() << "[ScriptEngine] Loaded" << m_scripts.size() << "scripts";
}

void ScriptEngineManager::saveHistory() {
    QJsonObject root;
    
    for (auto it = m_executionHistory.begin(); it != m_executionHistory.end(); ++it) {
        QJsonArray resultsJson;
        for (const ScriptExecutionResult& result : it.value()) {
            QJsonObject json;
            json["scriptId"] = result.scriptId;
            json["scriptName"] = result.scriptName;
            json["exitCode"] = result.exitCode;
            json["output"] = result.output;
            json["error"] = result.error;
            json["duration"] = result.duration;
            json["timestamp"] = result.timestamp;
            json["success"] = result.success;
            json["errorMessage"] = result.errorMessage;
            resultsJson.append(json);
        }
        root[it.key()] = resultsJson;
    }
    
    QFile file(m_historyFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

void ScriptEngineManager::loadHistory() {
    QFile file(m_historyFile);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    
    for (auto it = root.begin(); it != root.end(); ++it) {
        QString scriptId = it.key();
        QJsonArray resultsJson = it->toArray();
        
        QList<ScriptExecutionResult> results;
        for (const QJsonValue& value : resultsJson) {
            QJsonObject json = value.toObject();
            
            ScriptExecutionResult result;
            result.scriptId = json["scriptId"].toString();
            result.scriptName = json["scriptName"].toString();
            result.exitCode = json["exitCode"].toInt(-1);
            result.output = json["output"].toString();
            result.error = json["error"].toString();
            result.duration = json["duration"].toInt(0);
            result.timestamp = json["timestamp"].toVariant().toLongLong(0);
            result.success = json["success"].toBool(false);
            result.errorMessage = json["errorMessage"].toString();
            
            results.append(result);
        }
        
        m_executionHistory[scriptId] = results;
    }
}

QString ScriptEngineManager::detectInterpreter(ScriptLanguage lang) const {
    switch (lang) {
        case ScriptLanguage::Python:
            if (!QStandardPaths::findExecutable("python3").isEmpty()) return "python3";
            if (!QStandardPaths::findExecutable("python").isEmpty()) return "python";
            return "python3";
        case ScriptLanguage::JavaScript:
            return "node";
        case ScriptLanguage::Bash:
            return "bash";
        case ScriptLanguage::Shell:
            return "sh";
        case ScriptLanguage::PowerShell:
            return "powershell";
        default:
            return QString();
    }
}

QStringList ScriptEngineManager::buildCommand(const ScriptConfig& config) const {
    QStringList command;
    
    switch (config.language) {
        case ScriptLanguage::Python:
            command << detectInterpreter(config.language);
            if (!config.filePath.isEmpty()) command << config.filePath;
            command << config.arguments;
            break;
        case ScriptLanguage::JavaScript:
            command << detectInterpreter(config.language);
            if (!config.filePath.isEmpty()) command << config.filePath;
            command << config.arguments;
            break;
        case ScriptLanguage::Bash:
            command << "bash" << "-c" << config.code;
            command << config.arguments;
            break;
        default:
            break;
    }
    
    return command;
}

#include "ScriptEngine.moc"
