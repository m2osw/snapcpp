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
 *      Copyright (c) 2011-2013 Made to Order Software Corp.
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
#ifndef QCASSANDRA_PRIVATE_H
#define QCASSANDRA_PRIVATE_H

#include "QtCassandra/QCassandra.h"
#include "QtCassandra/QCassandraColumnPredicate.h"

// In thrift 0.8.0
//   . sockaddr is not defined if we don't include netinet/in.h first
#include <netinet/in.h>
#include "thrift-gencpp-cassandra/Cassandra.h"
#include "thrift-gencpp-cassandra/cassandra_types.h"

#include <stdexcept>

namespace apache { namespace thrift { namespace transport {
    class TSSLSocketFactory;
} } }

namespace QtCassandra
{

class QCassandraPrivate
{
public:
    QCassandraPrivate(QCassandra *parent);
    ~QCassandraPrivate();

    bool connect(const QString& host, int port, const QString& password);
    void disconnect();
    bool isConnected() const;
    void synchronizeSchemaVersions(int timeout);

    QString clusterName() const;
    QString protocolVersion() const;
    QString partitioner() const;
    QString snitch() const;

    void setContext(const QString& context);
    void contexts() const;
    void createContext(const QCassandraContext& context);
    void updateContext(const QCassandraContext& context);
    void dropContext(const QCassandraContext& context);

    void createTable(const QCassandraTable *table);
    void updateTable(const QCassandraTable *table);
    void dropTable(const QString& table_name);
    void truncateTable(const QCassandraTable *table);

    void insertValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    void getValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void getCounter(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void addValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t value);
    int32_t getCellCount(const QString& table_name, const QByteArray& row_key, const QCassandraColumnPredicate& column_predicate);
    uint32_t getColumnSlice(QCassandraTable& table, const QByteArray& row_key, QCassandraColumnPredicate& column_predicate);
    void remove(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t timestamp, consistency_level_t consistency_level);
    uint32_t getRowSlices(QCassandraTable& table, QCassandraRowPredicate& row_predicate);

private:
    // forbid direct copies
    QCassandraPrivate(const QCassandraPrivate&) {}

    void mustBeConnected() const throw(std::runtime_error);

    // we are tightly coupled with our parent so we can use a bare pointer
    QCassandra *                                                    f_parent;
    boost::shared_ptr<apache::thrift::transport::TTransport>        f_socket;
    boost::shared_ptr<apache::thrift::transport::TTransport>        f_transport;
    boost::shared_ptr<apache::thrift::protocol::TProtocol>          f_protocol;
    boost::shared_ptr<org::apache::cassandra::CassandraClient>      f_client;
};

} // namespace QtCassandra
#endif
//#ifdef QCASSANDRA_PRIVATE_H
// vim: ts=4 sw=4 et
