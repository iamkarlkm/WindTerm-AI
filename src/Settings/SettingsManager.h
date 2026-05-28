#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>

class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(QObject* parent = nullptr);
    
    static SettingsManager* instance(QObject* parent = nullptr);
    
    bool exportSettings(const QString& filePath, bool exportThemes = true, 
                        bool exportBookmarks = true, bool exportConnections = true,
                        bool exportCommandHistory = true);
    
    bool importSettings(const QString& filePath, bool importThemes = true,
                        bool importBookmarks = true, bool importConnections = true,
                        bool importCommandHistory = true);
    
    QString getExportData(bool themes, bool bookmarks, bool connections, bool history) const;
    bool importData(const QString& jsonData, bool themes, bool bookmarks, bool connections, bool history);
    
private:
    QJsonObject collectThemes() const;
    QJsonObject collectBookmarks() const;
    QJsonObject collectConnections() const;
    QJsonObject collectCommandHistory() const;
    
    bool restoreThemes(const QJsonObject& data);
    bool restoreBookmarks(const QJsonObject& data);
    bool restoreConnections(const QJsonObject& data);
    bool restoreCommandHistory(const QJsonObject& data);
    
    static SettingsManager* s_instance;
};

#endif
