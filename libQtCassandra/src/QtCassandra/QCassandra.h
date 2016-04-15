/*
 * Header:
 *      QCassandra.h
 *
 * Description:
 *      Handling of the cassandra::CassandraClient and corresponding transports,
 *      protocols, sockets, etc.
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

#include "QtCassandra/QCassandraContext.h"
#include "QtCassandra/QCassandraQuery.h"
#include "QtCassandra/QCassandraSchema.h"
#include "QtCassandra/QCassandraVersion.h"

#include <memory>

namespace QtCassandra
{

class KsDef;
class CfDef;
class ColumnDef;

// Handling of the transport and CassandraClient objects
class QCassandra
    : public QObject
    , public std::enable_shared_from_this<QCassandra>
{
public:
    typedef std::shared_ptr<QCassandra> pointer_t;

    static pointer_t create();
    virtual ~QCassandra();

    static int versionMajor();
    static int versionMinor();
    static int versionPatch();
    static const char *version();

    // connection functions
    bool connect(const QString& host = "localhost", const int port = 9042 );
    bool connect(const QStringList& hosts, const int port = 9042 );
    void disconnect();
    bool isConnected() const;
    const QString& clusterName() const;
    const QString& protocolVersion() const;
    //const QCassandraClusterInformation& clusterInformation() const;
    const QString& partitioner() const;
    const QString& snitch() const;

    QCassandraSession::pointer_t    session() const { return f_session; }

    // context functions (the database [Cassandra keyspace])
    QCassandraContext::pointer_t currentContext() const { return f_current_context; }
    QCassandraContext::pointer_t context(const QString& context_name);
    const QCassandraContexts& contexts() const;

    QCassandraContext::pointer_t findContext(const QString& context_name) const;
    QCassandraContext& operator[] (const QString& context_name);
    const QCassandraContext& operator[] (const QString& context_name) const;

    void dropContext(const QString& context_name);

    // time stamp helper
    static int64_t timeofday();

private:
    QCassandra();

    //QCassandraContext::pointer_t currentContext() const;
    void setCurrentContext(QCassandraContext::pointer_t c);
    void clearCurrentContextIf(const QCassandraContext& c);

    //void retrieveColumn   ( ColumnDef& cf_def, QCassandraSchema::SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::pointer_t column ) const;
    //void retrieveTable    ( CfDef& cf_def    , QCassandraSchema::SessionMeta::KeyspaceMeta::TableMeta::pointer_t table ) const;
    void retrieveContext  ( QCassandraSchema::SessionMeta::KeyspaceMeta::pointer_t keyspace ) const;
    void retrieveContext  ( const QString& context_name ) const;

    friend class QCassandraContext;

    QCassandraSession::pointer_t            f_session;
    QCassandraContext::pointer_t            f_current_context;
    mutable controlled_vars::flbool_t       f_contexts_read;
    QCassandraContexts                      f_contexts;
    QString                                 f_cluster_name;
    QString                                 f_protocol_version;
    //mutable QCassandraClusterInformation    f_cluster_information;
    QString                                 f_partitioner;
    QString                                 f_snitch;
};

} // namespace QtCassandra

// vim: ts=4 sw=4 et
