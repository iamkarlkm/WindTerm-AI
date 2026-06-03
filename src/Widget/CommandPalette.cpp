#include "CommandPalette.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <algorithm>

CommandPalette* CommandPalette::s_instance = nullptr;

CommandPalette::CommandPalette(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
    , m_searchBox(new QLineEdit())
    , m_resultList(new QListWidget())
    , m_hideTimer(new QTimer(this))
    , m_selectOnShow(true) {
    
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    
    setupUI();
    
    connect(m_searchBox, &QLineEdit::textChanged, this, &CommandPalette::onSearchTextChanged);
    connect(m_resultList, &QListWidget::itemDoubleClicked, this, &CommandPalette::onItemDoubleClicked);
    connect(m_resultList, &QListWidget::itemClicked, this, &CommandPalette::onItemClicked);
    connect(m_hideTimer, &QTimer::timeout, this, &CommandPalette::hideWithDelay);
}

CommandPalette* CommandPalette::instance() {
    if (!s_instance) {
        s_instance = new CommandPalette();
    }
    return s_instance;
}

void CommandPalette::setupUI() {
    setMinimumSize(600, 400);
    
    // 设置搜索框样式
    m_searchBox->setPlaceholderText("Type a command...");
    m_searchBox->setMinimumHeight(40);
    m_searchBox->setStyleSheet(
        "QLineEdit {"
        "    font-size: 16px;"
        "    padding: 10px;"
        "    border: 2px solid #3daee9;"
        "    border-radius: 5px;"
        "}"
    );
    
    // 设置结果列表样式
    m_resultList->setMinimumHeight(300);
    m_resultList->setStyleSheet(
        "QListWidget {"
        "    font-size: 14px;"
        "    border: 1px solid #ccc;"
        "    border-radius: 5px;"
        "}"
        "QListWidget::item {"
        "    padding: 8px;"
        "    border-bottom: 1px solid #eee;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #3daee9;"
        "    color: white;"
        "}"
    );
    
    // 布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);
    layout->addWidget(m_searchBox);
    layout->addWidget(m_resultList);
    
    setLayout(layout);
}

void CommandPalette::registerCommand(const QString& id, const QString& name,
                                     const std::function<void()>& callback,
                                     const QString& description,
                                     const QString& category,
                                     const QString& shortcut) {
    CommandEntry entry;
    entry.id = id;
    entry.name = name;
    entry.description = description;
    entry.category = category;
    entry.shortcut = shortcut;
    entry.callback = callback;
    entry.usageCount = 0;
    entry.lastUsedTime = 0;
    
    m_commands[id] = entry;
}

void CommandPalette::unregisterCommand(const QString& id) {
    m_commands.remove(id);
}

CommandEntry CommandPalette::getCommand(const QString& id) const {
    return m_commands.value(id);
}

QList<CommandEntry> CommandPalette::getAllCommands() const {
    return m_commands.values();
}

QList<CommandEntry> CommandPalette::searchCommands(const QString& query) const {
    QList<CommandEntry> results;
    QString lowerQuery = query.toLower();
    
    for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
        const CommandEntry& entry = it.value();
        
        bool match = entry.name.toLower().contains(lowerQuery) ||
                    entry.description.toLower().contains(lowerQuery) ||
                    entry.id.toLower().contains(lowerQuery) ||
                    entry.category.toLower().contains(lowerQuery);
        
        if (match) {
            results.append(entry);
        }
    }
    
    // 按使用频率和相关性排序
    std::sort(results.begin(), results.end(), [](const CommandEntry& a, const CommandEntry& b) {
        return a.usageCount > b.usageCount;
    });
    
    return results;
}

QList<CommandEntry> CommandPalette::getCommandsByCategory(const QString& category) const {
    QList<CommandEntry> results;
    for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
        if (it->category == category) {
            results.append(it.value());
        }
    }
    return results;
}

QStringList CommandPalette::getCategories() const {
    QStringList categories;
    QSet<QString> catSet;
    for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
        catSet.insert(it->category);
    }
    categories = catSet.values();
    categories.sort();
    return categories;
}

void CommandPalette::show() {
    // 居中显示
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screenGeometry = desktop->availableGeometry();
    
    int x = screenGeometry.x() + (screenGeometry.width() - width()) / 2;
    int y = screenGeometry.y() + (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // 清空搜索框并聚焦
    m_searchBox->clear();
    m_searchBox->setFocus();
    
    // 显示所有命令
    updateSearchResults();
    
    QWidget::show();
    emit paletteShown();
}

void CommandPalette::hide() {
    m_searchBox->clear();
    QWidget::hide();
    emit paletteHidden();
}

void CommandPalette::toggle() {
    if (isVisible()) {
        hide();
    } else {
        show();
    }
}

bool CommandPalette::isVisible() const {
    return QWidget::isVisible();
}

void CommandPalette::executeCommand(const QString& id) {
    if (m_commands.contains(id)) {
        CommandEntry& entry = m_commands[id];
        recordUsage(id);
        
        if (entry.callback) {
            entry.callback();
        }
        
        emit commandExecuted(id);
        hide();
    }
}

void CommandPalette::recordUsage(const QString& id) {
    if (m_commands.contains(id)) {
        m_commands[id].usageCount++;
        m_commands[id].lastUsedTime = QDateTime::currentMSecsSinceEpoch();
    }
}

void CommandPalette::exportCommands(const QString& filePath) {
    QJsonObject json;
    QJsonArray commandsJson;
    
    for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
        QJsonObject cmdJson;
        cmdJson["id"] = it.key();
        cmdJson["name"] = it->name;
        cmdJson["description"] = it->description;
        cmdJson["category"] = it->category;
        cmdJson["shortcut"] = it->shortcut;
        commandsJson.append(cmdJson);
    }
    
    json["commands"] = commandsJson;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
}

void CommandPalette::importCommands(const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject json = doc.object();
        QJsonArray commandsJson = json["commands"].toArray();
        
        for (const QJsonValue& cmdValue : commandsJson) {
            QJsonObject cmdJson = cmdValue.toObject();
            QString id = cmdJson["id"].toString();
            
            if (!m_commands.contains(id)) {
                CommandEntry entry;
                entry.id = id;
                entry.name = cmdJson["name"].toString();
                entry.description = cmdJson["description"].toString();
                entry.category = cmdJson["category"].toString();
                entry.shortcut = cmdJson["shortcut"].toString();
                
                m_commands[id] = entry;
            }
        }
    }
}

void CommandPalette::keyPressEvent(QKeyEvent* event) {
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
            selectAndExecuteCurrent();
            break;
            
        case Qt::Key_Escape:
            hide();
            break;
            
        default:
            QWidget::keyPressEvent(event);
    }
}

void CommandPalette::focusOutEvent(QFocusEvent* event) {
    // 失去焦点时延迟隐藏，允许点击列表项
    m_hideTimer->start(200);
    QWidget::focusOutEvent(event);
}

void CommandPalette::onSearchTextChanged(const QString& text) {
    updateSearchResults();
}

void CommandPalette::onItemDoubleClicked(QListWidgetItem* item) {
    int index = m_resultList->row(item);
    if (index >= 0 && index < m_filteredCommands.size()) {
        executeCommand(m_filteredCommands[index].id);
    }
}

void CommandPalette::onItemClicked(QListWidgetItem* item) {
    // 单击时选中但不执行
    m_resultList->setCurrentItem(item);
}

void CommandPalette::hideWithDelay() {
    m_hideTimer->stop();
    if (!m_resultList->underMouse() && !m_searchBox->hasFocus()) {
        hide();
    }
}

void CommandPalette::updateSearchResults() {
    m_resultList->clear();
    
    QString searchText = m_searchBox->text();
    m_filteredCommands = searchCommands(searchText);
    
    for (const CommandEntry& cmd : m_filteredCommands) {
        QListWidgetItem* item = new QListWidgetItem();
        
        QString text = cmd.name;
        if (!cmd.shortcut.isEmpty()) {
            text += QString("  [%1]").arg(cmd.shortcut);
        }
        item->setText(text);
        
        if (!cmd.description.isEmpty()) {
            item->setToolTip(cmd.description);
        }
        
        m_resultList->addItem(item);
    }
    
    // 自动选中第一项
    if (m_resultList->count() > 0 && m_selectOnShow) {
        m_resultList->setCurrentRow(0);
    }
}

void CommandPalette::selectAndExecuteCurrent() {
    QListWidgetItem* currentItem = m_resultList->currentItem();
    if (currentItem) {
        int index = m_resultList->row(currentItem);
        if (index >= 0 && index < m_filteredCommands.size()) {
            executeCommand(m_filteredCommands[index].id);
        }
    }
}

#include "CommandPalette.moc"
