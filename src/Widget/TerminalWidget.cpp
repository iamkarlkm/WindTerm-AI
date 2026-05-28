#include "TerminalWidget.h"
#include "TerminalPane.h"
#include "SplitterContainer.h"
#include "ConnectionDialog.h"
#include "ThemeDialog.h"
#include "CommandSearchDialog.h"
#include "BookmarksDialog.h"
#include "ImportExportDialog.h"
#include "PluginManagerDialog.h"
#include "AiAssistantDialog.h"
#include "FileTransferDialog.h"
#include "RecordingDialog.h"
#include "TerminalSearchDialog.h"
#include "MemoryFragment/MemoryFragmentStore.h"
#include "Ssh/ConnectionManager.h"
#include "Ssh/SshChannelSession.h"
#include "CommandHistory/CommandHistoryStore.h"
#include "Bookmarks/BookmarksStore.h"
#include "AiIntegration/AiClient.h"
#include "AiIntegration/AiConfig.h"
#include "plugins/PluginManager.h"
#include "plugins/PluginContext.h"
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QDebug>
#include <QJsonArray>

TerminalWidget::TerminalWidget(QWidget* parent)
    : QWidget(parent), m_splitter(nullptr), m_activePane(nullptr), 
      m_memoryStore(nullptr), m_connectionManager(nullptr), 
      m_commandHistoryStore(nullptr), m_bookmarksStore(nullptr),
      m_pluginManager(nullptr), m_pluginContext(nullptr),
      m_aiClient(nullptr), m_aiDialog(nullptr), m_recordingDialog(nullptr), m_searchDialog(nullptr), m_paneCounter(0) {
    initWidget();
}

TerminalWidget::TerminalWidget(const TerminalConfig& config, QWidget* parent)
    : QWidget(parent), m_config(config), m_splitter(nullptr), m_activePane(nullptr), 
      m_memoryStore(nullptr), m_connectionManager(nullptr), 
      m_commandHistoryStore(nullptr), m_bookmarksStore(nullptr),
      m_pluginManager(nullptr), m_pluginContext(nullptr),
      m_aiClient(nullptr), m_aiDialog(nullptr), m_recordingDialog(nullptr), m_searchDialog(nullptr), m_paneCounter(0) {
    initWidget();
}

TerminalWidget::~TerminalWidget() {
    for (TerminalPane* pane : m_splitter->allPanes()) {
        pane->stop();
    }
}

void TerminalWidget::startShell(const QString& shell, const QString& workDir) {
    if (m_splitter->paneCount() == 0) {
        TerminalPane* pane = new TerminalPane(m_splitter);
        pane->setPaneId(++m_paneCounter);
        pane->setFontFamily(m_config.fontFamily);
        pane->setFontSize(m_config.fontSize);
        pane->setColors(m_config.backgroundColor, m_config.foregroundColor);
        m_splitter->addPane(pane);
        
        connect(pane, &TerminalPane::titleChanged, this, [this, pane](const QString& title) {
            if (pane == m_activePane) {
                emit titleChanged(title);
            }
        });
    }
    
    for (TerminalPane* pane : m_splitter->allPanes()) {
        pane->startShell(shell, workDir);
    }
}

void TerminalWidget::write(const QString& text) {
    write(text.toUtf8());
}

void TerminalWidget::write(const QByteArray& data) {
    if (m_activePane) {
        m_activePane->write(data);
    }
}

void TerminalWidget::clear() {
    for (TerminalPane* pane : m_splitter->allPanes()) {
        pane->session()->clearBuffer();
    }
}

void TerminalWidget::setConfig(const TerminalConfig& config) {
    m_config = config;
    for (TerminalPane* pane : m_splitter->allPanes()) {
        pane->setFontFamily(config.fontFamily);
        pane->setFontSize(config.fontSize);
        pane->setColors(config.backgroundColor, config.foregroundColor);
    }
}

TerminalPane* TerminalWidget::activePane() const {
    return m_activePane;
}

QList<TerminalPane*> TerminalWidget::allPanes() const {
    return m_splitter->allPanes();
}

void TerminalWidget::copyToClipboard() {
    if (m_activePane) {
        m_activePane->session()->copyToClipboard();
    }
}

void TerminalWidget::pasteFromClipboard() {
    if (m_activePane) {
        m_activePane->session()->pasteFromClipboard();
    }
}

void TerminalWidget::selectLine(int row) {
    // Selection is handled at pane level
}

int TerminalWidget::scrollbackSize() const {
    if (m_activePane) {
        return m_activePane->session()->scrollbackSize();
    }
    return 0;
}

int TerminalWidget::paneCount() const {
    return m_splitter->paneCount();
}

bool TerminalWidget::splitPane(Qt::Orientation orientation) {
    if (m_activePane) {
        return m_splitter->splitPane(m_activePane, orientation);
    }
    return false;
}

bool TerminalWidget::closePane(TerminalPane* pane) {
    return m_splitter->closePane(pane);
}

void TerminalWidget::onSplitRequested(Qt::Orientation orientation) {
    splitPane(orientation);
}

void TerminalWidget::onCloseRequested() {
    if (m_activePane) {
        closePane(m_activePane);
        if (m_splitter->paneCount() == 0) {
            emit allPanesClosed();
        }
    }
}

void TerminalWidget::onPaneFocusRequested() {
    TerminalPane* pane = qobject_cast<TerminalPane*>(sender());
    if (pane && pane != m_activePane) {
        m_activePane = pane;
        emit paneActivated(pane);
        emit titleChanged(pane->session()->title());
    }
}

void TerminalWidget::initWidget() {
    setFocusPolicy(Qt::StrongFocus);
    
    m_memoryStore = MemoryFragmentStore::instance(this);
    if (!m_memoryStore->isInitialized()) {
        m_memoryStore->initialize();
    }
    
    m_connectionManager = ConnectionManager::instance(this);
    if (!m_connectionManager->isInitialized()) {
        m_connectionManager->initialize();
    }
    
    m_commandHistoryStore = CommandHistoryStore::instance(this);
    if (!m_commandHistoryStore->isInitialized()) {
        m_commandHistoryStore->initialize();
    }
    
    m_bookmarksStore = BookmarksStore::instance(this);
    if (!m_bookmarksStore->isInitialized()) {
        m_bookmarksStore->initialize();
    }
    
    m_aiClient = new AiClient(this);
    QSettings settings("WindTerm", "Terminal");
    AiConfig config = AiConfig::load(&settings);
    m_aiClient->setConfig(config);
    
    m_pluginContext = new PluginContext(this);
    m_pluginContext->setApplicationVersion("0.2.0");
    
    m_pluginManager = new PluginManager(this);
    m_pluginManager->setPluginContext(m_pluginContext);
    
    QString pluginPath = QCoreApplication::applicationDirPath() + "/plugins";
    m_pluginManager->setPluginDirectories({pluginPath});
    m_pluginManager->scanPlugins();
    
    auto plugins = m_pluginManager->getAllPlugins();
    for (const auto& info : plugins) {
        if (m_pluginManager->startPlugin(info.metadata.id)) {
            qDebug() << "[PluginManager] Started:" << info.metadata.name;
        }
    }
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    m_splitter = new SplitterContainer(this);
    layout->addWidget(m_splitter);
    
    setupConnections();
}

void TerminalWidget::setupConnections() {
    connect(m_splitter, &SplitterContainer::paneActivated, this, [this](TerminalPane* pane) {
        m_activePane = pane;
        emit paneActivated(pane);
        emit titleChanged(pane->session()->title());
    });
    
    connect(m_splitter, &SplitterContainer::lastPaneClosed, this, [this]() {
        emit allPanesClosed();
    });
}

void TerminalWidget::showConnectionDialog() {
    ConnectionDialog dialog(m_connectionManager, this);
    connect(&dialog, &ConnectionDialog::connectRequested,
            this, &TerminalWidget::connectToSsh);
    dialog.exec();
}

void TerminalWidget::connectToSsh(const ConnectionProfile& profile) {
    if (m_splitter->paneCount() == 0) {
        TerminalPane* pane = new TerminalPane(m_splitter);
        pane->setPaneId(++m_paneCounter);
        pane->setFontFamily(m_config.fontFamily);
        pane->setFontSize(m_config.fontSize);
        pane->setColors(m_config.backgroundColor, m_config.foregroundColor);
        m_splitter->addPane(pane);
        
        connect(pane, &TerminalPane::titleChanged, this, [this, pane](const QString& title) {
            if (pane == m_activePane) {
                emit titleChanged(title);
            }
        });
    }
    
    TerminalPane* pane = m_activePane;
    if (!pane) {
        pane = m_splitter->allPanes().first();
    }
    
    SshChannelSession* sshSession = m_connectionManager->createSession();
    
    connect(sshSession, &SshChannelSession::connected, this, [this, pane, profile]() {
        QString title = QStringLiteral("SSH: %1@%2:%3").arg(profile.username).arg(profile.host).arg(profile.port);
        pane->session()->setTitle(title);
        emit titleChanged(title);
    });
    
    connect(sshSession, &SshChannelSession::dataReceived, this, [this, pane](const QByteArray& data) {
        pane->session()->write(data);
        sendToRecorder(data);
    });
    
    connect(pane, &TerminalPane::dataAvailable, sshSession, [sshSession](const QByteArray& data) {
        sshSession->write(data);
    });
    
    connect(sshSession, &SshChannelSession::disconnected, this, [this, pane]() {
        pane->session()->write(QStringLiteral("\r\n[SSH 连接已断开]\r\n").toUtf8());
    });
    
    connect(sshSession, &SshChannelSession::error, this, [this](const QString& error) {
        qWarning() << "[SSH Error]" << error;
    });
    
    if (sshSession->connectToServer(profile.toSshConfig())) {
        QString title = QStringLiteral("SSH: %1@%2:%3").arg(profile.username).arg(profile.host).arg(profile.port);
        pane->session()->setTitle(title);
        emit titleChanged(title);
    }
}

void TerminalWidget::showThemeDialog() {
    ThemeDialog dialog(this);
    connect(&dialog, &ThemeDialog::themeSelected, this, &TerminalWidget::setTheme);
    dialog.exec();
}

void TerminalWidget::setTheme(const ThemeConfig& theme) {
    m_config.fontFamily = theme.fontFamily;
    m_config.fontSize = theme.fontSize;
    m_config.backgroundColor = theme.background;
    m_config.foregroundColor = theme.foreground;
    m_config.cursorColor = theme.cursor;
    
    for (TerminalPane* pane : m_splitter->allPanes()) {
        pane->setTheme(theme);
    }
}

QJsonObject TerminalWidget::saveSessionState() const {
    QJsonObject state;
    state["tabName"] = m_tabName;
    state["paneCount"] = m_splitter->paneCount();
    state["fontFamily"] = m_config.fontFamily;
    state["fontSize"] = m_config.fontSize;
    
    QJsonArray panes;
    for (TerminalPane* pane : m_splitter->allPanes()) {
        QJsonObject paneObj;
        paneObj["id"] = pane->paneId();
        paneObj["rows"] = pane->session()->rows();
        paneObj["cols"] = pane->session()->cols();
        paneObj["title"] = pane->session()->title();
        
        TerminalPane* active = const_cast<TerminalWidget*>(this)->activePane();
        paneObj["isActive"] = (pane == active);
        
        panes.append(paneObj);
    }
    state["panes"] = panes;
    
    return state;
}

void TerminalWidget::restoreSessionState(const QJsonObject& state) {
    m_tabName = state["tabName"].toString();
    
    QJsonArray panes = state["panes"].toArray();
    for (int i = 0; i < panes.size(); ++i) {
        QJsonObject paneObj = panes[i].toObject();
        
        TerminalPane* pane = new TerminalPane(m_splitter);
        pane->setPaneId(paneObj["id"].toInt());
        pane->setFontFamily(state["fontFamily"].toString());
        pane->setFontSize(state["fontSize"].toInt());
        pane->setColors(m_config.backgroundColor, m_config.foregroundColor);
        
        int rows = paneObj["rows"].toInt();
        int cols = paneObj["cols"].toInt();
        if (rows > 0 && cols > 0) {
            pane->resizeTerminal(rows, cols);
        }
        
        m_splitter->addPane(pane);
        
        connect(pane, &TerminalPane::titleChanged, this, [this, pane](const QString& title) {
            if (pane == m_activePane) {
                emit titleChanged(title);
            }
        });
        
        if (paneObj["isActive"].toBool()) {
            m_activePane = pane;
        }
    }
}

void TerminalWidget::setTabName(const QString& name) {
    m_tabName = name;
}

void TerminalWidget::showCommandSearchDialog() {
    CommandSearchDialog dialog(m_commandHistoryStore, this);
    connect(&dialog, &CommandSearchDialog::commandSelected, this, [this](const QString& command) {
        if (m_activePane) {
            m_activePane->write(command.toUtf8());
        }
    });
    dialog.exec();
}

void TerminalWidget::showBookmarksDialog() {
    BookmarksDialog dialog(m_bookmarksStore, this);
    connect(&dialog, &BookmarksDialog::bookmarkSelected, this, [this](const QString& path) {
        if (m_activePane) {
            QString cdCommand = QStringLiteral("cd \"%1\"\r\n").arg(path);
            m_activePane->write(cdCommand.toUtf8());
        }
    });
    dialog.exec();
}

void TerminalWidget::showImportExportDialog() {
    ImportExportDialog dialog(this);
    dialog.exec();
}

void TerminalWidget::showPluginManagerDialog() {
    PluginManagerDialog dialog(m_pluginManager, this);
    dialog.exec();
}

void TerminalWidget::showAiAssistantDialog() {
    if (!m_aiDialog) {
        m_aiDialog = new AiAssistantDialog(m_aiClient, this);
        if (m_activePane) {
            m_aiDialog->setContext(
                m_activePane->session()->ptyManager()->workingDirectory(),
                QStringList()
            );
        }
    }
    m_aiDialog->show();
    m_aiDialog->raise();
    m_aiDialog->activateWindow();
}

void TerminalWidget::sendToAi(const QString& prompt) {
    showAiAssistantDialog();
    if (m_aiDialog && !prompt.isEmpty()) {
        if (m_activePane) {
            m_aiDialog->setContext(
                m_activePane->session()->ptyManager()->workingDirectory(),
                QStringList()
            );
        }
    }
}

void TerminalWidget::showFileTransferDialog() {
    FileTransferDialog dialog(m_connectionManager, this);
    dialog.exec();
}

void TerminalWidget::showRecordingDialog() {
    if (!m_recordingDialog) {
        m_recordingDialog = new RecordingDialog(this);
        connect(m_recordingDialog, &RecordingDialog::playbackOutput, this, [this](const QByteArray& data) {
            if (m_activePane) {
                m_activePane->write(data);
            }
        });
    }
    m_recordingDialog->show();
    m_recordingDialog->raise();
    m_recordingDialog->activateWindow();
}

void TerminalWidget::sendToRecorder(const QByteArray& data) {
    if (m_recordingDialog && m_recordingDialog->recorder()->isRecording()) {
        m_recordingDialog->recorder()->recordOutput(data);
    }
}

void TerminalWidget::showTerminalSearchDialog() {
    if (!m_searchDialog) {
        m_searchDialog = new TerminalSearchDialog(this);
        connect(m_searchDialog, &TerminalSearchDialog::searchRequested, this, [this](const QString& text, bool forward) {
            if (m_activePane) {
                m_activePane->searchInBuffer(text, forward);
            }
        });
        connect(m_searchDialog, &TerminalSearchDialog::searchClosed, this, [this]() {
            if (m_activePane) {
                m_activePane->clearSearchHighlight();
            }
        });
    }
    
    m_searchDialog->show();
    m_searchDialog->raise();
    m_searchDialog->activateWindow();
    m_searchDialog->move(geometry().center() - QPoint(m_searchDialog->width() / 2, 40));
    m_searchDialog->focusSearchEdit();
}
