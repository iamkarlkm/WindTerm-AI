#ifndef COMMAND_DATABASE_H
#define COMMAND_DATABASE_H
#include <QObject>
#include <QSqlDatabase>
#include <QStringList>

class CommandDatabase : public QObject {
    Q_OBJECT
public:
    explicit CommandDatabase(QObject* parent = nullptr);
    bool initialize();
    void recordCommand(const QString& command, const QString& workingDir);
    QStringList getHistory(const QString& workingDir, int limit = 100);
    void clear();
private:
    QSqlDatabase m_db;
};
#endif
