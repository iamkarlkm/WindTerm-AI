#ifndef QUICK_CONNECT_PANEL_H
#define QUICK_CONNECT_PANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>

struct QuickConnectEntry {
    QString id;
    QString name;
    QString host;
    int port;
    QString username;
    QString protocol;  // ssh, telnet, serial
    QString group;
    int usageCount;
    qint64 lastUsed;
    
    QuickConnectEntry() : port(22), usageCount(0), lastUsed(0), protocol("ssh") {}
};

class QuickConnectPanel : public QWidget {
    Q_OBJECT
public:
    explicit QuickConnectPanel(QWidget* parent = nullptr);
    
    static QuickConnectPanel* instance();
    
    // 连接条目管理
    QString addEntry(const QuickConnectEntry& entry);
    void updateEntry(const QString& id, const QuickConnectEntry& entry);
    void deleteEntry(const QString& id);
    
    // 查询
    QuickConnectEntry getEntry(const QString& id) const;
    QList<QuickConnectEntry> getAllEntries() const;
    QList<QuickConnectEntry> searchEntries(const QString& query) const;
    QList<QuickConnectEntry> getEntriesByGroup(const QString& group) const;
    QStringList getGroups() const;
    
    // UI 控制
    void show();
    void hide();
    void toggle();
    bool isVisible() const;
    
    // 快速连接
    void connectTo(const QString& id);
    void quickConnect(const QString& host, int port = 22, const QString& username = "");
    
    // 使用统计
    void recordUsage(const QString& id);
    
    // 导入导出
    void exportEntries(const QString& filePath);
    void importEntries(const QString& filePath);
    
    // 预设
    void loadPresets();
    
signals:
    void connectionRequested(const QuickConnectEntry& entry);
    void panelShown();
    void panelHidden();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onEntryDoubleClicked(QListWidgetItem* item);
    void onProtocolChanged(const QString& protocol);
    void onConnectClicked();

private:
    void setupUI();
    void updateSearchResults();
    void setDefaultPortForProtocol(const QString& protocol);
    
    static QuickConnectPanel* s_instance;
    
    QLineEdit* m_searchBox;
    QListWidget* m_resultList;
    QComboBox* m_protocolBox;
    QLineEdit* m_hostBox;
    QLineEdit* m_portBox;
    QLineEdit* m_usernameBox;
    
    QMap<QString, QuickConnectEntry> m_entries;
    QList<QuickConnectEntry> m_filteredEntries;
    
    bool m_showPresets;
};

#endif
