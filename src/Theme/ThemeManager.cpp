#include "ThemeManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

ThemeManager* ThemeManager::s_instance = nullptr;

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent) {
    loadBuiltInThemes();
    loadCustomThemes();
    
    if (m_themes.isEmpty()) {
        m_themes.append(ThemeConfig::defaultTheme());
    }
    m_currentTheme = m_themes.first();
}

ThemeManager* ThemeManager::instance() {
    if (!s_instance) {
        s_instance = new ThemeManager();
    }
    return s_instance;
}

void ThemeManager::setTheme(const ThemeConfig& theme) {
    m_currentTheme = theme;
    emit themeChanged(m_currentTheme);
}

QVector<ThemeConfig> ThemeManager::availableThemes() const {
    return m_themes;
}

void ThemeManager::saveTheme(const ThemeConfig& theme) {
    QString dir = themesDirectory();
    QDir().mkpath(dir);
    
    QString fileName = dir + "/" + theme.name + ".json";
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(theme.toJson());
        file.write(doc.toJson());
        file.close();
        
        int existingIndex = -1;
        for (int i = 0; i < m_themes.size(); ++i) {
            if (m_themes[i].name == theme.name) {
                existingIndex = i;
                break;
            }
        }
        
        if (existingIndex >= 0) {
            m_themes[existingIndex] = theme;
        } else {
            m_themes.append(theme);
        }
    }
}

void ThemeManager::deleteTheme(const QString& name) {
    QString fileName = themesDirectory() + "/" + name + ".json";
    QFile::remove(fileName);
    
    for (int i = m_themes.size() - 1; i >= 0; --i) {
        if (m_themes[i].name == name) {
            m_themes.removeAt(i);
        }
    }
}

QString ThemeManager::themesDirectory() const {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataDir + "/themes";
}

void ThemeManager::loadBuiltInThemes() {
    m_themes.append(ThemeConfig::defaultTheme());
    m_themes.append(ThemeConfig::draculaTheme());
    m_themes.append(ThemeConfig::monokaiTheme());
    m_themes.append(ThemeConfig::solarizedDarkTheme());
}

void ThemeManager::loadCustomThemes() {
    QString dir = themesDirectory();
    QDir themesDir(dir);
    
    if (!themesDir.exists()) {
        return;
    }
    
    QStringList filters;
    filters << "*.json";
    QStringList files = themesDir.entryList(filters, QDir::Files);
    
    for (const QString& file : files) {
        ThemeConfig theme = loadThemeFromFile(dir + "/" + file);
        if (theme.isValid()) {
            bool exists = false;
            for (const auto& t : m_themes) {
                if (t.name == theme.name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                m_themes.append(theme);
            }
        }
    }
}

ThemeConfig ThemeManager::loadThemeFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return ThemeConfig();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (doc.isNull() || !doc.isObject()) {
        return ThemeConfig();
    }
    
    return ThemeConfig::fromJson(doc.object());
}
