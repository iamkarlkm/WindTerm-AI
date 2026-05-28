#include "CommandSearchDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QShortcut>

CommandSearchDialog::CommandSearchDialog(CommandHistoryStore* store, QWidget* parent)
    : QDialog(parent), m_store(store) {
    setWindowTitle(QStringLiteral("命令历史搜索"));
    setMinimumSize(600, 400);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    setupUI();
    
    QShortcut* deleteShortcut = new QShortcut(QKeySequence::Delete, this);
    connect(deleteShortcut, &QShortcut::activated, this, &CommandSearchDialog::onDeleteSelected);
    
    m_searchEdit->setFocus();
    loadResults();
}

void CommandSearchDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("输入命令关键词搜索... (Ctrl+R 搜索历史)"));
    m_searchEdit->setClearButtonEnabled(true);
    mainLayout->addWidget(m_searchEdit);
    
    m_resultsList = new QListWidget(this);
    m_resultsList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_resultsList);
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: gray; font-size: 11px;");
    mainLayout->addWidget(m_statusLabel);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton* deleteButton = new QPushButton(QStringLiteral("删除"), this);
    buttonLayout->addWidget(deleteButton);
    
    QPushButton* selectButton = new QPushButton(QStringLiteral("选择"), this);
    buttonLayout->addWidget(selectButton);
    
    QPushButton* cancelButton = new QPushButton(QStringLiteral("取消"), this);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CommandSearchDialog::onSearchTextChanged);
    connect(m_resultsList, &QListWidget::itemDoubleClicked, this, &CommandSearchDialog::onItemDoubleClicked);
    connect(selectButton, &QPushButton::clicked, this, &CommandSearchDialog::onReturnPressed);
    connect(deleteButton, &QPushButton::clicked, this, &CommandSearchDialog::onDeleteSelected);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_resultsList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) {
            QListWidgetItem* item = m_resultsList->item(row);
            if (item) {
                QString cmd = item->data(Qt::UserRole).toString();
                m_statusLabel->setText(QStringLiteral("按 Enter 选择: ") + cmd);
            }
        }
    });
}

void CommandSearchDialog::loadResults() {
    m_resultsList->clear();
    
    QVector<CommandHistoryEntry> entries = m_store->recent(100);
    
    for (const CommandHistoryEntry& entry : entries) {
        QListWidgetItem* item = new QListWidgetItem(m_resultsList);
        item->setData(Qt::UserRole, entry.command);
        item->setData(Qt::UserRole + 1, entry.id);
        
        QString displayText = entry.command;
        QString meta = QStringLiteral("[%1 次] %2").arg(entry.usageCount).arg(entry.timestamp.toString("yyyy-MM-dd hh:mm"));
        
        item->setText(displayText);
        item->setToolTip(meta);
    }
    
    m_statusLabel->setText(QStringLiteral("共 %1 条历史命令").arg(entries.size()));
}

void CommandSearchDialog::onSearchTextChanged(const QString& text) {
    m_resultsList->clear();
    
    if (text.isEmpty()) {
        loadResults();
        return;
    }
    
    QVector<CommandHistoryEntry> entries = m_store->search(text, 50);
    
    for (const CommandHistoryEntry& entry : entries) {
        QListWidgetItem* item = new QListWidgetItem(m_resultsList);
        item->setData(Qt::UserRole, entry.command);
        item->setData(Qt::UserRole + 1, entry.id);
        
        QString displayText = entry.command;
        QString meta = QStringLiteral("[%1 次] %2").arg(entry.usageCount).arg(entry.timestamp.toString("yyyy-MM-dd hh:mm"));
        
        item->setText(displayText);
        item->setToolTip(meta);
    }
    
    m_statusLabel->setText(QStringLiteral("找到 %1 条匹配结果").arg(entries.size()));
}

void CommandSearchDialog::onItemDoubleClicked(QListWidgetItem* item) {
    QString command = item->data(Qt::UserRole).toString();
    emit commandSelected(command);
    accept();
}

void CommandSearchDialog::onReturnPressed() {
    QListWidgetItem* item = m_resultsList->currentItem();
    if (item) {
        onItemDoubleClicked(item);
    }
}

void CommandSearchDialog::onDeleteSelected() {
    QListWidgetItem* item = m_resultsList->currentItem();
    if (!item) return;
    
    qint64 id = item->data(Qt::UserRole + 1).toLongLong();
    m_store->deleteEntry(id);
    
    int currentRow = m_resultsList->currentRow();
    m_resultsList->takeItem(currentRow);
    
    if (m_resultsList->count() == 0) {
        loadResults();
    }
    
    m_statusLabel->setText(QStringLiteral("已删除"));
}
