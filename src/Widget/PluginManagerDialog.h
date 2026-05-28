#ifndef PLUGIN_MANAGER_DIALOG_H
#define PLUGIN_MANAGER_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include "../plugins/PluginManager.h"

class PluginManagerDialog : public QDialog {
    Q_OBJECT
public:
    explicit PluginManagerDialog(PluginManager* manager, QWidget* parent = nullptr);
    
private slots:
    void onPluginSelected();
    void onLoadPlugin();
    void onStartPlugin();
    void onStopPlugin();
    void onUnloadPlugin();
    void onRefresh();
    
private:
    void setupUI();
    void updatePluginList();
    void updateButtons();
    QString stateToString(PluginState state);
    
    PluginManager* m_manager;
    QListWidget* m_pluginList;
    QPushButton* m_loadButton;
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QPushButton* m_unloadButton;
    QPushButton* m_refreshButton;
    QPushButton* m_closeButton;
    QLabel* m_infoLabel;
};

#endif
