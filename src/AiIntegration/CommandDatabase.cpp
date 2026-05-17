#include "CommandDatabase.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

CommandDatabase::CommandDatabase(QObject* parent) : QObject(parent) {}
bool CommandDatabase::initialize() {
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ai_commands.db";
    QDir(QFileInfo(dbPath).absolutePath()).mkpath(".");
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) { qDebug() << "DB open failed:" << m_db.lastError(); return false; }
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS commands (id INTEGER PRIMARY KEY AUTOINCREMENT, command TEXT NOT NULL, working_dir TEXT, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_dir ON commands(working_dir)");
    return true;
}
void CommandDatabase::recordCommand(const QString& command, const QString& workingDir) {
    if (!m_db.isOpen()) return;
    QSqlQuery query;
    query.prepare("INSERT INTO commands (command, working_dir) VALUES (?, ?)");
    query.addBindValue(command); query.addBindValue(workingDir); query.exec();
}
QStringList CommandDatabase::getHistory(const QString& workingDir, int limit) {
    QStringList result;
    if (!m_db.isOpen()) return result;
    QSqlQuery query;
    query.prepare("SELECT command FROM commands WHERE working_dir = ? ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(workingDir); query.addBindValue(limit);
    if (query.exec()) { while (query.next()) result << query.value(0).toString(); }
    return result;
}
void CommandDatabase::clear() { if (m_db.isOpen()) QSqlQuery().exec("DELETE FROM commands"); }
