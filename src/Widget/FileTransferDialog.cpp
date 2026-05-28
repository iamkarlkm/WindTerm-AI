#include "FileTransferDialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QDir>

FileTransferDialog::FileTransferDialog(ConnectionManager* connManager, QWidget* parent)
    : QDialog(parent), m_connManager(connManager) {
    setupUI();
}

void FileTransferDialog::setupUI() {
    setWindowTitle("File Transfer (SCP/SFTP)");
    resize(800, 600);
    
    m_scpClient = new ScpClient(this);
    
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("Remote:"));
    m_remotePathEdit = new QLineEdit("/");
    m_remotePathEdit->setPlaceholderText("Remote directory path");
    pathLayout->addWidget(m_remotePathEdit);
    pathLayout->addWidget(new QLabel("Local:"));
    m_localPathEdit = new QLineEdit(QDir::homePath());
    m_localPathEdit->setPlaceholderText("Local directory path");
    pathLayout->addWidget(m_localPathEdit);
    
    m_connectButton = new QPushButton("Connect");
    m_refreshButton = new QPushButton("Refresh");
    m_refreshButton->setEnabled(false);
    
    pathLayout->addWidget(m_connectButton);
    pathLayout->addWidget(m_refreshButton);
    
    m_remoteFileList = new QTableWidget(this);
    m_remoteFileList->setColumnCount(4);
    m_remoteFileList->setHorizontalHeaderLabels({"Name", "Size", "Type", "Modified"});
    m_remoteFileList->horizontalHeader()->setStretchLastSection(true);
    m_remoteFileList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_remoteFileList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_remoteFileList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    auto* buttonLayout = new QHBoxLayout();
    m_uploadButton = new QPushButton("Upload File");
    m_downloadButton = new QPushButton("Download");
    m_uploadDirButton = new QPushButton("Upload Directory");
    m_downloadDirButton = new QPushButton("Download Directory");
    
    buttonLayout->addWidget(m_uploadButton);
    buttonLayout->addWidget(m_downloadButton);
    buttonLayout->addWidget(m_uploadDirButton);
    buttonLayout->addWidget(m_downloadDirButton);
    buttonLayout->addStretch();
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    
    m_statusLabel = new QLabel("Disconnected", this);
    
    mainLayout->addLayout(pathLayout);
    mainLayout->addWidget(m_remoteFileList);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addWidget(m_statusLabel);
    
    connect(m_connectButton, &QPushButton::clicked, this, &FileTransferDialog::onConnect);
    connect(m_refreshButton, &QPushButton::clicked, this, &FileTransferDialog::onRefresh);
    connect(m_uploadButton, &QPushButton::clicked, this, &FileTransferDialog::onUpload);
    connect(m_downloadButton, &QPushButton::clicked, this, &FileTransferDialog::onDownload);
    connect(m_uploadDirButton, &QPushButton::clicked, this, &FileTransferDialog::onSelectLocalDirectory);
    connect(m_downloadDirButton, &QPushButton::clicked, this, &FileTransferDialog::onSelectLocalDirectory);
    connect(m_remoteFileList, &QTableWidget::itemDoubleClicked, this, &FileTransferDialog::onRemoteItemDoubleClicked);
    
    connect(m_scpClient, &ScpClient::transferProgress, this, &FileTransferDialog::onTransferProgress);
    connect(m_scpClient, &ScpClient::transferStarted, this, &FileTransferDialog::onTransferStarted);
    connect(m_scpClient, &ScpClient::transferFinished, this, &FileTransferDialog::onTransferFinished);
    connect(m_scpClient, &ScpClient::error, this, &FileTransferDialog::onScpError);
}

void FileTransferDialog::setConnectionProfile(const ConnectionProfile& profile) {
    m_currentProfile = profile;
}

void FileTransferDialog::onConnect() {
    if (m_isConnected) {
        m_scpClient->disconnect();
        m_isConnected = false;
        m_connectButton->setText("Connect");
        m_refreshButton->setEnabled(false);
        m_uploadButton->setEnabled(false);
        m_downloadButton->setEnabled(false);
        updateStatus("Disconnected");
        return;
    }
    
    updateStatus("Connecting...");
    
    if (m_scpClient->connect(m_currentProfile.toSshConfig())) {
        m_isConnected = true;
        m_connectButton->setText("Disconnect");
        m_refreshButton->setEnabled(true);
        m_uploadButton->setEnabled(true);
        m_downloadButton->setEnabled(true);
        updateStatus("Connected");
        onRefresh();
    } else {
        updateStatus("Connection failed");
    }
}

void FileTransferDialog::onRefresh() {
    if (!m_isConnected) return;
    
    updateStatus("Refreshing...");
    QString remotePath = m_remotePathEdit->text();
    auto files = m_scpClient->listRemoteDirectory(remotePath);
    updateRemoteFileList(files);
    updateStatus(QString("Loaded %1 items").arg(files.size()));
}

void FileTransferDialog::onUpload() {
    if (!m_isConnected) return;
    
    QString localFile = QFileDialog::getOpenFileName(this, "Select File to Upload");
    if (localFile.isEmpty()) return;
    
    QString remotePath = m_remotePathEdit->text();
    updateStatus(QString("Uploading: %1").arg(localFile));
    
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    bool success = m_scpClient->uploadFile(localFile, remotePath);
    if (success) {
        updateStatus("Upload complete");
        onRefresh();
    }
}

void FileTransferDialog::onDownload() {
    if (!m_isConnected) return;
    
    int row = m_remoteFileList->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Warning", "Please select a file to download");
        return;
    }
    
    QString fileName = m_remoteFileList->item(row, 0)->text();
    QString remotePath = m_remotePathEdit->text() + "/" + fileName;
    QString localPath = QFileDialog::getSaveFileName(this, "Save File", QDir::homePath() + "/" + fileName);
    
    if (localPath.isEmpty()) return;
    
    updateStatus(QString("Downloading: %1").arg(fileName));
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    bool success = m_scpClient->downloadFile(remotePath, localPath);
    if (success) {
        updateStatus("Download complete");
    }
}

void FileTransferDialog::onSelectLocalDirectory() {
    QString localDir = QFileDialog::getExistingDirectory(this, "Select Local Directory");
    if (localDir.isEmpty()) return;
    m_localPathEdit->setText(localDir);
}

void FileTransferDialog::onSelectLocalFile() {
    QString localFile = QFileDialog::getOpenFileName(this, "Select Local File");
    if (localFile.isEmpty()) return;
    m_localPathEdit->setText(localFile);
}

void FileTransferDialog::onRemoteItemDoubleClicked(QTableWidgetItem* item) {
    int row = item->row();
    QString fileName = m_remoteFileList->item(row, 0)->text();
    QString fileType = m_remoteFileList->item(row, 2)->text();
    
    if (fileType == "Directory") {
        QString currentPath = m_remotePathEdit->text();
        QString newPath = currentPath == "/" ? "/" + fileName : currentPath + "/" + fileName;
        m_remotePathEdit->setText(newPath);
        onRefresh();
    }
}

void FileTransferDialog::onTransferProgress(qint64 transferred, qint64 total) {
    if (total > 0) {
        int percent = (transferred * 100) / total;
        m_progressBar->setValue(percent);
        updateStatus(QString("Transferring: %1 / %2 (%3%)")
            .arg(formatSize(transferred))
            .arg(formatSize(total))
            .arg(percent));
    }
}

void FileTransferDialog::onTransferStarted(const QString& fileName) {
    updateStatus(QString("Starting: %1").arg(fileName));
}

void FileTransferDialog::onTransferFinished(const QString& fileName, bool success) {
    m_progressBar->setVisible(false);
    if (success) {
        updateStatus(QString("Completed: %1").arg(fileName));
        onRefresh();
    } else {
        updateStatus(QString("Failed: %1").arg(fileName));
    }
}

void FileTransferDialog::onScpError(const QString& message) {
    m_progressBar->setVisible(false);
    updateStatus(QString("Error: %1").arg(message));
    QMessageBox::critical(this, "SCP Error", message);
}

void FileTransferDialog::updateRemoteFileList(const QList<ScpFileInfo>& files) {
    m_remoteFileList->setRowCount(files.size());
    
    for (int i = 0; i < files.size(); ++i) {
        const auto& file = files[i];
        
        m_remoteFileList->setItem(i, 0, new QTableWidgetItem(file.name));
        m_remoteFileList->setItem(i, 1, new QTableWidgetItem(formatSize(file.size)));
        m_remoteFileList->setItem(i, 2, new QTableWidgetItem(file.isDirectory ? "Directory" : "File"));
        m_remoteFileList->setItem(i, 3, new QTableWidgetItem("-"));
        
        if (file.isDirectory) {
            m_remoteFileList->item(i, 0)->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            m_remoteFileList->item(i, 0)->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
        }
    }
}

void FileTransferDialog::updateStatus(const QString& text) {
    m_statusLabel->setText(text);
}

QString FileTransferDialog::formatSize(qint64 bytes) {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024 * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}
