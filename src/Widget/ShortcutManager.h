#ifndef SHORTCUT_MANAGER_H
#define SHORTCUT_MANAGER_H

#include <QObject>
#include <QMap>
#include <QKeySequence>
#include <QAction>

struct ShortcutEntry {
    QString id;
    QString description;
    QKeySequence defaultKey;
    QKeySequence currentKey;
    QString category;
    bool enabled;
    
    ShortcutEntry() : enabled(true) {}
};

class ShortcutManager : public QObject {
    Q_OBJECT
public:
    explicit ShortcutManager(QObject* parent = nullptr);
    
    static ShortcutManager* instance();
    
    // 快捷键注册
    void registerShortcut(const QString& id, const QString& description, 
                         const QKeySequence& defaultKey, const QString& category = "General");
    void unregisterShortcut(const QString& id);
    
    // 快捷键查询
    QKeySequence getShortcut(const QString& id) const;
    ShortcutEntry getShortcutEntry(const QString& id) const;
    QMap<QString, ShortcutEntry> getAllShortcuts() const;
    QMap<QString, ShortcutEntry> getShortcutsByCategory(const QString& category) const;
    QStringList getCategories() const;
    
    // 快捷键绑定
    void bindShortcut(const QString& id, QAction* action);
    void bindShortcut(const QString& id, const std::function<void()>& callback);
    
    // 快捷键修改
    void setShortcut(const QString& id, const QKeySequence& keySequence);
    void resetShortcut(const QString& id);
    void resetAllShortcuts();
    
    // 导入导出
    void exportShortcuts(const QString& filePath);
    void importShortcuts(const QString& filePath);
    
    // 预设配置
    void loadPreset(const QString& presetName);  // "default", "vim", "emacs", "vscode"
    
    // 冲突检测
    bool hasConflict(const QKeySequence& keySequence) const;
    QString findConflictingShortcut(const QKeySequence& keySequence) const;
    
    // 上下文管理
    void setContext(const QString& context);  // "global", "terminal", "editor"
    QString currentContext() const { return m_currentContext; }
    
signals:
    void shortcutChanged(const QString& id);
    void conflictDetected(const QString& id1, const QString& id2);

private:
    void setupDefaultShortcuts();
    void setupVimShortcuts();
    void setupEmacsShortcuts();
    void setupVscodeShortcuts();
    
    static ShortcutManager* s_instance;
    
    QMap<QString, ShortcutEntry> m_shortcuts;
    QMap<QString, QAction*> m_actionBindings;
    QMap<QString, std::function<void()>> m_callbackBindings;
    QMap<QKeySequence, QString> m_keyBindings;
    
    QString m_currentContext;
    bool m_initialized;
};

#endif
