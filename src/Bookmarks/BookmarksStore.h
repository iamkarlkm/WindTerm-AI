#ifndef BOOKMARK_STORE_H
#define BOOKMARK_STORE_H

#include <QObject>
#include <QSqlDatabase>
#include <QVector>
#include <QDateTime>

struct BookmarkEntry {
    qint64 id;
    QString name;
    QString path;
    QString category;
    QString description;
    QDateTime createdAt;
    int usageCount;
    
    BookmarkEntry() : id(0), usageCount(0) {
        createdAt = QDateTime::currentDateTime();
    }
};

class BookmarksStore : public QObject {
    Q_OBJECT
public:
    explicit BookmarksStore(QObject* parent = nullptr);
    
    static BookmarksStore* instance(QObject* parent = nullptr);
    
    bool initialize();
    bool isInitialized() const { return m_initialized; }
    
    void addBookmark(const QString& name, const QString& path, const QString& category = QString(), const QString& description = QString());
    bool updateBookmark(const BookmarkEntry& entry);
    bool deleteBookmark(qint64 id);
    
    QVector<BookmarkEntry> getAllBookmarks() const;
    QVector<BookmarkEntry> getBookmarksByCategory(const QString& category) const;
    QVector<BookmarkEntry> search(const QString& query) const;
    QStringList getCategories() const;
    
    QString databasePath() const;
    
signals:
    void bookmarkAdded(const BookmarkEntry& entry);
    void bookmarkDeleted(qint64 id);
    
private:
    bool createTables();
    
    QSqlDatabase m_database;
    bool m_initialized;
    
    static BookmarksStore* s_instance;
};

#endif
