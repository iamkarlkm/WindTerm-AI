#include "TerminalInfoPlugin.h"
#include <QtDebug>
#include <QTime>

TerminalInfoPlugin::TerminalInfoPlugin(QObject* parent) : PluginInterface(parent) {}

TerminalInfoPlugin::~TerminalInfoPlugin() { shutdown(); }

PluginMetadata TerminalInfoPlugin::metadata() const {
    PluginMetadata meta;
    meta.id = "terminal-info";
    meta.name = "Terminal Info";
    meta.version = "1.0.0";
    meta.description = "Displays session information and command statistics";
    meta.author = "WindTerm-AI Team";
    meta.type = PluginType::UISupplement;
    return meta;
}

bool TerminalInfoPlugin::initialize(PluginContext* context) {
    m_context = context;
    setState(PluginState::Initialized);
    qDebug() << "[TerminalInfo] Plugin initialized";
    return true;
}

void TerminalInfoPlugin::shutdown() {
    m_context = nullptr;
    setState(PluginState::Stopped);
    qDebug() << "[TerminalInfo] Plugin shutdown";
}

void TerminalInfoPlugin::onSessionStarted(const QString& sessionId) {
    m_currentSession = sessionId;
    m_commandCount = 0;
    m_sessionStartTime = QTime::currentTime();
    
    QString msg = QString("[Session started: %1]").arg(sessionId);
    if (m_context) {
        m_context->sendNotification("Terminal Info", msg);
    }
}

void TerminalInfoPlugin::onCommandExecuted(const QString& command) {
    m_commandCount++;
    Q_UNUSED(command);
}

void TerminalInfoPlugin::onWorkingDirectoryChanged(const QString& path) {
    Q_UNUSED(path);
}
