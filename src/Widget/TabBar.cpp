#include "TabBar.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>

TabBar::TabBar(QWidget* parent)
    : QWidget(parent)
    , m_currentIndex(-1)
    , m_hoverIndex(-1)
    , m_pressedIndex(-1)
    , m_movable(true)
    , m_tabsClosable(true)
    , m_expanding(false)
    , m_documentMode(false)
    , m_closeButtonWidth(20)
    , m_tabSpacing(2)
    , m_tabMinWidth(100)
    , m_tabMaxWidth(200)
    , m_dragging(false) {
    
    setFixedHeight(32);
    setMouseTracking(true);
    setAcceptDrops(true);
    
    // 默认样式
    setStyleSheet(
        "TabBar {"
        "    background-color: #2d2d2d;"
        "    border-bottom: 1px solid #3e3e3e;"
        "}"
    );
}

int TabBar::addTab(const QString& title, const QString& icon) {
    TabInfo tab;
    tab.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    tab.title = title;
    tab.icon = icon;
    tab.closable = true;
    tab.modified = false;
    
    m_tabs.append(tab);
    
    if (m_currentIndex == -1) {
        m_currentIndex = 0;
    }
    
    updateLayout();
    update();
    
    return m_tabs.size() - 1;
}

int TabBar::insertTab(int index, const QString& title, const QString& icon) {
    if (index < 0) index = 0;
    if (index > m_tabs.size()) index = m_tabs.size();
    
    TabInfo tab;
    tab.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    tab.title = title;
    tab.icon = icon;
    tab.closable = true;
    
    m_tabs.insert(index, tab);
    
    if (index <= m_currentIndex) {
        m_currentIndex++;
    }
    
    updateLayout();
    update();
    
    return index;
}

void TabBar::removeTab(int index) {
    if (index < 0 || index >= m_tabs.size()) return;
    
    m_tabs.remove(index);
    
    if (m_currentIndex >= m_tabs.size()) {
        m_currentIndex = m_tabs.size() - 1;
    }
    
    updateLayout();
    update();
    
    emit currentChanged(m_currentIndex);
}

void TabBar::clearTabs() {
    m_tabs.clear();
    m_currentIndex = -1;
    m_hoverIndex = -1;
    update();
}

void TabBar::setTabText(int index, const QString& text) {
    if (index >= 0 && index < m_tabs.size()) {
        m_tabs[index].title = text;
        update();
    }
}

void TabBar::setTabIcon(int index, const QString& icon) {
    if (index >= 0 && index < m_tabs.size()) {
        m_tabs[index].icon = icon;
        update();
    }
}

void TabBar::setTabToolTip(int index, const QString& tip) {
    if (index >= 0 && index < m_tabs.size()) {
        m_tabs[index].color = QColor(tip);
        update();
    }
}

void TabBar::setTabModified(int index, bool modified) {
    if (index >= 0 && index < m_tabs.size()) {
        m_tabs[index].modified = modified;
        update();
    }
}

void TabBar::setTabEnabled(int index, bool enabled) {
    if (index >= 0 && index < m_tabs.size()) {
        m_tabs[index].closable = enabled;
        update();
    }
}

void TabBar::setTabClosable(int index, bool closable) {
    if (index >= 0 && index < m_tabs.size()) {
        m_tabs[index].closable = closable;
        update();
    }
}

QString TabBar::tabText(int index) const {
    if (index >= 0 && index < m_tabs.size()) {
        return m_tabs[index].title;
    }
    return QString();
}

QString TabBar::tabIcon(int index) const {
    if (index >= 0 && index < m_tabs.size()) {
        return m_tabs[index].icon;
    }
    return QString();
}

QString TabBar::tabToolTip(int index) const {
    if (index >= 0 && index < m_tabs.size()) {
        return m_tabs[index].color.name();
    }
    return QString();
}

bool TabBar::isTabModified(int index) const {
    if (index >= 0 && index < m_tabs.size()) {
        return m_tabs[index].modified;
    }
    return false;
}

bool TabBar::isTabEnabled(int index) const {
    if (index >= 0 && index < m_tabs.size()) {
        return m_tabs[index].closable;
    }
    return false;
}

void TabBar::setCurrentIndex(int index) {
    if (index < 0) index = 0;
    if (index >= m_tabs.size()) index = m_tabs.size() - 1;
    
    if (m_currentIndex != index) {
        m_currentIndex = index;
        update();
        emit currentChanged(m_currentIndex);
    }
}

void TabBar::setMovable(bool movable) {
    m_movable = movable;
}

void TabBar::setExpanding(bool expanding) {
    m_expanding = expanding;
    updateLayout();
    update();
}

void TabBar::setTabsClosable(bool closable) {
    m_tabsClosable = closable;
    update();
}

void TabBar::setDocumentMode(bool documentMode) {
    m_documentMode = documentMode;
    update();
}

void TabBar::setStyleSheet(const QString& styleSheet) {
    QWidget::setStyleSheet(styleSheet);
    update();
}

void TabBar::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    painter.fillRect(rect(), QColor("#2d2d2d"));
    
    // 绘制标签页
    for (int i = 0; i < m_tabs.size(); ++i) {
        QRect tabR = tabRect(i);
        if (!tabR.isValid()) continue;
        
        const TabInfo& tab = m_tabs[i];
        bool isSelected = (i == m_currentIndex);
        bool isHover = (i == m_hoverIndex);
        
        // 标签页背景
        if (isSelected) {
            painter.fillRect(tabR, QColor("#3e3e3e"));
        } else if (isHover) {
            painter.fillRect(tabR, QColor("#353535"));
        }
        
        // 选中指示线
        if (isSelected) {
            painter.fillRect(tabR.left(), tabR.bottom() - 2, tabR.width(), 2, QColor("#3daee9"));
        }
        
        // 文本
        QString text = tab.title;
        if (tab.modified) {
            text += " *";
        }
        
        QFont font = painter.font();
        font.setPointSize(10);
        painter.setFont(font);
        
        QRect textR = tabR;
        textR.setLeft(textR.left() + 10);
        textR.setRight(textR.right() - (m_tabsClosable && tab.closable ? m_closeButtonWidth + 5 : 5));
        
        painter.setPen(isSelected ? QColor("#ffffff") : QColor("#aaaaaa"));
        painter.drawText(textR, Qt::AlignLeft | Qt::AlignVCenter, text);
        
        // 关闭按钮
        if (m_tabsClosable && tab.closable) {
            QRect closeR(tabR.right() - m_closeButtonWidth - 5, 
                        tabR.center().y() - 8, 16, 16);
            
            if (isHover && m_hoverIndex == i) {
                painter.setPen(QColor("#ff6b6b"));
                painter.drawText(closeR, Qt::AlignCenter, "×");
            } else {
                painter.setPen(QColor("#888888"));
                painter.drawText(closeR, Qt::AlignCenter, "×");
            }
        }
    }
    
    // 绘制分隔线
    painter.setPen(QColor("#3e3e3e"));
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

void TabBar::mousePressEvent(QMouseEvent* event) {
    int index = tabAt(event->pos());
    
    if (index >= 0) {
        m_pressedIndex = index;
        m_dragStartPos = event->pos();
        m_dragging = false;
        
        // 检查关闭按钮点击
        if (m_tabsClosable && m_tabs[index].closable) {
            QRect closeR = tabRect(index);
            closeR.setLeft(closeR.right() - m_closeButtonWidth - 5);
            closeR.setWidth(m_closeButtonWidth);
            
            if (closeR.contains(event->pos())) {
                emit tabCloseRequested(index);
                m_pressedIndex = -1;
                return;
            }
        }
        
        setCurrentIndex(index);
        emit tabBarClicked(index);
    }
    
    update();
}

void TabBar::mouseMoveEvent(QMouseEvent* event) {
    int index = tabAt(event->pos());
    
    if (index != m_hoverIndex) {
        m_hoverIndex = index;
        update();
    }
    
    // 拖拽检测
    if (m_pressedIndex >= 0 && m_movable && !m_dragging) {
        if ((event->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
            m_dragging = true;
            startDrag(m_pressedIndex);
        }
    }
}

void TabBar::mouseReleaseEvent(QMouseEvent* event) {
    if (m_dragging) {
        m_dragging = false;
    }
    
    m_pressedIndex = -1;
    update();
}

void TabBar::leaveEvent(QEvent* event) {
    m_hoverIndex = -1;
    update();
    QWidget::leaveEvent(event);
}

void TabBar::resizeEvent(QResizeEvent* event) {
    updateLayout();
    QWidget::resizeEvent(event);
}

void TabBar::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

void TabBar::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void TabBar::dropEvent(QDropEvent* event) {
    const QMimeData* mime = event->mimeData();
    if (mime && mime->hasFormat("application/x-tabindex")) {
        int fromIndex = mime->data("application/x-tabindex").toInt();
        int toIndex = tabAt(event->pos());
        
        if (fromIndex >= 0 && toIndex >= 0 && fromIndex != toIndex) {
            TabInfo tab = m_tabs.takeAt(fromIndex);
            m_tabs.insert(toIndex, tab);
            
            if (m_currentIndex == fromIndex) {
                m_currentIndex = toIndex;
            } else if (fromIndex < m_currentIndex && toIndex >= m_currentIndex) {
                m_currentIndex--;
            } else if (fromIndex > m_currentIndex && toIndex <= m_currentIndex) {
                m_currentIndex++;
            }
            
            emit tabMoved(fromIndex, toIndex);
            update();
        }
    }
    event->acceptProposedAction();
}

int TabBar::tabAt(const QPoint& pos) const {
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (tabRect(i).contains(pos)) {
            return i;
        }
    }
    return -1;
}

QRect TabBar::tabRect(int index) const {
    if (index < 0 || index >= m_tabs.size()) return QRect();
    
    int totalWidth = width();
    int availableWidth = totalWidth - m_tabSpacing * (m_tabs.size() - 1);
    
    int tabWidth;
    if (m_expanding) {
        tabWidth = availableWidth / m_tabs.size();
    } else {
        QFontMetrics fm(font());
        int maxWidth = 0;
        for (const TabInfo& tab : m_tabs) {
            int w = fm.horizontalAdvance(tab.title) + 20;
            if (m_tabsClosable && tab.closable) w += m_closeButtonWidth;
            maxWidth = qMax(maxWidth, w);
        }
        tabWidth = qBound(m_tabMinWidth, maxWidth, m_tabMaxWidth);
        
        // 如果标签页太多，压缩宽度
        if (tabWidth * m_tabs.size() > availableWidth) {
            tabWidth = availableWidth / m_tabs.size();
        }
    }
    
    int x = 0;
    for (int i = 0; i < index; ++i) {
        x += tabWidth + m_tabSpacing;
    }
    
    return QRect(x, 0, tabWidth, height() - 1);
}

void TabBar::updateLayout() {
    updateGeometry();
}

void TabBar::startDrag(int index) {
    QDrag* drag = new QDrag(this);
    QMimeData* mime = new QMimeData;
    mime->setData("application/x-tabindex", QByteArray::number(index));
    drag->setMimeData(mime);
    
    drag->exec(Qt::MoveAction);
}

#include "TabBar.moc"
