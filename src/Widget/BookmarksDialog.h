#ifndef BOOKMARKS_DIALOG_H
#define BOOKMARKS_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <Bookmarks/BookmarksStore.h>

class BookmarksDialog : public QDialog {
    Q_OBJECT
public:
    explicit BookmarksDialog(BookmarksStore* store, QWidget* parent = nullptr);
    
signals:
    void bookmarkSelected(const QString& path);
    
private slots:
    void onSearchTextChanged(const QString& text);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onAddBookmark();
    void onEditBookmark();
    void onDeleteBookmark();
    void onJumpToBookmark();
    void onCategoryChanged(int index);
    
private:
    void setupUI();
    void loadBookmarks();
    void loadCategories();
    
    BookmarksStore* m_store;
    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QListWidget* m_bookmarkList;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_jumpButton;
    QLabel* m_statusLabel;
};

#endif
