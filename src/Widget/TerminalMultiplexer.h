#ifndef TERMINAL_MULTIPLEXER_H
#define TERMINAL_MULTIPLEXER_H

#include <QObject>
#include <QMap>
#include <QSplitter>

class TerminalWidget;

struct PaneLayout {
    enum Orientation { Horizontal, Vertical };
    
    Orientation orientation;
    int size;  // percentage 0-100
    QString sessionId;
    QList<PaneLayout> children;  // for nested layouts
    
    PaneLayout() : orientation(Horizontal), size(50) {}
};

class TerminalMultiplexer : public QObject {
    Q_OBJECT
public:
    explicit TerminalMultiplexer(QObject* parent = nullptr);
    
    // 面板管理
    TerminalWidget* createPane(const QString& title = "");
    void closePane(TerminalWidget* pane);
    void splitPane(TerminalWidget* pane, PaneLayout::Orientation orientation);
    void mergePanes(TerminalWidget* pane1, TerminalWidget* pane);
    
    // 布局管理
    void setLayout(const PaneLayout& layout);
    PaneLayout currentLayout() const;
    void saveLayout(const QString& filePath);
    void loadLayout(const QString& filePath);
    
    // 面板切换
    void focusNextPane();
    void focusPreviousPane();
    void focusPane(int index);
    TerminalWidget* currentPane() const { return m_currentPane; }
    
    // 面板信息
    QList<TerminalWidget*> allPanes() const;
    int paneCount() const { return m_panes.size(); }
    
    // 快捷键绑定
    void registerShortcut(const QString& action, const QKeySequence& shortcut);
    void triggerAction(const QString& action);
    
    // 预定义布局
    void layoutTwoHorizontal();
    void layoutTwoVertical();
    void layoutThreeColumns();
    void layoutThreeRows();
    void layoutGrid2x2();
    
signals:
    void paneCreated(TerminalWidget* pane);
    void paneClosed(TerminalWidget* pane);
    void layoutChanged();
    void focusChanged(TerminalWidget* pane);

private:
    void updatePaneSizes();
    void connectPane(TerminalWidget* pane);
    
    QSplitter* m_splitter;
    QMap<QString, TerminalWidget*> m_panes;
    TerminalWidget* m_currentPane;
    PaneLayout m_currentLayout;
    
    QMap<QString, QKeySequence> m_shortcuts;
};

#endif
