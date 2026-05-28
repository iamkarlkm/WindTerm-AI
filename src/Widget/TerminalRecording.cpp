#include "TerminalRecording.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>

// TerminalRecorder implementation

TerminalRecorder::TerminalRecorder(QObject* parent) : QObject(parent) {}

TerminalRecorder::~TerminalRecorder() {
    if (m_isRecording) stopRecording();
}

bool TerminalRecorder::startRecording(const QString& filePath) {
    if (m_isRecording) return false;
    
    QDir dir(QFileInfo(filePath).absolutePath());
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit error("Failed to create directory");
            return false;
        }
    }
    
    m_frames.clear();
    m_currentFilePath = filePath;
    m_startTime = QDateTime::currentDateTime();
    m_elapsedSeconds = 0;
    m_isRecording = true;
    
    emit recordingStarted(filePath);
    qDebug() << "[TerminalRecorder] Recording started:" << filePath;
    return true;
}

void TerminalRecorder::stopRecording() {
    if (!m_isRecording) return;
    
    m_isRecording = false;
    
    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Failed to open file for writing");
        return;
    }
    
    QJsonArray frames;
    for (const auto& frame : m_frames) {
        QJsonArray frameArr;
        frameArr.append(frame.timestamp);
        frameArr.append("o");
        frameArr.append(QString::fromUtf8(frame.output));
        frames.append(frameArr);
    }
    
    QJsonObject header;
    header["version"] = 2;
    header["width"] = 80;
    header["height"] = 24;
    header["title"] = "WindTerm-AI Recording";
    header["env"] = QJsonObject{{"TERM", "xterm-256color"}};
    
    QJsonObject doc;
    doc["header"] = header;
    doc["stdout"] = frames;
    
    QJsonDocument jsonDoc(doc);
    file.write(jsonDoc.toJson(QJsonDocument::Compact));
    file.close();
    
    int count = m_frames.size();
    m_frames.clear();
    
    emit recordingStopped(m_currentFilePath, count);
    qDebug() << "[TerminalRecorder] Recording stopped:" << m_currentFilePath << "Frames:" << count;
}

void TerminalRecorder::recordOutput(const QByteArray& data) {
    if (!m_isRecording || data.isEmpty()) return;
    
    RecordingFrame frame;
    frame.timestamp = m_startTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    frame.output = data;
    m_frames.append(frame);
}

qint64 TerminalRecorder::duration() const {
    return m_frames.isEmpty() ? 0 : 
        static_cast<qint64>(m_frames.last().timestamp * 1000);
}

bool TerminalRecorder::exportAsciinema(const QString& filePath) {
    if (m_frames.isEmpty()) return false;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Failed to open file for export");
        return false;
    }
    
    QJsonArray frames;
    for (const auto& frame : m_frames) {
        QJsonArray frameArr;
        frameArr.append(frame.timestamp);
        frameArr.append("o");
        frameArr.append(QString::fromUtf8(frame.output));
        frames.append(frameArr);
    }
    
    QJsonObject header;
    header["version"] = 2;
    header["width"] = 80;
    header["height"] = 24;
    header["title"] = "WindTerm-AI Recording";
    header["env"] = QJsonObject{{"TERM", "xterm-256color"}};
    
    QJsonObject doc;
    doc["header"] = header;
    doc["stdout"] = frames;
    
    QJsonDocument jsonDoc(doc);
    file.write(jsonDoc.toJson(QJsonDocument::Compact));
    file.close();
    
    return true;
}

// TerminalPlayer implementation

TerminalPlayer::TerminalPlayer(QObject* parent) : QObject(parent) {
    m_playTimer = new QTimer(this);
    m_playTimer->setSingleShot(true);
    connect(m_playTimer, &QTimer::timeout, this, &TerminalPlayer::onPlayTimer);
}

bool TerminalPlayer::loadRecording(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error("Failed to open recording file");
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        emit error("Invalid recording file");
        return false;
    }
    
    QJsonObject obj = doc.object();
    if (!obj.contains("stdout")) {
        emit error("Invalid recording format");
        return false;
    }
    
    QJsonArray frames = obj["stdout"].toArray();
    m_frames.clear();
    
    for (const auto& frameVal : frames) {
        QJsonArray frameArr = frameVal.toArray();
        if (frameArr.size() >= 3) {
            RecordingFrame frame;
            frame.timestamp = frameArr[0].toDouble();
            frame.output = frameArr[2].toString().toUtf8();
            m_frames.append(frame);
        }
    }
    
    m_currentFrame = 0;
    qDebug() << "[TerminalPlayer] Loaded recording:" << filePath << "Frames:" << m_frames.size();
    return true;
}

bool TerminalPlayer::loadAsciinema(const QString& filePath) {
    return loadRecording(filePath);
}

void TerminalPlayer::startPlayback() {
    if (m_frames.isEmpty()) return;
    
    if (m_isPaused) {
        m_isPaused = false;
        m_isPlaying = true;
        onPlayTimer();
        emit playbackStarted();
        return;
    }
    
    m_currentFrame = 0;
    m_isPlaying = true;
    m_isPaused = false;
    
    emit playbackStarted();
    onPlayTimer();
}

void TerminalPlayer::pausePlayback() {
    if (!m_isPlaying) return;
    
    m_isPaused = !m_isPaused;
    if (m_isPaused) {
        m_playTimer->stop();
        emit playbackPaused();
    } else {
        onPlayTimer();
        emit playbackStarted();
    }
}

void TerminalPlayer::stopPlayback() {
    m_playTimer->stop();
    m_isPlaying = false;
    m_isPaused = false;
    m_currentFrame = 0;
    emit playbackStopped();
}

double TerminalPlayer::progress() const {
    if (m_frames.isEmpty()) return 0.0;
    return static_cast<double>(m_currentFrame) / m_frames.size();
}

void TerminalPlayer::onPlayTimer() {
    if (m_isPaused || !m_isPlaying) return;
    
    if (m_currentFrame >= m_frames.size()) {
        stopPlayback();
        emit playbackFinished();
        return;
    }
    
    playNextFrame();
    
    if (m_currentFrame < m_frames.size()) {
        double delay = (m_frames[m_currentFrame].timestamp - 
                       (m_currentFrame > 0 ? m_frames[m_currentFrame - 1].timestamp : 0)) * 1000.0 / m_playbackSpeed;
        delay = qMax(1.0, delay);
        m_playTimer->start(static_cast<int>(delay));
    }
}

void TerminalPlayer::playNextFrame() {
    if (m_currentFrame < m_frames.size()) {
        emit outputReceived(m_frames[m_currentFrame].output);
        emit progressChanged(m_currentFrame + 1, m_frames.size(), progress());
        m_currentFrame++;
    }
}
