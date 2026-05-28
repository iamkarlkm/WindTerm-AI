#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "Theme/ThemeConfig.h"
#include <QObject>
#include <QVector>
#include <QString>

class ThemeManager : public QObject {
    Q_OBJECT
public:
    explicit ThemeManager(QObject* parent = nullptr);
    
    static ThemeManager* instance();
    
    const ThemeConfig& currentTheme() const { return m_currentTheme; }
    void setTheme(const ThemeConfig& theme);
    
    QVector<ThemeConfig> availableThemes() const;
    void saveTheme(const ThemeConfig& theme);
    void deleteTheme(const QString& name);
    
    QString themesDirectory() const;
    
signals:
    void themeChanged(const ThemeConfig& theme);
    
private:
    void loadBuiltInThemes();
    void loadCustomThemes();
    ThemeConfig loadThemeFromFile(const QString& path);
    
    ThemeConfig m_currentTheme;
    QVector<ThemeConfig> m_themes;
    
    static ThemeManager* s_instance;
};

#endif
