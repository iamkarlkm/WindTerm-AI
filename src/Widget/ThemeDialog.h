#ifndef THEME_DIALOG_H
#define THEME_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include "Theme/ThemeConfig.h"
#include "Theme/ThemeManager.h"

class ThemeDialog : public QDialog {
    Q_OBJECT
public:
    explicit ThemeDialog(QWidget* parent = nullptr);
    
signals:
    void themeSelected(const ThemeConfig& theme);
    
private slots:
    void onThemeSelected(QListWidgetItem* current);
    void onApplyTheme();
    void onSaveCustomTheme();
    
private:
    void setupUI();
    void loadThemes();
    
    ThemeManager* m_themeManager;
    QListWidget* m_themeList;
    QPushButton* m_applyButton;
    QPushButton* m_saveButton;
    QPushButton* m_closeButton;
};

#endif
