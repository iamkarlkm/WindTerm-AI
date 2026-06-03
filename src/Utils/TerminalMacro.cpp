#include "TerminalMacro.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QThread>

TerminalMacroRecorder* TerminalMacroRecorder::s_instance = nullptr;

TerminalMacroRecorder::TerminalMacroRecorder(QObject* parent)
    : QObject(parent)
    , m_isRecording(false)
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_currentActionIndex(0)
    , m_playbackTimer(new QTimer(this)) {
    
    m_recordingFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/macros";
    QDir().mkpath(m_recordingFile);
    
    loadRecordings();
    
    connect(m_playbackTimer, &QTimer::timeout, this, &TerminalMacroRecorder::onPlaybackTimeout);
}

TerminalMacroRecorder* TerminalMacroRecorder::instance() {
    if (!s_instance) {
        s_instance = new TerminalMacroRecorder();
    }
    return s_instance;
}

void TerminalMacroRecorder::startRecording(const QString& sessionId) {
    if (m_isRecording) return;
    
    m_isRecording = true;
    m_currentSessionId = sessionId;
    m_currentActions.clear();
    
    qDebug() << "[TerminalMacro] Started recording session:" << sessionId;
    emit recordingStarted();
}

void TerminalMacroRecorder::stopRecording() {
    if (!m_isRecording) return;
    
    m_isRecording = false;
    m_currentSessionId.clear();
    
    qDebug() << "[TerminalMacro] Stopped recording," << m_currentActions.size() << "actions";
}

void TerminalMacroRecorder::recordKeyPress(int key, Qt::KeyboardModifiers modifiers) {
    if (!m_isRecording) return;
    
    MacroAction action;
    action.type = MacroActionType::SendKey;
    action.data = QVariant::fromValue(qMakePair(key, (int)modifiers));
    action.delay = 50;
    
    m_currentActions.append(action);
}

void TerminalMacroRecorder::recordText(const QString& text) {
    if (!m_isRecording) return;
    
    MacroAction action;
    action.type = MacroActionType::SendText;
    action.parameter = text;
    action.delay = 50;
    
    m_currentActions.append(action);
}

void TerminalMacroRecorder::recordWait(int milliseconds) {
    if (!m_isRecording) return;
    
    MacroAction action;
    action.type = MacroActionType::Wait;
    action.delay = milliseconds;
    
    m_currentActions.append(action);
}

void TerminalMacroRecorder::recordWaitForText(const QString& text, int timeout) {
    if (!m_isRecording) return;
    
    MacroAction action;
    action.type = MacroActionType::WaitForText;
    action.parameter = text;
    action.delay = timeout;
    
    m_currentActions.append(action);
}

QString TerminalMacroRecorder::saveRecording(const QString& name, const QString& description) {
    if (!m_isRecording && m_currentActions.isEmpty()) {
        return QString();
    }
    
    MacroRecording recording;
    recording.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    recording.name = name;
    recording.description = description;
    recording.actions = m_currentActions;
    recording.createdAt = QDateTime::currentMSecsSinceEpoch();
    recording.modifiedAt = recording.createdAt;
    recording.usageCount = 0;
    recording.averageDuration = 0;
    recording.category = "General";
    
    m_recordings[recording.id] = recording;
    
    // 保存录制后清空当前动作
    m_currentActions.clear();
    m_isRecording = false;
    
    saveRecordings();
    
    emit recordingStopped(recording.id);
    
    qDebug() << "[TerminalMacro] Saved recording:" << recording.id << name;
    
    return recording.id;
}

void TerminalMacroRecorder::deleteRecording(const QString& id) {
    m_recordings.remove(id);
    saveRecordings();
}

MacroRecording TerminalMacroRecorder::getRecording(const QString& id) const {
    return m_recordings.value(id);
}

QList<MacroRecording> TerminalMacroRecorder::getAllRecordings() const {
    return m_recordings.values();
}

QList<MacroRecording> TerminalMacroRecorder::getRecordingsByCategory(const QString& category) const {
    QList<MacroRecording> result;
    for (auto it = m_recordings.begin(); it != m_recordings.end(); ++it) {
        if (it->category == category) {
            result.append(it.value());
        }
    }
    return result;
}

QStringList TerminalMacroRecorder::getCategories() const {
    QStringList categories;
    QSet<QString> catSet;
    for (auto it = m_recordings.begin(); it != m_recordings.end(); ++it) {
        catSet.insert(it->category);
    }
    categories = catSet.values();
    categories.sort();
    return categories;
}

bool TerminalMacroRecorder::playMacro(const QString& id, const QString& sessionId) {
    if (!m_recordings.contains(id)) return false;
    
    if (m_isPlaying) {
        stopPlayback();
    }
    
    const MacroRecording& recording = m_recordings[id];
    
    m_isPlaying = true;
    m_isPaused = false;
    m_currentSessionId = sessionId;
    m_currentActions = recording.actions;
    m_currentActionIndex = 0;
    m_playbackStartTime.start();
    
    qDebug() << "[TerminalMacro] Started playing macro:" << id << recording.name;
    emit playbackStarted();
    emit playbackProgress(0, m_currentActions.size());
    
    // 执行第一个动作
    if (!m_currentActions.isEmpty()) {
        executeAction(m_currentActions[0], sessionId);
    }
    
    return true;
}

void TerminalMacroRecorder::stopPlayback() {
    m_isPlaying = false;
    m_isPaused = false;
    m_currentActions.clear();
    m_currentActionIndex = 0;
    m_playbackTimer->stop();
    
    emit playbackFinished();
}

void TerminalMacroRecorder::pausePlayback() {
    if (!m_isPlaying || m_isPaused) return;
    
    m_isPaused = true;
    m_playbackTimer->stop();
}

void TerminalMacroRecorder::resumePlayback() {
    if (!m_isPaused) return;
    
    m_isPaused = false;
    executeAction(m_currentActions[m_currentActionIndex], m_currentSessionId);
}

void TerminalMacroRecorder::addManualAction(const QString& macroId, const MacroAction& action) {
    if (!m_recordings.contains(macroId)) return;
    
    m_recordings[macroId].actions.append(action);
    m_recordings[macroId].modifiedAt = QDateTime::currentMSecsSinceEpoch();
    
    saveRecordings();
}

void TerminalMacroRecorder::removeAction(const QString& macroId, int index) {
    if (!m_recordings.contains(macroId)) return;
    
    m_recordings[macroId].actions.removeAt(index);
    m_recordings[macroId].modifiedAt = QDateTime::currentMSecsSinceEpoch();
    
    saveRecordings();
}

void TerminalMacroRecorder::updateAction(const QString& macroId, int index, const MacroAction& action) {
    if (!m_recordings.contains(macroId)) return;
    
    m_recordings[macroId].actions[index] = action;
    m_recordings[macroId].modifiedAt = QDateTime::currentMSecsSinceEpoch();
    
    saveRecordings();
}

void TerminalMacroRecorder::exportMacro(const QString& id, const QString& filePath) {
    if (!m_recordings.contains(id)) return;
    
    const MacroRecording& recording = m_recordings[id];
    
    QJsonArray actionsJson;
    for (const MacroAction& action : recording.actions) {
        QJsonObject json;
        json["type"] = static_cast<int>(action.type);
        json["parameter"] = action.parameter;
        json["delay"] = action.delay;
        json["repeat"] = action.repeat;
        json["data"] = QJsonValue::fromVariant(action.data);
        actionsJson.append(json);
    }
    
    QJsonObject json;
    json["id"] = recording.id;
    json["name"] = recording.name;
    json["description"] = recording.description;
    json["category"] = recording.category;
    json["triggerShortcut"] = recording.triggerShortcut;
    json["actions"] = actionsJson;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
}

void TerminalMacroRecorder::importMacro(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject json = doc.object();
    
    MacroRecording recording;
    recording.id = json["id"].toString();
    recording.name = json["name"].toString();
    recording.description = json["description"].toString();
    recording.category = json["category"].toString("General");
    recording.triggerShortcut = json["triggerShortcut"].toString();
    recording.createdAt = QDateTime::currentMSecsSinceEpoch();
    recording.modifiedAt = recording.createdAt;
    
    QJsonArray actionsJson = json["actions"].toArray();
    for (const QJsonValue& value : actionsJson) {
        QJsonObject actionJson = value.toObject();
        
        MacroAction action;
        action.type = static_cast<MacroActionType>(actionJson["type"].toInt());
        action.parameter = actionJson["parameter"].toString();
        action.delay = actionJson["delay"].toInt(0);
        action.repeat = actionJson["repeat"].toInt(1);
        action.data = actionJson["data"].toVariant();
        
        recording.actions.append(action);
    }
    
    m_recordings[recording.id] = recording;
    saveRecordings();
}

void TerminalMacroRecorder::setVariable(const QString& name, const QVariant& value) {
    m_variables[name] = value;
}

QVariant TerminalMacroRecorder::getVariable(const QString& name) const {
    return m_variables.value(name);
}

void TerminalMacroRecorder::clearVariables() {
    m_variables.clear();
}

bool TerminalMacroRecorder::executeAction(const MacroAction& action, const QString& sessionId) {
    Q_UNUSED(sessionId)
    
    switch (action.type) {
        case MacroActionType::SendText:
            return executeSendText(interpolateVariables(action.parameter), sessionId);
            
        case MacroActionType::SendKey: {
            QPair<int, int> keyData = action.data.value<QPair<int, int>>();
            return executeSendKey(keyData.first, static_cast<Qt::KeyboardModifiers>(keyData.second), sessionId);
        }
            
        case MacroActionType::Wait:
            if (action.delay > 0) {
                m_playbackTimer->start(action.delay);
                return true;
            }
            break;
            
        case MacroActionType::WaitForText:
            return executeWaitForText(action.parameter, action.delay, sessionId);
            
        default:
            break;
    }
    
    // 执行下一个动作
    m_currentActionIndex++;
    if (m_currentActionIndex >= m_currentActions.size()) {
        stopPlayback();
    } else {
        emit playbackProgress(m_currentActionIndex, m_currentActions.size());
        emit actionExecuted(m_currentActionIndex);
        executeAction(m_currentActions[m_currentActionIndex], sessionId);
    }
    
    return true;
}

bool TerminalMacroRecorder::executeSendText(const QString& text, const QString& sessionId) {
    // In real implementation, send text to terminal session
    Q_UNUSED(sessionId)
    qDebug() << "[TerminalMacro] Send text:" << text;
    return true;
}

bool TerminalMacroRecorder::executeSendKey(int key, Qt::KeyboardModifiers modifiers, const QString& sessionId) {
    // In real implementation, send key to terminal session
    Q_UNUSED(sessionId)
    qDebug() << "[TerminalMacro] Send key:" << key << modifiers;
    return true;
}

bool TerminalMacroRecorder::executeWaitForText(const QString& text, int timeout, const QString& sessionId) {
    // In real implementation, wait for text in terminal output
    Q_UNUSED(sessionId)
    Q_UNUSED(text)
    Q_UNUSED(timeout)
    qDebug() << "[TerminalMacro] Wait for text:" << text;
    return true;
}

void TerminalMacroRecorder::onPlaybackTimeout() {
    m_playbackTimer->stop();
    
    // 执行下一个动作
    m_currentActionIndex++;
    if (m_currentActionIndex >= m_currentActions.size()) {
        stopPlayback();
    } else {
        emit playbackProgress(m_currentActionIndex, m_currentActions.size());
        executeAction(m_currentActions[m_currentActionIndex], m_currentSessionId);
    }
}

QString TerminalMacroRecorder::interpolateVariables(const QString& text) const {
    QString result = text;
    
    for (auto it = m_variables.begin(); it != m_variables.end(); ++it) {
        result.replace(QString("${%1}").arg(it.key()), it.value().toString());
    }
    
    return result;
}

void TerminalMacroRecorder::saveRecordings() {
    QJsonArray recordingsJson;
    
    for (auto it = m_recordings.begin(); it != m_recordings.end(); ++it) {
        const MacroRecording& recording = it.value();
        
        QJsonArray actionsJson;
        for (const MacroAction& action : recording.actions) {
            QJsonObject json;
            json["type"] = static_cast<int>(action.type);
            json["parameter"] = action.parameter;
            json["delay"] = action.delay;
            json["repeat"] = action.repeat;
            json["data"] = QJsonValue::fromVariant(action.data);
            actionsJson.append(json);
        }
        
        QJsonObject json;
        json["id"] = recording.id;
        json["name"] = recording.name;
        json["description"] = recording.description;
        json["category"] = recording.category;
        json["triggerShortcut"] = recording.triggerShortcut;
        json["createdAt"] = recording.createdAt;
        json["modifiedAt"] = recording.modifiedAt;
        json["usageCount"] = recording.usageCount;
        json["actions"] = actionsJson;
        
        recordingsJson.append(json);
    }
    
    QFile file(m_recordingFile + "/macros.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(recordingsJson).toJson(QJsonDocument::Indented));
    }
}

void TerminalMacroRecorder::loadRecordings() {
    QFile file(m_recordingFile + "/macros.json");
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonArray recordingsJson = QJsonDocument::fromJson(file.readAll()).array();
    
    for (const QJsonValue& value : recordingsJson) {
        QJsonObject json = value.toObject();
        
        MacroRecording recording;
        recording.id = json["id"].toString();
        recording.name = json["name"].toString();
        recording.description = json["description"].toString();
        recording.category = json["category"].toString("General");
        recording.triggerShortcut = json["triggerShortcut"].toString();
        recording.createdAt = json["createdAt"].toVariant().toLongLong();
        recording.modifiedAt = json["modifiedAt"].toVariant().toLongLong();
        recording.usageCount = json["usageCount"].toInt(0);
        
        QJsonArray actionsJson = json["actions"].toArray();
        for (const QJsonValue& actionValue : actionsJson) {
            QJsonObject actionJson = actionValue.toObject();
            
            MacroAction action;
            action.type = static_cast<MacroActionType>(actionJson["type"].toInt());
            action.parameter = actionJson["parameter"].toString();
            action.delay = actionJson["delay"].toInt(0);
            action.repeat = actionJson["repeat"].toInt(1);
            action.data = actionJson["data"].toVariant();
            
            recording.actions.append(action);
        }
        
        m_recordings[recording.id] = recording;
    }
    
    qDebug() << "[TerminalMacro] Loaded" << m_recordings.size() << "macros";
}

#include "TerminalMacro.moc"
