#ifndef TERMINAL_SEARCH_DIALOG_H
#define TERMINAL_SEARCH_DIALOG_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QKeyEvent>

class TerminalSearchDialog : public QWidget {
    Q_OBJECT
public:
    explicit TerminalSearchDialog(QWidget* parent = nullptr);
    
    void focusSearchEdit();

signals:
    void searchRequested(const QString& text, bool forward);
    void searchClosed();

private slots:
    void onFindForward();
    void onFindBackward();
    void onClose();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QLineEdit* m_searchEdit;
    QPushButton* m_forwardBtn;
    QPushButton* m_backwardBtn;
    QPushButton* m_closeBtn;
    QLabel* m_statusLabel;
};

#endif
