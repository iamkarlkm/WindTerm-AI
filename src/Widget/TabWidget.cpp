#include "TabWidget.h"
#include <QMouseEvent>
#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QDebug>

TabBar::TabBar(QWidget* parent)
    : QTabBar(parent), m_editor(nullptr) {
}

void TabBar::mouseDoubleClickEvent(QMouseEvent* event) {
    int index = tabAt(event->pos());
    if (index >= 0) {
        emit tabRenameRequested(index);
        event->accept();
        return;
    }
    
    QTabBar::mouseDoubleClickEvent(event);
}

void TabBar::contextMenuEvent(QContextMenuEvent* event) {
    int index = tabAt(event->pos());
    if (index < 0) {
        QTabBar::contextMenuEvent(event);
        return;
    }
    
    QMenu menu(this);
    
    QAction* renameAction = menu.addAction(QStringLiteral("重命名"));
    connect(renameAction, &QAction::triggered, this, [this, index]() {
        emit tabRenameRequested(index);
    });
    
    menu.addSeparator();
    
    if (count() > 1) {
        QAction* closeOthersAction = menu.addAction(QStringLiteral("关闭其他标签"));
        connect(closeOthersAction, &QAction::triggered, this, [this, index]() {
            emit tabCloseOthersRequested(index);
        });
    }
    
    QAction* closeAllAction = menu.addAction(QStringLiteral("关闭所有标签"));
    connect(closeAllAction, &QAction::triggered, this, [this]() {
        emit tabCloseAllRequested();
    });
    
    menu.exec(event->globalPos());
}

void TabBar::finishRename() {
    if (m_editor) {
        QString newName = m_editor->text().trimmed();
        int index = m_editor->property("tabIndex").toInt();
        
        if (!newName.isEmpty() && index >= 0 && index < count()) {
            setTabText(index, newName);
        }
        
        m_editor->deleteLater();
        m_editor = nullptr;
    }
}

void TabBar::startEditing(int index) {
    if (m_editor) {
        finishRename();
    }
    
    QRect rect = tabRect(index);
    if (!rect.isValid()) return;
    
    m_editor = new QLineEdit(this);
    m_editor->setProperty("tabIndex", index);
    m_editor->setText(tabText(index));
    m_editor->setGeometry(rect);
    m_editor->selectAll();
    m_editor->setFocus();
    
    connect(m_editor, &QLineEdit::returnPressed, this, &TabBar::finishRename);
    connect(m_editor, &QLineEdit::editingFinished, this, &TabBar::finishRename);
    connect(m_editor, &QLineEdit::destroyed, this, [this]() {
        m_editor = nullptr;
    });
    
    m_editor->show();
}


TabWidget::TabWidget(QWidget* parent)
    : QTabWidget(parent), m_tabEditable(true) {
    
    m_tabBar = new TabBar(this);
    setTabBar(m_tabBar);
    
    connect(m_tabBar, &TabBar::tabRenameRequested, this, &TabWidget::tabRenameRequested);
    connect(m_tabBar, &TabBar::tabCloseOthersRequested, this, &TabWidget::tabCloseOthersRequested);
    connect(m_tabBar, &TabBar::tabCloseAllRequested, this, &TabWidget::tabCloseAllRequested);
}

void TabWidget::setTabEditable(bool editable) {
    m_tabEditable = editable;
}

void TabWidget::saveSession(const QString& key) {
    QSettings settings;
    settings.beginGroup(key);
    settings.setValue("tabCount", count());
    settings.setValue("currentIndex", currentIndex());
    
    for (int i = 0; i < count(); ++i) {
        QString tabKey = QStringLiteral("tab_%1").arg(i);
        settings.beginGroup(tabKey);
        settings.setValue("name", tabText(i));
        settings.setValue("toolTip", tabToolTip(i));
        settings.endGroup();
    }
    
    settings.endGroup();
    settings.sync();
}

void TabWidget::restoreSession(const QString& key) {
    QSettings settings;
    settings.beginGroup(key);
    
    int tabCount = settings.value("tabCount", 0).toInt();
    int currentIndex = settings.value("currentIndex", 0).toInt();
    
    settings.endGroup();
    
    if (tabCount > 0) {
        qDebug() << "[TabWidget] Restoring session with" << tabCount << "tabs";
    }
}
