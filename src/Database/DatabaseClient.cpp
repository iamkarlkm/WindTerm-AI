#include "DatabaseClient.h"
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDriver>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>
#include <QUuid>
#include <QSqlTableModel>

DatabaseClient* DatabaseClient::s_instance = nullptr;

DatabaseClient::DatabaseClient(QObject* parent) : QObject(parent) {
}

DatabaseClient::~DatabaseClient() {
    for (auto it = m_databases.begin(); it != m_databases.end(); ++it) {
        it->close();
    }
}

DatabaseClient* DatabaseClient::instance() {
    if (!s_instance) s_instance = new DatabaseClient();
    return s_instance;
}

QString DatabaseClient::createConnection(const DatabaseConfig& config) {
    QString connectionId = generateConnectionId();
    
    DatabaseConfig cfg = config;
    cfg.id = connectionId;
    m_connections[connectionId] = cfg;
    m_connected[connectionId] = false;
    
    return connectionId;
}

bool DatabaseClient::testConnection(const QString& connectionId) {
    if (!m_connections.contains(connectionId)) return false;
    
    const DatabaseConfig& config = m_connections[connectionId];
    
    // 创建临时连接测试
    QString driverName = getDriverName(config.type);
    QSqlDatabase testDb = QSqlDatabase::addDatabase(driverName, "test_" + connectionId);
    
    testDb.setHostName(config.host);
    testDb.setPort(config.port);
    testDb.setDatabaseName(config.database);
    testDb.setUserName(config.username);
    testDb.setPassword(config.password);
    testDb.setConnectOptions(config.sslMode);
    
    bool ok = testDb.open();
    testDb.close();
    QSqlDatabase::removeDatabase("test_" + connectionId);
    
    return ok;
}

bool DatabaseClient::connect(const QString& connectionId) {
    if (!m_connections.contains(connectionId)) return false;
    
    const DatabaseConfig& config = m_connections[connectionId];
    
    QString driverName = getDriverName(config.type);
    QSqlDatabase db = QSqlDatabase::addDatabase(driverName, connectionId);
    
    db.setHostName(config.host);
    db.setPort(config.port);
    db.setDatabaseName(config.database);
    db.setUserName(config.username);
    db.setPassword(config.password);
    db.setConnectOptions(config.sslMode);
    
    if (db.open()) {
        m_connected[connectionId] = true;
        m_databases[connectionId] = db;
        emit connected(connectionId);
        return true;
    }
    
    emit connectionError(connectionId, db.lastError().text());
    return false;
}

bool DatabaseClient::disconnect(const QString& connectionId) {
    if (!m_databases.contains(connectionId)) return false;
    
    m_databases[connectionId].close();
    QSqlDatabase::removeDatabase(connectionId);
    m_databases.remove(connectionId);
    m_connected[connectionId] = false;
    
    emit disconnected(connectionId);
    return true;
}

bool DatabaseClient::isConnected(const QString& connectionId) const {
    return m_connected.value(connectionId, false);
}

DatabaseConfig DatabaseClient::getConfig(const QString& connectionId) const {
    return m_connections.value(connectionId);
}

QList<QString> DatabaseClient::listConnections() const {
    return m_connections.keys();
}

bool DatabaseClient::deleteConnection(const QString& connectionId) {
    if (!m_connections.contains(connectionId)) return false;
    
    disconnect(connectionId);
    m_connections.remove(connectionId);
    return true;
}

QueryResult DatabaseClient::executeQuery(const QString& connectionId, const QString& sql) {
    QueryResult result;
    result.success = false;
    
    if (!m_databases.contains(connectionId)) {
        result.error = "Not connected";
        return result;
    }
    
    emit queryStarted(connectionId, sql);
    
    QElapsedTimer timer;
    timer.start();
    
    QSqlDatabase& db = m_databases[connectionId];
    QSqlQuery query(db);
    query.setForwardOnly(true);
    
    if (!query.exec(sql)) {
        result.error = query.lastError().text();
        result.executionTimeMs = timer.elapsed();
        emit queryError(connectionId, result.error);
        return result;
    }
    
    // 获取列名
    QSqlRecord record = query.record();
    for (int i = 0; i < record.count(); ++i) {
        result.columns.append(record.fieldName(i));
    }
    
    // 获取数据
    while (query.next()) {
        QVariantList row;
        for (int i = 0; i < record.count(); ++i) {
            row.append(query.value(i));
        }
        result.rows.append(row);
    }
    
    result.rowCount = result.rows.size();
    result.affectedRows = query.numRowsAffected();
    result.executionTimeMs = timer.elapsed();
    result.success = true;
    
    emit queryCompleted(connectionId, result);
    return result;
}

QueryResult DatabaseClient::executeScript(const QString& connectionId, const QString& scriptPath) {
    QueryResult result;
    
    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = "Cannot open script file";
        return result;
    }
    
    QString content = file.readAll();
    file.close();
    
    // 分割 SQL 语句
    QStringList statements = content.split(";", Qt::SkipEmptyParts);
    
    for (const QString& stmt : statements) {
        if (stmt.trimmed().isEmpty()) continue;
        
        result = executeQuery(connectionId, stmt.trimmed());
        if (!result.success) {
            return result;
        }
    }
    
    return result;
}

void DatabaseClient::cancelQuery(const QString& connectionId) {
    Q_UNUSED(connectionId)
    // 取消正在执行的查询
}

QStringList DatabaseClient::listDatabases(const QString& connectionId) {
    if (!m_databases.contains(connectionId)) return QStringList();
    
    QStringList databases;
    QSqlDatabase& db = m_databases[connectionId];
    
    // 不同数据库类型有不同的查询方式
    if (m_connections[connectionId].type == "mysql") {
        QSqlQuery query("SHOW DATABASES", db);
        while (query.next()) {
            databases.append(query.value(0).toString());
        }
    } else if (m_connections[connectionId].type == "postgresql") {
        QSqlQuery query("SELECT datname FROM pg_database WHERE datistemplate = false", db);
        while (query.next()) {
            databases.append(query.value(0).toString());
        }
    }
    
    return databases;
}

bool DatabaseClient::createDatabase(const QString& connectionId, const QString& dbName) {
    if (!m_databases.contains(connectionId)) return false;
    
    QString sql = "CREATE DATABASE " + dbName;
    QueryResult result = executeQuery(connectionId, sql);
    return result.success;
}

bool DatabaseClient::dropDatabase(const QString& connectionId, const QString& dbName) {
    if (!m_databases.contains(connectionId)) return false;
    
    QString sql = "DROP DATABASE " + dbName;
    QueryResult result = executeQuery(connectionId, sql);
    return result.success;
}

QList<TableInfo> DatabaseClient::listTables(const QString& connectionId, const QString& schema) const {
    QList<TableInfo> tables;
    
    if (!m_databases.contains(connectionId)) return tables;
    
    QSqlDatabase& db = m_databases.constFind(connectionId).value();
    QStringList tableNames = db.tables(QSql::Tables);
    
    for (const QString& tableName : tableNames) {
        TableInfo info;
        info.name = tableName;
        info.schema = schema;
        tables.append(info);
    }
    
    return tables;
}

TableInfo DatabaseClient::getTableInfo(const QString& connectionId, const QString& tableName) const {
    TableInfo info;
    
    if (!m_databases.contains(connectionId)) return info;
    
    QSqlDatabase& db = m_databases.constFind(connectionId).value();
    QSqlQuery query = db.tables(QSql::Columns);
    
    while (query.next()) {
        info.columns.append(query.value(0).toString());
    }
    
    info.name = tableName;
    return info;
}

bool DatabaseClient::createTable(const QString& connectionId, const QString& sql) {
    QueryResult result = executeQuery(connectionId, sql);
    return result.success;
}

bool DatabaseClient::dropTable(const QString& connectionId, const QString& tableName) {
    QString sql = "DROP TABLE " + tableName;
    QueryResult result = executeQuery(connectionId, sql);
    return result.success;
}

bool DatabaseClient::truncateTable(const QString& connectionId, const QString& tableName) {
    QString sql = "TRUNCATE TABLE " + tableName;
    QueryResult result = executeQuery(connectionId, sql);
    return result.success;
}

QueryResult DatabaseClient::selectData(const QString& connectionId, const QString& tableName, int limit) {
    QString sql = "SELECT * FROM " + tableName + " LIMIT " + QString::number(limit);
    return executeQuery(connectionId, sql);
}

int DatabaseClient::insertData(const QString& connectionId, const QString& tableName, const QVariantMap& data) {
    QStringList columns;
    QStringList values;
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        columns.append(it.key());
        values.append(":" + it.key());
    }
    
    QString sql = "INSERT INTO " + tableName + " (" + columns.join(", ") + ") VALUES (" + values.join(", ") + ")";
    
    QueryResult result = executeQuery(connectionId, sql);
    return result.affectedRows;
}

int DatabaseClient::updateData(const QString& connectionId, const QString& tableName, const QVariantMap& data, const QString& where) {
    QStringList sets;
    for (auto it = data.begin(); it != data.end(); ++it) {
        sets.append(it.key() + " = :" + it.key());
    }
    
    QString sql = "UPDATE " + tableName + " SET " + sets.join(", ");
    if (!where.isEmpty()) {
        sql += " WHERE " + where;
    }
    
    QueryResult result = executeQuery(connectionId, sql);
    return result.affectedRows;
}

int DatabaseClient::deleteData(const QString& connectionId, const QString& tableName, const QString& where) {
    QString sql = "DELETE FROM " + tableName;
    if (!where.isEmpty()) {
        sql += " WHERE " + where;
    }
    
    QueryResult result = executeQuery(connectionId, sql);
    return result.affectedRows;
}

bool DatabaseClient::exportTable(const QString& connectionId, const QString& tableName, const QString& filePath, const QString& format) {
    Q_UNUSED(connectionId)
    Q_UNUSED(tableName)
    Q_UNUSED(filePath)
    Q_UNUSED(format)
    // 实现导出逻辑
    return true;
}

bool DatabaseClient::importTable(const QString& connectionId, const QString& tableName, const QString& filePath, const QString& format) {
    Q_UNUSED(connectionId)
    Q_UNUSED(tableName)
    Q_UNUSED(filePath)
    Q_UNUSED(format)
    // 实现导入逻辑
    return true;
}

bool DatabaseClient::exportQuery(const QString& connectionId, const QString& query, const QString& filePath, const QString& format) {
    Q_UNUSED(connectionId)
    Q_UNUSED(query)
    Q_UNUSED(filePath)
    Q_UNUSED(format)
    // 实现导出逻辑
    return true;
}

QStringList DatabaseClient::getSchemas(const QString& connectionId) const {
    Q_UNUSED(connectionId)
    // 返回数据库 schema 列表
    return QStringList();
}

QStringList DatabaseClient::getViews(const QString& connectionId, const QString& schema) const {
    Q_UNUSED(connectionId)
    Q_UNUSED(schema)
    // 返回视图列表
    return QStringList();
}

QStringList DatabaseClient::getStoredProcedures(const QString& connectionId) const {
    Q_UNUSED(connectionId)
    // 返回存储过程列表
    return QStringList();
}

QStringList DatabaseClient::getFunctions(const QString& connectionId) const {
    Q_UNUSED(connectionId)
    // 返回函数列表
    return QStringList();
}

QString DatabaseClient::createConnectionId() const {
    return "db_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

QString DatabaseClient::getDriverName(const QString& type) const {
    if (type == "mysql") return "QMYSQL";
    if (type == "postgresql") return "QPSQL";
    if (type == "sqlite") return "QSQLITE";
    if (type == "mssql") return "QODBC";
    if (type == "oracle") return "QOCI";
    return "QSQLITE";
}

#include "DatabaseClient.moc"
