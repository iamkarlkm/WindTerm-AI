#include "CommandCompletion.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <algorithm>

CommandCompletionEngine* CommandCompletionEngine::s_instance = nullptr;

CommandCompletionEngine::CommandCompletionEngine(QObject* parent)
    : QObject(parent) {
}

CommandCompletionEngine* CommandCompletionEngine::instance() {
    if (!s_instance) {
        s_instance = new CommandCompletionEngine();
    }
    return s_instance;
}

void CommandCompletionEngine::initialize() {
    loadBuiltInCommands();
    scanSystemCommands();
    loadCommandDatabase();
    
    emit databaseLoaded();
}

void CommandCompletionEngine::loadBuiltInCommands() {
    // Linux 常用命令
    QList<QString> commonCommands = {
        "ls", "cd", "pwd", "cp", "mv", "rm", "mkdir", "rmdir", "touch",
        "cat", "less", "more", "head", "tail", "grep", "find", "locate",
        "chmod", "chown", "ln", "df", "du", "ps", "top", "htop", "kill",
        "ssh", "scp", "rsync", "curl", "wget", "ping", "netstat", "ss",
        "git", "docker", "kubectl", "npm", "pip", "python3", "node",
        "apt", "apt-get", "yum", "dnf", "pacman", "brew",
        "systemctl", "journalctl", "service",
        "vim", "nano", "emacs", "sed", "awk",
        "tar", "gzip", "gunzip", "zip", "unzip",
        "man", "info", "whatis", "which", "whereis", "type"
    };
    
    for (const QString& cmd : commonCommands) {
        CommandInfo info;
        info.name = cmd;
        info.category = "common";
        m_commands[cmd] = info;
    }
    
    // 详细命令信息
    m_commands["ls"].description = "List directory contents";
    m_commands["ls"].syntax = "ls [OPTION]... [FILE]...";
    m_commands["ls"].options << "-l" << "-a" << "-h" << "-t" << "-r" << "-S";
    m_commands["ls"].examples << "ls -la" << "ls -lh /var/log";
    
    m_commands["cd"].description = "Change directory";
    m_commands["cd"].syntax = "cd [DIRECTORY]";
    m_commands["cd"].examples << "cd /home/user" << "cd ..";
    
    m_commands["grep"].description = "Search text patterns";
    m_commands["grep"].syntax = "grep [OPTION]... PATTERN [FILE]...";
    m_commands["grep"].options << "-i" << "-r" << "-v" << "-n" << "-C" << "-A" << "-B";
    m_commands["grep"].optionDescriptions["-i"] = "Ignore case";
    m_commands["grep"].optionDescriptions["-r"] = "Recursive";
    m_commands["grep"].examples << "grep -i error logfile.txt" << "grep -r 'pattern' /path";
    
    m_commands["find"].description = "Search for files";
    m_commands["find"].syntax = "find [PATH] [EXPRESSION]";
    m_commands["find"].options << "-name" << "-type" << "-size" << "-mtime" << "-exec";
    m_commands["find"].examples << "find . -name '*.txt'" << "find /var -type f -size +10M";
    
    m_commands["ssh"].description = "OpenSSH remote login";
    m_commands["ssh"].syntax = "ssh [OPTION] [USER@]HOSTNAME [COMMAND]";
    m_commands["ssh"].options << "-p" << "-i" << "-C" << "-L" << "-R" << "-N";
    m_commands["ssh"].examples << "ssh user@host" << "ssh -p 2222 user@host";
    
    m_commands["git"].description = "Distributed version control";
    m_commands["git"].syntax = "git [OPTION]... [COMMAND] [ARG]...";
    m_commands["git"].aliases << "g";
    m_commands["git"].examples << "git status" << "git commit -m 'message'";
    
    m_commands["docker"].description = "Container management";
    m_commands["docker"].syntax = "docker [OPTION] COMMAND [ARG]...";
    m_commands["docker"].examples << "docker ps" << "docker run -it ubuntu bash";
    
    m_commands["curl"].description = "Transfer data from URL";
    m_commands["curl"].syntax = "curl [OPTION]... URL";
    m_commands["curl"].options << "-X" << "-H" << "-d" << "-o" << "-L" << "-k";
    m_commands["curl"].examples << "curl https://api.example.com" << "curl -X POST -d 'data' url";
    
    m_commands["systemctl"].description = "Control systemd services";
    m_commands["systemctl"].syntax = "systemctl [OPTION] COMMAND [NAME]...";
    m_commands["systemctl"].options << "start" << "stop" << "restart" << "status" << "enable" << "disable";
    m_commands["systemctl"].examples << "systemctl status nginx" << "systemctl restart docker";
}

void CommandCompletionEngine::scanSystemCommands() {
    QStringList paths = QStandardPaths::standardLocations(QStandardPaths::GenericBinLocation);
    
    for (const QString& path : paths) {
        QDir dir(path);
        if (!dir.exists()) continue;
        
        QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Executable, QDir::NoSort);
        for (const QFileInfo& file : files) {
            m_executables.insert(file.fileName());
            
            // 如果没有在命令数据库中，添加为简单命令
            if (!m_commands.contains(file.fileName())) {
                CommandInfo info;
                info.name = file.fileName();
                info.category = "system";
                m_commands[file.fileName()] = info;
            }
        }
    }
    
    qDebug() << "[CommandCompletion] Scanned" << m_executables.size() << "system commands";
}

void CommandCompletionEngine::loadCommandDatabase() {
    // 从文件加载扩展命令数据库
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/command_db.json";
    QFile file(dbPath);
    if (!file.exists()) return;
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();
        QJsonObject commandsJson = root["commands"].toObject();
        
        for (auto it = commandsJson.begin(); it != commandsJson.end(); ++it) {
            QString cmdName = it.key();
            QJsonObject cmdJson = it->toObject();
            
            CommandInfo info;
            info.name = cmdName;
            info.description = cmdJson["description"].toString();
            info.category = cmdJson["category"].toString();
            info.syntax = cmdJson["syntax"].toString();
            info.aliases = QJsonArray::fromStringList(cmdJson["aliases"].toArray().toVariantList().toStringList());
            info.options = QJsonArray::fromStringList(cmdJson["options"].toArray().toVariantList().toStringList());
            info.examples = QJsonArray::fromStringList(cmdJson["examples"].toArray().toVariantList().toStringList());
            
            m_commands[cmdName] = info;
        }
    }
}

QList<CompletionResult> CommandCompletionEngine::getCompletions(const QString& input, int cursorPosition) const {
    QList<CompletionResult> results;
    
    if (input.isEmpty() || cursorPosition == 0) {
        // 命令补全
        return getCommandCompletions("", 50);
    }
    
    // 解析输入
    QString beforeCursor = input.left(cursorPosition);
    QStringList parts = beforeCursor.split(' ', Qt::SkipEmptyParts);
    
    if (parts.isEmpty()) {
        return getCommandCompletions("", 50);
    }
    
    QString command = parts[0];
    
    // 检查是否是命令
    if (parts.size() == 1) {
        return getCommandCompletions(parts[0], 20);
    }
    
    // 检查是否是选项
    if (parts.last().startsWith("-")) {
        return getOptionCompletions(command, parts.last());
    }
    
    // 参数补全
    int argIndex = parts.size() - 1;
    return getArgumentCompletions(command, argIndex, parts.last());
}

QList<CompletionResult> CommandCompletionEngine::getCommandCompletions(const QString& prefix, int limit) const {
    QList<CompletionResult> results;
    QString lowerPrefix = prefix.toLower();
    
    // 从命令数据库匹配
    for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
        if (it.key().startsWith(lowerPrefix, Qt::CaseInsensitive)) {
            CompletionResult result;
            result.text = it.key();
            result.type = "command";
            result.description = it->description;
            result.relevance = calculateRelevance(it.key(), prefix);
            
            if (m_commandFrequency.contains(it.key())) {
                result.relevance += m_commandFrequency[it.key()] / 10;
            }
            
            result.icon = "terminal";
            results.append(result);
        }
    }
    
    // 从系统可执行文件匹配
    for (const QString& exe : m_executables) {
        if (exe.startsWith(lowerPrefix, Qt::CaseInsensitive)) {
            CompletionResult result;
            result.text = exe;
            result.type = "command";
            result.relevance = calculateRelevance(exe, prefix);
            result.icon = "application";
            
            if (!results.contains(result)) {
                results.append(result);
            }
        }
    }
    
    // 按相关性排序
    std::sort(results.begin(), results.end(), [](const CompletionResult& a, const CompletionResult& b) {
        return a.relevance > b.relevance;
    });
    
    if (results.size() > limit) {
        results.resize(limit);
    }
    
    return results;
}

QList<CompletionResult> CommandCompletionEngine::getOptionCompletions(const QString& command, const QString& prefix) const {
    QList<CompletionResult> results;
    
    if (!m_commands.contains(command)) return results;
    
    const CommandInfo& info = m_commands[command];
    
    for (const QString& option : info.options) {
        if (option.startsWith(prefix, Qt::CaseInsensitive)) {
            CompletionResult result;
            result.text = option;
            result.type = "option";
            result.description = info.optionDescriptions.value(option, "");
            result.icon = "option";
            result.relevance = 10;
            results.append(result);
        }
    }
    
    // 添加通用选项
    QStringList commonOptions = {"--help", "--version", "-h", "-v"};
    for (const QString& opt : commonOptions) {
        if (opt.startsWith(prefix)) {
            CompletionResult result;
            result.text = opt;
            result.type = "option";
            result.description = "Common option";
            result.icon = "option";
            result.relevance = 5;
            results.append(result);
        }
    }
    
    return results;
}

QList<CompletionResult> CommandCompletionEngine::getArgumentCompletions(const QString& command, int argIndex, const QString& prefix) const {
    QList<CompletionResult> results;
    
    // 文件路径补全
    if (prefix.startsWith("/") || prefix.startsWith("~") || prefix.startsWith("./") || prefix.startsWith("../")) {
        return completeFilePath(prefix);
    }
    
    // 某些命令的特殊参数补全
    if (command == "cd" || command == "ls" || command == "cat") {
        return completeFilePath(prefix, command == "cd");  // cd 只补全目录
    }
    
    // Git 特殊处理
    if (command == "git") {
        if (argIndex == 1) {
            QStringList gitCommands = {"status", "commit", "push", "pull", "merge", "branch", "checkout", "diff", "log", "add"};
            for (const QString& gc : gitCommands) {
                if (gc.startsWith(prefix)) {
                    CompletionResult result;
                    result.text = gc;
                    result.type = "subcommand";
                    result.description = "Git command";
                    result.relevance = 15;
                    results.append(result);
                }
            }
        }
    }
    
    // Docker 特殊处理
    if (command == "docker") {
        if (argIndex == 1) {
            QStringList dockerCommands = {"ps", "run", "build", "images", "container", "image", "volume", "network"};
            for (const QString& dc : dockerCommands) {
                if (dc.startsWith(prefix)) {
                    CompletionResult result;
                    result.text = dc;
                    result.type = "subcommand";
                    result.description = "Docker command";
                    result.relevance = 15;
                    results.append(result);
                }
            }
        }
    }
    
    return results;
}

QList<CompletionResult> CommandCompletionEngine::completeFilePath(const QString& path, bool directoriesOnly) const {
    QList<CompletionResult> results;
    
    QString basePath = path;
    QString fileName;
    
    // 分离目录和文件名
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash >= 0) {
        basePath = path.left(lastSlash + 1);
        fileName = path.mid(lastSlash + 1);
    } else {
        basePath = "./";
        fileName = path;
    }
    
    // 处理 ~
    if (basePath.startsWith("~/")) {
        basePath = QDir::homePath() + basePath.mid(1);
    }
    
    QDir dir(basePath);
    if (!dir.exists()) return results;
    
    QFileInfoList entries = dir.entryInfoList(fileName + "*", QDir::AllEntries | QDir::NoDotAndDotDot);
    
    for (const QFileInfo& entry : entries) {
        if (directoriesOnly && !entry.isDir()) continue;
        
        CompletionResult result;
        result.text = basePath + entry.fileName();
        result.type = entry.isDir() ? "directory" : "file";
        result.description = entry.isDir() ? "Directory" : "File";
        result.icon = entry.isDir() ? "folder" : "file";
        result.relevance = 10;
        results.append(result);
    }
    
    return results;
}

QString CommandCompletionEngine::suggestNextCommand(const QString& currentCommand, const QString& host) const {
    // 基于使用频率推荐
    if (!m_hostCommandFrequency.isEmpty()) {
        if (host.isEmpty() || !m_hostCommandFrequency.contains(host)) {
            // 全局推荐
            if (!m_commandFrequency.isEmpty()) {
                QString mostFrequent;
                int maxFreq = 0;
                for (auto it = m_commandFrequency.begin(); it != m_commandFrequency.end(); ++it) {
                    if (it.value() > maxFreq) {
                        maxFreq = it.value();
                        mostFrequent = it.key();
                    }
                }
                return mostFrequent;
            }
        }
    }
    
    // 基于当前命令的上下文推荐
    if (currentCommand == "cd") return "ls";
    if (currentCommand == "git add") return "git commit";
    if (currentCommand == "docker build") return "docker run";
    
    return QString();
}

QStringList CommandCompletionEngine::suggestRelatedCommands(const QString& command) const {
    QStringList related;
    
    if (command == "ls") related << "cd" << "pwd" << "find" << "du";
    if (command == "grep") related << "find" << "cat" << "less";
    if (command == "git") related << "svn" << "hg";
    if (command == "docker") related << "kubectl" << "podman";
    if (command == "ssh") related << "scp" << "rsync" << "sftp";
    if (command == "curl") related << "wget" << "httpie";
    if (command == "systemctl") related << "journalctl" << "service";
    
    return related;
}

QString CommandCompletionEngine::explainCommand(const QString& command) const {
    if (!m_commands.contains(command)) {
        return "Command not found in database";
    }
    
    const CommandInfo& info = m_commands[command];
    
    QString explanation = QString("**%1** - %2\n\n").arg(command, info.description);
    
    if (!info.syntax.isEmpty()) {
        explanation += QString("Syntax: `%1`\n\n").arg(info.syntax);
    }
    
    if (!info.options.isEmpty()) {
        explanation += "Common options:\n";
        for (const QString& opt : info.options) {
            QString desc = info.optionDescriptions.value(opt, "");
            explanation += QString("- `%1` %2\n").arg(opt, desc);
        }
        explanation += "\n";
    }
    
    if (!info.examples.isEmpty()) {
        explanation += "Examples:\n";
        for (const QString& ex : info.examples) {
            explanation += QString("```bash\n%1\n```\n").arg(ex);
        }
    }
    
    return explanation;
}

QString CommandCompletionEngine::generateCommand(const QString& naturalLanguage) const {
    // 简单的自然语言转命令
    QString nl = naturalLanguage.toLower();
    
    if (nl.contains("list") && (nl.contains("file") || nl.contains("directory"))) {
        return "ls -la";
    }
    if (nl.contains("find") && nl.contains("file")) {
        return "find . -name '*.txt'";
    }
    if (nl.contains("search") && nl.contains("text")) {
        return "grep -r 'pattern' .";
    }
    if (nl.contains("disk") || nl.contains("space")) {
        return "df -h";
    }
    if (nl.contains("process")) {
        return "ps aux";
    }
    if (nl.contains("kill") && nl.contains("process")) {
        return "kill -9 PID";
    }
    if (nl.contains("download")) {
        return "wget URL";
    }
    if (nl.contains("copy") && nl.contains("remote")) {
        return "scp file user@host:/path";
    }
    
    return QString();
}

CommandInfo CommandCompletionEngine::getCommandInfo(const QString& command) const {
    return m_commands.value(command);
}

bool CommandCompletionEngine::isCommandExists(const QString& command) const {
    return m_commands.contains(command) || m_executables.contains(command);
}

QString CommandCompletionEngine::getCommandHelp(const QString& command) const {
    if (!m_commands.contains(command)) return QString();
    
    const CommandInfo& info = m_commands[command];
    QString help = info.description;
    
    if (!info.syntax.isEmpty()) {
        help += "\n\nUsage: " + info.syntax;
    }
    
    return help;
}

QString CommandCompletionEngine::getCommandSyntax(const QString& command) const {
    if (!m_commands.contains(command)) return QString();
    return m_commands[command].syntax;
}

void CommandCompletionEngine::recordUsage(const QString& command) {
    m_commandFrequency[command]++;
    
    if (!m_currentHost.isEmpty()) {
        m_hostCommandFrequency[m_currentHost][command]++;
    }
    
    emit usageRecorded(command);
}

void CommandCompletionEngine::setHostContext(const QString& host) {
    m_currentHost = host;
}

QMap<QString, int> CommandCompletionEngine::getHostSpecificCommands(const QString& host, int limit) const {
    if (!m_hostCommandFrequency.contains(host)) return QMap<QString, int>();
    
    QMap<QString, int> freq = m_hostCommandFrequency[host];
    
    // 排序并限制
    QList<QPair<QString, int>> sorted;
    for (auto it = freq.begin(); it != freq.end(); ++it) {
        sorted.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sorted.begin(), sorted.end(), [](const QPair<QString,int>& a, const QPair<QString,int>& b) {
        return a.second > b.second;
    });
    
    QMap<QString, int> result;
    for (int i = 0; i < qMin(limit, sorted.size()); ++i) {
        result[sorted[i].first] = sorted[i].second;
    }
    return result;
}

QMap<int, QPair<int, QString>> CommandCompletionEngine::getSyntaxHighlights(const QString& command) const {
    QMap<int, QPair<int, QString>> highlights;
    
    // 命令名高亮
    QString cmdName = command.split(' ')[0];
    if (!cmdName.isEmpty()) {
        highlights[0] = qMakePair(cmdName.length(), QString("keyword"));
    }
    
    // 选项高亮
    QRegularExpression optionRegex(R"(-[a-zA-Z]+|--[a-zA-Z-]+)");
    auto it = optionRegex.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        highlights[match.capturedStart()] = qMakePair(match.capturedLength(), QString("option"));
    }
    
    // 字符串高亮
    QRegularExpression stringRegex(R"(['"][^'"]*['"])");
    auto sit = stringRegex.globalMatch(command);
    while (sit.hasNext()) {
        QRegularExpressionMatch match = sit.next();
        highlights[match.capturedStart()] = qMakePair(match.capturedLength(), QString("string"));
    }
    
    return highlights;
}

QList<CommandCompletionEngine::ArgumentHint> CommandCompletionEngine::getArgumentHints(const QString& command) const {
    QList<ArgumentHint> hints;
    
    if (command == "ls") {
        hints.append({1, "DIRECTORY", "path", false, "Target directory"});
    } else if (command == "cd") {
        hints.append({1, "DIRECTORY", "path", true, "Directory to change to"});
    } else if (command == "grep") {
        hints.append({1, "PATTERN", "text", true, "Search pattern"});
        hints.append({2, "FILE", "file", false, "File to search"});
    } else if (command == "ssh") {
        hints.append({1, "USER@HOST", "text", true, "Remote host"});
        hints.append({2, "COMMAND", "command", false, "Command to execute"});
    }
    
    return hints;
}

QList<CommandCompletionEngine::Diagnostic> CommandCompletionEngine::diagnoseCommand(const QString& command) const {
    QList<Diagnostic> diagnostics;
    
    if (command.isEmpty()) {
        Diagnostic diag;
        diag.position = 0;
        diag.length = 0;
        diag.message = "Empty command";
        diag.severity = "warning";
        diagnostics.append(diag);
        return diagnostics;
    }
    
    // 检查命令是否存在
    QString cmdName = command.split(' ')[0];
    if (!isCommandExists(cmdName)) {
        Diagnostic diag;
        diag.position = 0;
        diag.length = cmdName.length();
        diag.message = QString("Command not found: %1").arg(cmdName);
        diag.severity = "error";
        diag.suggestion = "Check spelling or use 'which' to verify";
        diagnostics.append(diag);
    }
    
    // 检查未匹配的引号
    int singleQuotes = command.count('\'');
    int doubleQuotes = command.count('"');
    if (singleQuotes % 2 != 0) {
        Diagnostic diag;
        diag.position = command.indexOf('\'');
        diag.length = 1;
        diag.message = "Unmatched single quote";
        diag.severity = "error";
        diagnostics.append(diag);
    }
    if (doubleQuotes % 2 != 0) {
        Diagnostic diag;
        diag.position = command.indexOf('"');
        diag.length = 1;
        diag.message = "Unmatched double quote";
        diag.severity = "error";
        diagnostics.append(diag);
    }
    
    return diagnostics;
}

int CommandCompletionEngine::calculateRelevance(const QString& completion, const QString& input) const {
    int relevance = 10;
    
    // 前缀匹配加分
    if (completion.startsWith(input, Qt::CaseInsensitive)) {
        relevance += 20;
    }
    
    // 包含输入加分
    if (completion.contains(input, Qt::CaseInsensitive)) {
        relevance += 10;
    }
    
    // 短命令优先
    if (completion.length() < 5) {
        relevance += 5;
    }
    
    return relevance;
}

QStringList CommandCompletionEngine::executeAndCapture(const QString& command) const {
    QProcess process;
    process.start(command);
    process.waitForFinished(5000);
    
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    return output.split('\n', Qt::SkipEmptyParts);
}

void CommandCompletionEngine::parseManPage(const QString& command) {
    // 解析 man page 获取详细信息
    QProcess process;
    process.start("man", QStringList() << command);
    process.waitForFinished(5000);
    
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    // 解析 man page 内容...
}

#include "CommandCompletion.moc"
