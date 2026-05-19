#ifndef TERMINAL_MAIN_WINDOW_H
#define TERMINAL_MAIN_WINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QSettings>

class TerminalWidget;
class QSplitter;
class QTabWidget;
class QAction;

class TerminalMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit TerminalMainWindow(QWidget* parent = nullptr);
    ~TerminalMainWindow() override;
    
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
    void onBackendChanged();
    void onSelectionChanged(const QString& text);
    void onScrollPositionChanged(int current, int max);
    
private:
    void setupMenu();
    void setupToolBar();
    void setupStatusBar();
    void loadSettings();
    void saveSettings();
    
    TerminalWidget* createTerminal();
    
    TerminalWidget* m_activeTerminal;
    QTabWidget* m_tabWidget;
    QSplitter* m_splitter;
    
    QLabel* m_statusLabel;
    QLabel* m_selectionLabel;
    QLabel* m_scrollLabel;
    
    QSettings* m_settings;
    QAction* m_fullscreenAction;
    
    int m_zoomLevel;
    bool m_isFullscreen;
};

#endif
