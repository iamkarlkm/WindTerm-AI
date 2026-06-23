#ifndef WINDTERM_LOGGER_H
#define WINDTERM_LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4,
    Fatal = 5,
    Off = 6
};

class LogStream {
public:
    LogStream(LogLevel level, const QString &component);
    ~LogStream();

    LogStream(const LogStream &) = delete;
    LogStream &operator=(const LogStream &) = delete;

    template<typename T>
    LogStream &operator<<(const T &value) {
        m_stream << value;
        return *this;
    }

private:
    LogLevel m_level;
    QString m_component;
    QString m_buffer;
    QTextStream m_stream;
};

class Logger {
public:
    static Logger &instance();

    void init(const QString &logFilePath = QString(),
              LogLevel minLevel = LogLevel::Debug,
              bool alsoStderr = true);

    void setLogFile(const QString &path);
    void setMinLevel(LogLevel level);
    void setStderrEnabled(bool enabled);
    void flush();

    LogLevel minLevel() const { return m_minLevel; }
    QString logFilePath() const { return m_logFilePath; }

    LogStream stream(LogLevel level, const QString &component);

    void qtMessageHandler(QtMsgType type,
                          const QMessageLogContext &context,
                          const QString &msg);

    void write(LogLevel level, const QString &component,
               const QString &message);

private:
    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    static const char *levelToString(LogLevel level);
    static LogLevel qtMsgTypeToLevel(QtMsgType type);

    QFile m_logFile;
    QTextStream m_fileStream;
    QMutex m_mutex;
    LogLevel m_minLevel;
    bool m_stderrEnabled;
    QString m_logFilePath;
    bool m_initialized;
};

#define LOG_DEBUG(comp)    Logger::instance().stream(LogLevel::Debug, comp)
#define LOG_INFO(comp)     Logger::instance().stream(LogLevel::Info, comp)
#define LOG_WARN(comp)     Logger::instance().stream(LogLevel::Warning, comp)
#define LOG_ERROR(comp)    Logger::instance().stream(LogLevel::Error, comp)
#define LOG_CRITICAL(comp) Logger::instance().stream(LogLevel::Critical, comp)
#define LOG_FATAL(comp)    Logger::instance().stream(LogLevel::Fatal, comp)

#endif // WINDTERM_LOGGER_H
