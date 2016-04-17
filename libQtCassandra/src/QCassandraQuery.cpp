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
#include "CassTools.h"

#include "cassandra.h"

#include <as2js/json.h>

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <QtCore>

/** \class QCassandraSession
 * \brief Creates and maintains a CQL session with the Cassandra server
 *
 * This class handles such things as the connection to the Cassandra
 * server and hiding all of the cassandra-cpp library interface.
 *
 * The interface does not seem to manage lifetimes of objects it creates,
 * so we put in many smart pointers with customer deleters to make sure
 * that object are returned to the free store upon destruction.
 *
 * Also, this class, in conjuction with QCassandraQuery, provide a set of façades
 * to hide and encapuslate the details of the cassandra-cpp driver. This allows
 * us to use the CQL interface seemlessly, but without having to worry about
 * object lifetimes and garbage collection.
 *
 * \sa QCassandraQuery
 */

/** \class QCassandraQuery
 * \brief Encapulates the cassandra-cpp driver to handle query and retrieval.
 *
 * The cassandra-cpp driver interface does not manage lifetimes of objects
 * it creates, leaving it up to the user to remember to return heap objects
 * to the free store. This, of course, isn't thread safe at all, nor is it
 * consistent with good OO design principles and patterns like the RAII paradigm.
 *
 * Here, we provide an object that encapsulates all of the cass_* calls and
 * bare pointers returned by those calls using std::share_pointers and
 * custom deleters. This should help us avoid memory leaks in addition to being
 * thread-safe and exception-safe.
 *
 * \sa QCassandraSession
 */

namespace QtCassandra
{

using namespace CassTools;

namespace
{
    const CassRow* getRowFromIterator( CassTools::iterator_pointer_t iter )
    {
        return cass_iterator_get_row( iter.get() );
    }

    void getMapFromJsonObject( QCassandraQuery::string_map_t& json_map, const QString& data )
    {
        json_map.clear();
        if( data.isEmpty() || data == "null" )
        {
            return;
        }

        as2js::JSON::pointer_t load_json( std::make_shared<as2js::JSON>() );
        as2js::StringInput::pointer_t in( std::make_shared<as2js::StringInput>(data.toStdString()) );
        as2js::JSON::JSONValue::pointer_t opts( load_json->parse(in) );
        const auto& options( opts->get_object() );
        json_map.clear();
        for( const auto& elm : options )
        {
            json_map[elm.first.to_utf8()] = elm.second->get_string().to_utf8();
        }
    }

    void getDataFromJsonMap( const QCassandraQuery::string_map_t& json_map, std::string& data )
    {
        data.clear();
        if( json_map.empty() )
        {
            return;
        }

        as2js::Position pos;
        as2js::JSON::JSONValue::object_t   json_obj;
        as2js::JSON::JSONValue::pointer_t  top_level_val( std::make_shared<as2js::JSON::JSONValue>(pos,json_obj) );

        for( const auto& elm : json_map )
        {
            as2js::String key( elm.first  );
            as2js::String val( elm.second );
            top_level_val->set_member( key,
                std::make_shared<as2js::JSON::JSONValue>( pos, val )
            );
        }

        as2js::StringOutput::pointer_t out( std::make_shared<as2js::StringOutput>() );
        as2js::JSON::pointer_t save_json( std::make_shared<as2js::JSON>() );
        save_json->set_value( top_level_val );
        as2js::String header("");
        save_json->output( out, header );
        data = out->get_string().to_utf8();
    }
}


using namespace CassTools;


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


QCassandraSession::pointer_t QCassandraSession::create()
{
    // Can't use make_shared<> here
    //
    return pointer_t( new QCassandraSession );
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
void QCassandraSession::connect( const QString& host, const int port )
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
 *                      computer hostnames, domain names, etc.)
 * \param[in] port      The connection port, defaults to 9042.
 *
 * \return throws std::runtime_error on failure
 */
void QCassandraSession::connect( const QStringList& host_list, const int port )
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
        contact_points << host.toStdString();
    }

    f_cluster.reset( cass_cluster_new(), clusterDeleter() );
    cass_cluster_set_contact_points( f_cluster.get(),
                                     contact_points.str().c_str() );

    cass_cluster_set_port( f_cluster.get(), port );
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


/** \brief Return a smart pointer to the cassandra-cpp cluster object.
 */
cluster_pointer_t QCassandraSession::cluster() const
{
    return f_cluster;
}

/** \brief Return a smart pointer to the cassandra-cpp session object.
 */
session_pointer_t QCassandraSession::session() const
{
    return f_session;
}

/** \brief Return a smart pointer to the cassandra-cpp connection future object.
 */
future_pointer_t QCassandraSession::connection() const
{
    return f_connection;
}


/** \brief Construct a query object and manage the lifetime of the query session.
 *
 * \sa QCassandraQuery
 */
QCassandraQuery::QCassandraQuery( QCassandraSession::pointer_t session )
    : f_session( session )
    , f_consistencyLevel(CONSISTENCY_LEVEL_DEFAULT)
    , f_timestamp(0)
{
}


/** \brief Destruct a query object.
 *
 * \sa end()
 */
QCassandraQuery::~QCassandraQuery()
{
    end();
}


/** \brief Current consistency level
 *
 * The default is CONSISTENCY_LEVEL_DEFAULT, which leaves the level to whatever
 * the cassandra-cpp-driver library deems appropriate.
 */
consistency_level_t	QCassandraQuery::consistencyLevel() const
{
    return f_consistencyLevel;
}


/** \brief Set the consistency level.
 *
 * Sets the consistency level to be added to the query statement. This
 * can be called before or after the query() method.
 *
 * \sa query()
 * \sa consistencyLevel()
 */
void QCassandraQuery::setConsistencyLevel( consistency_level_t level )
{
    f_consistencyLevel = level;
    setStatementConsistency();
}


int64_t QCassandraQuery::timestamp() const
{
    return f_timestamp;
}


void QCassandraQuery::setTimestamp( int64_t val )
{
    f_timestamp = val;
    setStatementTimestamp();
}


/** \brief Internal method which sets the consistency in the query statement.
 */
void QCassandraQuery::setStatementConsistency()
{
    if( !f_queryStmt )
    {
        // Do nothing if the statement hasn't been made yet.
        return;
    }

    if( f_consistencyLevel == CONSISTENCY_LEVEL_DEFAULT )
    {
        // Don't set the level, leave the statement at system default.
        return;
    }

    /* Unsuppored consistency levels
       CASS_CONSISTENCY_SERIAL
       CASS_CONSISTENCY_LOCAL_SERIAL
       CASS_CONSISTENCY_LOCAL_ONE
       */
    CassConsistency consist( CASS_CONSISTENCY_UNKNOWN );

    if     ( CONSISTENCY_LEVEL_ONE          == f_consistencyLevel ) consist = CASS_CONSISTENCY_ONE;          
    else if( CONSISTENCY_LEVEL_QUORUM       == f_consistencyLevel ) consist = CASS_CONSISTENCY_QUORUM;       
    else if( CONSISTENCY_LEVEL_LOCAL_QUORUM == f_consistencyLevel ) consist = CASS_CONSISTENCY_LOCAL_QUORUM; 
    else if( CONSISTENCY_LEVEL_EACH_QUORUM  == f_consistencyLevel ) consist = CASS_CONSISTENCY_EACH_QUORUM;  
    else if( CONSISTENCY_LEVEL_ALL          == f_consistencyLevel ) consist = CASS_CONSISTENCY_ALL;          
    else if( CONSISTENCY_LEVEL_ANY          == f_consistencyLevel ) consist = CASS_CONSISTENCY_ANY;          
    else if( CONSISTENCY_LEVEL_TWO          == f_consistencyLevel ) consist = CASS_CONSISTENCY_TWO;          
    else if( CONSISTENCY_LEVEL_THREE        == f_consistencyLevel ) consist = CASS_CONSISTENCY_THREE;        
    else throw std::runtime_error( "Unsupported consistency level!" );

    if( consist == CASS_CONSISTENCY_UNKNOWN )
    {
        throw std::runtime_error( "This should never happen! Consistency has not been set!" );
    }

    cass_statement_set_consistency( f_queryStmt.get(), consist );
}


/** \brief Internal method which sets the timestamp in the query statement.
 */
void QCassandraQuery::setStatementTimestamp()
{
    if( !f_queryStmt )
    {
        // Do nothing if the statement hasn't been made yet.
        return;
    }

    if( f_timestamp == 0 )
    {
        // Don't set the timestamp, leave the statement at system default.
        return;
    }

    cass_int64_t cass_time( static_cast<cass_int64_t>(f_timestamp) );

    cass_statement_set_timestamp( f_queryStmt.get(), cass_time );
}


/** \brief Create a query statement.
 *
 * In order to use the CQL interface, you need to first specify a query string,
 * along with a bind_count (for (?) placeholders.
 *
 * For example:
 *
 *  SELECT id, name, description FROM inventory WHERE id = ? AND name = ?;
 *
 * You would pass in the select string above in the query_string parameter,
 * then specify a bind_count of 2.
 *
 * \param query_string[in]  CQL query string
 * \param bind_count[in]    number of parameters to bind
 * 
 */
void QCassandraQuery::query( const QString &query_string, const int bind_count )
{
    f_queryStmt.reset(
        cass_statement_new( query_string.toStdString().c_str(), bind_count )
        , statementDeleter()
        );

    setStatementConsistency();
    setStatementTimestamp();

    f_queryString = query_string;
}


/** \brief Set the paging size for the current query.
 *
 * Call this method after you have called the query() method, but before
 * calling the start() method. If you do not go in order, then your query
 * will not be paged properly (it will default to a LIMIT of 10000 records.
 * See the cassandra-cpp docs).
 *
 * \sa query()
 *
 */
void QCassandraQuery::setPagingSize( const int size )
{
    cass_statement_set_paging_size( f_queryStmt.get(), size );
}


/** \brief Bind a Boolean to the numbered place holder
 *
 * This binds a value to the numbered placeholder in the current query.
 *
 * \param num[in]   placeholder number
 * \param value[in] value to bind to the query
 *
 * \sa query()
 */
void QCassandraQuery::bindBool( const size_t num, const bool value )
{
   cass_statement_bind_bool( f_queryStmt.get(), num, value? cass_true: cass_false );
}


/** \brief Bind a 32-bit signed integer to the numbered place holder
 *
 * This binds a value to the numbered placeholder in the current query.
 *
 * \param num[in]   placeholder number
 * \param value[in] value to bind to the query
 *
 * \sa query()
 */
void QCassandraQuery::bindInt32( const size_t num, const int32_t value )
{
   cass_statement_bind_int32( f_queryStmt.get(), num, value );
}


/** \brief Bind a 64-bit signed integer to the numbered place holder
 *
 * This binds a value to the numbered placeholder in the current query.
 *
 * \param num[in]   placeholder number
 * \param value[in] value to bind to the query
 *
 * \sa query()
 */
void QCassandraQuery::bindInt64( const size_t num, const int64_t value )
{
   cass_statement_bind_int64( f_queryStmt.get(), num, value );
}


/** \brief Bind a floating point value to the numbered place holder
 *
 * This binds a value to the numbered placeholder in the current query.
 *
 * \param num[in]   placeholder number
 * \param value[in] value to bind to the query
 *
 * \sa query()
 */
void QCassandraQuery::bindFloat( const size_t num, const float value )
{
   cass_statement_bind_float( f_queryStmt.get(), num, value );
}


/** \brief Bind a double value to the numbered place holder
 *
 * This binds a value to the numbered placeholder in the current query.
 *
 * \param num[in]   placeholder number
 * \param value[in] value to bind to the query
 *
 * \sa query()
 */
void QCassandraQuery::bindDouble( const size_t num, const double value )
{
   cass_statement_bind_double( f_queryStmt.get(), num, value );
}


/** \brief Bind a Qt string to the numbered place holder
 *
 * This binds a value to the numbered placeholder in the current query.
 *
 * \param num[in]   placeholder number
 * \param value[in] value to bind to the query
 *
 * \sa query()
 */
void QCassandraQuery::bindString( const size_t num, const QString &value )
{
    bindByteArray( num, value.toUtf8() );
}


/** \brief Bind a Qt byte-array to the numbered place holder
 *
 * This binds a value to the numbered placeholder in the current query.
 *
 * \param num[in]   placeholder number
 * \param value[in] value to bind to the query
 *
 * \sa query()
 */
void QCassandraQuery::bindByteArray( const size_t num, const QByteArray &value )
{
    cass_statement_bind_string_n( f_queryStmt.get(), num, value.constData(), value.size() );
}


void QCassandraQuery::bindJsonMap( const size_t num, const string_map_t& value )
{
    std::string data;
    getDataFromJsonMap( value, data );
    cass_statement_bind_string( f_queryStmt.get(), num, data.c_str() );
}


void QCassandraQuery::bindMap( const size_t num, const string_map_t& value )
{
    collection_pointer_t map( cass_collection_new( CASS_COLLECTION_TYPE_MAP, value.size() ), collectionDeleter() );
    for( const auto& pair : value )
    {
        cass_collection_append_string( map.get(), pair.first.c_str()  );
        cass_collection_append_string( map.get(), pair.second.c_str() );
    }
    //
    cass_statement_bind_collection( f_queryStmt.get(), num, map.get() );
}


/** \brief Start the query
 *
 * This method assumes that you have called the query() method already, and
 * optionally specified the paging size and any binding values to the query.
 *
 * \sa query(), setPagingSize(), bindInt32(), bindInt64(), bindString(), bindByteArray()
 */
void QCassandraQuery::start()
{
//std::cout << "Executing query=[" << f_queryString.toStdString() << "]" << std::endl;
    f_sessionFuture.reset( cass_session_execute( f_session->session().get(), f_queryStmt.get() ) , futureDeleter() );
    throwIfError( QString("Error in query string [%1]!").arg(f_queryString) );
    f_queryResult.reset( cass_future_get_result(f_sessionFuture.get()), resultDeleter() );
    f_rowsIterator.reset( cass_iterator_from_result(f_queryResult.get()), iteratorDeleter() );
}


/** \brief End the query and reset all of the pointers
 *
 * Call this to reset the query and destroy all of the cassandra-cpp object.
 *
 * \sa start()
 */
void QCassandraQuery::end()
{
    f_queryString.clear();
    f_rowsIterator.reset();
    f_queryResult.reset();
    f_sessionFuture.reset();
    f_queryStmt.reset();
}

/** \brief Get the next row in the result set
 *
 * After you start your query, call this method to get the first/next row
 * in the result set. When you reach the end of the result set (or the current page),
 * it will return false.
 *
 * \sa query(), start(), nextPage()
 */
bool QCassandraQuery::nextRow()
{
    return cass_iterator_next( f_rowsIterator.get() );
}


/** \brief Get the next page in the result set
 *
 * Once nextRow() returns false, and you have paging turned on, then call this
 * method to get the next page of results. When there are no more pages, this
 * will return false.
 *
 * \sa query(), start(), setPagingSize(), nextRow()
 */
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


/** \brief Internal method for throwing after the query fails.
 *
 * \sa start()
 */
void QCassandraQuery::throwIfError( const QString& msg )
{
    const CassError code( cass_future_error_code( f_sessionFuture.get() ) );
    if( code != CASS_OK )
    {
        const char* message = 0;
        size_t length       = 0;
        cass_future_error_message( f_sessionFuture.get(), &message, &length );
        QByteArray errmsg( message, length );
        std::stringstream ss;
        ss << msg.toStdString() << "! Cassandra error: code=" << static_cast<unsigned int>(code)
           << ", error={" << cass_error_desc(code)
           << "}, message={" << errmsg.data()
           << "} aborting operation!";
        throw std::runtime_error( ss.str().c_str() );
    }
}


/** \brief Internal method to get Boolean value from row
 */
bool QCassandraQuery::getBoolFromValue( const CassValue* value ) const
{
    cass_bool_t return_val = cass_false;
    cass_value_get_bool( value, &return_val );
    return return_val == cass_true;
}


/** \brief Get named Boolean column value
 *
 * \param name[in] name of column
 */
bool QCassandraQuery::getBoolColumn( const QString &name ) const
{
    const CassValue* value = cass_row_get_column_by_name( getRowFromIterator(f_rowsIterator), name.toStdString().c_str() );
    return getBoolFromValue( value );
}


/** \brief Get Boolean column value by number
 *
 * \param num[in] position of column in the result set
 */
bool QCassandraQuery::getBoolColumn( const int num ) const
{
    const CassValue* value = cass_row_get_column( getRowFromIterator(f_rowsIterator), num );
    return getBoolFromValue( value );
}


/** \brief Get named integer column value
 *
 * \param name[in] name of column
 */
int32_t QCassandraQuery::getInt32Column( const QString &name ) const
{
    int32_t return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( getRowFromIterator(f_rowsIterator), name.toStdString().c_str() );
    cass_value_get_int32( value, &return_val );
    return return_val;
}


/** \brief Get integer column value by position
 *
 * \param num[in] position of column in the result set
 */
int32_t QCassandraQuery::getInt32Column( const int num ) const
{
    int32_t return_val = 0;
    const CassValue* value = cass_row_get_column( getRowFromIterator(f_rowsIterator), num );
    cass_value_get_int32( value, &return_val );
    return return_val;
}


/** \brief Get named counter column value
 *
 * \param name[in] name of column
 */
int64_t QCassandraQuery::getInt64Column( const QString &name ) const
{
    int64_t return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( getRowFromIterator(f_rowsIterator), name.toStdString().c_str() );
    cass_value_get_int64( value, &return_val );
    return return_val;
}


/** \brief Get counter column value by position
 *
 * \param num[in] position of column in the result set
 */
int64_t QCassandraQuery::getInt64Column( const int num ) const
{
    int64_t return_val = 0;
    const CassValue* value = cass_row_get_column( getRowFromIterator(f_rowsIterator), num );
    cass_value_get_int64( value, &return_val );
    return return_val;
}


/** \brief Get named float column value
 *
 * \param name[in] name of column
 */
float QCassandraQuery::getFloatColumn( const QString &name ) const
{
    float return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( getRowFromIterator(f_rowsIterator), name.toStdString().c_str() );
    cass_value_get_float( value, &return_val );
    return return_val;
}


/** \brief Get float column value by position
 *
 * \param num[in] position of column in the result set
 */
float QCassandraQuery::getFloatColumn( const int num ) const
{
    float return_val = 0;
    const CassValue* value = cass_row_get_column( getRowFromIterator(f_rowsIterator), num );
    cass_value_get_float( value, &return_val );
    return return_val;
}


/** \brief Get named double column value
 *
 * \param name[in] name of column
 */
double QCassandraQuery::getDoubleColumn( const QString &name ) const
{
    double return_val = 0;
    const CassValue* value = cass_row_get_column_by_name( getRowFromIterator(f_rowsIterator), name.toStdString().c_str() );
    cass_value_get_double( value, &return_val );
    return return_val;
}


/** \brief Get double column value by position
 *
 * \param num[in] position of column in the result set
 */
double QCassandraQuery::getDoubleColumn( const int num ) const
{
    double return_val = 0;
    const CassValue* value = cass_row_get_column( getRowFromIterator(f_rowsIterator), num );
    cass_value_get_double( value, &return_val );
    return return_val;
}


/** \brief Internal method to extract a byte array from a CassValue
 *
 * \param value[in] pointer to a Cassandra value
 */
QByteArray QCassandraQuery::getByteArrayFromValue( const CassValue * value ) const
{
    const char *    byte_value = 0;
    size_t          value_len  = 0;
    cass_value_get_string( value, &byte_value, &value_len );
    return QByteArray::fromRawData( byte_value, value_len );
}


/** \brief Get named string column value
 *
 * \param name[in] name of column
 */
QString QCassandraQuery::getStringColumn( const QString& name ) const
{
    return getByteArrayColumn( name ).data();
}


/** \brief Get string column value by position
 *
 * \param num[in] position of column in the result set
 */
QString QCassandraQuery::getStringColumn( const int num ) const
{
    return getByteArrayColumn( num ).data();
}


/** \brief Get named byte array column value
 *
 * \param name[in] name of column
 */
QByteArray QCassandraQuery::getByteArrayColumn( const QString &name ) const
{
    const CassValue* value = cass_row_get_column_by_name( getRowFromIterator(f_rowsIterator), name.toStdString().c_str() );
    return getByteArrayFromValue( value );
}


/** \brief Get byte array column value by position
 *
 * \param num[in] position of column in the result set
 */
QByteArray QCassandraQuery::getByteArrayColumn( const int num ) const
{
    const CassValue* value = cass_row_get_column( getRowFromIterator(f_rowsIterator), num );
    return getByteArrayFromValue( value );
}


/** \brief Get named JSON map column value
 *
 * \param name[in] name of column
 */
QCassandraQuery::string_map_t QCassandraQuery::getJsonMapColumn ( const QString& name ) const
{
    string_map_t json_map;
    getMapFromJsonObject( json_map, getStringColumn( name ) );
    return json_map;
}


/** \brief Get JSON map column value by position
 *
 * \param num[in] position of column in the result set
 */
QCassandraQuery::string_map_t QCassandraQuery::getJsonMapColumn ( const int num ) const
{
    string_map_t json_map;
    getMapFromJsonObject( json_map, getStringColumn( num ) );
    return json_map;
}


/** \brief Get Cassandra map column value from CassValue
 *
 * \param value[in] pointer to Cassandra value
 */
QCassandraQuery::string_map_t QCassandraQuery::getMapFromValue( const CassValue* value ) const
{
    string_map_t ret_map;
    iterator_pointer_t map_iter( cass_iterator_from_map( value ), iteratorDeleter() );
    while( cass_iterator_next( map_iter.get() ) )
    {
        const CassValue* key( cass_iterator_get_map_key   ( map_iter.get() ) );
        const CassValue* val( cass_iterator_get_map_value ( map_iter.get() ) );

        const char *    byte_value = 0;
        size_t          value_len  = 0;
        cass_value_get_string( key, &byte_value, &value_len );
        QByteArray ba_key( QByteArray::fromRawData( byte_value, value_len ) );
        //
        cass_value_get_string( val, &byte_value, &value_len );
        QByteArray ba_val( QByteArray::fromRawData( byte_value, value_len ) );

        ret_map[ba_key.data()] = ba_val.data();
    }

    return ret_map;
}


/** \brief Get named Cassandra map column value
 *
 * \param name[in] name of column
 */
QCassandraQuery::string_map_t QCassandraQuery::getMapColumn ( const QString& name ) const
{
    const CassValue* value = cass_row_get_column_by_name( getRowFromIterator(f_rowsIterator), name.toStdString().c_str() );
    return getMapFromValue( value );
}


/** \brief Get Cassandra map column value by position
 *
 * \param num[in] position of column in the result set
 */
QCassandraQuery::string_map_t QCassandraQuery::getMapColumn ( const int num ) const
{
    const CassValue* value = cass_row_get_column( getRowFromIterator(f_rowsIterator), num );
    return getMapFromValue( value );
}


} // namespace QtCassandra

// vim: ts=4 sw=4 et
