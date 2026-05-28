#ifndef TERMINAL_RECORDING_H
#define TERMINAL_RECORDING_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QDateTime>

struct RecordingFrame {
    double timestamp;
    QByteArray output;
};

class TerminalRecorder : public QObject {
    Q_OBJECT
public:
    explicit TerminalRecorder(QObject* parent = nullptr);
    ~TerminalRecorder() override;
    
    bool startRecording(const QString& filePath);
    void stopRecording();
    void recordOutput(const QByteArray& data);
    
    bool isRecording() const { return m_isRecording; }
    QString currentFilePath() const { return m_currentFilePath; }
    int frameCount() const { return m_frames.size(); }
    qint64 duration() const;
    
    bool exportAsciinema(const QString& filePath);
    
signals:
    void recordingStarted(const QString& filePath);
    void recordingStopped(const QString& filePath, int frameCount);
    void error(const QString& message);

private:
    bool m_isRecording = false;
    QString m_currentFilePath;
    QVector<RecordingFrame> m_frames;
    QDateTime m_startTime;
    double m_elapsedSeconds = 0;
};

class TerminalPlayer : public QObject {
    Q_OBJECT
public:
    explicit TerminalPlayer(QObject* parent = nullptr);
    
    bool loadRecording(const QString& filePath);
    bool loadAsciinema(const QString& filePath);
    void startPlayback();
    void pausePlayback();
    void stopPlayback();
    void setPlaybackSpeed(double speed) { m_playbackSpeed = speed; }
    
    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }
    int totalFrames() const { return m_frames.size(); }
    int currentFrame() const { return m_currentFrame; }
    double progress() const;
    
signals:
    void outputReceived(const QByteArray& data);
    void playbackStarted();
    void playbackPaused();
    void playbackStopped();
    void playbackFinished();
    void progressChanged(int current, int total, double percent);
    void error(const QString& message);

private slots:
    void onPlayTimer();

private:
    void playNextFrame();
    
    QVector<RecordingFrame> m_frames;
    int m_currentFrame = 0;
    double m_playbackSpeed = 1.0;
    bool m_isPlaying = false;
    bool m_isPaused = false;
    QTimer* m_playTimer;
};

#endif
