#include "AiAssistantDialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QScrollBar>
#include <QSettings>
#include <QTextBlock>

AiAssistantDialog::AiAssistantDialog(AiClient* client, QWidget* parent)
    : QDialog(parent), m_client(client) {
    setupUI();
    
    connect(m_client, &AiClient::responseReceived, this, &AiAssistantDialog::onResponseReceived);
    connect(m_client, &AiClient::responseChunk, this, &AiAssistantDialog::onResponseChunk);
    connect(m_client, &AiClient::responseFinished, this, &AiAssistantDialog::onResponseFinished);
    connect(m_client, &AiClient::errorOccurred, this, &AiAssistantDialog::onError);
}

void AiAssistantDialog::setupUI() {
    setWindowTitle("AI Assistant");
    resize(600, 500);
    
    auto* mainLayout = new QVBoxLayout(this);
    
    m_chatView = new QTextEdit(this);
    m_chatView->setReadOnly(true);
    m_chatView->setPlaceholderText("Ask me anything about terminal commands, scripts, or system administration...");
    
    auto* inputLayout = new QHBoxLayout();
    m_inputEdit = new QLineEdit(this);
    m_inputEdit->setPlaceholderText("Type your question... (or /ai in terminal)");
    m_sendButton = new QPushButton("Send", this);
    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setEnabled(false);
    m_settingsButton = new QPushButton("Settings", this);
    
    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(m_sendButton);
    inputLayout->addWidget(m_cancelButton);
    inputLayout->addWidget(m_settingsButton);
    
    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("color: gray;");
    
    mainLayout->addWidget(m_chatView);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(inputLayout);
    
    connect(m_sendButton, &QPushButton::clicked, this, &AiAssistantDialog::onSend);
    connect(m_cancelButton, &QPushButton::clicked, m_client, &AiClient::cancel);
    connect(m_settingsButton, &QPushButton::clicked, this, &AiAssistantDialog::onSettings);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &AiAssistantDialog::onSend);
}

void AiAssistantDialog::setContext(const QString& workingDir, const QStringList& recentCommands) {
    m_workingDir = workingDir;
    m_recentCommands = recentCommands;
    m_client->sendContext(workingDir, recentCommands);
}

void AiAssistantDialog::onSend() {
    QString prompt = m_inputEdit->text().trimmed();
    if (prompt.isEmpty() || m_client->isProcessing()) return;
    
    m_inputEdit->clear();
    appendMessage("You", prompt);
    
    m_sendButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    m_isReceiving = true;
    m_currentResponse.clear();
    updateStatus("Thinking...");
    
    QString context;
    if (!m_workingDir.isEmpty()) {
        context += QString("Working directory: %1\n").arg(m_workingDir);
    }
    if (!m_recentCommands.isEmpty()) {
        context += "\nRecent commands:\n";
        int count = qMin(5, m_recentCommands.size());
        for (int i = 0; i < count; ++i) {
            context += QString("- %1\n").arg(m_recentCommands[i]);
        }
    }
    
    m_client->sendPrompt(prompt, context);
}

void AiAssistantDialog::onResponseReceived(const QString& response) {
    m_currentResponse = response;
    appendMessage("AI", response);
    updateStatus("Ready");
    m_isReceiving = false;
    m_sendButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
}

void AiAssistantDialog::onResponseChunk(const QString& chunk) {
    m_currentResponse += chunk;
    
    QTextCursor cursor = m_chatView->textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
    }
    
    if (cursor.block().text().startsWith("AI: ")) {
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(chunk);
    }
    
    m_chatView->verticalScrollBar()->setValue(m_chatView->verticalScrollBar()->maximum());
}

void AiAssistantDialog::onResponseFinished() {
    m_isReceiving = false;
    m_sendButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    updateStatus("Ready");
}

void AiAssistantDialog::onError(const QString& error) {
    updateStatus(QString("Error: %1").arg(error));
    m_isReceiving = false;
    m_sendButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    QMessageBox::warning(this, "AI Error", error);
}

void AiAssistantDialog::onSettings() {
    QSettings settings("WindTerm", "Terminal");
    AiConfig config = AiConfig::load(&settings);
    
    QString newKey = QInputDialog::getText(this, "API Key",
        "Enter your API key:", QLineEdit::Password, config.apiKey);
    if (!newKey.isNull()) {
        config.apiKey = newKey;
        config.enabled = !newKey.isEmpty();
        config.save(&settings);
        m_client->setConfig(config);
        updateStatus("Settings updated");
    }
}

void AiAssistantDialog::appendMessage(const QString& role, const QString& content) {
    QTextCursor cursor(m_chatView->document());
    cursor.movePosition(QTextCursor::End);
    
    QTextCharFormat format;
    format.setFontWeight(QFont::Bold);
    cursor.insertText(QString("%1:\n").arg(role), format);
    
    format.setFontWeight(QFont::Normal);
    cursor.insertText(content + "\n\n", format);
    
    m_chatView->verticalScrollBar()->setValue(m_chatView->verticalScrollBar()->maximum());
}

void AiAssistantDialog::updateStatus(const QString& text) {
    m_statusLabel->setText(text);
}
