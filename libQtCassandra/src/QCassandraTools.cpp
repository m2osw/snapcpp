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

#include "QtCassandra/QCassandraTools.h"

#include <QtCore>

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

void iteratorDeleter::operator()(CassIterator* p) const
{
    cass_iterator_free(p);
}

void statementDeleter::operator()(CassStatement* p) const
{
    cass_statement_free(p);
}

void sessionDeleter::operator()(CassSession* p) const
{
    cass_session_free(p);
}


QByteArray getByteArrayFromRow( const CassRow* row, const int column_num )
{
    const char *    byte_value = 0;
    size_t          value_len  = 0;
    const CassValue* value = cass_row_get_column( row, column_num );
    cass_value_get_string( value, &byte_value, &value_len );
    return QByteArray::fromRawData( byte_value, value_len );
}


QByteArray getByteArrayFromRow( const CassRow* row, const QString& column_name )
{
    const char *    byte_value = 0;
    size_t          value_len  = 0;
    const CassValue* value = cass_row_get_column_by_name( row, column_name.toUtf8().data() );
    cass_value_get_string( value, &byte_value, &value_len );
    return QByteArray::fromRawData( byte_value, value_len );
}


int32_t getIntFromRow( const CassRow* row, const QString& column_name )
{
    int32_t return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( row, column_name.toUtf8().data() );
    cass_value_get_int32( value, &return_val );
    return return_val;
}


int64_t getCounterFromRow( const CassRow* row, const QString& column_name )
{
    int64_t return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( row, column_name.toUtf8().data() );
    cass_value_get_int64( value, &return_val );
    return return_val;
}


void throwIfError( future_pointer_t result_future, const QString& msg )
{
    const CassError code( cass_future_error_code( result_future.get() ) );
    if( code != CASS_OK )
    {
        const char* message = 0;
        size_t length       = 0;
        cass_future_error_message( result_future.get(), &message, &length );
        QByteArray errmsg( message, length );
        std::stringstream ss;
        ss << msg.toUtf8().data() << "! Cassandra error: code=" << static_cast<unsigned int>(code)
           << ", error={" << cass_error_desc(code)
           << "}, message={" << errmsg.data()
           << "} aborting operation!";
        throw std::runtime_error( ss.str().c_str() );
    }
}

}
// namespace QtCassandra

// vim: ts=4 sw=4 et
