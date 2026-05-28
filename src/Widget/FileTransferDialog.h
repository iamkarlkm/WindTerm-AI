#ifndef FILE_TRANSFER_DIALOG_H
#define FILE_TRANSFER_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "Ssh/ScpClient.h"
#include "Ssh/ConnectionManager.h"

class FileTransferDialog : public QDialog {
    Q_OBJECT
public:
    explicit FileTransferDialog(ConnectionManager* connManager, QWidget* parent = nullptr);
    
    void setConnectionProfile(const ConnectionProfile& profile);
    
private slots:
    void onConnect();
    void onUpload();
    void onDownload();
    void onRefresh();
    void onTransferProgress(qint64 transferred, qint64 total);
    void onTransferStarted(const QString& fileName);
    void onTransferFinished(const QString& fileName, bool success);
    void onScpError(const QString& message);
    void onRemoteItemDoubleClicked(QTableWidgetItem* item);
    void onSelectLocalFile();
    void onSelectLocalDirectory();
    
private:
    void setupUI();
    void updateRemoteFileList(const QList<ScpFileInfo>& files);
    void updateStatus(const QString& text);
    QString formatSize(qint64 bytes);
    
    ConnectionManager* m_connManager;
    ConnectionProfile m_currentProfile;
    ScpClient* m_scpClient;
    
    QLineEdit* m_remotePathEdit;
    QLineEdit* m_localPathEdit;
    QTableWidget* m_remoteFileList;
    QPushButton* m_connectButton;
    QPushButton* m_uploadButton;
    QPushButton* m_downloadButton;
    QPushButton* m_refreshButton;
    QPushButton* m_uploadDirButton;
    QPushButton* m_downloadDirButton;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    
    bool m_isConnected = false;
};

#endif
