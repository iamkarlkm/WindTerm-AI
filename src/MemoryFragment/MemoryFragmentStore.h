#ifndef MEMORY_FRAGMENT_STORE_H
#define MEMORY_FRAGMENT_STORE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QList>
#include <QString>

#include "MemoryFragment.h"

class MemoryFragmentStore : public QObject {
    Q_OBJECT
public:
    explicit MemoryFragmentStore(QObject* parent = nullptr);
    ~MemoryFragmentStore() override;
    
    static MemoryFragmentStore* instance(QObject* parent = nullptr);
    
    bool initialize(const QString& dbPath = QString());
    bool isInitialized() const { return m_initialized; }
    
    qint64 createFragment(const MemoryFragment& fragment);
    bool updateFragment(const MemoryFragment& fragment);
    bool deleteFragment(qint64 id);
    
    QList<MemoryFragment> loadAll();
    QList<MemoryFragment> search(const QString& keyword);
    MemoryFragment getFragment(qint64 id);
    
    qint64 count() const;
    
signals:
    void fragmentCreated(qint64 id);
    void fragmentUpdated(qint64 id);
    void fragmentDeleted(qint64 id);
    void databaseError(const QString& error);
    
private:
    bool createTables();
    QString defaultDbPath() const;
    
    QSqlDatabase m_database;
    bool m_initialized;
    
    static MemoryFragmentStore* s_instance;
};

#endif
