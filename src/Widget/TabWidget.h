#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>
#include <QTabBar>
#include <QLineEdit>
#include <QMenu>

class TabBar : public QTabBar {
    Q_OBJECT
public:
    explicit TabBar(QWidget* parent = nullptr);
    
signals:
    void tabRenameRequested(int index);
    void tabCloseOthersRequested(int index);
    void tabCloseAllRequested();
    
protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    
private slots:
    void finishRename();
    
private:
    void startEditing(int index);
    
    QLineEdit* m_editor;
};

class TabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit TabWidget(QWidget* parent = nullptr);
    
    void setTabEditable(bool editable);
    void saveSession(const QString& key);
    void restoreSession(const QString& key);
    
signals:
    void tabRenameRequested(int index);
    void tabCloseOthersRequested(int index);
    void tabCloseAllRequested();
    
private:
    TabBar* m_tabBar;
    bool m_tabEditable;
};

#endif
