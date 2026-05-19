#ifndef PTY_MANAGER_H
#define PTY_MANAGER_H

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef Q_OS_MACOS
#include <pty.h>
#endif

#endif

struct PtyConfig {
    QString shell;
    QStringList args;
    QString workingDirectory;
    QStringList environment;
    int rows = 24;
    int cols = 80;
};

class PtyManager : public QObject {
    Q_OBJECT
public:
    explicit PtyManager(QObject* parent = nullptr);
    ~PtyManager() override;
    
    bool start(const PtyConfig& config);
    void stop();
    
    void write(const QByteArray& data);
    void resize(int rows, int cols);
    void sendSignal(int signal);
    
    bool isRunning() const;
    int processId() const { return m_pid; }
    QString workingDirectory() const { return m_workingDirectory; }
    
    void setPtySize(int rows, int cols);
    
signals:
    void dataReceived(const QByteArray& data);
    void processStarted();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void errorOccurred(const QString& error);
    
private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void checkProcessStatus();
    
private:
    void setupUnixPty(const PtyConfig& config);
    void setupWindowsConsole(const PtyConfig& config);
    void readFromPty();
    
    QProcess* m_process;
    QTimer* m_checkTimer;
    int m_masterFd;
    int m_pid;
    QString m_workingDirectory;
    bool m_isRunning;
    
    QByteArray m_readBuffer;
};

#endif
