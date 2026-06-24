#include "TerminalSession.h"
#include "Utility/TerminalOutputFilter.h"
#include <QApplication>
#include <QClipboard>
#include <QDebug>

TerminalSession::TerminalSession(QObject* parent)
    : QObject(parent), m_pty(nullptr), m_state(nullptr), m_needsRenderUpdate(false), m_bracketedPasteEnabled(false) {
    m_pty = new PtyManager(this);
    m_state = new TerminalState(24, 80, this);
    
    connect(m_pty, &PtyManager::dataReceived, this, &TerminalSession::onPtyData);
    connect(m_pty, &PtyManager::errorOccurred, this, &TerminalSession::onPtyError);
    connect(m_pty, &PtyManager::processFinished, this, &TerminalSession::onPtyFinished);
    
    connect(m_state, &TerminalState::cursorMoved, this, &TerminalSession::cursorMoved);
    connect(m_state, &TerminalState::screenUpdated, this, &TerminalSession::screenUpdated);
    connect(m_state, &TerminalState::scrollbackChanged, this, &TerminalSession::scrollbackChanged);
    connect(m_state, &TerminalState::titleChanged, this, [this](const QString& title) {
        m_title = title;
        emit titleChanged(m_title);
    });
}

TerminalSession::~TerminalSession() {
    stop();
}

bool TerminalSession::start(const PtyConfig& config) {
    m_renderBuffer.resize(config.rows);
    for (int i = 0; i < config.rows; i++) {
        m_renderBuffer[i].resize(config.cols);
    }
    
    bool result = m_pty->start(config);
    if (result) {
        m_state->resize(config.rows, config.cols);
    }
    return result;
}

void TerminalSession::stop() {
    m_pty->stop();
}

void TerminalSession::write(const QByteArray& data) {
    m_pty->write(data);
}

void TerminalSession::resize(int rows, int cols) {
    m_state->resize(rows, cols);
    m_pty->resize(rows, cols);
    
    m_renderBuffer.resize(rows);
    for (int i = 0; i < rows; i++) {
        m_renderBuffer[i].resize(cols);
    }
}

void TerminalSession::sendSignal(int signal) {
    m_pty->sendSignal(signal);
}

const QVector<StyledChar>& TerminalSession::line(int row) const {
    if (row >= 0 && row < m_renderBuffer.size()) {
        return m_renderBuffer[row];
    }
    static QVector<StyledChar> emptyLine;
    return emptyLine;
}

void TerminalSession::copyToClipboard() const {
    m_state->copyToClipboard();
}

void TerminalSession::pasteFromClipboard() {
    if (QApplication::clipboard()) {
        QString text = QApplication::clipboard()->text();
        if (!text.isEmpty()) {
            if (m_bracketedPasteEnabled) {
                QByteArray pasteData;
                pasteData.append("\x1b[200~");
                pasteData.append(text.toUtf8());
                pasteData.append("\x1b[201~");
                write(pasteData);
            } else {
                write(text.toUtf8());
            }
        }
    }
}

void TerminalSession::onPtyData(const QByteArray& data) {
    processAnsiData(data);
}

void TerminalSession::onPtyError(const QString& error) {
    qWarning() << "[TerminalSession] PTY error:" << error;
}

void TerminalSession::onPtyFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    emit processFinished(exitCode);
}

void TerminalSession::processAnsiData(const QByteArray& data) {
    TerminalOutputFilter::instance().processOutput(data);

    if (data.contains("\x1b[?2004h")) {
        m_bracketedPasteEnabled = true;
    } else if (data.contains("\x1b[?2004l")) {
        m_bracketedPasteEnabled = false;
    }
    
    if (data.contains('\x07')) {
        emit bellRequested();
    }
    
    m_state->write(data);
    
    int rows = m_state->rows();
    int cols = m_state->cols();
    
    if (m_renderBuffer.size() != rows) {
        m_renderBuffer.resize(rows);
        for (int i = 0; i < rows; i++) {
            if (m_renderBuffer[i].size() != cols) {
                m_renderBuffer[i].resize(cols);
            }
        }
    }
    
    for (int row = 0; row < rows; row++) {
        const QVector<TerminalCell>& cells = m_state->line(row);
        for (int col = 0; col < cols && col < cells.size(); col++) {
            m_renderBuffer[row][col] = StyledChar(cells[col].character, cells[col].style);
        }
        for (int col = cells.size(); col < cols; col++) {
            m_renderBuffer[row][col] = StyledChar();
        }
    }
    
    m_needsRenderUpdate = true;
    emit screenUpdated();
}

QVector<StyledChar> TerminalSession::convertLine(const QVector<TerminalCell>& cells) const {
    QVector<StyledChar> styled;
    styled.reserve(cells.size());
    
    for (const TerminalCell& cell : cells) {
        styled.append(StyledChar(cell.character, cell.style));
    }
    
    return styled;
}

void TerminalSession::clearBuffer() {
    m_state->clearHistory();
    emit screenUpdated();
}

void TerminalSession::setTitle(const QString& title) {
    m_title = title;
    emit titleChanged(m_title);
}
