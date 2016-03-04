/*
 * Text:
 *      QCassandraTools.cpp
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

#include "QCassandraTools.h"
#include <cassandra.h>
#include <sstream>

namespace QtCassandra
{

void clusterDeleter::operator()(CassCluster* p) const
{
    cass_cluster_free(p);
}

void resultDeleter::operator()(const CassResult* p) const
{
    cass_result_free(p);
}

void futureDeleter::operator()(CassFuture* p) const
{
    cass_future_free(p);
}

void statementDeleter::operator()(CassStatement* p) const
{
    cass_statement_free(p);
}

void sessionDeleter::operator()(CassSession* p) const
{
    cass_session_free(p);
}

void throwIfError( future_pointer_t result_future, const QString& msg = "Cassandra error" )
{
    const CassError code( cass_future_error_code( result_future.get() ) );
    if( code != CASS_OK )
    {
        std::stringstream ss;
        ss << msg << "! Cassandra error: code=" << static_cast<unsigned int>(code) << ", message={" << cass_error_desc(code) << "}, aborting operation!";
        throw std::runtime_error( ss.str().c_str() );
    }
}

}
// namespace QtCassandra

// vim: ts=4 sw=4 et
