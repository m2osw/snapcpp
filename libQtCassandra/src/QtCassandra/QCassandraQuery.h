/*
 * Text:
 *      QCassandraQuery.h
 *
 * Description:
 *      Handling of the cassandra::CfDef (Column Family Definition).
 *
 * Documentation:
 *      See each function below.
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

#include "QtCassandra/QCassandraTools.h"

#include <QString>
#include <QByteArray>

namespace QtCassandra
{

namespace CassTools
{
    typedef std::shared_ptr<CassCluster>      cluster_pointer_t;
    typedef std::shared_ptr<CassFuture>       future_pointer_t;
    typedef std::shared_ptr<CassIterator>     iterator_pointer_t;
    typedef std::shared_ptr<const CassResult> result_pointer_t;
    typedef std::shared_ptr<CassSession>      session_pointer_t;
    typedef std::shared_ptr<CassStatement>    statement_pointer_t;

    struct clusterDeleter
    { 
        void operator()(CassCluster* p) const;
    };

    struct futureDeleter
    { 
        void operator()(CassFuture* p) const;
    };

    struct iteratorDeleter
    {
        void operator()(CassIterator* p) const;
    };

    struct resultDeleter
    {
        void operator()(const CassResult* p) const;
    };

    struct sessionDeleter
    { 
        void operator()(CassSession* p) const;
    };

    struct statementDeleter
    { 
        void operator()(CassStatement* p) const;
    };
}


class QCassandraSession
{
public:
    typedef std::shared_ptr<QCassandraSession> pointer_t;

    QCassandraSession();
    ~QCassandraSession();

    void connect( const QString& host = "localhost", const int port = 9042 );
    void connect( const QStringList& hosts         , const int port = 9042 );
    void disconnect();
    bool isConnected() const;

    CassTools::cluster_pointer_t cluster()    const;
    CassTools::session_pointer_t session()    const;
    CassTools::future_pointer_t  connection() const;

private:
    CassTools::cluster_pointer_t       f_cluster;
    CassTools::session_pointer_t       f_session;
    CassTools::future_pointer_t        f_connection;
};

class QCassandraQuery
{
public:
    std::shared_ptr<QCassandraQuery> pointer_t;

    QCassandraQuery( QCassandraSession::pointer_t session );

    void       query         ( const QString& query_string, const int bind_count = 0 );
    void       setPagingSize ( const int size );
    void       bindInt       ( const int num, const int         value );
    void       bindString    ( const int num, const QString&    value );
    void       bindByteArray ( const int num, const QByteArray& value );

    void       start();
    bool       next();

    bool       getBoolColumn      ( const QString& name ) const;
    bool       getBoolColumn      ( const int      num  ) const;
    int32_t    getIntColumn       ( const QString& name ) const;
    int32_t    getIntColumn       ( const int      num  ) const;
    int64_t    getCounter         ( const QString& name ) const;
    int64_t    getCounter         ( const int      num  ) const;
    QString    getStringColumn    ( const QString& name ) const;
    QString    getStringColumn    ( const int      num  ) const;
    QByteArray getByteArrayColumn ( const QString& name ) const;
    QByteArray getByteArrayColumn ( const int      num  ) const;

private:
    // Current query
    //
    QCassandraSession::pointer_t   f_session;
    QString                        f_queryString;
    CassTools::statement_pointer_t f_queryStmt;
    CassTools::future_pointer_t    f_sessionFuture;
    CassTools::result_pointer_t    f_queryResult;
    CassTools::iterator_pointer_t  f_rowsIterator;

    void throwIfError( const QString& msg )
};


}
// namespace QtCassandra

    
// vim: ts=4 sw=4 et
