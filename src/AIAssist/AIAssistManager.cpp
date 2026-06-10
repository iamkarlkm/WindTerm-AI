#include "AIAssistManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>
#include <QStandardPaths>

AIAssistManager* AIAssistManager::s_instance = nullptr;

AIAssistManager::AIAssistManager(QObject* parent) : QObject(parent) {
    loadErrorPatterns();
    loadNLPPatterns();
    loadKnowledgeBase();
}

AIAssistManager* AIAssistManager::instance() {
    if (!s_instance) s_instance = new AIAssistManager();
    return s_instance;
}

void AIAssistManager::loadErrorPatterns() {
    // Common error patterns and solutions
    m_errorPatterns["permission denied"] = {
        "Check file permissions with 'ls -la'",
        "Use 'sudo' if you have privileges",
        "Check ownership with 'stat <file>'"
    };
    m_errorPatterns["command not found"] = {
        "Check spelling",
        "Verify PATH with 'echo $PATH'",
        "Install required package",
        "Use 'which <command>' to locate"
    };
    m_errorPatterns["no such file"] = {
        "Verify file path",
        "Check current directory with 'pwd'",
        "Use 'find' to locate file"
    };
    m_errorPatterns["connection refused"] = {
        "Check if service is running",
        "Verify port with 'netstat -tlnp'",
        "Check firewall rules",
        "Verify hostname/IP"
    };
    m_errorPatterns["timeout"] = {
        "Check network connectivity",
        "Increase timeout value",
        "Check remote service status"
    };
    m_errorPatterns["out of memory"] = {
        "Check memory with 'free -h'",
        "Close other applications",
        "Optimize command parameters",
        "Increase swap space"
    };
    m_errorPatterns["disk full"] = {
        "Check disk with 'df -h'",
        "Clean with 'du -sh *'",
        "Remove old logs in /var/log",
        "Clear package cache"
    };
}

void AIAssistManager::loadNLPPatterns() {
    // Natural language to command patterns
    m_nlpPatterns["list files"] = "ls -la";
    m_nlpPatterns["show disk space"] = "df -h";
    m_nlpPatterns["check memory"] = "free -h";
    m_nlpPatterns["show processes"] = "ps aux";
    m_nlpPatterns["kill process"] = "kill -9 PID";
    m_nlpPatterns["find file"] = "find . -name '*.txt'";
    m_nlpPatterns["search text"] = "grep -r 'pattern' .";
    m_nlpPatterns["download file"] = "wget URL";
    m_nlpPatterns["extract archive"] = "tar -xvf file.tar.gz";
    m_nlpPatterns["compress file"] = "tar -czvf archive.tar.gz folder/";
    m_nlpPatterns["copy to remote"] = "scp file user@host:/path";
    m_nlpPatterns["sync files"] = "rsync -av source/ dest/";
    m_nlpPatterns["check port"] = "netstat -tlnp | grep PORT";
    m_nlpPatterns["view log"] = "tail -f /var/log/syslog";
    m_nlpPatterns["restart service"] = "systemctl restart SERVICE";
    m_nlpPatterns["check status"] = "systemctl status SERVICE";
    m_nlpPatterns["docker containers"] = "docker ps -a";
    m_nlpPatterns["docker images"] = "docker images";
    m_nlpPatterns["kubernetes pods"] = "kubectl get pods";
    m_nlpPatterns["git status"] = "git status";
    m_nlpPatterns["git commit"] = "git add . && git commit -m 'message'";
    m_nlpPatterns["git push"] = "git push origin branch";
}

void AIAssistManager::loadKnowledgeBase() {
    // Load from file if exists
    QString kbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ai_knowledge.json";
    QFile file(kbPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        // Load knowledge base from JSON
    }
}

QList<AIDiagnostic> AIAssistManager::diagnoseCommand(const QString& command, const QString& output) const {
    QList<AIDiagnostic> diagnostics;
    
    // Empty command check
    if (command.trimmed().isEmpty()) {
        AIDiagnostic diag;
        diag.position = 0;
        diag.length = 0;
        diag.message = "Empty command";
        diag.severity = "warning";
        diag.suggestion = "Enter a command to execute";
        diagnostics.append(diag);
        return diagnostics;
    }
    
    // Command existence check
    QString cmdName = command.split(' ')[0];
    QStringList commonCommands = {"ls", "cd", "pwd", "cp", "mv", "rm", "cat", "grep", "find", "ssh"};
    if (!commonCommands.contains(cmdName) && cmdName.length() < 3) {
        AIDiagnostic diag;
        diag.position = 0;
        diag.length = cmdName.length();
        diag.message = "Unknown command: " + cmdName;
        diag.severity = "error";
        diag.suggestion = "Check spelling or use 'which " + cmdName + "'";
        diag.relatedDoc = "man " + cmdName;
        diagnostics.append(diag);
    }
    
    // Quote matching
    int singleQuotes = command.count('\'');
    int doubleQuotes = command.count('"');
    if (singleQuotes % 2 != 0) {
        AIDiagnostic diag;
        diag.position = command.indexOf('\'');
        diag.length = 1;
        diag.message = "Unmatched single quote";
        diag.severity = "error";
        diagnostics.append(diag);
    }
    if (doubleQuotes % 2 != 0) {
        AIDiagnostic diag;
        diag.position = command.indexOf('"');
        diag.length = 1;
        diag.message = "Unmatched double quote";
        diag.severity = "error";
        diagnostics.append(diag);
    }
    
    // Dangerous command warnings
    if (command.contains("rm -rf /") || command.contains("rm -rf /*")) {
        AIDiagnostic diag;
        diag.position = 0;
        diag.length = command.length();
        diag.message = "DANGEROUS: This will delete all files!";
        diag.severity = "error";
        diag.suggestion = "Never run 'rm -rf /' or 'rm -rf /*'";
        diagnostics.append(diag);
    }
    
    // Sudo without command
    if (command == "sudo") {
        AIDiagnostic diag;
        diag.position = 0;
        diag.length = 4;
        diag.message = "sudo requires a command";
        diag.severity = "warning";
        diag.suggestion = "Add command after sudo, e.g., 'sudo apt update'";
        diagnostics.append(diag);
    }
    
    emit diagnosticCompleted(diagnostics);
    return diagnostics;
}

QList<AIDiagnostic> AIAssistManager::diagnoseError(const QString& errorMessage) const {
    QList<AIDiagnostic> diagnostics;
    QString lowerError = errorMessage.toLower();
    
    for (auto it = m_errorPatterns.begin(); it != m_errorPatterns.end(); ++it) {
        if (lowerError.contains(it.key())) {
            AIDiagnostic diag;
            diag.position = 0;
            diag.length = errorMessage.length();
            diag.message = errorMessage;
            diag.severity = "error";
            diag.suggestion = it.value().join("\n");
            diagnostics.append(diag);
        }
    }
    
    return diagnostics;
}

QString AIAssistManager::suggestFix(const QString& error, const QString& command) const {
    QString lowerError = error.toLower();
    
    if (lowerError.contains("permission denied")) {
        if (command.contains("sudo")) {
            return "Check file permissions or run as root user";
        }
        return "Try: sudo " + command;
    }
    
    if (lowerError.contains("no such file")) {
        return "Use 'find' to locate the file or check the path";
    }
    
    if (lowerError.contains("connection refused")) {
        return "Check if the service is running: systemctl status SERVICE";
    }
    
    if (lowerError.contains("command not found")) {
        QString cmd = error.split("'").value(1, "command");
        return "Install with: sudo apt install " + cmd;
    }
    
    return "Review the error message and check documentation";
}

QList<AIRecommendation> AIAssistManager::getRecommendations(const QString& context, int limit) const {
    QList<AIRecommendation> recommendations;
    
    // Context-based recommendations
    if (context.contains("docker")) {
        AIRecommendation rec;
        rec.type = "workflow";
        rec.title = "Docker Best Practices";
        rec.description = "Use multi-stage builds to reduce image size";
        rec.example = "FROM node:14 AS builder\n...\nFROM alpine:latest";
        rec.confidence = 85;
        rec.tags << "docker" << "optimization";
        recommendations.append(rec);
    }
    
    if (context.contains("git")) {
        AIRecommendation rec;
        rec.type = "command";
        rec.title = "Interactive Git Log";
        rec.description = "Use git log with graph visualization";
        rec.example = "git log --oneline --graph --all";
        rec.confidence = 80;
        rec.tags << "git" << "productivity";
        recommendations.append(rec);
    }
    
    if (context.contains("ssh")) {
        AIRecommendation rec;
        rec.type = "security";
        rec.title = "SSH Key Authentication";
        rec.description = "Use SSH keys instead of passwords";
        rec.example = "ssh-keygen -t ed25519 && ssh-copy-id user@host";
        rec.confidence = 90;
        rec.tags << "ssh" << "security";
        recommendations.append(rec);
    }
    
    if (context.contains("kubectl")) {
        AIRecommendation rec;
        rec.type = "command";
        rec.title = "Kubernetes Debugging";
        rec.description = "Get detailed pod information";
        rec.example = "kubectl describe pod POD_NAME";
        rec.confidence = 85;
        rec.tags << "kubernetes" << "debugging";
        recommendations.append(rec);
    }
    
    // Limit results
    if (recommendations.size() > limit) {
        recommendations.resize(limit);
    }
    
    emit recommendationUpdated(recommendations);
    return recommendations;
}

QStringList AIAssistManager::suggestNextCommands(const QString& commandHistory, int limit) const {
    QStringList suggestions;
    
    // Simple pattern-based suggestions
    if (commandHistory.contains("cd ")) {
        suggestions << "ls -la";
    }
    if (commandHistory.contains("git add")) {
        suggestions << "git commit -m 'message'";
    }
    if (commandHistory.contains("docker build")) {
        suggestions << "docker run -it IMAGE";
    }
    if (commandHistory.contains("kubectl apply")) {
        suggestions << "kubectl get pods";
    }
    if (commandHistory.contains("npm install")) {
        suggestions << "npm run dev";
    }
    if (commandHistory.contains("python3")) {
        suggestions << "pip3 install package";
    }
    
    if (suggestions.size() > limit) {
        suggestions.resize(limit);
    }
    
    return suggestions;
}

QString AIAssistManager::suggestOptimization(const QString& command) const {
    // Suggest optimizations
    if (command.contains("cat") && command.contains("|")) {
        return "Consider using 'head' or 'tail' instead of cat for large files";
    }
    if (command.contains("find") && !command.contains("-type")) {
        return "Add '-type f' or '-type d' to optimize find command";
    }
    if (command.contains("grep") && command.contains("-r") && !command.contains("--include")) {
        return "Add '--include=*.ext' to limit grep search scope";
    }
    if (command.contains("ls -l") && command.split(' ').size() > 5) {
        return "Use 'stat' for detailed file information instead of ls";
    }
    return QString();
}

CommandExplanation AIAssistManager::explainCommand(const QString& command) const {
    CommandExplanation explanation;
    explanation.command = command;
    
    QStringList parts = command.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return explanation;
    
    QString cmd = parts[0];
    explanation.summary = "Executes the '" + cmd + "' command";
    
    // Build breakdown
    for (int i = 0; i < parts.size(); ++i) {
        if (i == 0) {
            explanation.breakdown.append("Command: " + cmd);
        } else if (parts[i].startsWith("-")) {
            explanation.breakdown.append("Option: " + parts[i]);
        } else {
            explanation.breakdown.append("Argument " + QString::number(i) + ": " + parts[i]);
        }
    }
    
    // Add examples based on command
    if (cmd == "ls") {
        explanation.examples << "ls -la  # List all files with details"
                           << "ls -lh /path  # List with human-readable sizes";
        explanation.relatedCommands << "cd" << "pwd" << "find" << "du";
    } else if (cmd == "grep") {
        explanation.examples << "grep -r 'pattern' .  # Recursive search"
                           << "grep -i 'error' logfile.txt  # Case-insensitive";
        explanation.relatedCommands << "find" << "awk" << "sed";
    } else if (cmd == "docker") {
        explanation.examples << "docker ps -a  # List all containers"
                           << "docker run -it ubuntu bash  # Start container";
        explanation.relatedCommands << "kubectl" << "podman";
    }
    
    return explanation;
}

QString AIAssistManager::explainError(const QString& error) const {
    if (error.contains("permission denied", Qt::CaseInsensitive)) {
        return "You don't have permission to perform this action. Try using 'sudo' or check file permissions.";
    }
    if (error.contains("command not found", Qt::CaseInsensitive)) {
        return "The command doesn't exist or isn't in your PATH. Check spelling or install the required package.";
    }
    if (error.contains("no such file", Qt::CaseInsensitive)) {
        return "The specified file or directory doesn't exist. Verify the path and try again.";
    }
    if (error.contains("connection refused", Qt::CaseInsensitive)) {
        return "The remote service isn't accepting connections. Check if the service is running and firewall rules.";
    }
    return "An error occurred. Check the error message and consult documentation.";
}

QString AIAssistManager::explainOutput(const QString& command, const QString& output) const {
    Q_UNUSED(command)
    Q_UNUSED(output)
    // Simplified implementation
    return "Output analysis would require AI model integration";
}

NaturalLanguageParse AIAssistManager::parseNaturalLanguage(const QString& text) const {
    NaturalLanguageParse result;
    QString lowerText = text.toLower();
    
    // Detect intent
    if (lowerText.contains("list") || lowerText.contains("show") || lowerText.contains("display")) {
        result.intent = "list";
    } else if (lowerText.contains("create") || lowerText.contains("make") || lowerText.contains("new")) {
        result.intent = "create";
    } else if (lowerText.contains("delete") || lowerText.contains("remove") || lowerText.contains("rm")) {
        result.intent = "delete";
    } else if (lowerText.contains("start") || lowerText.contains("run")) {
        result.intent = "start";
    } else if (lowerText.contains("stop") || lowerText.contains("kill")) {
        result.intent = "stop";
    } else if (lowerText.contains("find") || lowerText.contains("search")) {
        result.intent = "search";
    } else if (lowerText.contains("edit") || lowerText.contains("modify") || lowerText.contains("change")) {
        result.intent = "edit";
    } else {
        result.intent = "unknown";
    }
    
    // Detect action
    if (lowerText.contains("file")) {
        result.action = "file";
    } else if (lowerText.contains("process")) {
        result.action = "process";
    } else if (lowerText.contains("container") || lowerText.contains("docker")) {
        result.action = "container";
    } else if (lowerText.contains("pod") || lowerText.contains("kubernetes") || lowerText.contains("k8s")) {
        result.action = "pod";
    } else if (lowerText.contains("service")) {
        result.action = "service";
    }
    
    // Generate command
    result.generatedCommand = generateCommand(text);
    result.confidence = 70;
    
    return result;
}

QString AIAssistManager::generateCommand(const QString& naturalLanguage) const {
    QString lowerText = naturalLanguage.toLower();
    
    // Match patterns
    for (auto it = m_nlpPatterns.begin(); it != m_nlpPatterns.end(); ++it) {
        if (lowerText.contains(it.key())) {
            return it.value();
        }
    }
    
    // Pattern-based generation
    if (lowerText.contains("ssh") && lowerText.contains("host")) {
        return "ssh user@hostname";
    }
    if (lowerText.contains("curl") || lowerText.contains("api")) {
        return "curl -X GET https://api.example.com";
    }
    if (lowerText.contains("port") && lowerText.contains("check")) {
        return "netstat -tlnp | grep PORT";
    }
    if (lowerText.contains("memory") && lowerText.contains("free")) {
        return "free -h";
    }
    if (lowerText.contains("cpu") && lowerText.contains("usage")) {
        return "top -bn1 | head -20";
    }
    if (lowerText.contains("disk") && lowerText.contains("usage")) {
        return "df -h";
    }
    
    return QString();
}

QStringList AIAssistManager::generateAlternatives(const QString& naturalLanguage) const {
    QStringList alternatives;
    QString cmd = generateCommand(naturalLanguage);
    
    if (cmd.contains("ls")) {
        alternatives << "ls -lh" << "ls -la" << "tree -L 2";
    } else if (cmd.contains("ps")) {
        alternatives << "ps aux" << "top" << "htop";
    } else if (cmd.contains("grep")) {
        alternatives << "grep -r" << "grep -i" << "ag 'pattern'";
    } else if (cmd.contains("find")) {
        alternatives << "find . -name" << "locate pattern" << "fd pattern";
    }
    
    return alternatives;
}

void AIAssistManager::recordContext(const QString& host, const QString& command, const QString& result) {
    Q_UNUSED(result)
    m_contextPatterns[host][command]++;
    emit contextLearned(command);
}

void AIAssistManager::recordPattern(const QString& pattern, const QString& command) {
    m_nlpPatterns[pattern] = command;
}

QMap<QString, int> AIAssistManager::getContextPatterns(const QString& host) const {
    return m_contextPatterns.value(host);
}

void AIAssistManager::addKnowledge(const QString& category, const QString& key, const QVariant& value) {
    m_knowledgeBase[category + "::" + key] = value;
}

QVariant AIAssistManager::getKnowledge(const QString& category, const QString& key) const {
    return m_knowledgeBase.value(category + "::" + key);
}

QString AIAssistManager::generateScript(const QString& description, const QString& language) const {
    if (language == "bash") {
        return "#!/bin/bash\n# " + description + "\n\nset -e\n\n# Your code here\n\necho 'Script completed'";
    }
    if (language == "python") {
        return "#!/usr/bin/env python3\n# " + description + "\n\ndef main():\n    pass\n\nif __name__ == '__main__':\n    main()";
    }
    return "# " + description + "\n# Script generation requires " + language;
}

QString AIAssistManager::generateDockerfile(const QString& appType, const QString& language) const {
    if (language == "python") {
        return "FROM python:3.9-slim\nWORKDIR /app\nCOPY requirements.txt .\nRUN pip install -r requirements.txt\nCOPY . .\nCMD [\"python\", \"app.py\"]";
    }
    if (language == "node") {
        return "FROM node:16-alpine\nWORKDIR /app\nCOPY package*.json ./\nRUN npm install\nCOPY . .\nCMD [\"node\", \"app.js\"]";
    }
    return "# Dockerfile for " + appType + "\nFROM alpine:latest\nCMD [\"/bin/sh\"]";
}

QString AIAssistManager::generateK8sManifest(const QString& appName, int replicas) const {
    return "apiVersion: apps/v1\nkind: Deployment\nmetadata:\n  name: " + appName +
           "\nspec:\n  replicas: " + QString::number(replicas) +
           "\n  selector:\n    matchLabels:\n      app: " + appName +
           "\n  template:\n    metadata:\n      labels:\n        app: " + appName +
           "\n    spec:\n      containers:\n      - name: " + appName +
           "\n        image: " + appName + ":latest";
}

QList<AIAssistManager::SecurityWarning> AIAssistManager::checkCommandSecurity(const QString& command) const {
    QList<SecurityWarning> warnings;
    
    if (command.contains("sudo") && command.contains("rm -rf")) {
        SecurityWarning warn;
        warn.issue = "Destructive command with sudo";
        warn.severity = "critical";
        warn.recommendation = "Avoid using 'sudo rm -rf'. Double-check the path.";
        warnings.append(warn);
    }
    
    if (command.contains("chmod 777")) {
        SecurityWarning warn;
        warn.issue = "Overly permissive file permissions";
        warn.severity = "high";
        warn.recommendation = "Use more restrictive permissions (e.g., 755 or 644)";
        warnings.append(warn);
    }
    
    if (command.contains("curl") && command.contains("|") && command.contains("sh")) {
        SecurityWarning warn;
        warn.issue = "Piping curl to shell";
        warn.severity = "high";
        warn.recommendation = "Download and review scripts before executing";
        warn.cve = "CWE-676";
        warnings.append(warn);
    }
    
    if (command.contains("password") || command.contains("secret") || command.contains("token")) {
        SecurityWarning warn;
        warn.issue = "Sensitive data in command";
        warn.severity = "medium";
        warn.recommendation = "Use environment variables or secret management";
        warnings.append(warn);
    }
    
    return warnings;
}

AIAssistManager::PerformanceAnalysis AIAssistManager::analyzeCommandPerformance(const QString& command) const {
    PerformanceAnalysis analysis;
    
    if (command.contains("find") && command.contains("/") && command.split(' ').size() < 5) {
        analysis.bottleneck = "Unrestricted find command";
        analysis.suggestion = "Add -type, -name, or limit search path";
        analysis.estimatedImprovement = 50;
    } else if (command.contains("grep") && command.contains("-r") && !command.contains("--include")) {
        analysis.bottleneck = "Recursive grep without file filter";
        analysis.suggestion = "Add --include='*.ext' to limit search";
        analysis.estimatedImprovement = 30;
    } else if (command.contains("cat") && command.contains("|")) {
        analysis.bottleneck = "Unnecessary cat usage (UUOC)";
        analysis.suggestion = "Use input redirection instead of cat";
        analysis.estimatedImprovement = 10;
    } else {
        analysis.bottleneck = "No obvious performance issues";
        analysis.suggestion = "Command appears optimized";
        analysis.estimatedImprovement = 0;
    }
    
    return analysis;
}

QString AIAssistManager::matchPattern(const QString& text, const QMap<QString, QString>& patterns) const {
    QString lowerText = text.toLower();
    for (auto it = patterns.begin(); it != patterns.end(); ++it) {
        if (lowerText.contains(it.key())) {
            return it.value();
        }
    }
    return QString();
}

int AIAssistManager::calculateConfidence(const QString& command, const QString& intent) const {
    Q_UNUSED(command)
    Q_UNUSED(intent)
    return 70;  // Simplified
}

#include "AIAssistManager.moc"
