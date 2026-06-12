#ifndef DATABASECLIENT_H
#define DATABASECLIENT_H

#include <QObject>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

/**
 * @brief 数据库连接配置
 */
struct DatabaseConfig {
    QString id;
    QString name;
    QString type;        // mysql, postgresql, sqlite, mssql, oracle
    QString host;
    int port;
    QString database;
    QString username;
    QString password;
    QString sslMode;     // disable, require, verify-ca, verify-full
    int connectTimeout = 30;
    int queryTimeout = 300;
};

/**
 * @brief 查询结果
 */
struct QueryResult {
    QStringList columns;
    QList<QVariantList> rows;
    int rowCount;
    int affectedRows;
    QString error;
    qint64 executionTimeMs;
    bool success;
};

/**
 * @brief 表信息
 */
struct TableInfo {
    QString name;
    QString schema;
    int rowCount;
    qint64 size;
    QStringList columns;
    QString primaryKey;
    QList<QString> indexes;
};

/**
 * @brief 数据库客户端 - 多数据库支持
 * 
 * 功能:
 * - 多数据库类型支持 (MySQL/PostgreSQL/SQLite/MSSQL/Oracle)
 * - SQL 编辑器
 * - 查询结果展示
 * - 表结构浏览
 * - 数据导出导入
 */
class DatabaseClient : public QObject {
    Q_OBJECT

public:
    explicit DatabaseClient(QObject* parent = nullptr);
    ~DatabaseClient();

    // 连接管理
    QString createConnection(const DatabaseConfig& config);
    bool testConnection(const QString& connectionId);
    bool connect(const QString& connectionId);
    bool disconnect(const QString& connectionId);
    bool isConnected(const QString& connectionId) const;
    DatabaseConfig getConfig(const QString& connectionId) const;
    QList<QString> listConnections() const;
    bool deleteConnection(const QString& connectionId);
    
    // SQL 执行
    QueryResult executeQuery(const QString& connectionId, const QString& sql);
    QueryResult executeScript(const QString& connectionId, const QString& scriptPath);
    void cancelQuery(const QString& connectionId);
    
    // 数据库操作
    QStringList listDatabases(const QString& connectionId);
    bool createDatabase(const QString& connectionId, const QString& dbName);
    bool dropDatabase(const QString& connectionId, const QString& dbName);
    
    // 表操作
    QList<TableInfo> listTables(const QString& connectionId, const QString& schema = "") const;
    TableInfo getTableInfo(const QString& connectionId, const QString& tableName) const;
    bool createTable(const QString& connectionId, const QString& sql);
    bool dropTable(const QString& connectionId, const QString& tableName);
    bool truncateTable(const QString& connectionId, const QString& tableName);
    
    // 数据操作
    QueryResult selectData(const QString& connectionId, const QString& tableName, int limit = 100);
    int insertData(const QString& connectionId, const QString& tableName, const QVariantMap& data);
    int updateData(const QString& connectionId, const QString& tableName, const QVariantMap& data, const QString& where);
    int deleteData(const QString& connectionId, const QString& tableName, const QString& where);
    
    // 导出导入
    bool exportTable(const QString& connectionId, const QString& tableName, const QString& filePath, const QString& format = "csv");
    bool importTable(const QString& connectionId, const QString& tableName, const QString& filePath, const QString& format = "csv");
    bool exportQuery(const QString& connectionId, const QString& query, const QString& filePath, const QString& format = "csv");
    
    // 元数据
    QStringList getSchemas(const QString& connectionId) const;
    QStringList getViews(const QString& connectionId, const QString& schema = "") const;
    QStringList getStoredProcedures(const QString& connectionId) const;
    QStringList getFunctions(const QString& connectionId) const;

signals:
    void connected(const QString& connectionId);
    void disconnected(const QString& connectionId);
    void queryStarted(const QString& connectionId, const QString& sql);
    void queryCompleted(const QString& connectionId, const QueryResult& result);
    void queryError(const QString& connectionId, const QString& error);
    void connectionError(const QString& connectionId, const QString& error);

private:
    QString createConnectionString(const DatabaseConfig& config) const;
    QString getDriverName(const QString& type) const;
    
    QMap<QString, DatabaseConfig> m_connections;
    QMap<QString, QSqlDatabase> m_databases;
    QMap<QString, bool> m_connected;
    QMap<QString, QSqlQuery*> m_activeQueries;
    
    static DatabaseClient* s_instance;

public:
    static DatabaseClient* instance();
};

#endif // DATABASECLIENT_H
