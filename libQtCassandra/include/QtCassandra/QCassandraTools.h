/*
 * Text:
 *      QCassandraTools.h
 *
 * Description:
 *      Helper code for the cassandra-cpp-driver tools.
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

#include <QString>
#include <QByteArray>

#include <memory>

#include <cassandra.h>

namespace QtCassandra
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

    QByteArray getByteArrayFromRow ( const CassRow* row, const QString& column_name );
    QByteArray getByteArrayFromRow ( const CassRow* row, const int      column_num  );
    int32_t    getIntFromRow       ( const CassRow* row, const QString& column_name );
    int64_t    getCounterFromRow   ( const CassRow* row, const QString& column_name );

    void throwIfError( future_pointer_t result_future, const QString& msg = "Cassandra error" );
}
// namespace QtCassandra

// vim: ts=4 sw=4 et
