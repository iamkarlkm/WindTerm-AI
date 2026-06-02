#include "ShortcutManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QDebug>

ShortcutManager* ShortcutManager::s_instance = nullptr;

ShortcutManager::ShortcutManager(QObject* parent)
    : QObject(parent)
    , m_currentContext("global")
    , m_initialized(false) {
    setupDefaultShortcuts();
    m_initialized = true;
}

ShortcutManager* ShortcutManager::instance() {
    if (!s_instance) {
        s_instance = new ShortcutManager();
    }
    return s_instance;
}

void ShortcutManager::setupDefaultShortcuts() {
    // 文件操作
    registerShortcut("file.new_tab", "新建标签页", QKeySequence("Ctrl+T"), "File");
    registerShortcut("file.close_tab", "关闭标签页", QKeySequence("Ctrl+W"), "File");
    registerShortcut("file.new_window", "新建窗口", QKeySequence("Ctrl+N"), "File");
    
    // 终端操作
    registerShortcut("terminal.split_horizontal", "水平分割", QKeySequence("Ctrl+Shift+H"), "Terminal");
    registerShortcut("terminal.split_vertical", "垂直分割", QKeySequence("Ctrl+Shift+V"), "Terminal");
    registerShortcut("terminal.close_pane", "关闭面板", QKeySequence("Ctrl+Shift+Q"), "Terminal");
    registerShortcut("terminal.focus_next", "聚焦下一面板", QKeySequence("Ctrl+Alt+Right"), "Terminal");
    registerShortcut("terminal.focus_prev", "聚焦上一面板", QKeySequence("Ctrl+Alt+Left"), "Terminal");
    registerShortcut("terminal.focus_1", "聚焦面板 1", QKeySequence("Ctrl+Alt+1"), "Terminal");
    registerShortcut("terminal.focus_2", "聚焦面板 2", QKeySequence("Ctrl+Alt+2"), "Terminal");
    registerShortcut("terminal.focus_3", "聚焦面板 3", QKeySequence("Ctrl+Alt+3"), "Terminal");
    registerShortcut("terminal.focus_4", "聚焦面板 4", QKeySequence("Ctrl+Alt+4"), "Terminal");
    
    // 编辑操作
    registerShortcut("edit.copy", "复制", QKeySequence::Copy, "Edit");
    registerShortcut("edit.paste", "粘贴", QKeySequence::Paste, "Edit");
    registerShortcut("edit.select_all", "全选", QKeySequence::SelectAll, "Edit");
    registerShortcut("edit.clear", "清屏", QKeySequence("Ctrl+L"), "Edit");
    registerShortcut("edit.reset_terminal", "重置终端", QKeySequence("Ctrl+Shift+R"), "Edit");
    
    // 搜索
    registerShortcut("search.find", "查找", QKeySequence::Find, "Search");
    registerShortcut("search.find_next", "查找下一个", QKeySequence("Ctrl+Shift+N"), "Search");
    registerShortcut("search.find_prev", "查找上一个", QKeySequence("Ctrl+Shift+P"), "Search");
    
    // 视图
    registerShortcut("view.zoom_in", "放大", QKeySequence::ZoomIn, "View");
    registerShortcut("view.zoom_out", "缩小", QKeySequence::ZoomOut, "View");
    registerShortcut("view.zoom_reset", "重置缩放", QKeySequence("Ctrl+0"), "View");
    registerShortcut("view.fullscreen", "全屏", QKeySequence("F11"), "View");
    registerShortcut("view.toggle_sidebar", "切换侧边栏", QKeySequence("Ctrl+B"), "View");
    
    // SSH 连接
    registerShortcut("ssh.connect", "SSH 连接", QKeySequence("Ctrl+Shift+S"), "SSH");
    registerShortcut("ssh.quick_connect", "快速连接", QKeySequence("Ctrl+Shift+K"), "SSH");
    registerShortcut("ssh.bookmarks", "书签", QKeySequence("Ctrl+Shift+B"), "SSH");
    registerShortcut("ssh.session_manager", "会话管理", QKeySequence("Ctrl+Shift+M"), "SSH");
    
    // AI 功能
    registerShortcut("ai.assistant", "AI 助手", QKeySequence("Ctrl+Shift+A"), "AI");
    registerShortcut("ai.explain", "AI 解释", QKeySequence("Ctrl+Shift+E"), "AI");
    registerShortcut("ai.generate", "AI 生成命令", QKeySequence("Ctrl+Shift+G"), "AI");
    
    // 文件传输
    registerShortcut("transfer.scp", "SCP 传输", QKeySequence("Ctrl+Shift+F"), "Transfer");
    registerShortcut("transfer.sftp", "SFTP 浏览", QKeySequence("Ctrl+Shift+P"), "Transfer");
    
    // 主题
    registerShortcut("theme.switch", "切换主题", QKeySequence("Ctrl+Shift+T"), "Theme");
    registerShortcut("theme.dark", "深色主题", QKeySequence("Ctrl+Shift+D"), "Theme");
    registerShortcut("theme.light", "浅色主题", QKeySequence("Ctrl+Shift+L"), "Theme");
    
    // 录制
    registerShortcut("recording.start", "开始录制", QKeySequence("Ctrl+Alt+R"), "Recording");
    registerShortcut("recording.stop", "停止录制", QKeySequence("Ctrl+Alt+S"), "Recording");
    registerShortcut("recording.playback", "播放录制", QKeySequence("Ctrl+Alt+P"), "Recording");
    
    // 插件
    registerShortcut("plugin.manager", "插件管理", QKeySequence("Ctrl+Shift+P"), "Plugin");
}

void ShortcutManager::registerShortcut(const QString& id, const QString& description,
                                       const QKeySequence& defaultKey, const QString& category) {
    ShortcutEntry entry;
    entry.id = id;
    entry.description = description;
    entry.defaultKey = defaultKey;
    entry.currentKey = defaultKey;
    entry.category = category;
    entry.enabled = true;
    
    m_shortcuts[id] = entry;
    m_keyBindings[defaultKey] = id;
}

void ShortcutManager::unregisterShortcut(const QString& id) {
    if (m_shortcuts.contains(id)) {
        ShortcutEntry entry = m_shortcuts[id];
        m_keyBindings.remove(entry.currentKey);
        m_shortcuts.remove(id);
    }
}

QKeySequence ShortcutManager::getShortcut(const QString& id) const {
    if (m_shortcuts.contains(id)) {
        return m_shortcuts[id].currentKey;
    }
    return QKeySequence();
}

ShortcutEntry ShortcutManager::getShortcutEntry(const QString& id) const {
    return m_shortcuts.value(id);
}

QMap<QString, ShortcutEntry> ShortcutManager::getAllShortcuts() const {
    return m_shortcuts;
}

QMap<QString, ShortcutEntry> ShortcutManager::getShortcutsByCategory(const QString& category) const {
    QMap<QString, ShortcutEntry> result;
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        if (it->category == category) {
            result[it.key()] = it.value();
        }
    }
    return result;
}

QStringList ShortcutManager::getCategories() const {
    QStringList categories;
    QSet<QString> catSet;
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        catSet.insert(it->category);
    }
    categories = catSet.values();
    categories.sort();
    return categories;
}

void ShortcutManager::bindShortcut(const QString& id, QAction* action) {
    if (m_shortcuts.contains(id) && action) {
        action->setShortcut(m_shortcuts[id].currentKey);
        m_actionBindings[id] = action;
    }
}

void ShortcutManager::bindShortcut(const QString& id, const std::function<void()>& callback) {
    if (m_shortcuts.contains(id)) {
        m_callbackBindings[id] = callback;
    }
}

void ShortcutManager::setShortcut(const QString& id, const QKeySequence& keySequence) {
    if (!m_shortcuts.contains(id)) return;
    
    // 检查冲突
    if (hasConflict(keySequence)) {
        QString conflictId = findConflictingShortcut(keySequence);
        emit conflictDetected(id, conflictId);
    }
    
    // 移除旧绑定
    ShortcutEntry& entry = m_shortcuts[id];
    m_keyBindings.remove(entry.currentKey);
    
    // 设置新快捷键
    entry.currentKey = keySequence;
    m_keyBindings[keySequence] = id;
    
    // 更新绑定的 QAction
    if (m_actionBindings.contains(id)) {
        m_actionBindings[id]->setShortcut(keySequence);
    }
    
    emit shortcutChanged(id);
}

void ShortcutManager::resetShortcut(const QString& id) {
    if (m_shortcuts.contains(id)) {
        setShortcut(id, m_shortcuts[id].defaultKey);
    }
}

void ShortcutManager::resetAllShortcuts() {
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        setShortcut(it.key(), it->defaultKey);
    }
}

void ShortcutManager::exportShortcuts(const QString& filePath) {
    QJsonObject json;
    QJsonObject shortcutsJson;
    
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        QJsonObject entryJson;
        entryJson["description"] = it->description;
        entryJson["currentKey"] = it->currentKey.toString();
        entryJson["defaultKey"] = it->defaultKey.toString();
        entryJson["category"] = it->category;
        entryJson["enabled"] = it->enabled;
        shortcutsJson[it.key()] = entryJson;
    }
    
    json["shortcuts"] = shortcutsJson;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
}

void ShortcutManager::importShortcuts(const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject json = doc.object();
        QJsonObject shortcutsJson = json["shortcuts"].toObject();
        
        for (auto it = shortcutsJson.begin(); it != shortcutsJson.end(); ++it) {
            QString id = it.key();
            QJsonObject entryJson = it->toObject();
            
            if (m_shortcuts.contains(id)) {
                QString keyStr = entryJson["currentKey"].toString();
                QKeySequence keySequence(keyStr);
                setShortcut(id, keySequence);
            }
        }
    }
}

void ShortcutManager::loadPreset(const QString& presetName) {
    if (presetName == "default") {
        resetAllShortcuts();
    } else if (presetName == "vim") {
        setupVimShortcuts();
    } else if (presetName == "emacs") {
        setupEmacsShortcuts();
    } else if (presetName == "vscode") {
        setupVscodeShortcuts();
    }
}

bool ShortcutManager::hasConflict(const QKeySequence& keySequence) const {
    return m_keyBindings.contains(keySequence);
}

QString ShortcutManager::findConflictingShortcut(const QKeySequence& keySequence) const {
    if (m_keyBindings.contains(keySequence)) {
        return m_keyBindings[keySequence];
    }
    return QString();
}

void ShortcutManager::setContext(const QString& context) {
    m_currentContext = context;
}

void ShortcutManager::setupVimShortcuts() {
    // Vim 风格快捷键覆盖
    setShortcut("terminal.split_horizontal", QKeySequence("Ctrl+H"));
    setShortcut("terminal.split_vertical", QKeySequence("Ctrl+V"));
    setShortcut("terminal.focus_next", QKeySequence("Ctrl+J"));
    setShortcut("terminal.focus_prev", QKeySequence("Ctrl+K"));
}

void ShortcutManager::setupEmacsShortcuts() {
    // Emacs 风格快捷键覆盖
    setShortcut("edit.copy", QKeySequence("Ctrl+Shift+W"));
    setShortcut("edit.paste", QKeySequence("Ctrl+Y"));
    setShortcut("edit.select_all", QKeySequence("Ctrl+Space"));
}

void ShortcutManager::setupVscodeShortcuts() {
    // VS Code 风格快捷键覆盖
    setShortcut("view.fullscreen", QKeySequence("Ctrl+K Ctrl+F"));
    setShortcut("search.find", QKeySequence("Ctrl+F"));
    setShortcut("terminal.split_horizontal", QKeySequence("Ctrl+Shift+5"));
}

#include "ShortcutManager.moc"
