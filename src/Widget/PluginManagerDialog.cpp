#include "PluginManagerDialog.h"
#include <QMessageBox>

PluginManagerDialog::PluginManagerDialog(PluginManager* manager, QWidget* parent)
    : QDialog(parent), m_manager(manager) {
    setupUI();
    updatePluginList();
    
    connect(m_manager, &PluginManager::pluginStarted, this, [this]() { updatePluginList(); });
    connect(m_manager, &PluginManager::pluginStopped, this, [this]() { updatePluginList(); });
}

void PluginManagerDialog::setupUI() {
    setWindowTitle("Plugin Manager");
    resize(600, 500);
    
    auto* mainLayout = new QVBoxLayout(this);
    
    m_pluginList = new QListWidget(this);
    m_pluginList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_pluginList, &QListWidget::currentRowChanged, this, &PluginManagerDialog::onPluginSelected);
    
    auto* infoGroup = new QGroupBox("Plugin Info", this);
    auto* infoLayout = new QVBoxLayout(infoGroup);
    m_infoLabel = new QLabel("Select a plugin to view details", infoGroup);
    m_infoLabel->setWordWrap(true);
    infoLayout->addWidget(m_infoLabel);
    
    auto* buttonLayout = new QHBoxLayout();
    m_loadButton = new QPushButton("Load", this);
    m_startButton = new QPushButton("Start", this);
    m_stopButton = new QPushButton("Stop", this);
    m_unloadButton = new QPushButton("Unload", this);
    m_refreshButton = new QPushButton("Refresh", this);
    m_closeButton = new QPushButton("Close", this);
    
    buttonLayout->addWidget(m_loadButton);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_unloadButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_closeButton);
    
    connect(m_loadButton, &QPushButton::clicked, this, &PluginManagerDialog::onLoadPlugin);
    connect(m_startButton, &QPushButton::clicked, this, &PluginManagerDialog::onStartPlugin);
    connect(m_stopButton, &QPushButton::clicked, this, &PluginManagerDialog::onStopPlugin);
    connect(m_unloadButton, &QPushButton::clicked, this, &PluginManagerDialog::onUnloadPlugin);
    connect(m_refreshButton, &QPushButton::clicked, this, &PluginManagerDialog::onRefresh);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    mainLayout->addWidget(m_pluginList);
    mainLayout->addWidget(infoGroup);
    mainLayout->addLayout(buttonLayout);
    
    updateButtons();
}

void PluginManagerDialog::updatePluginList() {
    m_pluginList->clear();
    
    auto plugins = m_manager->getAllPlugins();
    for (const auto& info : plugins) {
        QString text = QString("%1 (%2) - %3")
            .arg(info.metadata.name)
            .arg(info.metadata.version)
            .arg(stateToString(info.state));
        m_pluginList->addItem(text);
    }
    
    if (plugins.isEmpty()) {
        m_pluginList->addItem("No plugins found. Place .so files in the plugins directory.");
    }
}

void PluginManagerDialog::updateButtons() {
    int row = m_pluginList->currentRow();
    bool hasSelection = row >= 0 && row < m_pluginList->count();
    
    m_loadButton->setEnabled(hasSelection);
    m_startButton->setEnabled(hasSelection);
    m_stopButton->setEnabled(hasSelection);
    m_unloadButton->setEnabled(hasSelection);
}

void PluginManagerDialog::onPluginSelected() {
    updateButtons();
    
    int row = m_pluginList->currentRow();
    auto plugins = m_manager->getAllPlugins();
    
    if (row >= 0 && row < plugins.size()) {
        const auto& info = plugins[row];
        QString details = QString(
            "<b>Name:</b> %1<br>"
            "<b>Version:</b> %2<br>"
            "<b>Author:</b> %3<br>"
            "<b>Type:</b> %4<br>"
            "<b>State:</b> %5<br>"
            "<b>Description:</b> %6<br>"
            "<b>Path:</b> %7"
        )
        .arg(info.metadata.name)
        .arg(info.metadata.version)
        .arg(info.metadata.author.isEmpty() ? "Unknown" : info.metadata.author)
        .arg(static_cast<int>(info.metadata.type))
        .arg(stateToString(info.state))
        .arg(info.metadata.description.isEmpty() ? "N/A" : info.metadata.description)
        .arg(info.filePath);
        
        if (!info.error.isEmpty()) {
            details += QString("<br><b>Error:</b> <font color='red'>%1</font>").arg(info.error);
        }
        
        m_infoLabel->setText(details);
    } else {
        m_infoLabel->setText("Select a plugin to view details");
    }
}

void PluginManagerDialog::onLoadPlugin() {
    int row = m_pluginList->currentRow();
    auto plugins = m_manager->getAllPlugins();
    
    if (row >= 0 && row < plugins.size()) {
        if (m_manager->loadPlugin(plugins[row].metadata.id)) {
            QMessageBox::information(this, "Success", "Plugin loaded successfully");
            updatePluginList();
        } else {
            QMessageBox::critical(this, "Error", "Failed to load plugin");
        }
    }
}

void PluginManagerDialog::onStartPlugin() {
    int row = m_pluginList->currentRow();
    auto plugins = m_manager->getAllPlugins();
    
    if (row >= 0 && row < plugins.size()) {
        if (m_manager->startPlugin(plugins[row].metadata.id)) {
            QMessageBox::information(this, "Success", "Plugin started successfully");
            updatePluginList();
        } else {
            QMessageBox::critical(this, "Error", "Failed to start plugin");
        }
    }
}

void PluginManagerDialog::onStopPlugin() {
    int row = m_pluginList->currentRow();
    auto plugins = m_manager->getAllPlugins();
    
    if (row >= 0 && row < plugins.size()) {
        if (m_manager->stopPlugin(plugins[row].metadata.id)) {
            QMessageBox::information(this, "Success", "Plugin stopped");
            updatePluginList();
        }
    }
}

void PluginManagerDialog::onUnloadPlugin() {
    int row = m_pluginList->currentRow();
    auto plugins = m_manager->getAllPlugins();
    
    if (row >= 0 && row < plugins.size()) {
        if (m_manager->unloadPlugin(plugins[row].metadata.id)) {
            QMessageBox::information(this, "Success", "Plugin unloaded");
            updatePluginList();
        }
    }
}

void PluginManagerDialog::onRefresh() {
    m_manager->scanPlugins();
    updatePluginList();
}

QString PluginManagerDialog::stateToString(PluginState state) {
    switch (state) {
        case PluginState::Unloaded: return "Unloaded";
        case PluginState::Loading: return "Loading";
        case PluginState::Loaded: return "Loaded";
        case PluginState::Initialized: return "Initialized";
        case PluginState::Running: return "Running";
        case PluginState::Stopped: return "Stopped";
        case PluginState::Error: return "Error";
        default: return "Unknown";
    }
}
