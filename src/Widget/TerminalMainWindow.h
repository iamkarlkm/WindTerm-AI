#ifndef TERMINAL_MAIN_WINDOW_H
#define TERMINAL_MAIN_WINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QSettings>

class TerminalWidget;
class TabWidget;
class QSplitter;
class QAction;
class PluginManager;
class PluginContext;

class TerminalMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit TerminalMainWindow(QWidget* parent = nullptr);
    ~TerminalMainWindow() override;

    TerminalWidget* activeTerminal() const { return m_activeTerminal; }
    
protected:
    void closeEvent(QCloseEvent* event) override;
    
private slots:
    void onNewTab();
    void onCloseTab();
    void onSplitHorizontal();
    void onSplitVertical();
    void onCopy();
    void onPaste();
    void onClear();
    void onSelectAll();
    void onFind();
    void onSettings();
    void onFullscreen();
    void onZoomIn();
    void onZoomOut();
    void onResetZoom();
    void onThemeChanged();
    void onOpenThemeDialog();
    void onBackendChanged();
    void onSelectionChanged(const QString& text);
    void onScrollPositionChanged(int current, int max);
    void onTabRenameRequested(int index);
    void onTabCloseOthersRequested(int index);
    void onTabCloseAllRequested();
    void onImportExport();
    void onPluginManager();
    void onAiAssistant();
    void onFileTransfer();
    void onRecording();
    
private:
    void setupMenu();
    void setupToolBar();
    void setupStatusBar();
    void loadSettings();
    void saveSettings();
    void saveSession();
    bool restoreSession();
    
    TerminalWidget* createTerminal();
    
    TerminalWidget* m_activeTerminal;
    TabWidget* m_tabWidget;
    QSplitter* m_splitter;
    
    QLabel* m_statusLabel;
    QLabel* m_selectionLabel;
    QLabel* m_scrollLabel;
    
    QSettings* m_settings;
    QAction* m_fullscreenAction;
    
    PluginManager* m_pluginManager;
    PluginContext* m_pluginContext;
    
    int m_zoomLevel;
    bool m_isFullscreen;
};

#endif
