#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QDir>
#include <QJsonArray>
#include <QtDebug>
#include "PluginInterface.h"
#include "PluginContext.h"
#include "PluginLoader.h"

struct PluginInfo {
    PluginMetadata metadata;
    PluginInterface* instance = nullptr;
    QString filePath;
    PluginState state = PluginState::Unloaded;
    QString error;
};

class PluginManager : public QObject {
    Q_OBJECT
public:
    explicit PluginManager(QObject* parent = nullptr) : QObject(parent) {}
    
    void setPluginDirectories(const QStringList& dirs) {
        m_pluginDirs = dirs;
    }
    
    void setPluginContext(PluginContext* context) {
        m_context = context;
    }
    
    void scanPlugins() {
        for (const auto& dir : m_pluginDirs) {
            QDir pluginDir(dir);
            if (!pluginDir.exists()) continue;
            
            auto filters = QDir::Files | QDir::NoDotAndDotDot;
            QStringList nameFilters;
            
#ifdef Q_OS_WIN
            nameFilters << "*.dll";
#elif defined(Q_OS_MAC)
            nameFilters << "*.dylib";
#else
            nameFilters << "*.so";
#endif
            
            pluginDir.setNameFilters(nameFilters);
            auto files = pluginDir.entryInfoList(filters);
            
            for (const auto& file : files) {
                QString path = file.absoluteFilePath();
                if (m_plugins.contains(path)) continue;
                
                PluginInfo info;
                info.filePath = path;
                info.metadata = loadMetadata(path);
                m_plugins[path] = info;
            }
        }
    }
    
    bool loadPlugin(const QString& pluginId) {
        QString filePath = findPluginPath(pluginId);
        if (filePath.isEmpty()) {
            qWarning() << "Plugin not found:" << pluginId;
            return false;
        }
        
        if (m_plugins[filePath].state != PluginState::Unloaded) {
            return false;
        }
        
        auto& info = m_plugins[filePath];
        if (!checkDependencies(info.metadata.dependencies)) {
            info.error = "Dependencies not satisfied";
            info.state = PluginState::Error;
            return false;
        }
        
        info.state = PluginState::Loading;
        
        PluginInterface* plugin = m_loader.loadPlugin(filePath);
        if (!plugin) {
            info.error = m_loader.lastError();
            info.state = PluginState::Error;
            qCritical() << "Failed to load plugin:" << info.error;
            return false;
        }
        
        info.instance = plugin;
        info.state = PluginState::Loaded;
        
        connect(plugin, &PluginInterface::stateChanged, this, [this, filePath](PluginState state) {
            if (m_plugins.contains(filePath)) {
                m_plugins[filePath].state = state;
            }
        });
        
        connect(plugin, &PluginInterface::sendTextToTerminal, this, &PluginManager::onPluginSendText);
        connect(plugin, &PluginInterface::requestNotification, this, &PluginManager::onPluginNotification);
        
        return true;
    }
    
    bool initializePlugin(const QString& pluginId) {
        QString filePath = findPluginPath(pluginId);
        if (filePath.isEmpty() || !m_plugins.contains(filePath)) return false;
        
        auto& info = m_plugins[filePath];
        if (info.state != PluginState::Loaded) return false;
        
        info.state = PluginState::Loading;
        
        if (!info.instance->initialize(m_context)) {
            info.error = "Initialization failed";
            info.state = PluginState::Error;
            qCritical() << "Failed to initialize plugin:" << pluginId;
            return false;
        }
        
        info.state = PluginState::Initialized;
        return true;
    }
    
    bool startPlugin(const QString& pluginId) {
        if (initializePlugin(pluginId)) {
            QString filePath = findPluginPath(pluginId);
            if (!filePath.isEmpty()) {
                m_plugins[filePath].state = PluginState::Running;
                emit pluginStarted(pluginId);
            }
            return true;
        }
        return false;
    }
    
    bool stopPlugin(const QString& pluginId) {
        QString filePath = findPluginPath(pluginId);
        if (filePath.isEmpty() || !m_plugins.contains(filePath)) return false;
        
        auto& info = m_plugins[filePath];
        if (info.state == PluginState::Unloaded) return false;
        
        if (info.instance) {
            info.instance->shutdown();
        }
        
        m_loader.unloadPlugin(filePath);
        info.instance = nullptr;
        info.state = PluginState::Stopped;
        
        emit pluginStopped(pluginId);
        return true;
    }
    
    bool unloadPlugin(const QString& pluginId) {
        stopPlugin(pluginId);
        QString filePath = findPluginPath(pluginId);
        if (!filePath.isEmpty()) {
            m_plugins.remove(filePath);
            return true;
        }
        return false;
    }
    
    QList<PluginInfo> getAllPlugins() const {
        return m_plugins.values();
    }
    
    PluginInterface* getPlugin(const QString& pluginId) const {
        QString filePath = findPluginPath(pluginId);
        if (!filePath.isEmpty() && m_plugins.contains(filePath)) {
            return m_plugins[filePath].instance;
        }
        return nullptr;
    }
    
    QStringList getLoadedPlugins() const {
        QStringList result;
        for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
            if (it->state == PluginState::Running) {
                result.append(it->metadata.id);
            }
        }
        return result;
    }
    
    void broadcastTerminalOutput(const QString& text) {
        for (auto& info : m_plugins) {
            if (info.instance && info.state == PluginState::Running) {
                info.instance->onTerminalOutput(text);
            }
        }
    }
    
    void broadcastCommandExecuted(const QString& command) {
        for (auto& info : m_plugins) {
            if (info.instance && info.state == PluginState::Running) {
                info.instance->onCommandExecuted(command);
            }
        }
    }
    
    void broadcastWorkingDirectoryChanged(const QString& path) {
        for (auto& info : m_plugins) {
            if (info.instance && info.state == PluginState::Running) {
                info.instance->onWorkingDirectoryChanged(path);
            }
        }
    }
    
    void broadcastSessionStarted(const QString& sessionId) {
        for (auto& info : m_plugins) {
            if (info.instance && info.state == PluginState::Running) {
                info.instance->onSessionStarted(sessionId);
            }
        }
    }
    
    void broadcastSessionEnded(const QString& sessionId) {
        for (auto& info : m_plugins) {
            if (info.instance && info.state == PluginState::Running) {
                info.instance->onSessionEnded(sessionId);
            }
        }
    }
    
    bool broadcastKeyEvent(int key, int modifiers, const QString& text) {
        for (auto& info : m_plugins) {
            if (info.instance && info.state == PluginState::Running) {
                if (info.instance->interceptKeyEvent(key, modifiers, text)) {
                    return true;
                }
            }
        }
        return false;
    }

signals:
    void pluginStarted(const QString& pluginId);
    void pluginStopped(const QString& pluginId);
    void pluginError(const QString& pluginId, const QString& error);
    void onPluginSendText(const QString& text);
    void onPluginNotification(const QString& title, const QString& message);

private:
    PluginMetadata loadMetadata(const QString& filePath) {
        PluginMetadata meta;
        QJsonObject obj = PluginLoader::parsePluginMetadata(filePath);
        
        meta.id = obj["id"].toString(QFileInfo(filePath).baseName());
        meta.name = obj["name"].toString(meta.id);
        meta.version = obj["version"].toString("1.0.0");
        meta.description = obj["description"].toString();
        meta.author = obj["author"].toString();
        
        if (obj.contains("type")) {
            QString typeStr = obj["type"].toString("TerminalHook");
            if (typeStr == "ThemeProvider") meta.type = PluginType::ThemeProvider;
            else if (typeStr == "CommandExtension") meta.type = PluginType::CommandExtension;
            else if (typeStr == "UISupplement") meta.type = PluginType::UISupplement;
            else if (typeStr == "ProtocolHandler") meta.type = PluginType::ProtocolHandler;
        }
        
        if (obj.contains("dependencies")) {
            QJsonArray deps = obj["dependencies"].toArray();
            for (const auto& dep : deps) {
                meta.dependencies.append(dep.toString());
            }
        }
        
        meta.extraData = obj;
        return meta;
    }
    
    QString findPluginPath(const QString& pluginId) const {
        for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
            if (it->metadata.id == pluginId) {
                return it.key();
            }
        }
        return QString();
    }
    
    bool checkDependencies(const QStringList& deps) const {
        for (const auto& dep : deps) {
            bool found = false;
            for (const auto& info : m_plugins) {
                if (info.metadata.id == dep && info.state >= PluginState::Running) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
    
    QStringList m_pluginDirs;
    PluginContext* m_context = nullptr;
    QMap<QString, PluginInfo> m_plugins;
    PluginLoader m_loader;
};

#endif
