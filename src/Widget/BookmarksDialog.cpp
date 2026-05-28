#include "BookmarksDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QInputDialog>
#include <QMessageBox>

BookmarksDialog::BookmarksDialog(BookmarksStore* store, QWidget* parent)
    : QDialog(parent), m_store(store) {
    setWindowTitle(QStringLiteral("书签管理"));
    setMinimumSize(600, 450);
    
    setupUI();
    loadBookmarks();
    loadCategories();
    
    m_searchEdit->setFocus();
}

void BookmarksDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QWidget* topWidget = new QWidget(this);
    QHBoxLayout* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    
    m_searchEdit = new QLineEdit(topWidget);
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索书签..."));
    m_searchEdit->setClearButtonEnabled(true);
    topLayout->addWidget(m_searchEdit, 1);
    
    m_categoryCombo = new QComboBox(topWidget);
    m_categoryCombo->addItem(QStringLiteral("全部分类"));
    topLayout->addWidget(m_categoryCombo);
    
    mainLayout->addWidget(topWidget);
    
    m_bookmarkList = new QListWidget(this);
    m_bookmarkList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_bookmarkList);
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: gray; font-size: 11px;");
    mainLayout->addWidget(m_statusLabel);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_addButton = new QPushButton(QStringLiteral("添加"), this);
    buttonLayout->addWidget(m_addButton);
    
    m_editButton = new QPushButton(QStringLiteral("编辑"), this);
    m_editButton->setEnabled(false);
    buttonLayout->addWidget(m_editButton);
    
    m_deleteButton = new QPushButton(QStringLiteral("删除"), this);
    m_deleteButton->setEnabled(false);
    buttonLayout->addWidget(m_deleteButton);
    
    m_jumpButton = new QPushButton(QStringLiteral("跳转"), this);
    m_jumpButton->setEnabled(false);
    buttonLayout->addWidget(m_jumpButton);
    
    QPushButton* cancelButton = new QPushButton(QStringLiteral("关闭"), this);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(m_searchEdit, &QLineEdit::textChanged, this, &BookmarksDialog::onSearchTextChanged);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookmarksDialog::onCategoryChanged);
    connect(m_bookmarkList, &QListWidget::itemDoubleClicked, this, &BookmarksDialog::onItemDoubleClicked);
    connect(m_bookmarkList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
        bool hasSelection = current != nullptr;
        m_editButton->setEnabled(hasSelection);
        m_deleteButton->setEnabled(hasSelection);
        m_jumpButton->setEnabled(hasSelection);
    });
    connect(m_addButton, &QPushButton::clicked, this, &BookmarksDialog::onAddBookmark);
    connect(m_editButton, &QPushButton::clicked, this, &BookmarksDialog::onEditBookmark);
    connect(m_deleteButton, &QPushButton::clicked, this, &BookmarksDialog::onDeleteBookmark);
    connect(m_jumpButton, &QPushButton::clicked, this, &BookmarksDialog::onJumpToBookmark);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::accept);
}

void BookmarksDialog::loadBookmarks() {
    m_bookmarkList->clear();
    
    QVector<BookmarkEntry> bookmarks = m_store->getAllBookmarks();
    
    for (const BookmarkEntry& bookmark : bookmarks) {
        QListWidgetItem* item = new QListWidgetItem(m_bookmarkList);
        item->setData(Qt::UserRole, bookmark.id);
        item->setData(Qt::UserRole + 1, bookmark.path);
        
        QString displayText = bookmark.name;
        if (!bookmark.category.isEmpty()) {
            displayText += QStringLiteral(" [%1]").arg(bookmark.category);
        }
        displayText += "\n" + bookmark.path;
        
        item->setText(displayText);
        item->setToolTip(bookmark.description);
    }
    
    m_statusLabel->setText(QStringLiteral("共 %1 个书签").arg(bookmarks.size()));
}

void BookmarksDialog::loadCategories() {
    QStringList categories = m_store->getCategories();
    
    m_categoryCombo->clear();
    m_categoryCombo->addItem(QStringLiteral("全部分类"));
    m_categoryCombo->addItems(categories);
}

void BookmarksDialog::onSearchTextChanged(const QString& text) {
    m_bookmarkList->clear();
    
    if (text.isEmpty()) {
        loadBookmarks();
        return;
    }
    
    QVector<BookmarkEntry> bookmarks = m_store->search(text);
    
    for (const BookmarkEntry& bookmark : bookmarks) {
        QListWidgetItem* item = new QListWidgetItem(m_bookmarkList);
        item->setData(Qt::UserRole, bookmark.id);
        item->setData(Qt::UserRole + 1, bookmark.path);
        
        QString displayText = bookmark.name;
        if (!bookmark.category.isEmpty()) {
            displayText += QStringLiteral(" [%1]").arg(bookmark.category);
        }
        displayText += "\n" + bookmark.path;
        
        item->setText(displayText);
    }
    
    m_statusLabel->setText(QStringLiteral("找到 %1 条匹配结果").arg(bookmarks.size()));
}

void BookmarksDialog::onItemDoubleClicked(QListWidgetItem* item) {
    QString path = item->data(Qt::UserRole + 1).toString();
    emit bookmarkSelected(path);
    accept();
}

void BookmarksDialog::onAddBookmark() {
    bool ok;
    QString name = QInputDialog::getText(this, QStringLiteral("添加书签"),
        QStringLiteral("书签名称:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.isEmpty()) return;
    
    QString path = QInputDialog::getText(this, QStringLiteral("添加书签"),
        QStringLiteral("路径:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || path.isEmpty()) return;
    
    QString category = QInputDialog::getText(this, QStringLiteral("添加书签"),
        QStringLiteral("分类 (可选):"), QLineEdit::Normal, QString(), &ok);
    
    QString description = QInputDialog::getText(this, QStringLiteral("添加书签"),
        QStringLiteral("描述 (可选):"), QLineEdit::Normal, QString(), &ok);
    
    m_store->addBookmark(name, path, category, description);
    loadBookmarks();
    loadCategories();
    
    m_statusLabel->setText(QStringLiteral("书签 '%1' 已添加").arg(name));
}

void BookmarksDialog::onEditBookmark() {
    QListWidgetItem* item = m_bookmarkList->currentItem();
    if (!item) return;
    
    qint64 id = item->data(Qt::UserRole).toLongLong();
    QString currentPath = item->data(Qt::UserRole + 1).toString();
    
    bool ok;
    QString newName = QInputDialog::getText(this, QStringLiteral("编辑书签"),
        QStringLiteral("书签名称:"), QLineEdit::Normal, item->text().split('\n').first(), &ok);
    if (!ok) return;
    
    QString newPath = QInputDialog::getText(this, QStringLiteral("编辑书签"),
        QStringLiteral("路径:"), QLineEdit::Normal, currentPath, &ok);
    if (!ok) return;
    
    BookmarkEntry entry;
    entry.id = id;
    entry.name = newName;
    entry.path = newPath;
    
    m_store->updateBookmark(entry);
    loadBookmarks();
    
    m_statusLabel->setText(QStringLiteral("书签已更新"));
}

void BookmarksDialog::onDeleteBookmark() {
    QListWidgetItem* item = m_bookmarkList->currentItem();
    if (!item) return;
    
    qint64 id = item->data(Qt::UserRole).toLongLong();
    QString name = item->text().split('\n').first();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除书签 '%1' 吗？").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_store->deleteBookmark(id);
        loadBookmarks();
        loadCategories();
        m_statusLabel->setText(QStringLiteral("书签已删除"));
    }
}

void BookmarksDialog::onJumpToBookmark() {
    QListWidgetItem* item = m_bookmarkList->currentItem();
    if (!item) return;
    
    QString path = item->data(Qt::UserRole + 1).toString();
    emit bookmarkSelected(path);
    accept();
}

void BookmarksDialog::onCategoryChanged(int index) {
    QString category = m_categoryCombo->itemText(index);
    m_bookmarkList->clear();
    
    if (category == QStringLiteral("全部分类")) {
        loadBookmarks();
        return;
    }
    
    QVector<BookmarkEntry> bookmarks = m_store->getBookmarksByCategory(category);
    
    for (const BookmarkEntry& bookmark : bookmarks) {
        QListWidgetItem* item = new QListWidgetItem(m_bookmarkList);
        item->setData(Qt::UserRole, bookmark.id);
        item->setData(Qt::UserRole + 1, bookmark.path);
        
        QString displayText = bookmark.name + "\n" + bookmark.path;
        item->setText(displayText);
    }
    
    m_statusLabel->setText(QStringLiteral("分类 '%1' 共 %2 个书签").arg(category).arg(bookmarks.size()));
}
