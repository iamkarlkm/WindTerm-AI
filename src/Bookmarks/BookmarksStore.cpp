#include "BookmarksStore.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

BookmarksStore* BookmarksStore::s_instance = nullptr;

BookmarksStore::BookmarksStore(QObject* parent)
    : QObject(parent), m_initialized(false) {
}

BookmarksStore* BookmarksStore::instance(QObject* parent) {
    if (!s_instance) {
        s_instance = new BookmarksStore(parent);
    }
    return s_instance;
}

bool BookmarksStore::initialize() {
    if (m_initialized) return true;
    
    QString dbPath = databasePath();
    QDir().mkpath(QFileInfo(dbPath).absolutePath());
    
    m_database = QSqlDatabase::addDatabase("QSQLITE", "Bookmarks");
    m_database.setDatabaseName(dbPath);
    
    if (!m_database.open()) {
        qWarning() << "[BookmarksStore] Failed to open database:" << m_database.lastError().text();
        return false;
    }
    
    if (!createTables()) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void BookmarksStore::addBookmark(const QString& name, const QString& path, const QString& category, const QString& description) {
    if (!m_initialized || name.trimmed().isEmpty() || path.trimmed().isEmpty()) return;
    
    QSqlQuery query(m_database);
    query.prepare(
        "INSERT INTO bookmarks (name, path, category, description, created_at, usage_count) "
        "VALUES (:name, :path, :category, :desc, :created, 0)"
    );
    query.bindValue(":name", name.trimmed());
    query.bindValue(":path", path.trimmed());
    query.bindValue(":category", category.trimmed());
    query.bindValue(":desc", description.trimmed());
    query.bindValue(":created", QDateTime::currentDateTime().toString(Qt::ISODate));
    
    if (query.exec()) {
        BookmarkEntry entry;
        entry.id = query.lastInsertId().toLongLong();
        entry.name = name.trimmed();
        entry.path = path.trimmed();
        entry.category = category.trimmed();
        entry.description = description.trimmed();
        emit bookmarkAdded(entry);
    }
}

bool BookmarksStore::updateBookmark(const BookmarkEntry& entry) {
    if (!m_initialized) return false;
    
    QSqlQuery query(m_database);
    query.prepare(
        "UPDATE bookmarks SET name = :name, path = :path, category = :category, "
        "description = :desc, usage_count = :count WHERE id = :id"
    );
    query.bindValue(":name", entry.name);
    query.bindValue(":path", entry.path);
    query.bindValue(":category", entry.category);
    query.bindValue(":desc", entry.description);
    query.bindValue(":count", entry.usageCount);
    query.bindValue(":id", entry.id);
    
    return query.exec();
}

bool BookmarksStore::deleteBookmark(qint64 id) {
    if (!m_initialized) return false;
    
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM bookmarks WHERE id = :id");
    query.bindValue(":id", id);
    
    if (query.exec()) {
        emit bookmarkDeleted(id);
        return true;
    }
    return false;
}

QVector<BookmarkEntry> BookmarksStore::getAllBookmarks() const {
    QVector<BookmarkEntry> results;
    if (!m_initialized) return results;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT id, name, path, category, description, created_at, usage_count FROM bookmarks ORDER BY usage_count DESC, name ASC");
    
    if (query.exec()) {
        while (query.next()) {
            BookmarkEntry entry;
            entry.id = query.value(0).toLongLong();
            entry.name = query.value(1).toString();
            entry.path = query.value(2).toString();
            entry.category = query.value(3).toString();
            entry.description = query.value(4).toString();
            entry.createdAt = QDateTime::fromString(query.value(5).toString(), Qt::ISODate);
            entry.usageCount = query.value(6).toInt();
            results.append(entry);
        }
    }
    
    return results;
}

QVector<BookmarkEntry> BookmarksStore::getBookmarksByCategory(const QString& category) const {
    QVector<BookmarkEntry> results;
    if (!m_initialized || category.isEmpty()) return results;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT id, name, path, category, description, created_at, usage_count FROM bookmarks WHERE category = :cat ORDER BY name ASC");
    query.bindValue(":cat", category);
    
    if (query.exec()) {
        while (query.next()) {
            BookmarkEntry entry;
            entry.id = query.value(0).toLongLong();
            entry.name = query.value(1).toString();
            entry.path = query.value(2).toString();
            entry.category = query.value(3).toString();
            entry.description = query.value(4).toString();
            entry.createdAt = QDateTime::fromString(query.value(5).toString(), Qt::ISODate);
            entry.usageCount = query.value(6).toInt();
            results.append(entry);
        }
    }
    
    return results;
}

QVector<BookmarkEntry> BookmarksStore::search(const QString& query) const {
    QVector<BookmarkEntry> results;
    if (!m_initialized || query.isEmpty()) return results;
    
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(
        "SELECT id, name, path, category, description, created_at, usage_count "
        "FROM bookmarks WHERE name LIKE :q OR path LIKE :q OR category LIKE :q "
        "ORDER BY usage_count DESC"
    );
    sqlQuery.bindValue(":q", "%" + query + "%");
    
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            BookmarkEntry entry;
            entry.id = sqlQuery.value(0).toLongLong();
            entry.name = sqlQuery.value(1).toString();
            entry.path = sqlQuery.value(2).toString();
            entry.category = sqlQuery.value(3).toString();
            entry.description = sqlQuery.value(4).toString();
            entry.createdAt = QDateTime::fromString(sqlQuery.value(5).toString(), Qt::ISODate);
            entry.usageCount = sqlQuery.value(6).toInt();
            results.append(entry);
        }
    }
    
    return results;
}

QStringList BookmarksStore::getCategories() const {
    QStringList categories;
    if (!m_initialized) return categories;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT category FROM bookmarks WHERE category != '' ORDER BY category");
    
    if (query.exec()) {
        while (query.next()) {
            categories.append(query.value(0).toString());
        }
    }
    
    return categories;
}

QString BookmarksStore::databasePath() const {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataDir + "/bookmarks.db";
}

bool BookmarksStore::createTables() {
    QSqlQuery query(m_database);
    
    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS bookmarks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            path TEXT NOT NULL,
            category TEXT DEFAULT '',
            description TEXT DEFAULT '',
            created_at TEXT NOT NULL,
            usage_count INTEGER DEFAULT 0
        )
    )";
    
    if (!query.exec(createTable)) {
        qWarning() << "[BookmarksStore] Failed to create table:" << query.lastError().text();
        return false;
    }
    
    QString createIndex = "CREATE INDEX IF NOT EXISTS idx_name ON bookmarks(name)";
    query.exec(createIndex);
    
    QString createIndex2 = "CREATE INDEX IF NOT EXISTS idx_category ON bookmarks(category)";
    query.exec(createIndex2);
    
    return true;
}
