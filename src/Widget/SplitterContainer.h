#ifndef SPLITTER_CONTAINER_H
#define SPLITTER_CONTAINER_H

#include <QSplitter>
#include <QWidget>
#include <QHash>
#include <QSet>

class TerminalPane;

class SplitterContainer : public QSplitter {
    Q_OBJECT
public:
    explicit SplitterContainer(QWidget* parent = nullptr);
    explicit SplitterContainer(Qt::Orientation orientation, QWidget* parent = nullptr);
    
    TerminalPane* activePane() const { return m_activePane; }
    QList<TerminalPane*> allPanes() const;
    int paneCount() const;
    
    void addPane(TerminalPane* pane);
    void removePane(TerminalPane* pane);
    bool splitPane(TerminalPane* pane, Qt::Orientation orientation);
    bool closePane(TerminalPane* pane);
    
    void setActivePane(TerminalPane* pane);
    
signals:
    void paneActivated(TerminalPane* pane);
    void paneRemoved(TerminalPane* pane);
    void lastPaneClosed();
    
protected:
    void childEvent(QChildEvent* event) override;
    
private:
    void handleSplitRequest(TerminalPane* source, Qt::Orientation orientation);
    void handleCloseRequest(TerminalPane* source);
    void handleFocusRequest(TerminalPane* source);
    void setupPane(TerminalPane* pane);
    
    TerminalPane* m_activePane;
    QSet<TerminalPane*> m_panes;
    int m_nextPaneId;
};

#endif
