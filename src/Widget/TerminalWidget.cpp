#include "TerminalWidget.h"
#include "TerminalPane.h"
#include "SplitterContainer.h"
#include <QVBoxLayout>
#include <QDebug>

TerminalWidget::TerminalWidget(QWidget* parent)
    : QWidget(parent), m_splitter(nullptr), m_activePane(nullptr), m_paneCounter(0) {
    initWidget();
}

TerminalWidget::TerminalWidget(const TerminalConfig& config, QWidget* parent)
    : QWidget(parent), m_config(config), m_splitter(nullptr), m_activePane(nullptr), m_paneCounter(0) {
    initWidget();
}

TerminalWidget::~TerminalWidget() {
    for (TerminalPane* pane : m_splitter->allPanes()) {
        pane->stop();
    }
}

void TerminalWidget::startShell(const QString& shell, const QString& workDir) {
    if (m_splitter->paneCount() == 0) {
        TerminalPane* pane = new TerminalPane(m_splitter);
        pane->setPaneId(++m_paneCounter);
        pane->setFontFamily(m_config.fontFamily);
        pane->setFontSize(m_config.fontSize);
        pane->setColors(m_config.backgroundColor, m_config.foregroundColor);
        m_splitter->addPane(pane);
        
        connect(pane, &TerminalPane::titleChanged, this, [this, pane](const QString& title) {
            if (pane == m_activePane) {
                emit titleChanged(title);
            }
        });
    }
    
    for (TerminalPane* pane : m_splitter->allPanes()) {
        pane->startShell(shell, workDir);
    }
}

void TerminalWidget::write(const QString& text) {
    write(text.toUtf8());
}

void TerminalWidget::write(const QByteArray& data) {
    if (m_activePane) {
        m_activePane->write(data);
    }
}

void TerminalWidget::clear() {
    for (TerminalPane* pane : m_splitter->allPanes()) {
        pane->session()->clearBuffer();
    }
}

void TerminalWidget::setConfig(const TerminalConfig& config) {
    m_config = config;
    for (TerminalPane* pane : m_splitter->allPanes()) {
        pane->setFontFamily(config.fontFamily);
        pane->setFontSize(config.fontSize);
        pane->setColors(config.backgroundColor, config.foregroundColor);
    }
}

TerminalPane* TerminalWidget::activePane() const {
    return m_activePane;
}

QList<TerminalPane*> TerminalWidget::allPanes() const {
    return m_splitter->allPanes();
}

void TerminalWidget::copyToClipboard() {
    if (m_activePane) {
        m_activePane->session()->copyToClipboard();
    }
}

void TerminalWidget::pasteFromClipboard() {
    if (m_activePane) {
        m_activePane->session()->pasteFromClipboard();
    }
}

void TerminalWidget::selectLine(int row) {
    // Selection is handled at pane level
}

int TerminalWidget::scrollbackSize() const {
    if (m_activePane) {
        return m_activePane->session()->scrollbackSize();
    }
    return 0;
}

int TerminalWidget::paneCount() const {
    return m_splitter->paneCount();
}

bool TerminalWidget::splitPane(Qt::Orientation orientation) {
    if (m_activePane) {
        return m_splitter->splitPane(m_activePane, orientation);
    }
    return false;
}

bool TerminalWidget::closePane(TerminalPane* pane) {
    return m_splitter->closePane(pane);
}

void TerminalWidget::onSplitRequested(Qt::Orientation orientation) {
    splitPane(orientation);
}

void TerminalWidget::onCloseRequested() {
    if (m_activePane) {
        closePane(m_activePane);
        if (m_splitter->paneCount() == 0) {
            emit allPanesClosed();
        }
    }
}

void TerminalWidget::onPaneFocusRequested() {
    TerminalPane* pane = qobject_cast<TerminalPane*>(sender());
    if (pane && pane != m_activePane) {
        m_activePane = pane;
        emit paneActivated(pane);
        emit titleChanged(pane->session()->title());
    }
}

void TerminalWidget::initWidget() {
    setFocusPolicy(Qt::StrongFocus);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    m_splitter = new SplitterContainer(this);
    layout->addWidget(m_splitter);
    
    setupConnections();
}

void TerminalWidget::setupConnections() {
    connect(m_splitter, &SplitterContainer::paneActivated, this, [this](TerminalPane* pane) {
        m_activePane = pane;
        emit paneActivated(pane);
        emit titleChanged(pane->session()->title());
    });
    
    connect(m_splitter, &SplitterContainer::lastPaneClosed, this, [this]() {
        emit allPanesClosed();
    });
}
