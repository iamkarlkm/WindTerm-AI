#include "ThemeSwitcherPlugin.h"
#include <QtDebug>

ThemeSwitcherPlugin::ThemeSwitcherPlugin(QObject* parent) : PluginInterface(parent) {}

ThemeSwitcherPlugin::~ThemeSwitcherPlugin() { shutdown(); }

PluginMetadata ThemeSwitcherPlugin::metadata() const {
    PluginMetadata meta;
    meta.id = "theme-switcher";
    meta.name = "Theme Switcher";
    meta.version = "1.0.0";
    meta.description = "Quick theme switching via keyboard shortcuts (Ctrl+Shift+T to cycle)";
    meta.author = "WindTerm-AI Team";
    meta.type = PluginType::ThemeProvider;
    return meta;
}

bool ThemeSwitcherPlugin::initialize(PluginContext* context) {
    m_context = context;
    
    QString saved = m_context->readSetting("theme/index").toString();
    if (!saved.isEmpty()) {
        m_currentIndex = saved.toInt();
    }
    
    setState(PluginState::Initialized);
    qDebug() << "[ThemeSwitcher] Plugin initialized, current theme:" << m_themes[m_currentIndex];
    return true;
}

void ThemeSwitcherPlugin::shutdown() {
    if (m_context) {
        m_context->writeSetting("theme/index", m_currentIndex);
    }
    m_context = nullptr;
    setState(PluginState::Stopped);
    qDebug() << "[ThemeSwitcher] Plugin shutdown";
}

bool ThemeSwitcherPlugin::interceptKeyEvent(int key, int modifiers, const QString& text) {
    Q_UNUSED(text);
    
    if (modifiers == (Qt::ControlModifier | Qt::ShiftModifier) && key == Qt::Key_T) {
        cycleTheme();
        return true;
    }
    
    return false;
}

void ThemeSwitcherPlugin::cycleTheme() {
    m_currentIndex = (m_currentIndex + 1) % m_themes.size();
    applyTheme(m_themes[m_currentIndex]);
}

void ThemeSwitcherPlugin::applyTheme(const QString& themeName) {
    if (m_context) {
        m_context->sendNotification("Theme Switcher", QString("Theme changed to: %1").arg(themeName));
        emit sendTextToTerminal(QString("\033]50;SetTheme=%1\007").arg(themeName));
        qDebug() << "[ThemeSwitcher] Applied theme:" << themeName;
    }
}
