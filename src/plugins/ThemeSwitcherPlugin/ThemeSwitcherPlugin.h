#ifndef THEME_SWITCHER_PLUGIN_H
#define THEME_SWITCHER_PLUGIN_H

#include "../PluginInterface.h"
#include "../PluginContext.h"

class ThemeSwitcherPlugin : public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "theme_switcher.json")
    
public:
    explicit ThemeSwitcherPlugin(QObject* parent = nullptr);
    ~ThemeSwitcherPlugin() override;
    
    PluginMetadata metadata() const override;
    bool initialize(PluginContext* context) override;
    void shutdown() override;
    
    bool interceptKeyEvent(int key, int modifiers, const QString& text) override;
    
private:
    void cycleTheme();
    void applyTheme(const QString& themeName);
    
    PluginContext* m_context = nullptr;
    QStringList m_themes = {"Solarized Dark", "Solarized Light", "Monokai", "GitHub Dark"};
    int m_currentIndex = 0;
};

#endif
