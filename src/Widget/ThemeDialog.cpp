#include "ThemeDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>

ThemeDialog::ThemeDialog(QWidget* parent)
    : QDialog(parent), m_themeManager(ThemeManager::instance()) {
    setWindowTitle(QStringLiteral("主题设置"));
    setMinimumSize(400, 500);
    
    setupUI();
    loadThemes();
}

void ThemeDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* group = new QGroupBox(QStringLiteral("可用主题"), this);
    QVBoxLayout* groupLayout = new QVBoxLayout(group);
    
    m_themeList = new QListWidget(this);
    m_themeList->setSelectionMode(QAbstractItemView::SingleSelection);
    groupLayout->addWidget(m_themeList);
    
    mainLayout->addWidget(group);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_applyButton = new QPushButton(QStringLiteral("应用"), this);
    m_applyButton->setEnabled(false);
    buttonLayout->addWidget(m_applyButton);
    
    m_saveButton = new QPushButton(QStringLiteral("保存当前"), this);
    buttonLayout->addWidget(m_saveButton);
    
    m_closeButton = new QPushButton(QStringLiteral("关闭"), this);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(m_themeList, &QListWidget::currentItemChanged,
            this, &ThemeDialog::onThemeSelected);
    connect(m_applyButton, &QPushButton::clicked, this, &ThemeDialog::onApplyTheme);
    connect(m_saveButton, &QPushButton::clicked, this, &ThemeDialog::onSaveCustomTheme);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void ThemeDialog::loadThemes() {
    m_themeList->clear();
    
    QVector<ThemeConfig> themes = m_themeManager->availableThemes();
    ThemeConfig currentTheme = m_themeManager->currentTheme();
    
    for (const ThemeConfig& theme : themes) {
        QListWidgetItem* item = new QListWidgetItem(theme.name, m_themeList);
        item->setData(Qt::UserRole, theme.name);
        
        if (theme.name == currentTheme.name) {
            item->setSelected(true);
        }
    }
}

void ThemeDialog::onThemeSelected(QListWidgetItem* current) {
    bool hasSelection = current != nullptr;
    m_applyButton->setEnabled(hasSelection);
}

void ThemeDialog::onApplyTheme() {
    QListWidgetItem* current = m_themeList->currentItem();
    if (!current) return;
    
    QString themeName = current->data(Qt::UserRole).toString();
    QVector<ThemeConfig> themes = m_themeManager->availableThemes();
    
    for (const ThemeConfig& theme : themes) {
        if (theme.name == themeName) {
            m_themeManager->setTheme(theme);
            emit themeSelected(theme);
            break;
        }
    }
}

void ThemeDialog::onSaveCustomTheme() {
    ThemeConfig currentTheme = m_themeManager->currentTheme();
    
    QString name = QInputDialog::getText(this, QStringLiteral("保存主题"),
        QStringLiteral("主题名称:"), QLineEdit::Normal, currentTheme.name + QStringLiteral(" (副本)"));
    
    if (name.isEmpty()) return;
    
    currentTheme.name = name;
    m_themeManager->saveTheme(currentTheme);
    loadThemes();
    
    QMessageBox::information(this, QStringLiteral("保存成功"),
        QStringLiteral("主题 '%1' 已保存。").arg(name));
}
