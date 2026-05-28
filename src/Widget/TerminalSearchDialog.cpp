#include "TerminalSearchDialog.h"

TerminalSearchDialog::TerminalSearchDialog(QWidget* parent)
    : QWidget(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);
    
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search in terminal...");
    m_searchEdit->setMinimumWidth(250);
    layout->addWidget(m_searchEdit);
    
    m_forwardBtn = new QPushButton("↓", this);
    m_forwardBtn->setToolTip("Find Next");
    m_forwardBtn->setMaximumWidth(30);
    layout->addWidget(m_forwardBtn);
    
    m_backwardBtn = new QPushButton("↑", this);
    m_backwardBtn->setToolTip("Find Previous");
    m_backwardBtn->setMaximumWidth(30);
    layout->addWidget(m_backwardBtn);
    
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setMinimumWidth(80);
    layout->addWidget(m_statusLabel);
    
    m_closeBtn = new QPushButton("×", this);
    m_closeBtn->setToolTip("Close (Escape)");
    m_closeBtn->setMaximumWidth(25);
    layout->addWidget(m_closeBtn);
    
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &TerminalSearchDialog::onFindForward);
    connect(m_forwardBtn, &QPushButton::clicked, this, &TerminalSearchDialog::onFindForward);
    connect(m_backwardBtn, &QPushButton::clicked, this, &TerminalSearchDialog::onFindBackward);
    connect(m_closeBtn, &QPushButton::clicked, this, &TerminalSearchDialog::onClose);
}

void TerminalSearchDialog::onFindForward() {
    QString text = m_searchEdit->text().trimmed();
    if (!text.isEmpty()) {
        emit searchRequested(text, true);
    }
}

void TerminalSearchDialog::onFindBackward() {
    QString text = m_searchEdit->text().trimmed();
    if (!text.isEmpty()) {
        emit searchRequested(text, false);
    }
}

void TerminalSearchDialog::onClose() {
    emit searchClosed();
    hide();
}

void TerminalSearchDialog::focusSearchEdit() {
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void TerminalSearchDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        onClose();
    } else {
        QWidget::keyPressEvent(event);
    }
}
