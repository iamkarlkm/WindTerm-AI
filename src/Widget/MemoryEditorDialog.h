#ifndef MEMORY_EDITOR_DIALOG_H
#define MEMORY_EDITOR_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "MemoryFragment/MemoryFragment.h"

class MemoryFragmentStore;

class MemoryEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit MemoryEditorDialog(MemoryFragmentStore* store, QWidget* parent = nullptr);
    explicit MemoryEditorDialog(MemoryFragmentStore* store, const QString& initialContent, QWidget* parent = nullptr);
    explicit MemoryEditorDialog(MemoryFragmentStore* store, const MemoryFragment& fragment, QWidget* parent = nullptr);
    
signals:
    void fragmentSaved(qint64 id);
    
private slots:
    void onSaveClicked();
    
private:
    void setupUI();
    void setEditMode(bool editing);
    
    MemoryFragmentStore* m_store;
    MemoryFragment m_fragment;
    bool m_isEditing;
    
    QLineEdit* m_titleEdit;
    QTextEdit* m_contentEdit;
    QLineEdit* m_terminalTypeEdit;
    QLineEdit* m_workingDirEdit;
    QTextEdit* m_sourceRemarkEdit;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

#endif
