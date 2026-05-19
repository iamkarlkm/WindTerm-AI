#include "TerminalMainWindow.h"
#include "TerminalWidget.h"
#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QTabWidget>
#include <QDebug>
#include <QScreen>

TerminalMainWindow::TerminalMainWindow(QWidget* parent)
    : QMainWindow(parent), m_activeTerminal(nullptr), m_tabWidget(nullptr),
      m_splitter(nullptr), m_statusLabel(nullptr), m_selectionLabel(nullptr),
      m_scrollLabel(nullptr), m_settings(nullptr), m_fullscreenAction(nullptr),
      m_zoomLevel(0), m_isFullscreen(false) {
    
    m_settings = new QSettings(QStringLiteral("WindTerm"), QStringLiteral("Terminal"), this);
    loadSettings();
    
    setWindowTitle(QStringLiteral("WindTerm Extensions - GPU Accelerated Terminal"));
    setMinimumSize(800, 600);
    
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget* widget = m_tabWidget->widget(index);
        m_tabWidget->removeTab(index);
        widget->deleteLater();
        if (m_tabWidget->count() == 0) {
            onNewTab();
        }
    });
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index >= 0) {
            m_activeTerminal = qobject_cast<TerminalWidget*>(m_tabWidget->widget(index));
            if (m_activeTerminal) {
                m_activeTerminal->setFocus();
            }
        }
    });
    
    setCentralWidget(m_tabWidget);
    
    setupMenu();
    setupToolBar();
    setupStatusBar();
    
    onNewTab();
}

TerminalMainWindow::~TerminalMainWindow() {
    saveSettings();
}

void TerminalMainWindow::setupMenu() {
    QMenuBar* menuBar = this->menuBar();
    
    QMenu* fileMenu = menuBar->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&New Tab"), this, &TerminalMainWindow::onNewTab, QKeySequence::New);
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
    themeMenu->addAction(QStringLiteral("&Dark"), this, &TerminalMainWindow::onThemeChanged)->setData(QStringLiteral("dark"));
    themeMenu->addAction(QStringLiteral("&Light"), this, &TerminalMainWindow::onThemeChanged)->setData(QStringLiteral("light"));
    themeMenu->addAction(QStringLiteral("&Solarized"), this, &TerminalMainWindow::onThemeChanged)->setData(QStringLiteral("solarized"));
    
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
    if (!action) return;
    
    QString theme = action->data().toString();
    TerminalConfig config;
    
    if (theme == QStringLiteral("dark")) {
        config.backgroundColor = QColor(30, 30, 30);
        config.foregroundColor = QColor(200, 200, 200);
    } else if (theme == QStringLiteral("light")) {
        config.backgroundColor = QColor(240, 240, 240);
        config.foregroundColor = QColor(0, 0, 0);
    } else if (theme == QStringLiteral("solarized")) {
        config.backgroundColor = QColor(0, 43, 54);
        config.foregroundColor = QColor(131, 148, 150);
    }
    
    if (m_activeTerminal) {
        config.fontFamily = m_activeTerminal->config().fontFamily;
        config.fontSize = m_activeTerminal->config().fontSize;
        config.bufferCapacity = m_activeTerminal->config().bufferCapacity;
        config.columns = m_activeTerminal->config().columns;
        config.rows = m_activeTerminal->config().rows;
        config.backend = m_activeTerminal->config().backend;
        m_activeTerminal->setConfig(config);
    }
    
    m_statusLabel->setText(QString(QStringLiteral("Theme: %1")).arg(theme));
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
    saveSettings();
    QMainWindow::closeEvent(event);
}
