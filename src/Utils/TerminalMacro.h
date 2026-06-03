#ifndef TERMINAL_MACRO_H
#define TERMINAL_MACRO_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QElapsedTimer>

enum class MacroActionType {
    SendText,
    SendKey,
    Wait,
    WaitForText,
    RunCommand,
    CaptureOutput,
    IfCondition,
    Loop
};

struct MacroAction {
    MacroActionType type;
    QString parameter;
    int delay;  // milliseconds
    int repeat;
    QString condition;
    QVariant data;
    
    MacroAction() : type(MacroActionType::SendText), delay(0), repeat(1) {}
};

struct MacroRecording {
    QString id;
    QString name;
    QString description;
    QList<MacroAction> actions;
    qint64 createdAt;
    qint64 modifiedAt;
    int usageCount;
    int averageDuration;  // milliseconds
    QString triggerShortcut;
    QString category;
    
    MacroRecording() : createdAt(0), modifiedAt(0), usageCount(0), averageDuration(0) {}
};

class TerminalMacroRecorder : public QObject {
    Q_OBJECT
public:
    explicit TerminalMacroRecorder(QObject* parent = nullptr);
    
    static TerminalMacroRecorder* instance();
    
    // 录制控制
    void startRecording(const QString& sessionId);
    void stopRecording();
    bool isRecording() const { return m_isRecording; }
    
    // 动作记录
    void recordKeyPress(int key, Qt::KeyboardModifiers modifiers);
    void recordText(const QString& text);
    void recordWait(int milliseconds);
    void recordWaitForText(const QString& text, int timeout = 5000);
    
    // 宏管理
    QString saveRecording(const QString& name, const QString& description = "");
    void deleteRecording(const QString& id);
    MacroRecording getRecording(const QString& id) const;
    QList<MacroRecording> getAllRecordings() const;
    QList<MacroRecording> getRecordingsByCategory(const QString& category) const;
    QStringList getCategories() const;
    
    // 宏播放
    bool playMacro(const QString& id, const QString& sessionId);
    void stopPlayback();
    bool isPlaying() const { return m_isPlaying; }
    void pausePlayback();
    void resumePlayback();
    bool isPaused() const { return m_isPaused; }
    
    // 宏编辑
    void addManualAction(const QString& macroId, const MacroAction& action);
    void removeAction(const QString& macroId, int index);
    void updateAction(const QString& macroId, int index, const MacroAction& action);
    
    // 导入导出
    void exportMacro(const QString& id, const QString& filePath);
    void importMacro(const QString& filePath);
    
    // 变量系统
    void setVariable(const QString& name, const QVariant& value);
    QVariant getVariable(const QString& name) const;
    void clearVariables();
    
signals:
    void recordingStarted();
    void recordingStopped(const QString& recordingId);
    void playbackStarted();
    void playbackFinished();
    void playbackProgress(int current, int total);
    void actionExecuted(int index);
    void errorOccurred(const QString& message);

private slots:
    void onPlaybackTimeout();

private:
    static TerminalMacroRecorder* s_instance;
    
    bool executeAction(const MacroAction& action, const QString& sessionId);
    bool executeSendText(const QString& text, const QString& sessionId);
    bool executeSendKey(int key, Qt::KeyboardModifiers modifiers, const QString& sessionId);
    bool executeWaitForText(const QString& text, int timeout, const QString& sessionId);
    
    QString interpolateVariables(const QString& text) const;
    
    bool m_isRecording;
    bool m_isPlaying;
    bool m_isPaused;
    QString m_currentSessionId;
    QList<MacroAction> m_currentActions;
    
    QMap<QString, MacroRecording> m_recordings;
    QString m_recordingFile;
    
    QMap<QString, QVariant> m_variables;
    
    QTimer* m_playbackTimer;
    int m_currentActionIndex;
    QElapsedTimer m_playbackStartTime;
};

#endif
