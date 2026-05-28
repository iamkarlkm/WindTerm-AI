#ifndef COMMAND_SEARCH_DIALOG_H
#define COMMAND_SEARCH_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <CommandHistory/CommandHistoryStore.h>

class CommandSearchDialog : public QDialog {
    Q_OBJECT
public:
    explicit CommandSearchDialog(CommandHistoryStore* store, QWidget* parent = nullptr);
    
signals:
    void commandSelected(const QString& command);
    
private slots:
    void onSearchTextChanged(const QString& text);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onReturnPressed();
    void onDeleteSelected();
    
private:
    void setupUI();
    void loadResults();
    
    CommandHistoryStore* m_store;
    QLineEdit* m_searchEdit;
    QListWidget* m_resultsList;
    QLabel* m_statusLabel;
};

#endif
