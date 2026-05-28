#include "RecordingDialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QDir>
#include <QStandardPaths>

RecordingDialog::RecordingDialog(QWidget* parent)
    : QDialog(parent) {
    m_recorder = new TerminalRecorder(this);
    m_player = new TerminalPlayer(this);
    setupUI();
    
    connect(m_recorder, &TerminalRecorder::recordingStarted, this, &RecordingDialog::onRecordingStarted);
    connect(m_recorder, &TerminalRecorder::recordingStopped, this, &RecordingDialog::onRecordingStopped);
    connect(m_recorder, &TerminalRecorder::error, this, [this](const QString& err) {
        QMessageBox::critical(this, "Recording Error", err);
    });
    
    connect(m_player, &TerminalPlayer::outputReceived, this, &RecordingDialog::onPlaybackOutput);
    connect(m_player, &TerminalPlayer::progressChanged, this, &RecordingDialog::onPlaybackProgress);
    connect(m_player, &TerminalPlayer::playbackFinished, this, &RecordingDialog::onPlaybackFinished);
    connect(m_player, &TerminalPlayer::error, this, [this](const QString& err) {
        QMessageBox::critical(this, "Playback Error", err);
    });
}

RecordingDialog::~RecordingDialog() {
    if (m_isRecording) m_recorder->stopRecording();
    if (m_isPlaying) m_player->stopPlayback();
}

void RecordingDialog::setupUI() {
    setWindowTitle("Terminal Recording & Playback");
    resize(500, 400);
    
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(new QLabel("File:"));
    m_filePathEdit = new QLineEdit(this);
    m_filePathEdit->setPlaceholderText("Recording file path (.cast)");
    fileLayout->addWidget(m_filePathEdit);
    
    m_loadButton = new QPushButton("Load", this);
    fileLayout->addWidget(m_loadButton);
    
    mainLayout->addLayout(fileLayout);
    
    auto* recordGroup = new QGroupBox("Recording", this);
    auto* recordLayout = new QVBoxLayout(recordGroup);
    
    m_recordingLabel = new QLabel("Not recording", recordGroup);
    recordLayout->addWidget(m_recordingLabel);
    
    auto* recordBtnLayout = new QHBoxLayout();
    m_recordButton = new QPushButton("Start Recording", recordGroup);
    m_stopRecordButton = new QPushButton("Stop", recordGroup);
    m_stopRecordButton->setEnabled(false);
    recordBtnLayout->addWidget(m_recordButton);
    recordBtnLayout->addWidget(m_stopRecordButton);
    recordLayout->addLayout(recordBtnLayout);
    
    mainLayout->addWidget(recordGroup);
    
    auto* playbackGroup = new QGroupBox("Playback", this);
    auto* playbackLayout = new QVBoxLayout(playbackGroup);
    
    m_playbackLabel = new QLabel("No recording loaded", playbackGroup);
    playbackLayout->addWidget(m_playbackLabel);
    
    m_progressBar = new QProgressBar(playbackGroup);
    m_progressBar->setValue(0);
    playbackLayout->addWidget(m_progressBar);
    
    auto* speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Speed:", playbackGroup));
    m_speedSlider = new QSlider(Qt::Horizontal, playbackGroup);
    m_speedSlider->setRange(1, 4);
    m_speedSlider->setValue(1);
    m_speedSlider->setTickPosition(QSlider::TicksBelow);
    m_speedSlider->setTickInterval(1);
    speedLayout->addWidget(m_speedSlider);
    m_speedLabel = new QLabel("1x", playbackGroup);
    speedLayout->addWidget(m_speedLabel);
    playbackLayout->addLayout(speedLayout);
    
    auto* playBtnLayout = new QHBoxLayout();
    m_playButton = new QPushButton("Play", playbackGroup);
    m_pauseButton = new QPushButton("Pause", playbackGroup);
    m_stopPlayButton = new QPushButton("Stop", playbackGroup);
    m_playButton->setEnabled(false);
    m_pauseButton->setEnabled(false);
    m_stopPlayButton->setEnabled(false);
    playBtnLayout->addWidget(m_playButton);
    playBtnLayout->addWidget(m_pauseButton);
    playBtnLayout->addWidget(m_stopPlayButton);
    playbackLayout->addLayout(playBtnLayout);
    
    mainLayout->addWidget(playbackGroup);
    
    m_statusLabel = new QLabel("Ready", this);
    mainLayout->addWidget(m_statusLabel);
    
    connect(m_recordButton, &QPushButton::clicked, this, &RecordingDialog::onStartRecording);
    connect(m_stopRecordButton, &QPushButton::clicked, this, &RecordingDialog::onStopRecording);
    connect(m_loadButton, &QPushButton::clicked, this, &RecordingDialog::onLoadRecording);
    connect(m_playButton, &QPushButton::clicked, this, &RecordingDialog::onStartPlayback);
    connect(m_pauseButton, &QPushButton::clicked, this, &RecordingDialog::onPausePlayback);
    connect(m_stopPlayButton, &QPushButton::clicked, this, &RecordingDialog::onStopPlayback);
    connect(m_speedSlider, &QSlider::valueChanged, this, &RecordingDialog::onSpeedChanged);
}

void RecordingDialog::onStartRecording() {
    QString filePath = m_filePathEdit->text().trimmed();
    if (filePath.isEmpty()) {
        filePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + 
                   "/windterm_recording.cast";
        m_filePathEdit->setText(filePath);
    }
    
    if (m_recorder->startRecording(filePath)) {
        m_isRecording = true;
        updateRecordingState();
    }
}

void RecordingDialog::onStopRecording() {
    if (m_isRecording) {
        m_recorder->stopRecording();
    }
}

void RecordingDialog::onLoadRecording() {
    QString filePath = QFileDialog::getOpenFileName(this, "Load Recording", QString(), "Asciinema Files (*.cast)");
    if (filePath.isEmpty()) return;
    
    m_filePathEdit->setText(filePath);
    
    if (m_player->loadRecording(filePath)) {
        m_playbackLabel->setText(QString("Loaded: %1 frames").arg(m_player->totalFrames()));
        m_playButton->setEnabled(true);
        m_statusLabel->setText("Recording loaded");
    }
}

void RecordingDialog::onStartPlayback() {
    if (m_player->totalFrames() > 0) {
        m_isPlaying = true;
        m_player->startPlayback();
        updatePlaybackState();
    }
}

void RecordingDialog::onPausePlayback() {
    m_player->pausePlayback();
    updatePlaybackState();
}

void RecordingDialog::onStopPlayback() {
    m_player->stopPlayback();
    m_isPlaying = false;
    m_progressBar->setValue(0);
    updatePlaybackState();
}

void RecordingDialog::onSpeedChanged(int value) {
    double speed = static_cast<double>(value);
    m_player->setPlaybackSpeed(speed);
    m_speedLabel->setText(QString("%1x").arg(speed, 0, 'f', 0));
}

void RecordingDialog::onRecordingStarted(const QString& filePath) {
    m_recordingLabel->setText(QString("Recording: %1").arg(QFileInfo(filePath).fileName()));
    m_statusLabel->setText("Recording started");
    updateRecordingState();
}

void RecordingDialog::onRecordingStopped(const QString& filePath, int frameCount) {
    m_recordingLabel->setText(QString("Saved: %1 (%2 frames)").arg(QFileInfo(filePath).fileName()).arg(frameCount));
    m_statusLabel->setText(QString("Recording saved: %1 frames").arg(frameCount));
    m_isRecording = false;
    updateRecordingState();
    
    if (m_player->loadRecording(filePath)) {
        m_playbackLabel->setText(QString("Loaded: %1 frames").arg(m_player->totalFrames()));
        m_playButton->setEnabled(true);
    }
}

void RecordingDialog::onPlaybackOutput(const QByteArray& data) {
    emit playbackOutput(data);
}

void RecordingDialog::onPlaybackProgress(int current, int total, double percent) {
    m_progressBar->setValue(static_cast<int>(percent * 100));
    m_playbackLabel->setText(QString("Playing: %1/%2").arg(current).arg(total));
}

void RecordingDialog::onPlaybackFinished() {
    m_isPlaying = false;
    m_progressBar->setValue(100);
    m_playbackLabel->setText("Playback finished");
    m_statusLabel->setText("Playback completed");
    updatePlaybackState();
}

void RecordingDialog::updateRecordingState() {
    m_recordButton->setEnabled(!m_isRecording);
    m_stopRecordButton->setEnabled(m_isRecording);
}

void RecordingDialog::updatePlaybackState() {
    m_playButton->setEnabled(!m_isPlaying && m_player->totalFrames() > 0);
    m_pauseButton->setEnabled(m_isPlaying);
    m_stopPlayButton->setEnabled(m_isPlaying || m_player->currentFrame() > 0);
}
