/*
 * Text:
 *      QCassandraQuery.cpp
 *
 * Description:
 *      Handling of the CQL interface.
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

#include "QtCassandra/QCassandraQuery.h"

#include <as2js/json.h>

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <QtCore>

namespace QtCassandra
{


namespace CassTools
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

}
// namespace CassTools

using namespace CassTools;

/** \class QCassandraSession
 * \brief TODO
 *
 */

/** \class QCassandraQuery
 * \brief TODO
 *
 */

/** \brief Initialize a QCassandraSession object
 *
 */
QCassandraSession::QCassandraSession()
    // f_cluster
    // f_session
    // f_connection
{
}

/** \brief Clean up the QCassandraSession object.
 *
 */
QCassandraSession::~QCassandraSession()
{
    disconnect();
}


/** \brief Connect to a Cassandra Cluster.
 *
 * This function connects to a Cassandra Cluster. Which cluster is determined
 * by the host and port parameters.
 *
 * One cluster may include many database contexts (i.e. keyspaces.) Each
 * database
 * context (keyspace) has a set of parameters defining its duplication mechanism
 * among other things. Before working with a database context, one must call the
 * the setCurrentContext() function.
 *
 * The function first disconnects the existing connection when there is one.
 *
 * Many other functions require you to call this connect() function first. You
 * are likely to get a runtime exception if you don't.
 *
 * Note that the previous connection is lost whether or not the new one
 * succeeds.
 *
 * \param[in] host      The host, defaults to "localhost" (an IP address,
 * computer
 *                      hostname, domain name, etc.)
 * \param[in] port      The connection port, defaults to 9042.
 * \param[in] password  Whether the connection makes use of encryption and a
 *                      password (if password is not an empty string).
 *
 * \return throws std::runtime_error on failure
 */
void QCassandraSession::connect( const QString &host, const int port )
{
    QStringList host_list;
    host_list << host;
    connect( host_list, port );
}


/** \brief Connect to a Cassandra Cluster.
 *
 * This function connects to a Cassandra Cluster. Which cluster is determined
 * by the host and port parameters.
 *
 * One cluster may include many database contexts (i.e. keyspaces.) Each
 * database
 * context (keyspace) has a set of parameters defining its duplication mechanism
 * among other things. Before working with a database context, one must call the
 * the setCurrentContext() function.
 *
 * The function first disconnects the existing connection when there is one.
 *
 * Many other functions require you to call this connect() function first. You
 * are likely to get a runtime exception if you don't.
 *
 * Note that the previous connection is lost whether or not the new one
 * succeeds.
 *
 * \param[in] host_list The list of hosts, AKA contact points (IP addresses,
 * computer
 *                      hostnames, domain names, etc.)
 * \param[in] port      The connection port, defaults to 9042.
 *
 * \return throws std::runtime_error on failure
 */
bool QCassandraSession::connect( const QStringList &host_list, const int port )
{
    // disconnect any existing connection
    disconnect();

    std::stringstream contact_points;
    for ( QString host : host_list )
    {
        if ( contact_points.str() != "" )
        {
            contact_points << ",";
        }
        contact_points << host.toUtf8().data();
    }

    f_cluster.reset( cass_cluster_new(), clusterDeleter() );
    cass_cluster_set_contact_points( f_cluster.get(),
                                     contact_points.str().c_str() );

    std::stringstream port_str;
    port_str << port;
    cass_cluster_set_contact_points( f_cluster.get(), port_str.str().c_str() );
    //
    f_session.reset( cass_session_new(), sessionDeleter() );
    f_connection.reset(
        cass_session_connect( f_session.get(), f_cluster.get() ),
        futureDeleter() );

    /* This operation will block until the result is ready */
    CassError rc = cass_future_error_code( f_connection.get() );
    if ( rc != CASS_OK )
    {
        const char *message;
        size_t message_length;
        cass_future_error_message( f_connection.get(), &message,
                                   &message_length );
        std::stringstream msg;
        msg << "Cannot connect to cassandra server! Reason=["
            << std::string( message ) << "]";

        f_connection.reset();
        f_session.reset();
        f_cluster.reset();
        throw std::runtime_error( msg.str().c_str() );
    }
}


/** \brief Break the connection to Cassandra.
 *
 * This function breaks the connection to Cassandra.
 *
 * This function has the side effect of clearing the cluster name,
 * protocol version, and current context.
 *
 * The function does not clear the default consistency level or
 * the default time out used by the schema synchronization. Those
 * can be changed by calling their respective functions.
 */
void QCassandraSession::disconnect()
{
    f_connection.reset();
    //
    if ( f_session )
    {
        future_pointer_t result( cass_session_close( f_session.get() ),
                                 futureDeleter() );
        cass_future_wait( result.get() );
    }
    //
    f_session.reset();
    f_cluster.reset();
}


/** \brief Check whether the object is connected to the server.
 *
 * This function returns true when this object is connected to the
 * backend Cassandra server.
 *
 * The function is fast and does not actually verify that the TCP/IP
 * connection is still up.
 *
 * \return true if connect() was called and succeeded.
 */
bool QCassandraSession::isConnected() const
{
    return f_connection && f_session && f_cluster;
}


cluster_pointer_t QCassandraSession::cluster() const
{
    return f_cluster;
}

session_pointer_t QCassandraSession::session() const
{
    return f_session;
}

future_pointer_t QCassandraSession::connection() const
{
    return f_connection;
}


QCassandraQuery::QCassandraQuery( QCassandraSession::pointer_t session )
    : f_session( session )
{
}


void QCassandraQuery::query( const QString &query_string,
                             const int bind_count = 0 )
{
    f_queryStmt.reset(
        cass_statement_new( query_string.toUtf8().data(), bind_count )
        , statementDeleter()
        );
    f_queryString = query_string;
}


void QCassandraQuery::setPagingSize( const int size )
{
    cass_statement_set_paging_size( f_queryStmt.get(), size );
}


void QCassandraQuery::bindInt32( const int num, const int32_t value )
{
   cass_statement_bind_int32( f_queryStmt.get(), num, value );
}


void QCassandraQuery::bindInt64( const int num, const int64_t value )
{
   cass_statement_bind_int64( f_queryStmt.get(), num, value );
}


void QCassandraQuery::bindString( const int num, const QString &value )
{
    bindByteArray( num, value.toUtf() );
}


void QCassandraQuery::bindByteArray( const int num, const QByteArray &value )
{
    cass_statement_bind_string_n( f_queryStmt.get(), num, value.constData(), value.size() );
}


void QCassandraQuery::start()
{
    f_sessionFuture.reset( cass_session_execute( f_session->session().get(), f_queryStmt.get() ) , futureDeleter() );
    throwIfError( QString("Error in query string [%1]!").arg(f_queryString) );
    f_queryResult.reset( cass_future_get_result(f_sessionFuture.get()), resultDeleter() );
    f_rowsIterator.reset( cass_iterator_from_result(f_queryResult.get()), iteratorDeleter() );
}


bool QCassandraQuery::nextRow()
{
    return cass_iterator_next( f_rowsIterator.get() );
}


bool QCassandraQuery::nextPage()
{
    if( !cass_result_has_more_pages( f_queryResult.get() ) )
    {
        return false;
    }

    cass_statement_set_paging_state( f_queryStmt.get(), f_queryResult.get() );

    // Reset the current query session, and run the next page
    //
    start();

    return true;
}


void QCassandraQuery::throwIfError( const QString& msg )
{
    const CassError code( cass_future_error_code( f_queryResult.get() ) );
    if( code != CASS_OK )
    {
        const char* message = 0;
        size_t length       = 0;
        cass_future_error_message( f_queryResult.get(), &message, &length );
        QByteArray errmsg( message, length );
        std::stringstream ss;
        ss << msg.toUtf8().data() << "! Cassandra error: code=" << static_cast<unsigned int>(code)
           << ", error={" << cass_error_desc(code)
           << "}, message={" << errmsg.data()
           << "} aborting operation!";
        throw std::runtime_error( ss.str().c_str() );
    }
}


bool QCassandraQuery::getBoolColumn( const QString &name ) const
{
    bool return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( f_rowsIterator.get(), name.toUtf8().data() );
    cass_value_get_bool( value, &return_val );
    return return_val;
}


bool QCassandraQuery::getBoolColumn( const int num ) const
{
    bool return_val = 0;
    const CassValue* value = cass_row_get_column( f_rowsIterator.get(), num );
    cass_value_get_bool( value, &return_val );
    return return_val;
}


int32_t QCassandraQuery::getIntColumn( const QString &name ) const
{
    int32_t return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( f_rowsIterator.get(), name.toUtf8().data() );
    cass_value_get_int32( value, &return_val );
    return return_val;
}


int32_t QCassandraQuery::getIntColumn( const int num ) const
{
    int32_t return_val = 0;
    const CassValue* value = cass_row_get_column( f_rowsIterator.get(), num );
    cass_value_get_int32( value, &return_val );
    return return_val;
}


int64_t QCassandraQuery::getCounterColumn( const QString &name ) const
{
    int64_t return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( f_rowsIterator.get(), name.toUtf8().data() );
    cass_value_get_int64( value, &return_val );
    return return_val;
}


int64_t QCassandraQuery::getCounterColumn( const int num ) const
{
    int64_t return_val = 0;
    const CassValue* value = cass_row_get_column( f_rowsIterator.get(), num );
    cass_value_get_int64( value, &return_val );
    return return_val;
}


float QCassandraQuery::getFloatColumn( const QString &name ) const
{
    float return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( f_rowsIterator.get(), name.toUtf8().data() );
    cass_value_get_float( value, &return_val );
    return return_val;
}


float QCassandraQuery::getFloatColumn( const int num ) const
{
    float return_val = 0;
    const CassValue* value = cass_row_get_column( f_rowsIterator.get(), num );
    cass_value_get_float( value, &return_val );
    return return_val;
}


double QCassandraQuery::getDoubleColumn( const QString &name ) const
{
    double return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( f_rowsIterator.get(), name.toUtf8().data() );
    cass_value_get_double( value, &return_val );
    return return_val;
}


double QCassandraQuery::getDoubleColumn( const int num ) const
{
    double return_val = 0;
    const CassValue* value = cass_row_get_column( f_rowsIterator.get(), num );
    cass_value_get_double( value, &return_val );
    return return_val;
}


QString QCassandraQuery::getStringColumn( const QString &name ) const
{
    return getByteArrayColumn( name ).data();
}


QString QCassandraQuery::getStringColumn( const int num ) const
{
    return getByteArrayColumn( num ).data();
}


QByteArray QCassandraQuery::getByteArrayColumn( const QString &name ) const
{
    const char *    byte_value = 0;
    size_t          value_len  = 0;
    const CassValue* value = cass_row_get_column_by_name( f_rowsIterator.get(), name.toUtf8().data() );
    cass_value_get_string( value, &byte_value, &value_len );
    return QByteArray::fromRawData( byte_value, value_len );
}


QByteArray QCassandraQuery::getByteArrayColumn( const int num ) const
{
    const char *    byte_value = 0;
    size_t          value_len  = 0;
    const CassValue* value = cass_row_get_column( f_rowsIterator.get(), num );
    cass_value_get_string( value, &byte_value, &value_len );
    return QByteArray::fromRawData( byte_value, value_len );
}


namespace
{
    QCassandraQuery::string_map_t getMapFromJsonObject( const QString& data )
    {
        as2js::JSON::pointer_t load_json( std::make_shared<as2js::JSON>() );
        as2js::StringInput::pointer_t in( std::make_shared<as2js::StringInput>(data.toStdString()) );
        as2js::JSON::JSONValue::pointer_t opts( load_json->parse(in) );
        const auto& options( opts->get_object() );
        qstring_map_t the_map;
        for( const auto& elm : options )
        {
            the_map[*elm.first.c_str()] = *elm.second.c_str();
        }
        return the_map;
    }
}


QCassandraQuery::string_map_t QCassandraQuery::getMapColumn ( const QString& name ) const
{
    return getMapFromJsonObject( getStringColumn( name ) );
}


QCassandraQuery::string_map_t QCassandraQuery::getMapColumn ( const int num ) const
{
    return getMapFromJsonObject( getStringColumn( num ) );
}


} // namespace QtCassandra

// vim: ts=4 sw=4 et
