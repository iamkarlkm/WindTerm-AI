#ifndef MEMORY_FRAGMENT_H
#define MEMORY_FRAGMENT_H

#include <QString>
#include <QDateTime>
#include <QMetaType>

struct MemoryFragmentContext {
    QString terminalType;
    QString workingDirectory;
    QString sessionId;
    QString commandHistory;
    
    static MemoryFragmentContext current();
};

struct MemoryFragment {
    qint64 id;
    QString title;
    QString content;
    QString terminalType;
    QString workingDirectory;
    QString sessionId;
    QString commandHistory;
    QString sourceType;       // selection, clipboard, manual
    QString sourceRemark;
    QDateTime createdAt;
    QDateTime updatedAt;
    
    MemoryFragment() 
        : id(0), sourceType("manual") {
        createdAt = QDateTime::currentDateTime();
        updatedAt = createdAt;
    }
    
    bool isValid() const { return id > 0 && !content.isEmpty(); }
    
    QString contentPreview(int maxLines = 3) const {
        QStringList lines = content.split('\n');
        if (lines.size() <= maxLines) {
            return content;
        }
        return lines.mid(0, maxLines).join('\n') + "\n...";
    }
    
    static QString sourceTypeLabel(const QString& type) {
        if (type == "selection") return QStringLiteral("选择文本");
        if (type == "clipboard") return QStringLiteral("剪贴板");
        return QStringLiteral("手动输入");
    }
};

Q_DECLARE_METATYPE(MemoryFragment)

#endif
