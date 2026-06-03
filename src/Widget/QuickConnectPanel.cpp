#include "QuickConnectPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <algorithm>

QuickConnectPanel* QuickConnectPanel::s_instance = nullptr;

QuickConnectPanel::QuickConnectPanel(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
    , m_searchBox(new QLineEdit())
    , m_resultList(new QListWidget())
    , m_protocolBox(new QComboBox())
    , m_hostBox(new QLineEdit())
    , m_portBox(new QLineEdit())
    , m_usernameBox(new QLineEdit())
    , m_showPresets(true) {
    
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMinimumSize(500, 400);
    
    setupUI();
    loadPresets();
    
    connect(m_searchBox, &QLineEdit::textChanged, this, &QuickConnectPanel::onSearchTextChanged);
    connect(m_resultList, &QListWidget::itemDoubleClicked, this, &QuickConnectPanel::onEntryDoubleClicked);
    connect(m_protocolBox, QOverload<const QString&>::of(&QComboBox::currentTextChanged), 
            this, &QuickConnectPanel::onProtocolChanged);
}

QuickConnectPanel* QuickConnectPanel::instance() {
    if (!s_instance) {
        s_instance = new QuickConnectPanel();
    }
    return s_instance;
}

void QuickConnectPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 搜索框
    m_searchBox->setPlaceholderText("Search connections...");
    m_searchBox->setMinimumHeight(35);
    m_searchBox->setStyleSheet(
        "QLineEdit {"
        "    font-size: 14px;"
        "    padding: 8px;"
        "    border: 2px solid #3daee9;"
        "    border-radius: 5px;"
        "}"
    );
    
    // 结果列表
    m_resultList->setMinimumHeight(200);
    m_resultList->setStyleSheet(
        "QListWidget {"
        "    font-size: 13px;"
        "    border: 1px solid #ccc;"
        "    border-radius: 5px;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #3daee9;"
        "    color: white;"
        "}"
    );
    
    // 快速连接表单
    QHBoxLayout* formLayout = new QHBoxLayout();
    
    m_protocolBox->addItems({"SSH", "Telnet", "Serial"});
    m_protocolBox->setMinimumWidth(80);
    
    m_hostBox->setPlaceholderText("Host");
    m_hostBox->setMinimumHeight(30);
    
    m_portBox->setPlaceholderText("Port");
    m_portBox->setMaximumWidth(60);
    m_portBox->setText("22");
    m_portBox->setMinimumHeight(30);
    
    m_usernameBox->setPlaceholderText("Username");
    m_usernameBox->setMinimumHeight(30);
    
    QPushButton* connectBtn = new QPushButton("Connect");
    connectBtn->setMinimumHeight(30);
    connectBtn->setMinimumWidth(80);
    connectBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #3daee9;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "    padding: 5px 15px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3498db;"
        "}"
    );
    
    connect(connectBtn, &QPushButton::clicked, this, &QuickConnectPanel::onConnectClicked);
    
    formLayout->addWidget(m_protocolBox);
    formLayout->addWidget(m_hostBox);
    formLayout->addWidget(m_portBox);
    formLayout->addWidget(m_usernameBox);
    formLayout->addWidget(connectBtn);
    
    // 添加到主布局
    mainLayout->addWidget(m_searchBox);
    mainLayout->addWidget(m_resultList);
    mainLayout->addLayout(formLayout);
    
    setLayout(mainLayout);
}

void QuickConnectPanel::loadPresets() {
    // 加载常用预设
    QList<QuickConnectEntry> presets = {
        {"localhost", "Localhost", "127.0.0.1", 22, "", "ssh", "Local", 0, 0},
        {"aws", "AWS EC2", "", 22, "ec2-user", "ssh", "Cloud", 0, 0},
        {"azure", "Azure VM", "", 22, "azureuser", "ssh", "Cloud", 0, 0},
        {"gcp", "GCP Compute", "", 22, "gcp-user", "ssh", "Cloud", 0, 0},
        {"raspberry", "Raspberry Pi", "raspberrypi.local", 22, "pi", "ssh", "IoT", 0, 0},
    };
    
    for (const QuickConnectEntry& preset : presets) {
        m_entries[preset.id] = preset;
    }
}

QString QuickConnectPanel::addEntry(const QuickConnectEntry& entry) {
    QuickConnectEntry newEntry = entry;
    if (newEntry.id.isEmpty()) {
        newEntry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    m_entries[newEntry.id] = newEntry;
    updateSearchResults();
    
    return newEntry.id;
}

void QuickConnectPanel::updateEntry(const QString& id, const QuickConnectEntry& entry) {
    if (m_entries.contains(id)) {
        m_entries[id] = entry;
        updateSearchResults();
    }
}

void QuickConnectPanel::deleteEntry(const QString& id) {
    m_entries.remove(id);
    updateSearchResults();
}

QuickConnectEntry QuickConnectPanel::getEntry(const QString& id) const {
    return m_entries.value(id);
}

QList<QuickConnectEntry> QuickConnectPanel::getAllEntries() const {
    return m_entries.values();
}

QList<QuickConnectEntry> QuickConnectPanel::searchEntries(const QString& query) const {
    QList<QuickConnectEntry> results;
    QString lowerQuery = query.toLower();
    
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        const QuickConnectEntry& entry = it.value();
        
        bool match = entry.name.toLower().contains(lowerQuery) ||
                    entry.host.toLower().contains(lowerQuery) ||
                    entry.username.toLower().contains(lowerQuery) ||
                    entry.group.toLower().contains(lowerQuery);
        
        if (match) {
            results.append(entry);
        }
    }
    
    // 按使用频率排序
    std::sort(results.begin(), results.end(), [](const QuickConnectEntry& a, const QuickConnectEntry& b) {
        return a.usageCount > b.usageCount;
    });
    
    return results;
}

QList<QuickConnectEntry> QuickConnectPanel::getEntriesByGroup(const QString& group) const {
    QList<QuickConnectEntry> results;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->group == group) {
            results.append(it.value());
        }
    }
    return results;
}

QStringList QuickConnectPanel::getGroups() const {
    QStringList groups;
    QSet<QString> groupSet;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        groupSet.insert(it->group);
    }
    groups = groupSet.values();
    groups.sort();
    return groups;
}

void QuickConnectPanel::show() {
    // 居中显示
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screenGeometry = desktop->availableGeometry();
    
    int x = screenGeometry.x() + (screenGeometry.width() - width()) / 2;
    int y = screenGeometry.y() + (screenGeometry.height() - height()) / 3;
    move(x, y);
    
    m_searchBox->clear();
    m_searchBox->setFocus();
    
    updateSearchResults();
    
    QWidget::show();
    emit panelShown();
}

void QuickConnectPanel::hide() {
    m_searchBox->clear();
    QWidget::hide();
    emit panelHidden();
}

void QuickConnectPanel::toggle() {
    if (isVisible()) {
        hide();
    } else {
        show();
    }
}

bool QuickConnectPanel::isVisible() const {
    return QWidget::isVisible();
}

void QuickConnectPanel::connectTo(const QString& id) {
    if (!m_entries.contains(id)) return;
    
    QuickConnectEntry entry = m_entries[id];
    recordUsage(id);
    
    emit connectionRequested(entry);
    hide();
}

void QuickConnectPanel::quickConnect(const QString& host, int port, const QString& username) {
    QuickConnectEntry entry;
    entry.host = host;
    entry.port = port;
    entry.username = username;
    entry.protocol = "ssh";
    entry.name = host;
    
    emit connectionRequested(entry);
    hide();
}

void QuickConnectPanel::recordUsage(const QString& id) {
    if (m_entries.contains(id)) {
        m_entries[id].usageCount++;
        m_entries[id].lastUsed = QDateTime::currentMSecsSinceEpoch();
    }
}

void QuickConnectPanel::exportEntries(const QString& filePath) {
    QJsonArray entriesJson;
    
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        QJsonObject json;
        json["id"] = it.key();
        json["name"] = it->name;
        json["host"] = it->host;
        json["port"] = it->port;
        json["username"] = it->username;
        json["protocol"] = it->protocol;
        json["group"] = it->group;
        entriesJson.append(json);
    }
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(entriesJson).toJson(QJsonDocument::Indented));
    }
}

void QuickConnectPanel::importEntries(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonArray entriesJson = QJsonDocument::fromJson(file.readAll()).array();
    
    for (const QJsonValue& value : entriesJson) {
        QJsonObject json = value.toObject();
        
        QuickConnectEntry entry;
        entry.id = json["id"].toString();
        entry.name = json["name"].toString();
        entry.host = json["host"].toString();
        entry.port = json["port"].toInt(22);
        entry.username = json["username"].toString();
        entry.protocol = json["protocol"].toString();
        entry.group = json["group"].toString();
        
        m_entries[entry.id] = entry;
    }
    
    updateSearchResults();
}

void QuickConnectPanel::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Down:
            if (m_resultList->currentRow() < m_resultList->count() - 1) {
                m_resultList->setCurrentRow(m_resultList->currentRow() + 1);
            }
            break;
            
        case Qt::Key_Up:
            if (m_resultList->currentRow() > 0) {
                m_resultList->setCurrentRow(m_resultList->currentRow() - 1);
            }
            break;
            
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (m_resultList->currentRow() >= 0) {
                QListWidgetItem* item = m_resultList->currentItem();
                if (item) {
                    onEntryDoubleClicked(item);
                }
            } else {
                onConnectClicked();
            }
            break;
            
        case Qt::Key_Escape:
            hide();
            break;
            
        default:
            QWidget::keyPressEvent(event);
    }
}

void QuickConnectPanel::focusOutEvent(QFocusEvent* event) {
    Q_UNUSED(event)
    // Don't auto-hide to allow form interaction
}

void QuickConnectPanel::onSearchTextChanged(const QString& text) {
    updateSearchResults();
}

void QuickConnectPanel::onEntryDoubleClicked(QListWidgetItem* item) {
    int index = m_resultList->row(item);
    if (index >= 0 && index < m_filteredEntries.size()) {
        connectTo(m_filteredEntries[index].id);
    }
}

void QuickConnectPanel::onProtocolChanged(const QString& protocol) {
    setDefaultPortForProtocol(protocol.toLower());
}

void QuickConnectPanel::onConnectClicked() {
    QuickConnectEntry entry;
    entry.protocol = m_protocolBox->currentText().toLower();
    entry.host = m_hostBox->text();
    entry.port = m_portBox->text().toInt();
    entry.username = m_usernameBox->text();
    entry.name = entry.host;
    
    if (!entry.host.isEmpty()) {
        emit connectionRequested(entry);
        hide();
    }
}

void QuickConnectPanel::updateSearchResults() {
    m_resultList->clear();
    
    QString searchText = m_searchBox->text();
    m_filteredEntries = searchEntries(searchText);
    
    for (const QuickConnectEntry& entry : m_filteredEntries) {
        QListWidgetItem* item = new QListWidgetItem();
        
        QString text = QString("%1 (%2:%3)").arg(entry.name, entry.host, QString::number(entry.port));
        if (!entry.username.isEmpty()) {
            text += QString(" - %1").arg(entry.username);
        }
        
        item->setText(text);
        item->setToolTip(QString("Group: %1\nProtocol: %2\nLast used: %3")
            .arg(entry.group, entry.protocol, 
                 entry.lastUsed > 0 ? QDateTime::fromMSecsSinceEpoch(entry.lastUsed).toString() : "Never"));
        
        m_resultList->addItem(item);
    }
}

void QuickConnectPanel::setDefaultPortForProtocol(const QString& protocol) {
    if (protocol == "ssh") {
        m_portBox->setText("22");
    } else if (protocol == "telnet") {
        m_portBox->setText("23");
    } else if (protocol == "serial") {
        m_portBox->setText("");
    }
}

#include "QuickConnectPanel.moc"
