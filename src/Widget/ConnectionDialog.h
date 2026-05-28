#ifndef CONNECTION_DIALOG_H
#define CONNECTION_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QListWidget>
#include <QStackedWidget>

class ConnectionManager;
struct ConnectionProfile;

class ConnectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnectionDialog(ConnectionManager* manager, QWidget* parent = nullptr);
    
signals:
    void connectRequested(const ConnectionProfile& profile);
    
private slots:
    void onQuickConnect();
    void onSavedProfileSelected(QListWidgetItem* current);
    void onConnectSavedProfile();
    void onProfileDeleted();
    void onProfileSaved();
    void onAuthMethodChanged(int index);
    void onLoadNewProfile();
    
private:
    void setupUI();
    void setupQuickConnectTab();
    void setupSavedProfilesTab();
    QWidget* createQuickConnectWidget();
    QWidget* createSavedProfilesWidget();
    void loadProfiles();
    
    void saveProfileAndConnect(const ConnectionProfile& profile);
    ConnectionProfile getProfileFromForm();
    void setProfileToForm(const ConnectionProfile& profile);
    void clearForm();
    
    ConnectionManager* m_manager;
    
    QTabWidget* m_tabWidget;
    
    QLineEdit* m_hostEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QComboBox* m_authMethodCombo;
    QLineEdit* m_privateKeyEdit;
    QPushButton* m_browseKeyButton;
    QPushButton* m_connectButton;
    QPushButton* m_saveAndConnectButton;
    
    QListWidget* m_profileList;
    QPushButton* m_connectSavedButton;
    QPushButton* m_deleteProfileButton;
    QPushButton* m_newProfileButton;
    QLineEdit* m_profileNameEdit;
};

#endif
