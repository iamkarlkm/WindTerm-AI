#include "SplitterContainer.h"
#include "TerminalPane.h"
#include <QVBoxLayout>
#include <QDebug>

SplitterContainer::SplitterContainer(QWidget* parent)
    : QSplitter(Qt::Horizontal, parent), m_activePane(nullptr), m_nextPaneId(1) {
    setChildrenCollapsible(false);
}

SplitterContainer::SplitterContainer(Qt::Orientation orientation, QWidget* parent)
    : QSplitter(orientation, parent), m_activePane(nullptr), m_nextPaneId(1) {
    setChildrenCollapsible(false);
}

QList<TerminalPane*> SplitterContainer::allPanes() const {
    QList<TerminalPane*> list;
    for (TerminalPane* pane : m_panes) {
        list.append(pane);
    }
    return list;
}

int SplitterContainer::paneCount() const {
    return m_panes.size();
}

void SplitterContainer::addPane(TerminalPane* pane) {
    if (!pane) return;
    
    pane->setPaneId(m_nextPaneId++);
    setupPane(pane);
    insertWidget(count(), pane);
    
    if (!m_activePane) {
        setActivePane(pane);
    }
    
    m_panes.insert(pane);
    pane->show();
}

void SplitterContainer::removePane(TerminalPane* pane) {
    if (!pane) return;
    
    m_panes.remove(pane);
    
    if (m_activePane == pane) {
        m_activePane = nullptr;
        if (!m_panes.isEmpty()) {
            setActivePane(*m_panes.begin());
        } else {
            emit lastPaneClosed();
        }
    }
    
    emit paneRemoved(pane);
    
    pane->deleteLater();
}

bool SplitterContainer::splitPane(TerminalPane* pane, Qt::Orientation orientation) {
    if (!pane || !m_panes.contains(pane)) return false;
    
    TerminalPane* newPane = new TerminalPane(this);
    newPane->setPaneId(m_nextPaneId++);
    setupPane(newPane);
    
    QWidget* paneParent = pane->parentWidget();
    SplitterContainer* targetSplitter = nullptr;
    
    if (auto* existing = qobject_cast<SplitterContainer*>(paneParent)) {
        if (existing->orientation() == orientation) {
            targetSplitter = existing;
        }
    }
    
    if (!targetSplitter) {
        targetSplitter = new SplitterContainer(orientation, paneParent);
        
        if (auto* parentSplitter = qobject_cast<QSplitter*>(paneParent)) {
            int index = parentSplitter->indexOf(pane);
            QList<int> sizes = parentSplitter->sizes();
            parentSplitter->insertWidget(index, targetSplitter);
            parentSplitter->setSizes(sizes);
        }
        
        targetSplitter->addWidget(pane);
    }
    
    targetSplitter->setOrientation(orientation);
    targetSplitter->addWidget(newPane);
    
    m_panes.insert(newPane);
    setActivePane(newPane);
    
    QList<int> equalSizes;
    for (int i = 0; i < targetSplitter->count(); i++) {
        equalSizes.append(1);
    }
    targetSplitter->setSizes(equalSizes);
    
    return true;
}

bool SplitterContainer::closePane(TerminalPane* pane) {
    if (!pane) return false;
    
    pane->stop();
    removePane(pane);
    return true;
}

void SplitterContainer::setActivePane(TerminalPane* pane) {
    if (m_activePane == pane) return;
    
    if (m_activePane) {
        m_activePane->setActive(false);
    }
    
    m_activePane = pane;
    if (m_activePane) {
        m_activePane->setActive(true);
        m_activePane->setFocus();
        emit paneActivated(m_activePane);
    }
}

void SplitterContainer::childEvent(QChildEvent* event) {
    if (event->type() == QEvent::ChildRemoved) {
        QObject* obj = event->child();
        if (auto* pane = qobject_cast<TerminalPane*>(obj)) {
            if (m_panes.contains(pane) && pane->parent() != this) {
                m_panes.remove(pane);
            }
        }
    }
    QSplitter::childEvent(event);
}

void SplitterContainer::handleSplitRequest(TerminalPane* source, Qt::Orientation orientation) {
    splitPane(source, orientation);
}

void SplitterContainer::handleCloseRequest(TerminalPane* source) {
    closePane(source);
}

void SplitterContainer::handleFocusRequest(TerminalPane* source) {
    setActivePane(source);
}

void SplitterContainer::setupPane(TerminalPane* pane) {
    connect(pane, &TerminalPane::focusRequested, this, [this, pane]() {
        handleFocusRequest(pane);
    });
    connect(pane, &TerminalPane::closeRequested, this, [this, pane]() {
        handleCloseRequest(pane);
    });
    connect(pane, &TerminalPane::splitRequested, this, [this, pane](Qt::Orientation orientation) {
        handleSplitRequest(pane, orientation);
    });
}
