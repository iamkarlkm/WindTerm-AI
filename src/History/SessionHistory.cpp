#include "SessionHistory.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

SessionHistoryManager* SessionHistoryManager::s_instance = nullptr;

SessionHistoryManager::SessionHistoryManager(QObject* parent)
    : QObject(parent) {
    
    m_historyFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/history.json";
    loadHistory();
}

SessionHistoryManager* SessionHistoryManager::instance() {
    if (!s_instance) {
        s_instance = new SessionHistoryManager();
    }
    return s_instance;
}

void SessionHistoryManager::addEntry(const HistoryEntry& entry) {
    HistoryEntry newEntry = entry;
    if (newEntry.id.isEmpty()) {
        newEntry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (newEntry.timestamp == 0) {
        newEntry.timestamp = QDateTime::currentMSecsSinceEpoch();
    }
    
    m_entries.append(newEntry);
    m_entryMap[newEntry.id] = newEntry;
    m_sessionIndex[newEntry.sessionId].append(newEntry.id);
    m_hostIndex[newEntry.host].append(newEntry.id);
    
    emit entryAdded(newEntry);
    
    // 定期保存
    if (m_entries.size() % 100 == 0) {
        saveHistory();
    }
}

void SessionHistoryManager::deleteEntry(const QString& id) {
    if (!m_entryMap.contains(id)) return;
    
    HistoryEntry entry = m_entryMap[id];
    m_entries.removeAll(entry);
    m_entryMap.remove(id);
    m_sessionIndex[entry.sessionId].removeAll(id);
    m_hostIndex[entry.host].removeAll(id);
    
    emit entryDeleted(id);
    saveHistory();
}

void SessionHistoryManager::updateEntry(const QString& id, const HistoryEntry& entry) {
    if (!m_entryMap.contains(id)) return;
    
    HistoryEntry& existing = m_entryMap[id];
    existing.command = entry.command;
    existing.output = entry.output;
    existing.duration = entry.duration;
    existing.exitCode = entry.exitCode;
    existing.isFavorite = entry.isFavorite;
    existing.tags = entry.tags;
    
    // 更新列表中的条目
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == id) {
            m_entries[i] = existing;
            break;
        }
    }
    
    emit entryUpdated(id);
    saveHistory();
}

HistoryEntry SessionHistoryManager::getEntry(const QString& id) const {
    return m_entryMap.value(id);
}

QList<HistoryEntry> SessionHistoryManager::searchHistory(const SearchQuery& query) const {
    QList<HistoryEntry> results;
    
    for (const HistoryEntry& entry : m_entries) {
        // 时间范围过滤
        if (query.startTime.isValid() && entry.timestamp < query.startTime.toMSecsSinceEpoch()) {
            continue;
        }
        if (query.endTime.isValid() && entry.timestamp > query.endTime.toMSecsSinceEpoch()) {
            continue;
        }
        
        // 主机过滤
        if (!query.host.isEmpty() && entry.host != query.host) {
            continue;
        }
        
        // 退出码过滤
        if (query.exitCode >= 0 && entry.exitCode != query.exitCode) {
            continue;
        }
        
        // 收藏过滤
        if (query.favoritesOnly && !entry.isFavorite) {
            continue;
        }
        
        // 标签过滤
        if (!query.tags.isEmpty()) {
            bool hasTag = false;
            for (const QString& tag : query.tags) {
                if (entry.tags.contains(tag)) {
                    hasTag = true;
                    break;
                }
            }
            if (!hasTag) continue;
        }
        
        // 文本搜索
        if (!query.text.isEmpty()) {
            bool match = false;
            if (query.useRegex) {
                match = matchRegex(entry.command, query.text) || matchRegex(entry.output, query.text);
            } else {
                match = entry.command.contains(query.text, Qt::CaseInsensitive) ||
                       entry.output.contains(query.text, Qt::CaseInsensitive);
            }
            if (!match) continue;
        }
        
        results.append(entry);
        
        if (results.size() >= query.limit) break;
    }
    
    // 按时间倒序排序
    std::sort(results.begin(), results.end(), [](const HistoryEntry& a, const HistoryEntry& b) {
        return a.timestamp > b.timestamp;
    });
    
    return results;
}

QList<HistoryEntry> SessionHistoryManager::getSessionHistory(const QString& sessionId, int limit) const {
    SearchQuery query;
    query.limit = limit;
    // 手动过滤 session
    QList<HistoryEntry> results;
    for (const HistoryEntry& entry : m_entries) {
        if (entry.sessionId == sessionId) {
            results.append(entry);
            if (results.size() >= limit) break;
        }
    }
    std::sort(results.begin(), results.end(), [](const HistoryEntry& a, const HistoryEntry& b) {
        return a.timestamp > b.timestamp;
    });
    return results;
}

QList<HistoryEntry> SessionHistoryManager::getHostHistory(const QString& host, int limit) const {
    SearchQuery query;
    query.host = host;
    query.limit = limit;
    return searchHistory(query);
}

QList<HistoryEntry> SessionHistoryManager::getFavoriteEntries() const {
    QList<HistoryEntry> favorites;
    for (const HistoryEntry& entry : m_entries) {
        if (entry.isFavorite) {
            favorites.append(entry);
        }
    }
    std::sort(favorites.begin(), favorites.end(), [](const HistoryEntry& a, const HistoryEntry& b) {
        return a.timestamp > b.timestamp;
    });
    return favorites;
}

QList<HistoryEntry> SessionHistoryManager::getRecentCommands(int limit) const {
    SearchQuery query;
    query.limit = limit;
    return searchHistory(query);
}

int SessionHistoryManager::getTotalCount() const {
    return m_entries.size();
}

int SessionHistoryManager::getSessionCount(const QString& sessionId) const {
    return m_sessionIndex.value(sessionId).size();
}

QStringList SessionHistoryManager::getUniqueHosts() const {
    return m_hostIndex.keys();
}

QStringList SessionHistoryManager::getUniqueTags() const {
    QSet<QString> tags;
    for (const HistoryEntry& entry : m_entries) {
        for (const QString& tag : entry.tags) {
            tags.insert(tag);
        }
    }
    return tags.values();
}

QMap<QString, int> SessionHistoryManager::getCommandFrequency(int limit) const {
    QMap<QString, int> freq;
    for (const HistoryEntry& entry : m_entries) {
        QString cmd = entry.command.trimmed();
        if (!cmd.isEmpty()) {
            freq[cmd]++;
        }
    }
    
    // 排序并限制数量
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

void SessionHistoryManager::toggleFavorite(const QString& id) {
    if (!m_entryMap.contains(id)) return;
    
    HistoryEntry& entry = m_entryMap[id];
    entry.isFavorite = !entry.isFavorite;
    
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == id) {
            m_entries[i].isFavorite = entry.isFavorite;
            break;
        }
    }
    
    emit entryUpdated(id);
    saveHistory();
}

void SessionHistoryManager::addTag(const QString& id, const QString& tag) {
    if (!m_entryMap.contains(id)) return;
    
    HistoryEntry& entry = m_entryMap[id];
    if (!entry.tags.contains(tag)) {
        entry.tags.append(tag);
    }
    
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == id) {
            m_entries[i].tags = entry.tags;
            break;
        }
    }
    
    emit entryUpdated(id);
    saveHistory();
}

void SessionHistoryManager::removeTag(const QString& id, const QString& tag) {
    if (!m_entryMap.contains(id)) return;
    
    HistoryEntry& entry = m_entryMap[id];
    entry.tags.removeAll(tag);
    
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == id) {
            m_entries[i].tags = entry.tags;
            break;
        }
    }
    
    emit entryUpdated(id);
    saveHistory();
}

void SessionHistoryManager::exportHistory(const QString& filePath, const QList<QString>& ids, const QString& format) {
    QJsonArray array;
    
    for (const QString& id : ids) {
        if (!m_entryMap.contains(id)) continue;
        
        const HistoryEntry& entry = m_entryMap[id];
        QJsonObject json;
        json["id"] = entry.id;
        json["command"] = entry.command;
        json["output"] = entry.output;
        json["timestamp"] = entry.timestamp;
        json["duration"] = entry.duration;
        json["exitCode"] = entry.exitCode;
        json["workingDirectory"] = entry.workingDirectory;
        json["host"] = entry.host;
        json["isFavorite"] = entry.isFavorite;
        json["tags"] = QJsonArray::fromStringList(entry.tags);
        
        array.append(json);
    }
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}

void SessionHistoryManager::exportToMarkdown(const QString& filePath, const QList<QString>& ids) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return;
    
    file.write("# Terminal Command History\n\n");
    
    for (const QString& id : ids) {
        if (!m_entryMap.contains(id)) continue;
        
        const HistoryEntry& entry = m_entryMap[id];
        QString date = QDateTime::fromMSecsSinceEpoch(entry.timestamp).toString("yyyy-MM-dd HH:mm:ss");
        
        file.write(QString("## %1\n\n").arg(date).toUtf8());
        file.write(QString("**Host:** %1\n\n").arg(entry.host).toUtf8());
        file.write("### Command\n```bash\n");
        file.write(entry.command.toUtf8());
        file.write("\n```\n\n### Output\n```\n");
        file.write(entry.output.toUtf8());
        file.write("\n```\n\n");
    }
}

void SessionHistoryManager::exportToHtml(const QString& filePath, const QList<QString>& ids) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return;
    
    QString html = R"(
<!DOCTYPE html>
<html>
<head><title>Command History</title>
<style>
body { font-family: monospace; background: #1e1e1e; color: #d4d4d4; padding: 20px; }
.entry { margin: 20px 0; padding: 15px; background: #2d2d2d; border-radius: 5px; }
.command { color: #9cdcfe; }
.output { color: #ce9178; background: #1e1e1e; padding: 10px; margin: 10px 0; }
.meta { color: #808080; font-size: 12px; }
</style>
</head>
<body>
<h1>Terminal Command History</h1>
)";
    
    for (const QString& id : ids) {
        if (!m_entryMap.contains(id)) continue;
        
        const HistoryEntry& entry = m_entryMap[id];
        QString date = QDateTime::fromMSecsSinceEpoch(entry.timestamp).toString("yyyy-MM-dd HH:mm:ss");
        
        html += QString("<div class='entry'>") +
                QString("<div class='meta'>%1 | Host: %2 | Exit: %3</div>").arg(date, entry.host, QString::number(entry.exitCode)) +
                QString("<div class='command'><strong>Command:</strong><pre>%1</pre></div>").arg(entry.command.toHtmlEscaped()) +
                QString("<div class='output'><strong>Output:</strong><pre>%1</pre></div>").arg(entry.output.toHtmlEscaped()) +
                "</div>";
    }
    
    html += "</body></html>";
    file.write(html.toUtf8());
}

void SessionHistoryManager::clearHistory(const QString& sessionId) {
    if (sessionId.isEmpty()) {
        m_entries.clear();
        m_entryMap.clear();
        m_sessionIndex.clear();
        m_hostIndex.clear();
    } else {
        // 删除特定会话的历史
        QList<QString> idsToRemove = m_sessionIndex.value(sessionId);
        for (const QString& id : idsToRemove) {
            deleteEntry(id);
        }
    }
    
    emit historyCleared();
    saveHistory();
}

void SessionHistoryManager::cleanupOldHistory(int daysToKeep) {
    qint64 cutoff = QDateTime::currentMSecsSinceEpoch() - (daysToKeep * 24 * 60 * 60 * 1000LL);
    
    QList<QString> toRemove;
    for (const HistoryEntry& entry : m_entries) {
        if (entry.timestamp < cutoff) {
            toRemove.append(entry.id);
        }
    }
    
    for (const QString& id : toRemove) {
        deleteEntry(id);
    }
}

QStringList SessionHistoryManager::suggestCommands(const QString& prefix, int limit) const {
    QStringList suggestions;
    QString lowerPrefix = prefix.toLower();
    
    for (const HistoryEntry& entry : m_entries) {
        if (entry.command.trimmed().startsWith(lowerPrefix, Qt::CaseInsensitive)) {
            suggestions.append(entry.command.trimmed());
        }
    }
    
    suggestions.removeDuplicates();
    
    if (suggestions.size() > limit) {
        suggestions.resize(limit);
    }
    
    return suggestions;
}

QString SessionHistoryManager::suggestNextCommand(const QString& currentCommand) const {
    // 查找当前命令后最常执行的命令
    QMap<QString, int> nextCmdFreq;
    
    for (int i = 0; i < m_entries.size() - 1; ++i) {
        if (m_entries[i].command.trimmed() == currentCommand.trimmed()) {
            QString nextCmd = m_entries[i + 1].command.trimmed();
            nextCmdFreq[nextCmd]++;
        }
    }
    
    if (nextCmdFreq.isEmpty()) return QString();
    
    // 返回频率最高的
    QString best;
    int maxFreq = 0;
    for (auto it = nextCmdFreq.begin(); it != nextCmdFreq.end(); ++it) {
        if (it.value() > maxFreq) {
            maxFreq = it.value();
            best = it.key();
        }
    }
    
    return best;
}

void SessionHistoryManager::saveHistory() {
    QJsonArray array;
    
    for (const HistoryEntry& entry : m_entries) {
        QJsonObject json;
        json["id"] = entry.id;
        json["sessionId"] = entry.sessionId;
        json["command"] = entry.command;
        json["output"] = entry.output;
        json["timestamp"] = entry.timestamp;
        json["duration"] = entry.duration;
        json["exitCode"] = entry.exitCode;
        json["workingDirectory"] = entry.workingDirectory;
        json["host"] = entry.host;
        json["isFavorite"] = entry.isFavorite;
        json["tags"] = QJsonArray::fromStringList(entry.tags);
        
        array.append(json);
    }
    
    QFile file(m_historyFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}

void SessionHistoryManager::loadHistory() {
    QFile file(m_historyFile);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    
    for (const QJsonValue& value : array) {
        QJsonObject json = value.toObject();
        
        HistoryEntry entry;
        entry.id = json["id"].toString();
        entry.sessionId = json["sessionId"].toString();
        entry.command = json["command"].toString();
        entry.output = json["output"].toString();
        entry.timestamp = json["timestamp"].toVariant().toLongLong();
        entry.duration = json["duration"].toInt(0);
        entry.exitCode = json["exitCode"].toInt(0);
        entry.workingDirectory = json["workingDirectory"].toString();
        entry.host = json["host"].toString();
        entry.isFavorite = json["isFavorite"].toBool(false);
        entry.tags = QJsonArray::fromStringList(json["tags"].toArray().toVariantList().toStringList());
        
        m_entries.append(entry);
        m_entryMap[entry.id] = entry;
        m_sessionIndex[entry.sessionId].append(entry.id);
        m_hostIndex[entry.host].append(entry.id);
    }
    
    qDebug() << "[SessionHistory] Loaded" << m_entries.size() << "history entries";
}

void SessionHistoryManager::rebuildIndex() {
    m_sessionIndex.clear();
    m_hostIndex.clear();
    
    for (const HistoryEntry& entry : m_entries) {
        m_sessionIndex[entry.sessionId].append(entry.id);
        m_hostIndex[entry.host].append(entry.id);
    }
}

bool SessionHistoryManager::matchRegex(const QString& text, const QString& pattern) const {
    QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
    return re.match(text).hasMatch();
}

#include "SessionHistory.moc"
