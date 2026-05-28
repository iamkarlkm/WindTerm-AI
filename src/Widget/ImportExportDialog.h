#ifndef IMPORT_EXPORT_DIALOG_H
#define IMPORT_EXPORT_DIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QRadioButton>

class ImportExportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImportExportDialog(QWidget* parent = nullptr);
    
private slots:
    void onExportFile();
    void onImportFile();
    void onExportClipboard();
    void onImportClipboard();
    void onModeChanged();
    
private:
    void setupUI();
    void updateButtons();
    bool getExportOptions(bool& themes, bool& bookmarks, bool& connections, bool& history);
    bool getImportOptions(bool& themes, bool& bookmarks, bool& connections, bool& history);
    
    enum class Mode { Export, Import };
    
    QRadioButton* m_exportRadio;
    QRadioButton* m_importRadio;
    QCheckBox* m_themesCheck;
    QCheckBox* m_bookmarksCheck;
    QCheckBox* m_connectionsCheck;
    QCheckBox* m_historyCheck;
    QTextEdit* m_dataPreview;
    QPushButton* m_fileButton;
    QPushButton* m_clipboardButton;
    QPushButton* m_closeButton;
    
    Mode m_currentMode;
};

#endif
