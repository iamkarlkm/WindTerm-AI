#include "MemoryEditorDialog.h"
#include "MemoryFragment/MemoryFragmentStore.h"
#include "MemoryFragment/MemoryFragment.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>

MemoryEditorDialog::MemoryEditorDialog(MemoryFragmentStore* store, QWidget* parent)
    : QDialog(parent), m_store(store), m_isEditing(false) {
    setWindowTitle(QStringLiteral("新增记忆碎片"));
    setMinimumSize(700, 500);
    
    m_fragment.sourceType = "manual";
    
    setupUI();
}

MemoryEditorDialog::MemoryEditorDialog(MemoryFragmentStore* store, const QString& initialContent, QWidget* parent)
    : QDialog(parent), m_store(store), m_isEditing(false) {
    setWindowTitle(QStringLiteral("剪贴板粘贴为记忆碎片"));
    setMinimumSize(700, 500);
    
    m_fragment.sourceType = "clipboard";
    m_fragment.content = initialContent;
    
    MemoryFragmentContext context = MemoryFragmentContext::current();
    m_fragment.terminalType = context.terminalType;
    m_fragment.workingDirectory = context.workingDirectory;
    m_fragment.sessionId = context.sessionId;
    
    setupUI();
}

MemoryEditorDialog::MemoryEditorDialog(MemoryFragmentStore* store, const MemoryFragment& fragment, QWidget* parent)
    : QDialog(parent), m_store(store), m_isEditing(true), m_fragment(fragment) {
    setWindowTitle(QStringLiteral("编辑记忆碎片"));
    setMinimumSize(700, 500);
    
    setupUI();
    setEditMode(true);
}

void MemoryEditorDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* infoGroup = new QGroupBox(QStringLiteral("基本信息"), this);
    QFormLayout* infoLayout = new QFormLayout(infoGroup);
    
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(QStringLiteral("输入标题（可选）"));
    infoLayout->addRow(QStringLiteral("标题:"), m_titleEdit);
    
    m_contentEdit = new QTextEdit(this);
    m_contentEdit->setFont(QFont(QStringLiteral("Monospace"), 11));
    m_contentEdit->setMinimumHeight(200);
    infoLayout->addRow(QStringLiteral("内容:"), m_contentEdit);
    
    m_sourceRemarkEdit = new QTextEdit(this);
    m_sourceRemarkEdit->setMaximumHeight(60);
    m_sourceRemarkEdit->setPlaceholderText(QStringLiteral("备注来源或其他说明信息"));
    infoLayout->addRow(QStringLiteral("备注:"), m_sourceRemarkEdit);
    
    mainLayout->addWidget(infoGroup);
    
    QGroupBox* contextGroup = new QGroupBox(QStringLiteral("上下文信息"), this);
    QFormLayout* contextLayout = new QFormLayout(contextGroup);
    
    m_terminalTypeEdit = new QLineEdit(this);
    m_terminalTypeEdit->setPlaceholderText(QStringLiteral("如: bash, zsh, cmd, powershell"));
    contextLayout->addRow(QStringLiteral("终端类型:"), m_terminalTypeEdit);
    
    m_workingDirEdit = new QLineEdit(this);
    m_workingDirEdit->setPlaceholderText(QStringLiteral("工作目录路径"));
    contextLayout->addRow(QStringLiteral("工作目录:"), m_workingDirEdit);
    
    mainLayout->addWidget(contextGroup);
    
    if (m_isEditing) {
        m_titleEdit->setText(m_fragment.title);
        m_contentEdit->setText(m_fragment.content);
        m_sourceRemarkEdit->setText(m_fragment.sourceRemark);
        m_terminalTypeEdit->setText(m_fragment.terminalType);
        m_workingDirEdit->setText(m_fragment.workingDirectory);
    } else {
        m_contentEdit->setText(m_fragment.content);
        
        MemoryFragmentContext context = MemoryFragmentContext::current();
        m_terminalTypeEdit->setText(m_fragment.terminalType.isEmpty() ? context.terminalType : m_fragment.terminalType);
        m_workingDirEdit->setText(m_fragment.workingDirectory.isEmpty() ? context.workingDirectory : m_fragment.workingDirectory);
    }
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_saveButton = new QPushButton(QStringLiteral("保存"), this);
    m_saveButton->setDefault(true);
    buttonLayout->addWidget(m_saveButton);
    
    m_cancelButton = new QPushButton(QStringLiteral("取消"), this);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(m_saveButton, &QPushButton::clicked, this, &MemoryEditorDialog::onSaveClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void MemoryEditorDialog::setEditMode(bool editing) {
    m_isEditing = editing;
}

void MemoryEditorDialog::onSaveClicked() {
    QString content = m_contentEdit->toPlainText().trimmed();
    if (content.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("验证错误"), QStringLiteral("内容不能为空。"));
        m_contentEdit->setFocus();
        return;
    }
    
    m_fragment.title = m_titleEdit->text().trimmed();
    m_fragment.content = content;
    m_fragment.sourceRemark = m_sourceRemarkEdit->toPlainText().trimmed();
    m_fragment.terminalType = m_terminalTypeEdit->text().trimmed();
    m_fragment.workingDirectory = m_workingDirEdit->text().trimmed();
    
    qint64 id;
    if (m_isEditing) {
        m_fragment.updatedAt = QDateTime::currentDateTime();
        bool success = m_store->updateFragment(m_fragment);
        if (!success) {
            QMessageBox::critical(this, QStringLiteral("保存失败"), QStringLiteral("更新记忆碎片失败，请检查数据库状态。"));
            return;
        }
        id = m_fragment.id;
    } else {
        m_fragment.createdAt = QDateTime::currentDateTime();
        m_fragment.updatedAt = m_fragment.createdAt;
        id = m_store->createFragment(m_fragment);
        if (id < 0) {
            QMessageBox::critical(this, QStringLiteral("保存失败"), QStringLiteral("创建记忆碎片失败，请检查数据库状态。"));
            return;
        }
    }
    
    emit fragmentSaved(id);
    accept();
}
