#ifndef TERMINAL_SESSION_H
#define TERMINAL_SESSION_H

#include <QObject>
#include <QVector>
#include <QColor>

#include "Terminal/PtyManager.h"
#include "Terminal/TerminalState.h"

struct StyledChar {
    QChar character;
    QColor foreground;
    QColor background;
    bool bold;
    bool italic;
    bool underline;
    bool reverse;
    
    StyledChar() 
        : character(' '), bold(false), italic(false), underline(false), reverse(false) {}
    
    StyledChar(QChar ch, const TextStyle& style)
        : character(ch),
          foreground(style.foreground),
          background(style.background),
          bold(style.bold),
          italic(style.italic),
          underline(style.underline),
          reverse(style.reverse) {}
};

class TerminalSession : public QObject {
    Q_OBJECT
public:
    explicit TerminalSession(QObject* parent = nullptr);
    ~TerminalSession() override;
    
    bool start(const PtyConfig& config);
    void stop();
    
    void write(const QByteArray& data);
    void resize(int rows, int cols);
    void sendSignal(int signal);
    
    const QVector<StyledChar>& line(int row) const;
    int rows() const { return m_state->rows(); }
    int cols() const { return m_state->cols(); }
    
    CursorInfo cursor() const { return m_state->cursor(); }
    int scrollbackSize() const { return m_state->scrollbackSize(); }
    
    QString title() const { return m_title; }
    bool isRunning() const { return m_pty->isRunning(); }
    
    void copyToClipboard() const;
    void pasteFromClipboard();
    void clearBuffer();
    
signals:
    void dataAvailable(const QByteArray& data);
    void screenUpdated();
    void cursorMoved(int row, int col);
    void titleChanged(const QString& title);
    void scrollbackChanged(int size);
    void processFinished(int exitCode);
    
private slots:
    void onPtyData(const QByteArray& data);
    void onPtyError(const QString& error);
    void onPtyFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
private:
    void processAnsiData(const QByteArray& data);
    QVector<StyledChar> convertLine(const QVector<TerminalCell>& cells) const;
    
    PtyManager* m_pty;
    TerminalState* m_state;
    QString m_title;
    
    QVector<QVector<StyledChar>> m_renderBuffer;
    bool m_needsRenderUpdate;
};

#endif
