#include "MemoryViewerDialog.h"
#include "MemoryFragment/MemoryFragmentStore.h"
#include "MemoryEditorDialog.h"
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>

MemoryViewerDialog::MemoryViewerDialog(MemoryFragmentStore* store, QWidget* parent)
    : QDialog(parent), m_store(store), m_selectedId(0) {
    setWindowTitle(QStringLiteral("记忆碎片"));
    setMinimumSize(900, 600);
    resize(900, 600);
    
    setupUI();
    
    connect(m_store, &MemoryFragmentStore::fragmentCreated, this, &MemoryViewerDialog::onFragmentCreated);
    connect(m_store, &MemoryFragmentStore::fragmentUpdated, this, &MemoryViewerDialog::onFragmentUpdated);
    connect(m_store, &MemoryFragmentStore::fragmentDeleted, this, &MemoryViewerDialog::onFragmentDeleted);
    
    loadFragments();
}

void MemoryViewerDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel(QStringLiteral("记忆碎片历史"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    
    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText(QStringLiteral("搜索标题、内容或工作目录..."));
    toolbarLayout->addWidget(m_searchBox);
    
    m_newButton = new QPushButton(QStringLiteral("新增"), this);
    toolbarLayout->addWidget(m_newButton);
    
    mainLayout->addLayout(toolbarLayout);
    
    QHBoxLayout* contentLayout = new QHBoxLayout();
    
    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    m_fragmentList = new QListWidget(this);
    m_fragmentList->setMinimumWidth(280);
    leftLayout->addWidget(m_fragmentList);
    
    contentLayout->addWidget(leftPanel, 1);
    
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    QGroupBox* detailGroup = new QGroupBox(QStringLiteral("详细信息"), this);
    QVBoxLayout* detailLayout = new QVBoxLayout(detailGroup);
    
    m_detailView = new QTextEdit(this);
    m_detailView->setReadOnly(true);
    m_detailView->setFont(QFont(QStringLiteral("Monospace"), 11));
    detailLayout->addWidget(m_detailView);
    
    rightLayout->addWidget(detailGroup, 1);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_editButton = new QPushButton(QStringLiteral("编辑"), this);
    m_editButton->setEnabled(false);
    buttonLayout->addWidget(m_editButton);
    
    m_deleteButton = new QPushButton(QStringLiteral("删除"), this);
    m_deleteButton->setEnabled(false);
    buttonLayout->addWidget(m_deleteButton);
    
    m_closeButton = new QPushButton(QStringLiteral("关闭"), this);
    buttonLayout->addWidget(m_closeButton);
    
    rightLayout->addLayout(buttonLayout);
    
    contentLayout->addWidget(rightPanel, 2);
    
    mainLayout->addLayout(contentLayout);
    
    connect(m_fragmentList, &QListWidget::currentItemChanged, this, &MemoryViewerDialog::onFragmentSelected);
    connect(m_searchBox, &QLineEdit::textChanged, this, &MemoryViewerDialog::onSearchTextChanged);
    connect(m_editButton, &QPushButton::clicked, this, &MemoryViewerDialog::onEditClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &MemoryViewerDialog::onDeleteClicked);
    connect(m_newButton, &QPushButton::clicked, this, &MemoryViewerDialog::onNewClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void MemoryViewerDialog::loadFragments() {
    QList<MemoryFragment> fragments = m_store->loadAll();
    
    m_fragmentList->clear();
    
    for (const MemoryFragment& frag : fragments) {
        QListWidgetItem* item = new QListWidgetItem(m_fragmentList);
        item->setData(Qt::UserRole, frag.id);
        
        QString displayText;
        displayText += frag.title.isEmpty() ? QStringLiteral("(无标题)") : frag.title;
        displayText += "\n";
        displayText += frag.createdAt.toString("yyyy-MM-dd hh:mm:ss");
        displayText += " | " + MemoryFragment::sourceTypeLabel(frag.sourceType);
        displayText += " | " + frag.terminalType;
        displayText += "\n";
        displayText += frag.workingDirectory;
        
        item->setText(displayText);
    }
    
    if (m_fragmentList->count() > 0) {
        m_fragmentList->setCurrentRow(0);
    }
}

void MemoryViewerDialog::onFragmentSelected(QListWidgetItem* current) {
    if (!current) {
        m_selectedId = 0;
        m_editButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        clearDisplay();
        return;
    }
    
    m_selectedId = current->data(Qt::UserRole).toLongLong();
    m_editButton->setEnabled(true);
    m_deleteButton->setEnabled(true);
    
    MemoryFragment fragment = m_store->getFragment(m_selectedId);
    if (fragment.isValid()) {
        displayFragment(fragment);
    }
}

void MemoryViewerDialog::displayFragment(const MemoryFragment& fragment) {
    QString html;
    html += "<h3>" + (fragment.title.isEmpty() ? QStringLiteral("(无标题)") : fragment.title.toHtmlEscaped()) + "</h3>";
    html += "<table style='font-size: 12px; color: #888;'>";
    html += "<tr><td>创建时间:</td><td>" + fragment.createdAt.toString("yyyy-MM-dd hh:mm:ss") + "</td></tr>";
    html += "<tr><td>更新时间:</td><td>" + fragment.updatedAt.toString("yyyy-MM-dd hh:mm:ss") + "</td></tr>";
    html += "<tr><td>来源:</td><td>" + MemoryFragment::sourceTypeLabel(fragment.sourceType).toHtmlEscaped() + "</td></tr>";
    if (!fragment.sourceRemark.isEmpty()) {
        html += "<tr><td>备注:</td><td>" + fragment.sourceRemark.toHtmlEscaped() + "</td></tr>";
    }
    html += "<tr><td>终端:</td><td>" + fragment.terminalType.toHtmlEscaped() + "</td></tr>";
    html += "<tr><td>工作目录:</td><td>" + fragment.workingDirectory.toHtmlEscaped() + "</td></tr>";
    html += "</table>";
    html += "<hr/>";
    html += "<pre style='font-family: Monospace, Consolas, monospace; white-space: pre-wrap;'>" + fragment.content.toHtmlEscaped() + "</pre>";
    
    if (!fragment.commandHistory.isEmpty()) {
        html += "<hr/>";
        html += "<h4>最近命令历史</h4>";
        html += "<pre style='font-family: Monospace, Consolas, monospace; white-space: pre-wrap;'>" + fragment.commandHistory.toHtmlEscaped() + "</pre>";
    }
    
    m_detailView->setHtml(html);
}

void MemoryViewerDialog::clearDisplay() {
    m_detailView->clear();
    m_detailView->setHtml("<p style='color: #888;'>请选择一个记忆碎片查看详情</p>");
}

void MemoryViewerDialog::onSearchTextChanged(const QString& text) {
    QList<MemoryFragment> fragments;
    if (text.isEmpty()) {
        fragments = m_store->loadAll();
    } else {
        fragments = m_store->search(text);
    }
    
    m_fragmentList->clear();
    
    for (const MemoryFragment& frag : fragments) {
        QListWidgetItem* item = new QListWidgetItem(m_fragmentList);
        item->setData(Qt::UserRole, frag.id);
        
        QString displayText;
        displayText += frag.title.isEmpty() ? QStringLiteral("(无标题)") : frag.title;
        displayText += "\n";
        displayText += frag.createdAt.toString("yyyy-MM-dd hh:mm:ss");
        displayText += " | " + MemoryFragment::sourceTypeLabel(frag.sourceType);
        displayText += " | " + frag.terminalType;
        displayText += "\n";
        displayText += frag.workingDirectory;
        
        item->setText(displayText);
    }
    
    if (m_fragmentList->count() > 0) {
        m_fragmentList->setCurrentRow(0);
    }
}

void MemoryViewerDialog::onEditClicked() {
    if (m_selectedId <= 0) return;
    
    MemoryFragment fragment = m_store->getFragment(m_selectedId);
    if (!fragment.isValid()) return;
    
    MemoryEditorDialog editor(m_store, fragment, this);
    if (editor.exec() == QDialog::Accepted) {
        MemoryFragment updated = m_store->getFragment(m_selectedId);
        if (updated.isValid()) {
            displayFragment(updated);
            
            for (int i = 0; i < m_fragmentList->count(); i++) {
                QListWidgetItem* item = m_fragmentList->item(i);
                if (item->data(Qt::UserRole).toLongLong() == m_selectedId) {
                    QString displayText;
                    displayText += updated.title.isEmpty() ? QStringLiteral("(无标题)") : updated.title;
                    displayText += "\n";
                    displayText += updated.createdAt.toString("yyyy-MM-dd hh:mm:ss");
                    displayText += " | " + MemoryFragment::sourceTypeLabel(updated.sourceType);
                    displayText += " | " + updated.terminalType;
                    displayText += "\n";
                    displayText += updated.workingDirectory;
                    
                    item->setText(displayText);
                    break;
                }
            }
        }
    }
}

void MemoryViewerDialog::onDeleteClicked() {
    if (m_selectedId <= 0) return;
    
    MemoryFragment fragment = m_store->getFragment(m_selectedId);
    if (!fragment.isValid()) return;
    
    QString title = fragment.title.isEmpty() ? fragment.contentPreview(1) : fragment.title;
    if (title.length() > 50) {
        title = title.left(50) + "...";
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("确认删除"),
        QStringLiteral("确定要删除记忆碎片 '%1' 吗？此操作不可撤销。").arg(title),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_store->deleteFragment(m_selectedId);
        m_selectedId = 0;
        m_editButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        clearDisplay();
    }
}

void MemoryViewerDialog::onNewClicked() {
    MemoryEditorDialog editor(m_store, this);
    if (editor.exec() == QDialog::Accepted) {
        loadFragments();
    }
}

void MemoryViewerDialog::onFragmentCreated(qint64 id) {
    Q_UNUSED(id);
    if (isVisible()) {
        loadFragments();
    }
}

void MemoryViewerDialog::onFragmentUpdated(qint64 id) {
    Q_UNUSED(id);
    if (isVisible()) {
        loadFragments();
    }
}

void MemoryViewerDialog::onFragmentDeleted(qint64 id) {
    Q_UNUSED(id);
    if (isVisible()) {
        loadFragments();
    }
}
