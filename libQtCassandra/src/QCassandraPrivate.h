/*
 * Text:
 *      QCassandraPrivate.h
 *
 * Description:
 *      Allows the context and table objects to access the cassandra server.
 *
 * Documentation:
 *      See the QCassandra.cpp file.
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

#include "QtCassandra/QCassandra.h"
#include "QtCassandra/QCassandraPredicate.h"
#include "QtCassandra/QCassandraTools.h"

#include <QString>
#include <QStringList>

#include <memory>
#include <stdexcept>

namespace QtCassandra
{

class QCassandraPrivate
{
public:
    QCassandraPrivate( QCassandra::pointer_t parent );
    ~QCassandraPrivate();

    // handle queries
    //
    future_pointer_t executeQuery( const QString& query ) const;
    void executeQuery( const QString& query, QStringList& values ) const;
    void executeQuery( const QString& table, const QString& column, QStringList& values ) const;

    cluster_pointer_t cluster()    const;
    session_pointer_t session()    const;
    future_pointer_t  connection() const;

    bool connect(const QString& host = "localhost", const int port = 9042 );
    bool connect(const QStringList& hosts, const int port = 9042 );
    void disconnect();
    bool isConnected() const;
    void synchronizeSchemaVersions(int timeout);

    QString clusterName()     const;
    QString protocolVersion() const;
    QString partitioner()     const;
    QString snitch()          const;

    void setContext(const QString& context);
    void contexts() const;
    void retrieve_context(const QString& context_name) const;
    void createContext(const QCassandraContext& context);
    void updateContext(const QCassandraContext& context);
    void dropContext(const QCassandraContext& context);

    void createTable  ( const QCassandraTable::pointer_t table);
    void updateTable  ( const QCassandraTable::pointer_t table);
    void dropTable    ( const QString& table_name);
    void truncateTable( const QCassandraTable *table);

    void insertValue( const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    void getValue   ( const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void getCounter ( const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void addValue   ( const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t value);

    int32_t     getCellCount  ( const QString& table_name, const QByteArray& row_key, const QCassandraColumnPredicate& column_predicate);
    uint32_t    getColumnSlice( QCassandraTable& table,    const QByteArray& row_key, QCassandraColumnPredicate& column_predicate);
    void        remove        ( const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t timestamp, consistency_level_t consistency_level);
    uint32_t    getRowSlices  ( QCassandraTable& table,    QCassandraRowPredicate::pointer_t row_predicate);

private:
    // forbid direct copies
    QCassandraPrivate(const QCassandraPrivate&) {}

    void mustBeConnected() const throw(std::runtime_error);

    QCassandra::pointer_t f_parent;

    // New CQL interface
    //
    cluster_pointer_t     f_cluster;
    session_pointer_t     f_session;
    future_pointer_t      f_connection;
};

} // namespace QtCassandra

// vim: ts=4 sw=4 et
