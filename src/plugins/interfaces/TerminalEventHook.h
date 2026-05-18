#ifndef TERMINAL_EVENT_HOOK_H
#define TERMINAL_EVENT_HOOK_H

#include <QObject>
#include <QString>

class TerminalEventHook : public QObject {
    Q_OBJECT
public:
    explicit TerminalEventHook(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~TerminalEventHook() = default;
    
    virtual bool interceptKeyEvent(int key, int modifiers, const QString& text) {
        Q_UNUSED(key); Q_UNUSED(modifiers); Q_UNUSED(text);
        return false;
    }
    virtual void onCommandExecuted(const QString& command) { Q_UNUSED(command); }
    virtual void onWorkingDirectoryChanged(const QString& path) { Q_UNUSED(path); }
    virtual QString getCommandHistory(int offset) { Q_UNUSED(offset); return QString(); }
    
    // 发送文本到终端输入行（不执行）
    virtual void sendTextToInput(const QString& text) { Q_UNUSED(text); }
    // 清空终端输入行
    virtual void clearInput() {}
    // 获取当前终端输入内容
    virtual QString getCurrentInput() { return QString(); }
    
signals:
    void commandReceived(const QString& command);
    void textInputReceived(const QString& text);
};

#endif
