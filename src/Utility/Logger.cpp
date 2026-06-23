#include "Utility/Logger.h"
#include <QCoreApplication>
#include <iostream>

static void globalQtMessageHandler(QtMsgType type,
                                   const QMessageLogContext &context,
                                   const QString &msg)
{
    Logger::instance().qtMessageHandler(type, context, msg);
}

LogStream::LogStream(LogLevel level, const QString &component)
    : m_level(level)
    , m_component(component)
    , m_stream(&m_buffer)
{
}

LogStream::~LogStream()
{
    m_stream.flush();
    Logger::instance().write(m_level, m_component, m_buffer);
}

Logger &Logger::instance()
{
    static Logger s_instance;
    return s_instance;
}

Logger::Logger()
    : m_minLevel(LogLevel::Debug)
    , m_stderrEnabled(true)
    , m_initialized(false)
{
}

Logger::~Logger()
{
    if (m_fileStream.device()) {
        m_fileStream.flush();
    }
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::init(const QString &logFilePath,
                  LogLevel minLevel,
                  bool alsoStderr)
{
    {
        QMutexLocker locker(&m_mutex);
        m_minLevel = minLevel;
        m_stderrEnabled = alsoStderr;
    }

    if (!m_initialized) {
        qInstallMessageHandler(globalQtMessageHandler);
        m_initialized = true;
    }

    if (!logFilePath.isEmpty()) {
        setLogFile(logFilePath);
    }
}

void Logger::setLogFile(const QString &path)
{
    QMutexLocker locker(&m_mutex);

    if (m_logFile.isOpen()) {
        m_fileStream.flush();
        m_logFile.close();
    }

    m_logFilePath = path;
    m_logFile.setFileName(path);

    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_fileStream.setDevice(&m_logFile);
    }
}

void Logger::setMinLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_minLevel = level;
}

void Logger::setStderrEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_stderrEnabled = enabled;
}

void Logger::flush()
{
    QMutexLocker locker(&m_mutex);
    if (m_fileStream.device()) {
        m_fileStream.flush();
    }
}

LogStream Logger::stream(LogLevel level, const QString &component)
{
    return LogStream(level, component);
}

void Logger::write(LogLevel level, const QString &component,
                   const QString &message)
{
    if (level < m_minLevel || level >= LogLevel::Off) {
        return;
    }

    QString timestamp = QDateTime::currentDateTime()
                            .toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = QString::fromUtf8(levelToString(level));
    QString line = QString("[%1] [%2] [%3] %4\n")
                       .arg(timestamp, levelStr, component, message);

    QMutexLocker locker(&m_mutex);

    if (m_stderrEnabled) {
        std::cerr << line.toUtf8().constData() << std::flush;
    }

    if (m_fileStream.device()) {
        m_fileStream << line;
        m_fileStream.flush();
    }
}

void Logger::qtMessageHandler(QtMsgType type,
                              const QMessageLogContext &context,
                              const QString &msg)
{
    LogLevel level = qtMsgTypeToLevel(type);

    QString component;
    if (context.category && context.category[0] != '\0'
        && strcmp(context.category, "default") != 0) {
        component = QString::fromUtf8(context.category);
    } else if (context.file) {
        const char *base = strrchr(context.file, '/');
        if (!base) base = strrchr(context.file, '\\');
        component = QString::fromUtf8(base ? base + 1 : context.file);
        int dot = component.lastIndexOf('.');
        if (dot > 0) component = component.left(dot);
    } else {
        component = "Qt";
    }

    write(level, component, msg);
}

const char *Logger::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:    return "DEBUG";
    case LogLevel::Info:     return "INFO ";
    case LogLevel::Warning:  return "WARN ";
    case LogLevel::Error:    return "ERROR";
    case LogLevel::Critical: return "FATAL";
    case LogLevel::Fatal:    return "FATAL";
    case LogLevel::Off:      return "OFF  ";
    }
    return "?????";
}

LogLevel Logger::qtMsgTypeToLevel(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return LogLevel::Debug;
    case QtInfoMsg:     return LogLevel::Info;
    case QtWarningMsg:  return LogLevel::Warning;
    case QtCriticalMsg: return LogLevel::Error;
    case QtFatalMsg:    return LogLevel::Fatal;
    }
    return LogLevel::Debug;
}
