#ifndef TAB_BAR_H
#define TAB_BAR_H

#include <QWidget>
#include <QVector>
#include <QDrag>

struct TabInfo {
    QString id;
    QString title;
    QString icon;
    bool closable;
    bool modified;
    QColor color;
    
    TabInfo() : closable(true), modified(false) {}
};

class TabBar : public QWidget {
    Q_OBJECT
public:
    explicit TabBar(QWidget* parent = nullptr);
    
    // 标签页管理
    int addTab(const QString& title, const QString& icon = "");
    int insertTab(int index, const QString& title, const QString& icon = "");
    void removeTab(int index);
    void clearTabs();
    
    // 标签页信息
    void setTabText(int index, const QString& text);
    void setTabIcon(int index, const QString& icon);
    void setTabToolTip(int index, const QString& tip);
    void setTabModified(int index, bool modified);
    void setTabEnabled(int index, bool enabled);
    void setTabClosable(int index, bool closable);
    
    QString tabText(int index) const;
    QString tabIcon(int index) const;
    QString tabToolTip(int index) const;
    bool isTabModified(int index) const;
    bool isTabEnabled(int index) const;
    
    // 当前标签页
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);
    
    int count() const { return m_tabs.size(); }
    
    // 拖放支持
    void setMovable(bool movable);
    bool isMovable() const { return m_movable; }
    
    // 外观
    void setExpanding(bool expanding);
    void setTabsClosable(bool closable);
    void setDocumentMode(bool documentMode);
    
    // 样式
    void setStyleSheet(const QString& styleSheet);
    
signals:
    void currentChanged(int index);
    void tabCloseRequested(int index);
    void tabMoved(int from, int to);
    void tabBarDoubleClicked(int index);
    void tabBarClicked(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    int tabAt(const QPoint& pos) const;
    QRect tabRect(int index) const;
    void updateLayout();
    void startDrag(int index);
    
    QVector<TabInfo> m_tabs;
    int m_currentIndex;
    int m_hoverIndex;
    int m_pressedIndex;
    bool m_movable;
    bool m_tabsClosable;
    bool m_expanding;
    bool m_documentMode;
    
    int m_closeButtonWidth;
    int m_tabSpacing;
    int m_tabMinWidth;
    int m_tabMaxWidth;
    
    QPoint m_dragStartPos;
    bool m_dragging;
};

#endif
