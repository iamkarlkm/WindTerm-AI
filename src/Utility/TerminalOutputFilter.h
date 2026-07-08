#ifndef WINDTERM_OUTPUT_FILTER_H
#define WINDTERM_OUTPUT_FILTER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QRegularExpression>
#include <QMutexLocker>
#include <QFile>
#include <QTextStream>
#include <QByteArray>

class QWebSocket;

enum class FilterAction {
    WriteFile = 0,
    WebSocket = 1,
    Both = 2
};

struct FilterRule {
    QString name;
    QRegularExpression regex;
    FilterAction action;
    QString filePath;
    QString wsUrl;
    bool enabled;

    FilterRule()
        : action(FilterAction::WriteFile)
        , enabled(true)
    {
    }
};

class TerminalOutputFilter : public QObject {
    Q_OBJECT
public:
    static TerminalOutputFilter &instance();

    void init(const QString &configPath = QString());

    bool loadConfig(const QString &configPath);
    void reloadConfig();

    void processOutput(const QByteArray &data);

    int ruleCount() const;

signals:
    void ruleMatched(const QString &ruleName, const QString &matchedText);
    void lineProcessed(const QString &line);
    void configReloaded(int ruleCount);

private:
    explicit TerminalOutputFilter(QObject *parent = nullptr);
    ~TerminalOutputFilter() override;

    bool parseRule(const QJsonObject &obj, int index);
    void writeToFile(const QString &path, const QString &text);
    void sendToWebSocket(const QString &url, const QString &text);

    QVector<FilterRule> m_rules;
    mutable QMutex m_mutex;
    QString m_configPath;
    QByteArray m_lineBuffer;
    bool m_initialized;

    struct WsSession {
        QWebSocket *socket;
        QString url;
    };
    QVector<WsSession> m_wsSessions;
};

#endif // WINDTERM_OUTPUT_FILTER_H
