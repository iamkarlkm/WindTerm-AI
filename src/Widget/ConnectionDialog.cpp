#include "ConnectionDialog.h"
#include "Ssh/ConnectionManager.h"
#include "Ssh/ConnectionProfile.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>

ConnectionDialog::ConnectionDialog(ConnectionManager* manager, QWidget* parent)
    : QDialog(parent), m_manager(manager) {
    setWindowTitle(QStringLiteral("SSH 连接"));
    setMinimumSize(600, 450);
    
    setupUI();
    loadProfiles();
}

void ConnectionDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget(this);
    
    setupQuickConnectTab();
    setupSavedProfilesTab();
    
    m_tabWidget->addTab(createQuickConnectWidget(), QStringLiteral("快速连接"));
    m_tabWidget->addTab(createSavedProfilesWidget(), QStringLiteral("已保存的连接"));
    
    mainLayout->addWidget(m_tabWidget);
}

QWidget* ConnectionDialog::createQuickConnectWidget() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* group = new QGroupBox(QStringLiteral("连接设置"), widget);
    QFormLayout* formLayout = new QFormLayout(group);
    
    m_hostEdit = new QLineEdit(widget);
    m_hostEdit->setPlaceholderText(QStringLiteral("主机地址"));
    formLayout->addRow(QStringLiteral("主机:"), m_hostEdit);
    
    m_portSpin = new QSpinBox(widget);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);
    formLayout->addRow(QStringLiteral("端口:"), m_portSpin);
    
    m_usernameEdit = new QLineEdit(widget);
    m_usernameEdit->setPlaceholderText(QStringLiteral("用户名"));
    formLayout->addRow(QStringLiteral("用户名:"), m_usernameEdit);
    
    m_authMethodCombo = new QComboBox(widget);
    m_authMethodCombo->addItem(QStringLiteral("密码"));
    m_authMethodCombo->addItem(QStringLiteral("公钥"));
    formLayout->addRow(QStringLiteral("认证方式:"), m_authMethodCombo);
    
    m_passwordEdit = new QLineEdit(widget);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(QStringLiteral("密码"));
    formLayout->addRow(QStringLiteral("密码:"), m_passwordEdit);
    
    QWidget* keyWidget = new QWidget();
    QHBoxLayout* keyLayout = new QHBoxLayout(keyWidget);
    keyLayout->setContentsMargins(0, 0, 0, 0);
    
    m_privateKeyEdit = new QLineEdit(keyWidget);
    m_privateKeyEdit->setPlaceholderText(QStringLiteral("私钥文件路径"));
    m_privateKeyEdit->setEnabled(false);
    keyLayout->addWidget(m_privateKeyEdit);
    
    m_browseKeyButton = new QPushButton(QStringLiteral("浏览..."), keyWidget);
    m_browseKeyButton->setEnabled(false);
    keyLayout->addWidget(m_browseKeyButton);
    
    formLayout->addRow(QStringLiteral("私钥:"), keyWidget);
    
    layout->addWidget(group);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_connectButton = new QPushButton(QStringLiteral("连接"), widget);
    buttonLayout->addWidget(m_connectButton);
    
    m_saveAndConnectButton = new QPushButton(QStringLiteral("保存并连接"), widget);
    buttonLayout->addWidget(m_saveAndConnectButton);
    
    layout->addLayout(buttonLayout);
    
    connect(m_authMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionDialog::onAuthMethodChanged);
    connect(m_connectButton, &QPushButton::clicked, this, &ConnectionDialog::onQuickConnect);
    connect(m_saveAndConnectButton, &QPushButton::clicked, this, &ConnectionDialog::onLoadNewProfile);
    connect(m_browseKeyButton, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择私钥文件"));
        if (!path.isEmpty()) {
            m_privateKeyEdit->setText(path);
        }
    });
    
    return widget;
}

QWidget* ConnectionDialog::createSavedProfilesWidget() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* group = new QGroupBox(QStringLiteral("已保存的连接"), widget);
    QVBoxLayout* groupLayout = new QVBoxLayout(group);
    
    m_profileNameEdit = new QLineEdit(widget);
    m_profileNameEdit->setPlaceholderText(QStringLiteral("筛选配置文件..."));
    groupLayout->addWidget(m_profileNameEdit);
    
    m_profileList = new QListWidget(widget);
    groupLayout->addWidget(m_profileList);
    
    layout->addWidget(group);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_newProfileButton = new QPushButton(QStringLiteral("新建"), widget);
    buttonLayout->addWidget(m_newProfileButton);
    
    m_connectSavedButton = new QPushButton(QStringLiteral("连接"), widget);
    m_connectSavedButton->setEnabled(false);
    buttonLayout->addWidget(m_connectSavedButton);
    
    m_deleteProfileButton = new QPushButton(QStringLiteral("删除"), widget);
    m_deleteProfileButton->setEnabled(false);
    buttonLayout->addWidget(m_deleteProfileButton);
    
    layout->addLayout(buttonLayout);
    
    connect(m_profileList, &QListWidget::currentItemChanged,
            this, &ConnectionDialog::onSavedProfileSelected);
    connect(m_connectSavedButton, &QPushButton::clicked, this, &ConnectionDialog::onConnectSavedProfile);
    connect(m_deleteProfileButton, &QPushButton::clicked, this, &ConnectionDialog::onProfileDeleted);
    connect(m_newProfileButton, &QPushButton::clicked, this, [this]() {
        m_tabWidget->setCurrentIndex(0);
        clearForm();
        m_hostEdit->setFocus();
    });
    
    return widget;
}

void ConnectionDialog::setupQuickConnectTab() {
    // Already done in createQuickConnectWidget
}

void ConnectionDialog::setupSavedProfilesTab() {
    // Already done in createSavedProfilesWidget
}

void ConnectionDialog::loadProfiles() {
    m_profileList->clear();
    
    QList<ConnectionProfile> profiles = m_manager->loadProfiles();
    
    for (const ConnectionProfile& profile : profiles) {
        QListWidgetItem* item = new QListWidgetItem(m_profileList);
        item->setData(Qt::UserRole, profile.id);
        
        QString displayText = profile.name;
        displayText += "\n" + profile.username + "@" + profile.host + ":" + QString::number(profile.port);
        displayText += " | " + profile.authMethodLabel();
        
        item->setText(displayText);
    }
}

void ConnectionDialog::onQuickConnect() {
    ConnectionProfile profile = getProfileFromForm();
    
    if (!profile.isValid()) {
        QMessageBox::warning(this, QStringLiteral("验证错误"), 
            QStringLiteral("请填写主机、端口和用户名。"));
        return;
    }
    
    if (profile.authMethod == SshAuthMethod::Password && profile.password.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("验证错误"), 
            QStringLiteral("密码认证需要填写密码。"));
        return;
    }
    
    emit connectRequested(profile);
    accept();
}

void ConnectionDialog::onSavedProfileSelected(QListWidgetItem* current) {
    bool hasSelection = current != nullptr;
    m_connectSavedButton->setEnabled(hasSelection);
    m_deleteProfileButton->setEnabled(hasSelection);
    
    if (hasSelection) {
        qint64 id = current->data(Qt::UserRole).toLongLong();
        ConnectionProfile profile = m_manager->getProfile(id);
        setProfileToForm(profile);
    }
}

void ConnectionDialog::onConnectSavedProfile() {
    qint64 id = m_profileList->currentItem()->data(Qt::UserRole).toLongLong();
    ConnectionProfile profile = m_manager->getProfile(id);
    
    if (!profile.isValid()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("配置文件无效。"));
        return;
    }
    
    emit connectRequested(profile);
    accept();
}

void ConnectionDialog::onProfileDeleted() {
    qint64 id = m_profileList->currentItem()->data(Qt::UserRole).toLongLong();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除此连接配置吗？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_manager->deleteProfile(id);
        loadProfiles();
    }
}

void ConnectionDialog::onProfileSaved() {
    loadProfiles();
}

void ConnectionDialog::onAuthMethodChanged(int index) {
    bool isPublicKey = (index == 1);
    m_passwordEdit->setEnabled(!isPublicKey);
    m_privateKeyEdit->setEnabled(isPublicKey);
    m_browseKeyButton->setEnabled(isPublicKey);
}

void ConnectionDialog::onLoadNewProfile() {
    ConnectionProfile profile = getProfileFromForm();
    
    if (!profile.isValid()) {
        QMessageBox::warning(this, QStringLiteral("验证错误"), 
            QStringLiteral("请填写主机、端口和用户名。"));
        return;
    }
    
    if (profile.name.isEmpty()) {
        profile.name = profile.username + "@" + profile.host;
    }
    
    qint64 id = m_manager->saveProfile(profile);
    if (id > 0) {
        loadProfiles();
        emit connectRequested(profile);
        accept();
    }
}

void ConnectionDialog::saveProfileAndConnect(const ConnectionProfile& profile) {
    qint64 id = m_manager->saveProfile(profile);
    if (id > 0) {
        loadProfiles();
        emit connectRequested(profile);
        accept();
    }
}

ConnectionProfile ConnectionDialog::getProfileFromForm() {
    ConnectionProfile profile;
    profile.host = m_hostEdit->text().trimmed();
    profile.port = m_portSpin->value();
    profile.username = m_usernameEdit->text().trimmed();
    profile.authMethod = m_authMethodCombo->currentIndex() == 1 
        ? SshAuthMethod::PublicKey : SshAuthMethod::Password;
    profile.password = m_passwordEdit->text();
    profile.privateKeyPath = m_privateKeyEdit->text().trimmed();
    profile.name = profile.username + "@" + profile.host;
    
    return profile;
}

void ConnectionDialog::setProfileToForm(const ConnectionProfile& profile) {
    m_hostEdit->setText(profile.host);
    m_portSpin->setValue(profile.port);
    m_usernameEdit->setText(profile.username);
    m_authMethodCombo->setCurrentIndex(
        profile.authMethod == SshAuthMethod::PublicKey ? 1 : 0
    );
    m_privateKeyEdit->setText(profile.privateKeyPath);
}

void ConnectionDialog::clearForm() {
    m_hostEdit->clear();
    m_portSpin->setValue(22);
    m_usernameEdit->clear();
    m_passwordEdit->clear();
    m_privateKeyEdit->clear();
    m_authMethodCombo->setCurrentIndex(0);
}
