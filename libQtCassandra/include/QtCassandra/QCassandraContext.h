/*
 * Header:
 *      QCassandraContext.h
 *
 * Description:
 *      Handling of the cassandra::KsDef.
 *
 * Documentation:
 *      See the corresponding .cpp file.
 *
 * License:
 *      Copyright (c) 2011-2014 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef QCASSANDRA_CONTEXT_H
#define QCASSANDRA_CONTEXT_H

#include "QCassandraTable.h"
#include "QCassandraColumnPredicate.h"

#include <memory>

namespace QtCassandra
{

class QCassandraContextPrivate;
class QCassandra;

// Cassandra KsDef object
class QCassandraContext
    : public QObject
    , public std::enable_shared_from_this<QCassandraContext>
{
public:
    typedef std::shared_ptr<QCassandraContext>  pointer_t;
    typedef QMap<QString, QString>              QCassandraContextOptions;
    typedef unsigned short                      host_identifier_t;
    static const host_identifier_t              NULL_HOST_ID = 0;
    static const host_identifier_t              LARGEST_HOST_ID = 10000;

    virtual ~QCassandraContext();

    QString contextName() const;

    void setStrategyClass(const QString& strategy_class);
    QString strategyClass() const;

    void setDescriptionOptions(const QCassandraContextOptions& options);
    const QCassandraContextOptions& descriptionOptions() const;
    void setDescriptionOption(const QString& option, const QString& value);
    QString descriptionOption(const QString& option) const;
    void eraseDescriptionOption(const QString& option);

    // tables
    QCassandraTable::pointer_t table(const QString& table_name);
    const QCassandraTables& tables() const;

    QCassandraTable::pointer_t findTable(const QString& table_name) const;
    QCassandraTable& operator[] (const QString& table_name);
    const QCassandraTable& operator[] (const QString& table_name) const;

    // replication
    void setReplicationFactor(int32_t factor);
    void unsetReplicationFactor();
    bool hasReplicationFactor() const;
    int32_t replicationFactor() const;
    void setDurableWrites(bool durable_writes);
    void unsetDurableWrites();
    bool hasDurableWrites() const;
    bool durableWrites() const;

    // handling
    void create();
    void update();
    void drop();
    void dropTable(const QString& table_name);
    void clearCache();

    // locks
    QString lockHostsKey() const;
    QCassandraTable::pointer_t lockTable();
    void addLockHost(const QString& host_name);
    void removeLockHost(const QString& host_name);
    void setLockTableName(const QString& lock_table_name);
    const QString& lockTableName() const;
    void setLockTimeout(int timeout);
    int lockTimeout() const;
    void setLockTtl(int ttl);
    int lockTtl() const;

    void setHostName(const QString& host_name);
    QString hostName() const;

    std::shared_ptr<QCassandra> parentCassandra() const;

private:
    typedef controlled_vars::auto_init<int32_t, 5> lock_timeout_t;
    typedef controlled_vars::auto_init<int32_t, 60> lock_ttl_t;

    void makeCurrent();
    QCassandraContext(std::shared_ptr<QCassandra> cassandra, const QString& context_name);

    // internal functions
    void parseContextDefinition(const void *data);
    void prepareContextDefinition(void *data) const;
    void createTable(const QCassandraTable *table);
    void updateTable(const QCassandraTable *table);
    void truncateTable(const QCassandraTable *table);
    void insertValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    bool getValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    bool getCounter(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void addValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t value);
    int32_t getCellCount(const QString& table_name, const QByteArray& row_key, const QCassandraColumnPredicate& column_predicate);
    uint32_t getColumnSlice(QCassandraTable& table, const QByteArray& row_key, QCassandraColumnPredicate& column_predicate);
    void remove(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t timestamp, consistency_level_t consistency_level);
    uint32_t getRowSlices(QCassandraTable& table, QCassandraRowPredicate& row_predicate);
    void synchronizeSchemaVersions();
    void unparent();

    friend class QCassandra;
    friend class QCassandraPrivate;
    friend class QCassandraContextPrivate;
    friend class QCassandraTable;
    friend class QCassandraLock;

    std::unique_ptr<QCassandraContextPrivate>   f_private;
    // f_cassandra is a parent that has a strong shared pointer over us so it
    // cannot disappear before we do, thus only a bare pointer is enough here
    // (there isn't a need to use a QWeakPointer or QPointer either)
    // Also, it cannot be a shared_ptr unless you make a restriction that
    // all instances must be allocated on the heap. Thus is the deficiency of
    // std::enabled_shared_from_this<>.
    std::shared_ptr<QCassandra>                 f_cassandra;
    QCassandraContextOptions                    f_options;
    QCassandraTables                            f_tables;
    QString                                     f_host_name;
    QString                                     f_lock_table_name;
    mutable controlled_vars::flbool_t           f_lock_accessed;
    lock_timeout_t                              f_lock_timeout;
    lock_ttl_t                                  f_lock_ttl;
};

typedef QMap<QString, QCassandraContext::pointer_t> QCassandraContexts;


} // namespace QtCassandra
#endif
//#ifndef QCASSANDRA_CONTEXT_H
// vim: ts=4 sw=4 et
