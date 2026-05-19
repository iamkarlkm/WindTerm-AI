#include "PtyManager.h"
#include <QDebug>

#ifdef Q_OS_UNIX
#include <QSocketNotifier>

static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

#endif

PtyManager::PtyManager(QObject* parent)
    : QObject(parent), m_process(nullptr), m_checkTimer(nullptr),
      m_masterFd(-1), m_pid(-1), m_isRunning(false) {
    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(1000);
    connect(m_checkTimer, &QTimer::timeout, this, &PtyManager::checkProcessStatus);
    m_checkTimer->start();
}

PtyManager::~PtyManager() {
    stop();
}

bool PtyManager::start(const PtyConfig& config) {
    if (m_isRunning) {
        stop();
    }
    
    m_workingDirectory = config.workingDirectory;
    
#ifdef Q_OS_UNIX
    setupUnixPty(config);
#else
    setupWindowsConsole(config);
#endif
    
    if (m_isRunning) {
        emit processStarted();
    }
    
    return m_isRunning;
}

void PtyManager::stop() {
    if (!m_isRunning) return;
    
    if (m_pid > 0) {
#ifdef Q_OS_UNIX
        kill(m_pid, SIGHUP);
        waitpid(m_pid, nullptr, 0);
#endif
        m_pid = -1;
    }
    
    if (m_process) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
        delete m_process;
        m_process = nullptr;
    }
    
    if (m_masterFd >= 0) {
#ifdef Q_OS_UNIX
        close(m_masterFd);
#endif
        m_masterFd = -1;
    }
    
    m_isRunning = false;
}

void PtyManager::write(const QByteArray& data) {
    if (!m_isRunning) return;
    
#ifdef Q_OS_UNIX
    if (m_masterFd >= 0) {
        ::write(m_masterFd, data.constData(), data.size());
    }
#endif
    
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->write(data);
    }
}

void PtyManager::resize(int rows, int cols) {
    if (!m_isRunning) return;
    
#ifdef Q_OS_UNIX
    struct winsize ws;
    ws.ws_row = rows;
    ws.ws_col = cols;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;
    
    if (m_masterFd >= 0) {
        ioctl(m_masterFd, TIOCSWINSZ, &ws);
    }
    
    if (m_pid > 0) {
        kill(m_pid, SIGWINCH);
    }
#endif
    
    if (m_process) {
        m_process->setProperty("terminalRows", rows);
        m_process->setProperty("terminalCols", cols);
    }
}

void PtyManager::sendSignal(int signal) {
    if (!m_isRunning || m_pid <= 0) return;
    
#ifdef Q_OS_UNIX
    kill(m_pid, signal);
#endif
}

bool PtyManager::isRunning() const {
    return m_isRunning;
}

void PtyManager::setPtySize(int rows, int cols) {
    resize(rows, cols);
}

void PtyManager::setupUnixPty(const PtyConfig& config) {
#ifdef Q_OS_UNIX
    struct termios termios;
    char slaveName[1024];
    
    m_masterFd = posix_openpt(O_RDWR | O_NOCTTY);
    if (m_masterFd < 0) {
        emit errorOccurred(QStringLiteral("Failed to open PTY: ") + QString::fromLocal8Bit(strerror(errno)));
        return;
    }
    
    if (grantpt(m_masterFd) < 0 || unlockpt(m_masterFd) < 0) {
        emit errorOccurred(QStringLiteral("Failed to setup PTY: ") + QString::fromLocal8Bit(strerror(errno)));
        close(m_masterFd);
        m_masterFd = -1;
        return;
    }
    
    char* name = ptsname(m_masterFd);
    if (!name) {
        emit errorOccurred(QStringLiteral("Failed to get PTY name: ") + QString::fromLocal8Bit(strerror(errno)));
        close(m_masterFd);
        m_masterFd = -1;
        return;
    }
    
    strncpy(slaveName, name, sizeof(slaveName) - 1);
    slaveName[sizeof(slaveName) - 1] = '\0';
    
    m_pid = fork();
    if (m_pid < 0) {
        emit errorOccurred(QStringLiteral("Failed to fork: ") + QString::fromLocal8Bit(strerror(errno)));
        close(m_masterFd);
        m_masterFd = -1;
        return;
    }
    
    if (m_pid == 0) {
        setsid();
        
        int slaveFd = open(slaveName, O_RDWR);
        if (slaveFd < 0) {
            _exit(1);
        }
        
        dup2(slaveFd, STDIN_FILENO);
        dup2(slaveFd, STDOUT_FILENO);
        dup2(slaveFd, STDERR_FILENO);
        if (slaveFd > 2) close(slaveFd);
        
        if (!config.workingDirectory.isEmpty()) {
            chdir(config.workingDirectory.toUtf8().constData());
        }
        
        struct winsize ws;
        ws.ws_row = config.rows;
        ws.ws_col = config.cols;
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;
        ioctl(STDIN_FILENO, TIOCSWINSZ, &ws);
        
        char** argv = new char*[config.args.size() + 2];
        QString shell = config.shell.isEmpty() ? QStringLiteral("/bin/bash") : config.shell;
        argv[0] = strdup(shell.toUtf8().constData());
        for (int i = 0; i < config.args.size(); i++) {
            argv[i + 1] = strdup(config.args[i].toUtf8().constData());
        }
        argv[config.args.size() + 1] = nullptr;
        
        char** envp = nullptr;
        if (!config.environment.isEmpty()) {
            envp = new char*[config.environment.size() + 1];
            for (int i = 0; i < config.environment.size(); i++) {
                envp[i] = strdup(config.environment[i].toUtf8().constData());
            }
            envp[config.environment.size()] = nullptr;
        }
        
        if (envp) {
            execve(argv[0], argv, envp);
        } else {
            execvp(argv[0], argv);
        }
        
        _exit(127);
    }
    
    setNonBlocking(m_masterFd);
    m_isRunning = true;
    
    QSocketNotifier* notifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this]() {
        readFromPty();
    });
#endif
}

void PtyManager::setupWindowsConsole(const PtyConfig& config) {
    m_process = new QProcess(this);
    
    if (!config.workingDirectory.isEmpty()) {
        m_process->setWorkingDirectory(config.workingDirectory);
    }
    
    if (!config.environment.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (const QString& entry : config.environment) {
            int eqIndex = entry.indexOf('=');
            if (eqIndex > 0) {
                env.insert(entry.left(eqIndex), entry.mid(eqIndex + 1));
            }
        }
        m_process->setProcessEnvironment(env);
    }
    
    QString program = config.shell.isEmpty() ? QStringLiteral("cmd.exe") : config.shell;
    QStringList args = config.args.isEmpty() ? QStringList() : config.args;
    
    connect(m_process, &QProcess::readyReadStandardOutput, this, &PtyManager::onReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &PtyManager::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PtyManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &PtyManager::onProcessError);
    
    m_process->start(program, args);
    
    if (m_process->waitForStarted(5000)) {
        m_isRunning = true;
        m_pid = m_process->processId();
    } else {
        emit errorOccurred(QStringLiteral("Failed to start process: ") + m_process->errorString());
        delete m_process;
        m_process = nullptr;
    }
}

void PtyManager::readFromPty() {
#ifdef Q_OS_UNIX
    if (m_masterFd < 0) return;
    
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = ::read(m_masterFd, buffer, sizeof(buffer))) > 0) {
        QByteArray data(buffer, bytesRead);
        emit dataReceived(data);
    }
#endif
}

void PtyManager::onReadyRead() {
    if (m_process) {
        QByteArray data = m_process->readAllStandardOutput();
        data += m_process->readAllStandardError();
        if (!data.isEmpty()) {
            emit dataReceived(data);
        }
    }
}

void PtyManager::onProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error);
    emit errorOccurred(m_process ? m_process->errorString() : QStringLiteral("Unknown error"));
    m_isRunning = false;
}

void PtyManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_isRunning = false;
    emit processFinished(exitCode, exitStatus);
}

void PtyManager::checkProcessStatus() {
#ifdef Q_OS_UNIX
    if (m_pid > 0) {
        int status;
        pid_t result = waitpid(m_pid, &status, WNOHANG);
        if (result > 0) {
            m_isRunning = false;
            m_pid = -1;
            int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            emit processFinished(exitCode, QProcess::NormalExit);
        } else if (result < 0 && errno == ECHILD) {
            m_isRunning = false;
            m_pid = -1;
        }
    }
#endif
}
