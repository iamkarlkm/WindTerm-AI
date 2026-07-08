#include "Utility/TerminalOutputFilter.h"
#include "Utility/TerminalOutputServer.h"
#include "Utility/Logger.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QWebSocket>
#include <QDateTime>

TerminalOutputFilter::TerminalOutputFilter(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
}

TerminalOutputFilter::~TerminalOutputFilter()
{
    for (auto &ws : m_wsSessions) {
        if (ws.socket) {
            ws.socket->close();
            ws.socket->deleteLater();
        }
    }
}

TerminalOutputFilter &TerminalOutputFilter::instance()
{
    static TerminalOutputFilter s_instance;
    return s_instance;
}

void TerminalOutputFilter::init(const QString &configPath)
{
    if (!configPath.isEmpty()) {
        loadConfig(configPath);
    }

    m_initialized = true;
}

bool TerminalOutputFilter::loadConfig(const QString &configPath)
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARN("OutputFilter") << "Cannot open config file:" << configPath;
        return false;
    }

    QByteArray raw = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR("OutputFilter")
            << "Config parse error:" << parseError.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("rules") || !root["rules"].isArray()) {
        LOG_ERROR("OutputFilter")
            << "Config missing 'rules' array";
        return false;
    }

    QJsonArray rulesArray = root["rules"].toArray();

    QMutexLocker locker(&m_mutex);
    m_rules.clear();

    for (int i = 0; i < rulesArray.size(); i++) {
        QJsonValue val = rulesArray[i];
        if (!val.isObject()) {
            LOG_WARN("OutputFilter") << "Rule" << i << "is not an object, skipping";
            continue;
        }
        parseRule(val.toObject(), i);
    }

    m_configPath = configPath;

    if (root.contains("macros") && root["macros"].isObject()) {
        TerminalOutputServer::instance().loadMacros(root["macros"].toObject());
    }

    LOG_INFO("OutputFilter") << "Loaded " << m_rules.size()
                             << " filter rules from " << configPath;
    emit configReloaded(m_rules.size());
    return true;
}

void TerminalOutputFilter::reloadConfig()
{
    if (!m_configPath.isEmpty()) {
        loadConfig(m_configPath);
    }
}

bool TerminalOutputFilter::parseRule(const QJsonObject &obj, int index)
{
    FilterRule rule;

    if (!obj.contains("name") || !obj["name"].isString()) {
        LOG_WARN("OutputFilter")
            << "Rule" << index << "missing 'name', skipping";
        return false;
    }
    rule.name = obj["name"].toString();

    if (!obj.contains("pattern") || !obj["pattern"].isString()) {
        LOG_WARN("OutputFilter")
            << "Rule" << index << "(" << rule.name << ") missing 'pattern', skipping";
        return false;
    }

    QString pattern = obj["pattern"].toString();
    rule.regex = QRegularExpression(pattern);
    if (!rule.regex.isValid()) {
        LOG_ERROR("OutputFilter")
            << "Rule" << index << "(" << rule.name
            << ") invalid regex:" << rule.regex.errorString();
        return false;
    }

    if (obj.contains("enabled") && obj["enabled"].isBool()) {
        rule.enabled = obj["enabled"].toBool();
    }

    QString actionStr = "file";
    if (obj.contains("action") && obj["action"].isString()) {
        actionStr = obj["action"].toString().toLower();
    }

    if (actionStr == "websocket" || actionStr == "ws") {
        rule.action = FilterAction::WebSocket;
    } else if (actionStr == "both") {
        rule.action = FilterAction::Both;
    } else {
        rule.action = FilterAction::WriteFile;
    }

    if (rule.action == FilterAction::WriteFile || rule.action == FilterAction::Both) {
        if (obj.contains("filePath") && obj["filePath"].isString()) {
            rule.filePath = obj["filePath"].toString();
        } else {
            rule.filePath = QString("/tmp/windterm-filter-%1.log")
                                .arg(rule.name);
        }
    }

    if (rule.action == FilterAction::WebSocket || rule.action == FilterAction::Both) {
        if (obj.contains("wsUrl") && obj["wsUrl"].isString()) {
            rule.wsUrl = obj["wsUrl"].toString();
        } else {
            LOG_WARN("OutputFilter")
                << "Rule" << rule.name
                << "has WebSocket action but no wsUrl, skipping";
            return false;
        }

        if (!rule.enabled) {
            m_rules.append(rule);
            return true;
        }

        bool found = false;
        for (const auto &ws : m_wsSessions) {
            if (ws.url == rule.wsUrl) {
                found = true;
                break;
            }
        }
        if (!found) {
            WsSession session;
            session.url = rule.wsUrl;
            session.socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
            session.socket->open(QUrl(rule.wsUrl));
            m_wsSessions.append(session);
            LOG_DEBUG("OutputFilter")
                << "WebSocket connecting to" << rule.wsUrl;
        }
    }

    m_rules.append(rule);
    return true;
}

void TerminalOutputFilter::processOutput(const QByteArray &data)
{
    QMutexLocker locker(&m_mutex);

    if (m_rules.isEmpty()) {
        return;
    }

    m_lineBuffer.append(data);

    while (true) {
        int idx = m_lineBuffer.indexOf('\n');
        if (idx < 0) {
            if (m_lineBuffer.size() > 65536) {
                m_lineBuffer = m_lineBuffer.right(32768);
            }
            break;
        }

        QByteArray lineBytes = m_lineBuffer.left(idx);
        m_lineBuffer = m_lineBuffer.mid(idx + 1);

        if (lineBytes.endsWith('\r')) {
            lineBytes.chop(1);
        }

        if (lineBytes.isEmpty()) {
            continue;
        }

        QString line = QString::fromUtf8(lineBytes);
        emit lineProcessed(line);

        for (const FilterRule &rule : m_rules) {
            if (!rule.enabled) {
                continue;
            }

            QRegularExpressionMatch match = rule.regex.match(line);
            if (!match.hasMatch()) {
                continue;
            }

            QString matchedText = match.captured(0);
            emit ruleMatched(rule.name, matchedText);

            if (rule.action == FilterAction::WriteFile || rule.action == FilterAction::Both) {
                writeToFile(rule.filePath, matchedText);
            }
            if (rule.action == FilterAction::WebSocket || rule.action == FilterAction::Both) {
                sendToWebSocket(rule.wsUrl, matchedText);
            }
        }
    }
}

void TerminalOutputFilter::writeToFile(const QString &path, const QString &text)
{
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
               << " " << text << "\n";
    }
}

void TerminalOutputFilter::sendToWebSocket(const QString &url, const QString &text)
{
    QJsonObject msg;
    msg["type"] = "filter_match";
    msg["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    msg["text"] = text;

    QJsonDocument doc(msg);

    QMutexLocker locker(&m_mutex);
    for (auto &ws : m_wsSessions) {
        if (ws.url == url && ws.socket
            && ws.socket->state() == QAbstractSocket::ConnectedState) {
            ws.socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
        }
    }
}

int TerminalOutputFilter::ruleCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_rules.size();
}
