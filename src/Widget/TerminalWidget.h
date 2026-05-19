#ifndef TERMINAL_WIDGET_H
#define TERMINAL_WIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVector>
#include <QColor>

#include "Renderer/RendererFactory.h"

struct TerminalConfig {
    QString fontFamily = QStringLiteral("Consolas");
    int fontSize = 14;
    int columns = 80;
    int rows = 24;
    int bufferCapacity = 10000;
    QColor backgroundColor = QColor(30, 30, 30);
    QColor foregroundColor = QColor(200, 200, 200);
    QColor cursorColor = QColor(200, 200, 200);
    bool enableBlinking = true;
    RendererBackend backend = RendererBackend::Auto;
};

class TerminalPane;
class SplitterContainer;

class TerminalWidget : public QWidget {
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    explicit TerminalWidget(const TerminalConfig& config, QWidget* parent = nullptr);
    ~TerminalWidget() override;
    
    void startShell(const QString& shell = QString(), const QString& workDir = QString());
    void write(const QString& text);
    void write(const QByteArray& data);
    void clear();
    
    void setConfig(const TerminalConfig& config);
    TerminalConfig config() const { return m_config; }
    
    TerminalPane* activePane() const;
    QList<TerminalPane*> allPanes() const;
    SplitterContainer* splitter() { return m_splitter; }
    
    void copyToClipboard();
    void pasteFromClipboard();
    void selectLine(int row);
    
    int paneCount() const;
    int scrollbackSize() const;
    bool splitPane(Qt::Orientation orientation);
    bool closePane(TerminalPane* pane);
    
signals:
    void titleChanged(const QString& title);
    void paneActivated(TerminalPane* pane);
    void allPanesClosed();
    void selectionChanged(const QString& text);
    void scrollPositionChanged(int current, int max);
    
public slots:
    void onSplitRequested(Qt::Orientation orientation);
    void onCloseRequested();
    void onPaneFocusRequested();
    
private:
    void initWidget();
    void setupConnections();
    
    TerminalConfig m_config;
    SplitterContainer* m_splitter;
    TerminalPane* m_activePane;
    int m_paneCounter;
};

#endif
