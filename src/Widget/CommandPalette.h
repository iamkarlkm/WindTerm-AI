#ifndef COMMAND_PALETTE_H
#define COMMAND_PALETTE_H

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QTimer>

struct CommandEntry {
    QString id;
    QString name;
    QString description;
    QString category;
    QString shortcut;
    std::function<void()> callback;
    int usageCount;
    int lastUsedTime;
    
    CommandEntry() : usageCount(0), lastUsedTime(0) {}
};

class CommandPalette : public QWidget {
    Q_OBJECT
public:
    explicit CommandPalette(QWidget* parent = nullptr);
    
    static CommandPalette* instance();
    
    // 命令注册
    void registerCommand(const QString& id, const QString& name, 
                        const std::function<void()>& callback,
                        const QString& description = "",
                        const QString& category = "General",
                        const QString& shortcut = "");
    
    void unregisterCommand(const QString& id);
    
    // 命令查询
    CommandEntry getCommand(const QString& id) const;
    QList<CommandEntry> getAllCommands() const;
    QList<CommandEntry> searchCommands(const QString& query) const;
    QList<CommandEntry> getCommandsByCategory(const QString& category) const;
    QStringList getCategories() const;
    
    // 显示/隐藏
    void show();
    void hide();
    void toggle();
    bool isVisible() const;
    
    // 执行命令
    void executeCommand(const QString& id);
    
    // 使用统计
    void recordUsage(const QString& id);
    
    // 导入导出
    void exportCommands(const QString& filePath);
    void importCommands(const QString& filePath);
    
signals:
    void commandExecuted(const QString& id);
    void paletteShown();
    void paletteHidden();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onItemClicked(QListWidgetItem* item);
    void hideWithDelay();

private:
    void setupUI();
    void updateSearchResults();
    void selectAndExecuteCurrent();
    
    static CommandPalette* s_instance;
    
    QLineEdit* m_searchBox;
    QListWidget* m_resultList;
    QTimer* m_hideTimer;
    
    QMap<QString, CommandEntry> m_commands;
    QList<CommandEntry> m_filteredCommands;
    
    bool m_selectOnShow;
};

#endif
