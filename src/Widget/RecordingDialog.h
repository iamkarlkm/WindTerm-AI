#ifndef RECORDING_DIALOG_H
#define RECORDING_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSlider>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "TerminalRecording.h"

class RecordingDialog : public QDialog {
    Q_OBJECT
public:
    explicit RecordingDialog(QWidget* parent = nullptr);
    ~RecordingDialog() override;
    
    TerminalRecorder* recorder() { return m_recorder; }
    TerminalPlayer* player() { return m_player; }

signals:
    void recordOutput(const QByteArray& data);
    void playbackOutput(const QByteArray& data);

private slots:
    void onStartRecording();
    void onStopRecording();
    void onLoadRecording();
    void onStartPlayback();
    void onPausePlayback();
    void onStopPlayback();
    void onSpeedChanged(int value);
    void onRecordingStarted(const QString& filePath);
    void onRecordingStopped(const QString& filePath, int frameCount);
    void onPlaybackOutput(const QByteArray& data);
    void onPlaybackProgress(int current, int total, double percent);
    void onPlaybackFinished();
    
private:
    void setupUI();
    void updateRecordingState();
    void updatePlaybackState();
    
    TerminalRecorder* m_recorder;
    TerminalPlayer* m_player;
    
    QLabel* m_statusLabel;
    QLabel* m_recordingLabel;
    QLabel* m_playbackLabel;
    QPushButton* m_recordButton;
    QPushButton* m_stopRecordButton;
    QPushButton* m_loadButton;
    QPushButton* m_playButton;
    QPushButton* m_pauseButton;
    QPushButton* m_stopPlayButton;
    QSlider* m_speedSlider;
    QLabel* m_speedLabel;
    QProgressBar* m_progressBar;
    QLineEdit* m_filePathEdit;
    
    bool m_isRecording = false;
    bool m_isPlaying = false;
};

#endif
