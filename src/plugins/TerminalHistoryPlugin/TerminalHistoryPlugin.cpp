#include "TerminalHistoryPlugin.h"
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QHostInfo>
#include <QtDebug>

TerminalHistoryPlugin::TerminalHistoryPlugin(QObject* parent) : TerminalEventHook(parent) {}
TerminalHistoryPlugin::~TerminalHistoryPlugin() { shutdown(); }

bool TerminalHistoryPlugin::initialize() { initDatabase(); return m_db.isValid(); }
void TerminalHistoryPlugin::shutdown() { if (m_db.isOpen()) m_db.close(); }

void TerminalHistoryPlugin::initDatabase() {
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/extensions/command_history.db";
    QDir(QFileInfo(dbPath).absolutePath()).mkpath(".");
    m_db = QSqlDatabase::addDatabase("QSQLITE", "terminal_history");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) { qCritical() << "DB open failed:" << m_db.lastError(); return; }
    QSqlQuery query(m_db);
    query.exec("CREATE TABLE IF NOT EXISTS command_history (id INTEGER PRIMARY KEY AUTOINCREMENT, command TEXT NOT NULL, working_directory TEXT, hostname TEXT, shell_type TEXT, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_working_dir ON command_history(working_directory)");
}

bool TerminalHistoryPlugin::interceptKeyEvent(int key, int modifiers, const QString& text) {
    Q_UNUSED(text);
    if (modifiers != Qt::NoModifier) return false;
    if (key == Qt::Key_Up) { QString cmd = queryHistoryByOffset(1); if (!cmd.isEmpty()) { emit commandReceived(cmd); return true; } }
    else if (key == Qt::Key_Down) { QString cmd = queryHistoryByOffset(-1); if (!cmd.isEmpty()) { emit commandReceived(cmd); return true; } }
    return false;
}

void TerminalHistoryPlugin::onCommandExecuted(const QString& command) { if (!command.trimmed().isEmpty()) saveCommand(command); }
void TerminalHistoryPlugin::onWorkingDirectoryChanged(const QString& path) { m_currentWorkingDir = path; }
QString TerminalHistoryPlugin::getCommandHistory(int offset) { return queryHistoryByOffset(offset); }

void TerminalHistoryPlugin::saveCommand(const QString& command) {
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO command_history (command, working_directory, hostname) VALUES (?, ?, ?)");
    query.addBindValue(command); query.addBindValue(m_currentWorkingDir); query.addBindValue(QHostInfo::localHostName());
    if (!query.exec()) qCritical() << "Failed to save:" << query.lastError();
}

QString TerminalHistoryPlugin::queryHistoryByOffset(int offset) {
    if (!m_db.isOpen()) return QString();
    QSqlQuery query(m_db);
    query.prepare("SELECT command FROM command_history WHERE working_directory = ? ORDER BY timestamp DESC LIMIT 1 OFFSET ?");
    query.addBindValue(m_currentWorkingDir); query.addBindValue(qAbs(offset));
    if (query.exec() && query.next()) return query.value(0).toString();
    return QString();
}
