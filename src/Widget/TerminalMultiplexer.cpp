#include "TerminalMultiplexer.h"
#include "TerminalWidget.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

TerminalMultiplexer::TerminalMultiplexer(QObject* parent)
    : QObject(parent)
    , m_splitter(new QSplitter(Qt::Horizontal))
    , m_currentPane(nullptr) {
}

TerminalWidget* TerminalMultiplexer::createPane(const QString& title) {
    TerminalWidget* pane = new TerminalWidget();
    pane->setWindowTitle(title);
    
    QString paneId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_panes[paneId] = pane;
    
    connectPane(pane);
    
    emit paneCreated(pane);
    qDebug() << "[TerminalMultiplexer] Created pane:" << paneId;
    
    return pane;
}

void TerminalMultiplexer::closePane(TerminalWidget* pane) {
    if (!pane) return;
    
    QString id;
    for (auto it = m_panes.begin(); it != m_panes.end(); ++it) {
        if (it.value() == pane) {
            id = it.key();
            m_panes.erase(it);
            break;
        }
    }
    
    if (m_currentPane == pane) {
        m_currentPane = nullptr;
    }
    
    pane->deleteLater();
    emit paneClosed(pane);
    emit layoutChanged();
}

void TerminalMultiplexer::splitPane(TerminalWidget* pane, PaneLayout::Orientation orientation) {
    if (!pane) return;
    
    // 创建新面板
    TerminalWidget* newPane = createPane("Split Pane");
    
    // 添加到分割器
    m_splitter->addWidget(pane);
    m_splitter->addWidget(newPane);
    
    // 设置分割方向
    m_splitter->setOrientation(orientation == PaneLayout::Horizontal ? Qt::Horizontal : Qt::Vertical);
    
    // 平均分配大小
    QList<int> sizes;
    sizes << m_splitter->width() / 2 << m_splitter->width() / 2;
    m_splitter->setSizes(sizes);
    
    // 聚焦新面板
    focusNextPane();
    
    emit layoutChanged();
}

void TerminalMultiplexer::mergePanes(TerminalWidget* pane1, TerminalWidget* pane2) {
    Q_UNUSED(pane1)
    Q_UNUSED(pane2)
    // TODO: Implement pane merging
}

void TerminalMultiplexer::setLayout(const PaneLayout& layout) {
    m_currentLayout = layout;
    emit layoutChanged();
}

PaneLayout TerminalMultiplexer::currentLayout() const {
    return m_currentLayout;
}

void TerminalMultiplexer::saveLayout(const QString& filePath) {
    QJsonObject json;
    json["layout"] = m_currentLayout.orientation == PaneLayout::Horizontal ? "horizontal" : "vertical";
    json["paneCount"] = m_panes.size();
    
    QJsonArray panes;
    for (auto it = m_panes.begin(); it != m_panes.end(); ++it) {
        QJsonObject paneJson;
        paneJson["id"] = it.key();
        paneJson["title"] = it.value()->windowTitle();
        panes.append(paneJson);
    }
    json["panes"] = panes;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
    }
}

void TerminalMultiplexer::loadLayout(const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject json = doc.object();
        
        QString orientation = json["layout"].toString();
        m_currentLayout.orientation = (orientation == "horizontal") ? 
            PaneLayout::Horizontal : PaneLayout::Vertical;
        
        // Restore panes from JSON
        QJsonArray panes = json["panes"].toArray();
        for (const QJsonValue& paneValue : panes) {
            QJsonObject paneJson = paneValue.toObject();
            createPane(paneJson["title"].toString());
        }
        
        emit layoutChanged();
    }
}

void TerminalMultiplexer::focusNextPane() {
    if (m_panes.isEmpty()) return;
    
    auto it = m_panes.find(m_currentPane);
    if (it == m_panes.end() || it == m_panes.end() - 1) {
        it = m_panes.begin();
    } else {
        ++it;
    }
    
    m_currentPane = it.value();
    m_currentPane->setFocus();
    emit focusChanged(m_currentPane);
}

void TerminalMultiplexer::focusPreviousPane() {
    if (m_panes.isEmpty()) return;
    
    auto it = m_panes.find(m_currentPane);
    if (it == m_panes.begin() || it == m_panes.end()) {
        it = m_panes.end() - 1;
    } else {
        --it;
    }
    
    m_currentPane = it.value();
    m_currentPane->setFocus();
    emit focusChanged(m_currentPane);
}

void TerminalMultiplexer::focusPane(int index) {
    if (index < 0 || index >= m_panes.size()) return;
    
    auto it = m_panes.begin();
    std::advance(it, index);
    
    m_currentPane = it.value();
    m_currentPane->setFocus();
    emit focusChanged(m_currentPane);
}

QList<TerminalWidget*> TerminalMultiplexer::allPanes() const {
    return m_panes.values();
}

void TerminalMultiplexer::registerShortcut(const QString& action, const QKeySequence& shortcut) {
    m_shortcuts[action] = shortcut;
}

void TerminalMultiplexer::triggerAction(const QString& action) {
    if (action == "split_horizontal") {
        if (m_currentPane) {
            splitPane(m_currentPane, PaneLayout::Horizontal);
        }
    } else if (action == "split_vertical") {
        if (m_currentPane) {
            splitPane(m_currentPane, PaneLayout::Vertical);
        }
    } else if (action == "focus_next") {
        focusNextPane();
    } else if (action == "focus_prev") {
        focusPreviousPane();
    } else if (action == "close_pane") {
        if (m_currentPane) {
            closePane(m_currentPane);
        }
    }
}

void TerminalMultiplexer::layoutTwoHorizontal() {
    PaneLayout layout;
    layout.orientation = PaneLayout::Horizontal;
    setLayout(layout);
    
    if (m_panes.isEmpty()) {
        createPane("Pane 1");
        createPane("Pane 2");
    }
    
    emit layoutChanged();
}

void TerminalMultiplexer::layoutTwoVertical() {
    PaneLayout layout;
    layout.orientation = PaneLayout::Vertical;
    setLayout(layout);
    
    if (m_panes.isEmpty()) {
        createPane("Pane 1");
        createPane("Pane 2");
    }
    
    emit layoutChanged();
}

void TerminalMultiplexer::layoutThreeColumns() {
    PaneLayout layout;
    layout.orientation = PaneLayout::Horizontal;
    setLayout(layout);
    
    while (m_panes.size() < 3) {
        createPane(QString("Pane %1").arg(m_panes.size() + 1));
    }
    
    emit layoutChanged();
}

void TerminalMultiplexer::layoutThreeRows() {
    PaneLayout layout;
    layout.orientation = PaneLayout::Vertical;
    setLayout(layout);
    
    while (m_panes.size() < 3) {
        createPane(QString("Pane %1").arg(m_panes.size() + 1));
    }
    
    emit layoutChanged();
}

void TerminalMultiplexer::layoutGrid2x2() {
    // Create 4 panes in a 2x2 grid
    while (m_panes.size() < 4) {
        createPane(QString("Pane %1").arg(m_panes.size() + 1));
    }
    
    emit layoutChanged();
}

void TerminalMultiplexer::connectPane(TerminalWidget* pane) {
    connect(pane, &TerminalWidget::destroyed, this, [this, pane]() {
        closePane(pane);
    });
    
    if (!m_currentPane) {
        m_currentPane = pane;
    }
}

#include "TerminalMultiplexer.moc"
