#ifndef MEMORY_VIEWER_DIALOG_H
#define MEMORY_VIEWER_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "MemoryFragment/MemoryFragment.h"

class MemoryFragmentStore;

class MemoryViewerDialog : public QDialog {
    Q_OBJECT
public:
    explicit MemoryViewerDialog(MemoryFragmentStore* store, QWidget* parent = nullptr);
    
signals:
    void editRequested(qint64 id);
    void deleteRequested(qint64 id);
    
private slots:
    void onFragmentSelected(QListWidgetItem* current);
    void onSearchTextChanged(const QString& text);
    void onEditClicked();
    void onDeleteClicked();
    void onNewClicked();
    void onFragmentCreated(qint64 id);
    void onFragmentUpdated(qint64 id);
    void onFragmentDeleted(qint64 id);
    
private:
    void setupUI();
    void loadFragments();
    void displayFragment(const MemoryFragment& fragment);
    void clearDisplay();
    
    MemoryFragmentStore* m_store;
    
    QListWidget* m_fragmentList;
    QTextEdit* m_detailView;
    QLineEdit* m_searchBox;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_newButton;
    QPushButton* m_closeButton;
    
    qint64 m_selectedId;
};

#endif
