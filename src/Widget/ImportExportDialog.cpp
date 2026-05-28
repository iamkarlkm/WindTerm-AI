#include "ImportExportDialog.h"
#include "Settings/SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QJsonDocument>

ImportExportDialog::ImportExportDialog(QWidget* parent)
    : QDialog(parent), m_currentMode(Mode::Export) {
    setWindowTitle(QStringLiteral("导入/导出设置"));
    setMinimumSize(500, 550);
    
    setupUI();
}

void ImportExportDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* modeGroup = new QGroupBox(QStringLiteral("操作模式"), this);
    QHBoxLayout* modeLayout = new QHBoxLayout(modeGroup);
    
    m_exportRadio = new QRadioButton(QStringLiteral("导出设置"), modeGroup);
    m_exportRadio->setChecked(true);
    modeLayout->addWidget(m_exportRadio);
    
    m_importRadio = new QRadioButton(QStringLiteral("导入设置"), modeGroup);
    modeLayout->addWidget(m_importRadio);
    
    mainLayout->addWidget(modeGroup);
    
    QGroupBox* dataGroup = new QGroupBox(QStringLiteral("数据类型"), this);
    QVBoxLayout* dataLayout = new QVBoxLayout(dataGroup);
    
    m_themesCheck = new QCheckBox(QStringLiteral("主题配置"), dataGroup);
    m_themesCheck->setChecked(true);
    dataLayout->addWidget(m_themesCheck);
    
    m_bookmarksCheck = new QCheckBox(QStringLiteral("书签"), dataGroup);
    m_bookmarksCheck->setChecked(true);
    dataLayout->addWidget(m_bookmarksCheck);
    
    m_connectionsCheck = new QCheckBox(QStringLiteral("SSH 连接配置"), dataGroup);
    m_connectionsCheck->setChecked(true);
    dataLayout->addWidget(m_connectionsCheck);
    
    m_historyCheck = new QCheckBox(QStringLiteral("命令历史"), dataGroup);
    m_historyCheck->setChecked(true);
    dataLayout->addWidget(m_historyCheck);
    
    mainLayout->addWidget(dataGroup);
    
    QGroupBox* previewGroup = new QGroupBox(QStringLiteral("数据预览"), this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    
    m_dataPreview = new QTextEdit(previewGroup);
    m_dataPreview->setReadOnly(true);
    m_dataPreview->setFont(QFont("Monospace", 9));
    previewLayout->addWidget(m_dataPreview);
    
    mainLayout->addWidget(previewGroup);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_fileButton = new QPushButton(QStringLiteral("导出到文件"), this);
    buttonLayout->addWidget(m_fileButton);
    
    m_clipboardButton = new QPushButton(QStringLiteral("导出到剪贴板"), this);
    buttonLayout->addWidget(m_clipboardButton);
    
    m_closeButton = new QPushButton(QStringLiteral("关闭"), this);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(m_exportRadio, &QRadioButton::toggled, this, &ImportExportDialog::onModeChanged);
    connect(m_fileButton, &QPushButton::clicked, this, &ImportExportDialog::onExportFile);
    connect(m_clipboardButton, &QPushButton::clicked, this, &ImportExportDialog::onExportClipboard);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    onModeChanged();
}

void ImportExportDialog::onModeChanged() {
    m_currentMode = m_exportRadio->isChecked() ? Mode::Export : Mode::Import;
    
    if (m_currentMode == Mode::Export) {
        m_fileButton->setText(QStringLiteral("导出到文件"));
        m_clipboardButton->setText(QStringLiteral("导出到剪贴板"));
        m_dataPreview->setReadOnly(true);
        m_dataPreview->clear();
        
        bool themes, bookmarks, connections, history;
        if (getExportOptions(themes, bookmarks, connections, history)) {
            QString data = SettingsManager::instance()->getExportData(themes, bookmarks, connections, history);
            m_dataPreview->setPlainText(data);
        }
    } else {
        m_fileButton->setText(QStringLiteral("从文件导入"));
        m_clipboardButton->setText(QStringLiteral("从剪贴板导入"));
        m_dataPreview->setReadOnly(false);
        m_dataPreview->clear();
        m_dataPreview->setPlaceholderText(QStringLiteral("粘贴要导入的 JSON 数据..."));
    }
}

void ImportExportDialog::onExportFile() {
    bool themes, bookmarks, connections, history;
    if (!getExportOptions(themes, bookmarks, connections, history)) return;
    
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("导出设置"),
        QString(), QStringLiteral("JSON Files (*.json)"));
    
    if (filePath.isEmpty()) return;
    
    if (!SettingsManager::instance()->exportSettings(filePath, themes, bookmarks, connections, history)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("无法导出设置到文件。"));
        return;
    }
    
    QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("设置已成功导出到文件。"));
}

void ImportExportDialog::onImportFile() {
    bool themes, bookmarks, connections, history;
    if (!getImportOptions(themes, bookmarks, connections, history)) return;
    
    QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("导入设置"),
        QString(), QStringLiteral("JSON Files (*.json)"));
    
    if (filePath.isEmpty()) return;
    
    if (!SettingsManager::instance()->importSettings(filePath, themes, bookmarks, connections, history)) {
        QMessageBox::warning(this, QStringLiteral("导入失败"), QStringLiteral("无法从文件导入设置。"));
        return;
    }
    
    QMessageBox::information(this, QStringLiteral("导入成功"), QStringLiteral("设置已成功从文件导入。"));
}

void ImportExportDialog::onExportClipboard() {
    bool themes, bookmarks, connections, history;
    if (!getExportOptions(themes, bookmarks, connections, history)) return;
    
    QString data = SettingsManager::instance()->getExportData(themes, bookmarks, connections, history);
    
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(data);
    
    QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("设置已复制到剪贴板。"));
}

void ImportExportDialog::onImportClipboard() {
    bool themes, bookmarks, connections, history;
    if (!getImportOptions(themes, bookmarks, connections, history)) return;
    
    QClipboard* clipboard = QApplication::clipboard();
    QString data = clipboard->text();
    
    if (data.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("导入失败"), QStringLiteral("剪贴板为空。"));
        return;
    }
    
    if (!SettingsManager::instance()->importData(data, themes, bookmarks, connections, history)) {
        QMessageBox::warning(this, QStringLiteral("导入失败"), QStringLiteral("无法从剪贴板导入设置。"));
        return;
    }
    
    QMessageBox::information(this, QStringLiteral("导入成功"), QStringLiteral("设置已成功从剪贴板导入。"));
}

bool ImportExportDialog::getExportOptions(bool& themes, bool& bookmarks, bool& connections, bool& history) {
    themes = m_themesCheck->isChecked();
    bookmarks = m_bookmarksCheck->isChecked();
    connections = m_connectionsCheck->isChecked();
    history = m_historyCheck->isChecked();
    
    if (!themes && !bookmarks && !connections && !history) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请至少选择一项要导出的数据类型。"));
        return false;
    }
    return true;
}

bool ImportExportDialog::getImportOptions(bool& themes, bool& bookmarks, bool& connections, bool& history) {
    themes = m_themesCheck->isChecked();
    bookmarks = m_bookmarksCheck->isChecked();
    connections = m_connectionsCheck->isChecked();
    history = m_historyCheck->isChecked();
    
    if (!themes && !bookmarks && !connections && !history) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请至少选择一项要导入的数据类型。"));
        return false;
    }
    return true;
}
