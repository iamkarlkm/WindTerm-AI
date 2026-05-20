#include "MemoryFragment.h"
#include <QDir>
#include <QDateTime>
#include <cstdlib>

MemoryFragmentContext MemoryFragmentContext::current() {
    MemoryFragmentContext context;
    
    const char* shellEnv = getenv("SHELL");
    context.terminalType = shellEnv ? QString(shellEnv) : QStringLiteral("unknown");
    
    const char* pwdEnv = getenv("PWD");
    context.workingDirectory = pwdEnv ? QString(pwdEnv) : QDir::homePath();
    
    context.sessionId = QString::number(QDateTime::currentMSecsSinceEpoch());
    context.commandHistory = QString();
    
    return context;
}
