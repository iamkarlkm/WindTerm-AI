#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QPluginLoader>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QMap>
#include "PluginInterface.h"

class PluginLoader : public QObject {
    Q_OBJECT
public:
    explicit PluginLoader(QObject* parent = nullptr) : QObject(parent) {}
    
    PluginInterface* loadPlugin(const QString& filePath) {
        auto* loader = new QPluginLoader(filePath, this);
        loader->setLoadHints(QLibrary::ExportExternalSymbolsHint);
        
        QObject* instance = loader->instance();
        if (!instance) {
            m_lastError = loader->errorString();
            delete loader;
            return nullptr;
        }
        
        auto* plugin = qobject_cast<PluginInterface*>(instance);
        if (!plugin) {
            m_lastError = QString("Plugin does not implement PluginInterface: %1").arg(filePath);
            loader->unload();
            delete loader;
            return nullptr;
        }
        
        m_loaders[filePath] = loader;
        return plugin;
    }
    
    bool unloadPlugin(const QString& filePath) {
        if (m_loaders.contains(filePath)) {
            auto* loader = m_loaders.take(filePath);
            bool ok = loader->unload();
            delete loader;
            return ok;
        }
        return false;
    }
    
    bool isLoaded(const QString& filePath) const {
        return m_loaders.contains(filePath);
    }
    
    QString lastError() const { return m_lastError; }
    
    static QJsonObject parsePluginMetadata(const QString& filePath) {
        QFileInfo fi(filePath);
        QString metaPath = fi.absolutePath() + "/" + fi.completeBaseName() + ".json";
        
        QFile file(metaPath);
        if (!file.open(QIODevice::ReadOnly)) {
            return QJsonObject();
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        return doc.isObject() ? doc.object() : QJsonObject();
    }

private:
    QMap<QString, QPluginLoader*> m_loaders;
    QString m_lastError;
};

#endif
