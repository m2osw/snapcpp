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
 *      Copyright (c) 2011-2016 Made to Order Software Corp.
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
#pragma once

#include "QtCassandra/QCassandraTable.h"

#include <memory>

namespace QtCassandra
{

class QCassandra;

class QCassandraContext
    : public QObject
    , public std::enable_shared_from_this<QCassandraContext>
{
public:
    typedef std::shared_ptr<QCassandraContext>  pointer_t;
    typedef unsigned short                      host_identifier_t;
    static const host_identifier_t              NULL_HOST_ID = 0;
    static const host_identifier_t              LARGEST_HOST_ID = 10000;

    virtual ~QCassandraContext();

    const QString& contextName() const;

    // fields
    //
    const QCassandraSchema::Value::map_t& fields() const;
    QCassandraSchema::Value::map_t&       fields();

    // tables
    QCassandraTable::pointer_t table(const QString& table_name);
    const QCassandraTables& tables();

    QCassandraTable::pointer_t findTable(const QString& table_name);
    QCassandraTable& operator[] (const QString& table_name);
    const QCassandraTable& operator[] (const QString& table_name) const;

    // Context maintenance
    void create();
    void update();
    void drop();
    void dropTable(const QString& table_name);
    void clearCache();
    void loadTables();

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
    typedef int32_t lock_timeout_t; // default to 5
    typedef int32_t lock_ttl_t;     // default to 60

    void makeCurrent();
    QCassandraContext(std::shared_ptr<QCassandra> cassandra, const QString& context_name);
    QCassandraContext(QCassandraContext const &) = delete;
    QCassandraContext & operator = (QCassandraContext const &) = delete;

    void resetSchema();
    void parseContextDefinition( QCassandraSchema::SessionMeta::KeyspaceMeta::pointer_t keyspace );
    QString getKeyspaceOptions();

    friend class QCassandra;

    // f_cassandra is a parent that has a strong shared pointer over us so it
    // cannot disappear before we do, thus only a bare pointer is enough here
    // (there isn't a need to use a QWeakPointer or QPointer either)
    // Also, it cannot be a shared_ptr unless you make a restriction that
    // all instances must be allocated on the heap. Thus is the deficiency of
    // std::enabled_shared_from_this<>.
    QCassandraSchema::SessionMeta::KeyspaceMeta::pointer_t f_schema;
    //
    std::weak_ptr<QCassandra>                   f_cassandra;
    QString                                     f_context_name;
    QCassandraTables                            f_tables;
    QString                                     f_host_name;
    QString                                     f_lock_table_name;
    mutable bool                                f_lock_accessed = false;
    lock_timeout_t                              f_lock_timeout = 5;
    lock_ttl_t                                  f_lock_ttl = 60;
};

typedef QMap<QString, QCassandraContext::pointer_t> QCassandraContexts;


} // namespace QtCassandra

// vim: ts=4 sw=4 et
