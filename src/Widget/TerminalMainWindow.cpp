#include "TerminalMainWindow.h"
#include "TabWidget.h"
#include "TerminalWidget.h"
#include "Theme/ThemeManager.h"
#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QDebug>
#include <QScreen>
#include <QJsonDocument>
#include "plugins/PluginManager.h"
#include "plugins/PluginContext.h"

TerminalMainWindow::TerminalMainWindow(QWidget* parent)
    : QMainWindow(parent), m_activeTerminal(nullptr), m_tabWidget(nullptr),
      m_splitter(nullptr), m_statusLabel(nullptr), m_selectionLabel(nullptr),
      m_scrollLabel(nullptr), m_settings(nullptr), m_fullscreenAction(nullptr),
      m_pluginManager(nullptr), m_pluginContext(nullptr),
      m_zoomLevel(0), m_isFullscreen(false) {
    
    m_settings = new QSettings(QStringLiteral("WindTerm"), QStringLiteral("Terminal"), this);
    
    setWindowTitle(QStringLiteral("WindTerm Extensions - GPU Accelerated Terminal"));
    setMinimumSize(800, 600);
    
    m_tabWidget = new TabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    
    connect(m_tabWidget, &TabWidget::tabCloseRequested, this, [this](int index) {
        QWidget* widget = m_tabWidget->widget(index);
        m_tabWidget->removeTab(index);
        widget->deleteLater();
        if (m_tabWidget->count() == 0) {
            onNewTab();
        }
    });
    
    connect(m_tabWidget, &TabWidget::currentChanged, this, [this](int index) {
        if (index >= 0) {
            m_activeTerminal = qobject_cast<TerminalWidget*>(m_tabWidget->widget(index));
            if (m_activeTerminal) {
                m_activeTerminal->setFocus();
            }
        }
    });
    
    connect(m_tabWidget, &TabWidget::tabRenameRequested, this, &TerminalMainWindow::onTabRenameRequested);
    connect(m_tabWidget, &TabWidget::tabCloseOthersRequested, this, &TerminalMainWindow::onTabCloseOthersRequested);
    connect(m_tabWidget, &TabWidget::tabCloseAllRequested, this, &TerminalMainWindow::onTabCloseAllRequested);
    
    setCentralWidget(m_tabWidget);
    
    setupMenu();
    setupToolBar();
    setupStatusBar();
    
    if (!restoreSession()) {
        onNewTab();
    }
}

TerminalMainWindow::~TerminalMainWindow() {
    saveSettings();
}

void TerminalMainWindow::setupMenu() {
    QMenuBar* menuBar = this->menuBar();
    
    QMenu* fileMenu = menuBar->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&New Tab"), this, &TerminalMainWindow::onNewTab, QKeySequence::New);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("&Import/Export Settings"), this, &TerminalMainWindow::onImportExport);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("E&xit"), this, &QMainWindow::close, QKeySequence::Quit);
    
    QMenu* editMenu = menuBar->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(QStringLiteral("&Copy"), this, &TerminalMainWindow::onCopy, QKeySequence::Copy);
    editMenu->addAction(QStringLiteral("&Paste"), this, &TerminalMainWindow::onPaste, QKeySequence::Paste);
    editMenu->addAction(QStringLiteral("Select &All"), this, &TerminalMainWindow::onSelectAll, QKeySequence::SelectAll);
    editMenu->addSeparator();
    editMenu->addAction(QStringLiteral("&Find"), this, &TerminalMainWindow::onFind, QKeySequence::Find);
    editMenu->addSeparator();
    editMenu->addAction(QStringLiteral("&Settings"), this, &TerminalMainWindow::onSettings, QKeySequence::Preferences);
    
    QMenu* toolsMenu = menuBar->addMenu(QStringLiteral("&Tools"));
    toolsMenu->addAction(QStringLiteral("AI &Assistant"), this, &TerminalMainWindow::onAiAssistant, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
    toolsMenu->addAction(QStringLiteral("&File Transfer..."), this, &TerminalMainWindow::onFileTransfer, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    toolsMenu->addAction(QStringLiteral("&Recording..."), this, &TerminalMainWindow::onRecording, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    toolsMenu->addAction(QStringLiteral("&Plugins..."), this, &TerminalMainWindow::onPluginManager, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    
    QMenu* viewMenu = menuBar->addMenu(QStringLiteral("&View"));
    viewMenu->addAction(QStringLiteral("Zoom &In"), this, &TerminalMainWindow::onZoomIn, QKeySequence::ZoomIn);
    viewMenu->addAction(QStringLiteral("Zoom &Out"), this, &TerminalMainWindow::onZoomOut, QKeySequence::ZoomOut);
    viewMenu->addAction(QStringLiteral("&Reset Zoom"), this, &TerminalMainWindow::onResetZoom, QKeySequence(Qt::CTRL | Qt::Key_0));
    viewMenu->addSeparator();
    m_fullscreenAction = viewMenu->addAction(QStringLiteral("F&ullscreen"), this, &TerminalMainWindow::onFullscreen, QKeySequence::FullScreen);
    viewMenu->addSeparator();
    viewMenu->addAction(QStringLiteral("Split &Horizontal"), this, &TerminalMainWindow::onSplitHorizontal, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
    viewMenu->addAction(QStringLiteral("Split &Vertical"), this, &TerminalMainWindow::onSplitVertical, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    viewMenu->addSeparator();
    viewMenu->addAction(QStringLiteral("&Clear"), this, &TerminalMainWindow::onClear);
    
    QMenu* themeMenu = viewMenu->addMenu(QStringLiteral("&Theme"));
    themeMenu->addAction(QStringLiteral("&Default"), this, &TerminalMainWindow::onThemeChanged)->setData(QStringLiteral("Default"));
    themeMenu->addAction(QStringLiteral("&Dracula"), this, &TerminalMainWindow::onThemeChanged)->setData(QStringLiteral("Dracula"));
    themeMenu->addAction(QStringLiteral("&Monokai"), this, &TerminalMainWindow::onThemeChanged)->setData(QStringLiteral("Monokai"));
    themeMenu->addAction(QStringLiteral("S&olarized Dark"), this, &TerminalMainWindow::onThemeChanged)->setData(QStringLiteral("Solarized Dark"));
    themeMenu->addSeparator();
    themeMenu->addAction(QStringLiteral("&More Themes..."), this, &TerminalMainWindow::onOpenThemeDialog);
    
    QMenu* backendMenu = viewMenu->addMenu(QStringLiteral("Render &Backend"));
    backendMenu->addAction(QStringLiteral("Au&to"), this, &TerminalMainWindow::onBackendChanged)->setData(QStringLiteral("auto"));
    backendMenu->addAction(QStringLiteral("&OpenGL"), this, &TerminalMainWindow::onBackendChanged)->setData(QStringLiteral("opengl"));
    backendMenu->addAction(QStringLiteral("&Metal"), this, &TerminalMainWindow::onBackendChanged)->setData(QStringLiteral("metal"));
    
    QMenu* helpMenu = menuBar->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(QStringLiteral("&About"), this, [this]() {
        QMessageBox::about(this, QStringLiteral("About WindTerm Extensions"),
            QString(
                "<h3>WindTerm Extensions</h3>"
                "<p>Version: 0.2.0</p>"
                "<p>GPU Accelerated Terminal Emulator</p>"
                "<p>Features: SDF Fonts, Multi-Backend, Circular Buffer</p>"
                "<p>Copyright &copy; 2026 WindTerm Team</p>"));
    });
}

void TerminalMainWindow::setupToolBar() {
    QToolBar* toolbar = addToolBar(QStringLiteral("Main"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    
    toolbar->addAction(QStringLiteral("New"), this, &TerminalMainWindow::onNewTab);
    toolbar->addSeparator();
    toolbar->addAction(QStringLiteral("Copy"), this, &TerminalMainWindow::onCopy);
    toolbar->addAction(QStringLiteral("Paste"), this, &TerminalMainWindow::onPaste);
    toolbar->addSeparator();
    toolbar->addAction(QStringLiteral("Split H"), this, &TerminalMainWindow::onSplitHorizontal);
    toolbar->addAction(QStringLiteral("Split V"), this, &TerminalMainWindow::onSplitVertical);
}

void TerminalMainWindow::setupStatusBar() {
    QStatusBar* status = statusBar();
    
    m_statusLabel = new QLabel(QStringLiteral("Ready"));
    status->addWidget(m_statusLabel);
    
    m_selectionLabel = new QLabel(QStringLiteral(""));
    status->addPermanentWidget(m_selectionLabel);
    
    m_scrollLabel = new QLabel(QStringLiteral(""));
    status->addPermanentWidget(m_scrollLabel);
}

void TerminalMainWindow::loadSettings() {
    restoreGeometry(m_settings->value(QStringLiteral("geometry")).toByteArray());
    restoreState(m_settings->value(QStringLiteral("windowState")).toByteArray());
}

void TerminalMainWindow::saveSettings() {
    m_settings->setValue(QStringLiteral("geometry"), saveGeometry());
    m_settings->setValue(QStringLiteral("windowState"), saveState());
    m_settings->sync();
}

TerminalWidget* TerminalMainWindow::createTerminal() {
    TerminalWidget* terminal = new TerminalWidget(this);
    connect(terminal, &TerminalWidget::selectionChanged, 
            this, &TerminalMainWindow::onSelectionChanged);
    connect(terminal, &TerminalWidget::scrollPositionChanged,
            this, &TerminalMainWindow::onScrollPositionChanged);
    return terminal;
}

void TerminalMainWindow::onNewTab() {
    TerminalWidget* terminal = createTerminal();
    int index = m_tabWidget->addTab(terminal, QString(QStringLiteral("Tab %1")).arg(m_tabWidget->count() + 1));
    m_tabWidget->setCurrentIndex(index);
    m_activeTerminal = terminal;
    terminal->setFocus();
    terminal->write(QStringLiteral("Welcome to WindTerm GPU Accelerated Terminal\r\n"));
    terminal->write(QStringLiteral("Use Ctrl+Shift+H/V to split, Ctrl+Shift+C/V to copy/paste\r\n"));
    terminal->write(QStringLiteral("Scroll with mouse wheel or Page Up/Down\r\n\r\n"));
    terminal->write(QStringLiteral("$ "));
}

void TerminalMainWindow::onCloseTab() {
    if (m_tabWidget->count() > 0) {
        int index = m_tabWidget->currentIndex();
        m_tabWidget->removeTab(index);
    }
}

void TerminalMainWindow::onSplitHorizontal() {
    if (m_activeTerminal) {
        QMessageBox::information(this, QStringLiteral("Split"), QStringLiteral("Split panes coming soon - use new tabs for now"));
    }
}

void TerminalMainWindow::onSplitVertical() {
    if (m_activeTerminal) {
        QMessageBox::information(this, QStringLiteral("Split"), QStringLiteral("Split panes coming soon - use new tabs for now"));
    }
}

void TerminalMainWindow::onCopy() {
    if (m_activeTerminal) {
        m_activeTerminal->copyToClipboard();
    }
}

void TerminalMainWindow::onPaste() {
    if (m_activeTerminal) {
        m_activeTerminal->pasteFromClipboard();
    }
}

void TerminalMainWindow::onClear() {
    if (m_activeTerminal) {
        m_activeTerminal->clear();
    }
}

void TerminalMainWindow::onSelectAll() {
    if (m_activeTerminal) {
        m_activeTerminal->selectLine(m_activeTerminal->scrollbackSize());
    }
}

void TerminalMainWindow::onFind() {
    QString text = QInputDialog::getText(this, QStringLiteral("Find"), QStringLiteral("Search text:"));
    if (!text.isEmpty() && m_activeTerminal) {
        m_statusLabel->setText(QString(QStringLiteral("Searching for: %1")).arg(text));
    }
}

void TerminalMainWindow::onSettings() {
    QMessageBox::information(this, QStringLiteral("Settings"), QStringLiteral("Settings dialog coming soon"));
}

void TerminalMainWindow::onFullscreen() {
    m_isFullscreen = !m_isFullscreen;
    if (m_isFullscreen) {
        showFullScreen();
    } else {
        showNormal();
    }
}

void TerminalMainWindow::onZoomIn() {
    if (m_activeTerminal) {
        m_zoomLevel++;
        TerminalConfig config = m_activeTerminal->config();
        config.fontSize = qMin(48, config.fontSize + 1);
        m_activeTerminal->setConfig(config);
        m_statusLabel->setText(QString(QStringLiteral("Font size: %1")).arg(config.fontSize));
    }
}

void TerminalMainWindow::onZoomOut() {
    if (m_activeTerminal) {
        m_zoomLevel--;
        TerminalConfig config = m_activeTerminal->config();
        config.fontSize = qMax(8, config.fontSize - 1);
        m_activeTerminal->setConfig(config);
        m_statusLabel->setText(QString(QStringLiteral("Font size: %1")).arg(config.fontSize));
    }
}

void TerminalMainWindow::onResetZoom() {
    if (m_activeTerminal) {
        m_zoomLevel = 0;
        TerminalConfig config = m_activeTerminal->config();
        config.fontSize = 14;
        m_activeTerminal->setConfig(config);
        m_statusLabel->setText(QStringLiteral("Font size reset to 14"));
    }
}

void TerminalMainWindow::onThemeChanged() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action || !m_activeTerminal) return;
    
    QString themeName = action->data().toString();
    ThemeManager* manager = ThemeManager::instance();
    QVector<ThemeConfig> themes = manager->availableThemes();
    
    for (const ThemeConfig& theme : themes) {
        if (theme.name == themeName) {
            manager->setTheme(theme);
            m_activeTerminal->setTheme(theme);
            m_statusLabel->setText(QString(QStringLiteral("Theme: %1")).arg(themeName));
            return;
        }
    }
}

void TerminalMainWindow::onOpenThemeDialog() {
    if (m_activeTerminal) {
        m_activeTerminal->showThemeDialog();
    }
}

void TerminalMainWindow::onBackendChanged() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;
    
    QString backend = action->data().toString();
    if (m_activeTerminal) {
        TerminalConfig config = m_activeTerminal->config();
        
        if (backend == QStringLiteral("auto")) config.backend = RendererBackend::Auto;
        else if (backend == QStringLiteral("opengl")) config.backend = RendererBackend::OpenGL;
        else if (backend == QStringLiteral("metal")) config.backend = RendererBackend::Metal;
        
        m_activeTerminal->setConfig(config);
        m_statusLabel->setText(QString(QStringLiteral("Backend: %1 (restart to apply)")).arg(backend));
    }
}

void TerminalMainWindow::onSelectionChanged(const QString& text) {
    if (!text.isEmpty()) {
        m_selectionLabel->setText(QString(QStringLiteral("Selected: %1 chars")).arg(text.length()));
    } else {
        m_selectionLabel->clear();
    }
}

void TerminalMainWindow::onScrollPositionChanged(int current, int max) {
    if (max > 0) {
        m_scrollLabel->setText(QString(QStringLiteral("Scroll: %1/%2")).arg(current).arg(max));
    } else {
        m_scrollLabel->clear();
    }
}

void TerminalMainWindow::closeEvent(QCloseEvent* event) {
    saveSession();
    saveSettings();
    QMainWindow::closeEvent(event);
}

void TerminalMainWindow::saveSession() {
    QSettings settings;
    settings.beginGroup(QStringLiteral("session"));
    
    int tabCount = m_tabWidget->count();
    settings.setValue("tabCount", tabCount);
    settings.setValue("currentIndex", m_tabWidget->currentIndex());
    
    for (int i = 0; i < tabCount; ++i) {
        TerminalWidget* terminal = qobject_cast<TerminalWidget*>(m_tabWidget->widget(i));
        if (terminal) {
            QString tabKey = QStringLiteral("tab_%1").arg(i);
            QJsonObject state = terminal->saveSessionState();
            state["tabTitle"] = m_tabWidget->tabText(i);
            settings.setValue(tabKey, QJsonDocument(state).toJson(QJsonDocument::Compact));
        }
    }
    
    settings.endGroup();
    settings.sync();
}

bool TerminalMainWindow::restoreSession() {
    QSettings settings;
    settings.beginGroup(QStringLiteral("session"));
    
    int tabCount = settings.value("tabCount", 0).toInt();
    int currentIndex = settings.value("currentIndex", 0).toInt();
    
    if (tabCount <= 0) {
        settings.endGroup();
        return false;
    }
    
    for (int i = 0; i < tabCount; ++i) {
        QString tabKey = QStringLiteral("tab_%1").arg(i);
        QString jsonStr = settings.value(tabKey).toString();
        
        if (jsonStr.isEmpty()) {
            onNewTab();
            continue;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (doc.isNull() || !doc.isObject()) {
            onNewTab();
            continue;
        }
        
        QJsonObject state = doc.object();
        TerminalWidget* terminal = createTerminal();
        
        int index = m_tabWidget->addTab(terminal, state["tabTitle"].toString(QStringLiteral("Tab %1").arg(i + 1)));
        terminal->restoreSessionState(state);
        
        if (i == currentIndex) {
            m_tabWidget->setCurrentIndex(index);
            m_activeTerminal = terminal;
        }
    }
    
    settings.endGroup();
    return true;
}

void TerminalMainWindow::onTabRenameRequested(int index) {
    TerminalWidget* terminal = qobject_cast<TerminalWidget*>(m_tabWidget->widget(index));
    if (!terminal) return;
    
    QString currentName = m_tabWidget->tabText(index);
    bool ok;
    QString newName = QInputDialog::getText(this, QStringLiteral("重命名标签"),
        QStringLiteral("标签名称:"), QLineEdit::Normal, currentName, &ok);
    
    if (ok && !newName.isEmpty()) {
        m_tabWidget->setTabText(index, newName);
        if (terminal) {
            terminal->setTabName(newName);
        }
    }
}

void TerminalMainWindow::onTabCloseOthersRequested(int index) {
    for (int i = m_tabWidget->count() - 1; i >= 0; --i) {
        if (i != index) {
            QWidget* widget = m_tabWidget->widget(i);
            m_tabWidget->removeTab(i);
            widget->deleteLater();
        }
    }
}

void TerminalMainWindow::onTabCloseAllRequested() {
    while (m_tabWidget->count() > 0) {
        QWidget* widget = m_tabWidget->widget(0);
        m_tabWidget->removeTab(0);
        widget->deleteLater();
    }
    onNewTab();
}

void TerminalMainWindow::onImportExport() {
    if (m_activeTerminal) {
        m_activeTerminal->showImportExportDialog();
    }
}

void TerminalMainWindow::onPluginManager() {
    if (m_activeTerminal) {
        m_activeTerminal->showPluginManagerDialog();
    }
}

void TerminalMainWindow::onAiAssistant() {
    if (m_activeTerminal) {
        m_activeTerminal->showAiAssistantDialog();
    }
}

void TerminalMainWindow::onFileTransfer() {
    if (m_activeTerminal) {
        m_activeTerminal->showFileTransferDialog();
    }
}

void TerminalMainWindow::onRecording() {
    if (m_activeTerminal) {
        m_activeTerminal->showRecordingDialog();
    }
}
