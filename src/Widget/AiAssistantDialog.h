#ifndef AI_ASSISTANT_DIALOG_H
#define AI_ASSISTANT_DIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "AiIntegration/AiClient.h"
#include "AiIntegration/AiConfig.h"

class AiAssistantDialog : public QDialog {
    Q_OBJECT
public:
    explicit AiAssistantDialog(AiClient* client, QWidget* parent = nullptr);
    
    void setContext(const QString& workingDir, const QStringList& recentCommands);
    
private slots:
    void onSend();
    void onResponseReceived(const QString& response);
    void onResponseChunk(const QString& chunk);
    void onResponseFinished();
    void onError(const QString& error);
    void onSettings();
    
private:
    void setupUI();
    void appendMessage(const QString& role, const QString& content);
    void updateStatus(const QString& text);
    
    AiClient* m_client;
    QTextEdit* m_chatView;
    QLineEdit* m_inputEdit;
    QPushButton* m_sendButton;
    QPushButton* m_cancelButton;
    QPushButton* m_settingsButton;
    QLabel* m_statusLabel;
    QString m_workingDir;
    QStringList m_recentCommands;
    bool m_isReceiving = false;
    QString m_currentResponse;
};

#endif
