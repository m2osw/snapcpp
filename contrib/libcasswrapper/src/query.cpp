/*
 * Text:
 *      query.cpp
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

#include "casswrapper/query.h"
#include "casswrapperImpl.h"

#include <as2js/json.h>

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <QtCore>


/** \class query
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
 * Modification: I moved all of the cass_* calls into a new layer of classes,
 * defined in casswrapperImpl.h/.cpp
 *
 * \sa session
 */

namespace casswrapper
{

namespace
{
    void getMapFromJsonObject( query::string_map_t& json_map, const QString& data )
    {
        json_map.clear();
        if( data.isEmpty() || data == "null" )
        {
            return;
        }

        as2js::JSON::pointer_t load_json( std::make_shared<as2js::JSON>() );
        as2js::StringInput::pointer_t in( std::make_shared<as2js::StringInput>(data.toUtf8().data()) );
        as2js::JSON::JSONValue::pointer_t opts( load_json->parse(in) );
        const auto& options( opts->get_object() );
        json_map.clear();
        for( const auto& elm : options )
        {
            json_map[elm.first.to_utf8()] = elm.second->get_string().to_utf8();
        }
    }

    void getDataFromJsonMap( const query::string_map_t& json_map, std::string& data )
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


query::query_exception_t::query_exception_t( future session_future, QString const& msg )
    : f_message(msg)
{
    const CassError code( session_future.get_error_code() );
    f_code   = static_cast<uint32_t>(code);
    f_errmsg = session_future.get_error_message();

    std::stringstream ss;
    ss << msg.toUtf8().data() << "! Cassandra error: code=" << f_code
       << ", error={" << cass_error_desc(code)
       << "}, message={" << f_errmsg.toUtf8().data()
       << "} aborting operation!";
    f_what = ss.str();
}


const char* query::query_exception_t::what() const throw()
{
    return f_what.c_str();
}



/** \brief Construct a query object and manage the lifetime of the query session.
 *
 * \sa query
 */
query::query( session::pointer_t session )
    : f_session( session )
{
    connect( this, &query::threadQueryFinished, this, &query::onThreadQueryFinished );
}


/** \brief Destruct a query object.
 *
 * \sa end()
 */
query::~query()
{
    end();
}


query::pointer_t query::create( session::pointer_t session )
{
    return pointer_t(new query( session ));
}


/** \brief Description of query instance.
 *
 * This property allows the user to set and read a string
 * description pertaining to a particular instance of a query.
 * This can be useful if you have a list of queries you are
 * referencing and want to output details to the user as to
 * which one is returning status.
 */
const QString& query::description() const
{
    return f_description;
}

void query::setDescription( const QString& val )
{
    f_description = val;
}


/** \brief Current consistency level
 *
 * The default is CONSISTENCY_LEVEL_DEFAULT, which leaves the level to whatever
 * the cassandra-cpp-driver library deems appropriate.
 */
query::consistency_level_t	query::consistencyLevel() const
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
void query::setConsistencyLevel( consistency_level_t level )
{
    f_consistencyLevel = level;
    setStatementConsistency();
}


int64_t query::timestamp() const
{
    return f_timestamp;
}


void query::setTimestamp( int64_t val )
{
    f_timestamp = val;
    setStatementTimestamp();
}


/** \brief Internal method which sets the consistency in the query statement.
 */
void query::setStatementConsistency()
{
    if( !f_queryStmt )
    {
        // Do nothing if the statement hasn't been made yet.
        return;
    }

    //if( f_consistencyLevel == CONSISTENCY_LEVEL_DEFAULT )
    //{
    //    // Don't set the level, leave the statement at system default.
    //    return;
    //}

    // At this time, except for a very few cases which probably do not
    // matter, we always want to use QUORUM so here we always force
    // QUORUM which makes it a lot easier.
    //

    /* Unsuppored consistency levels
       CASS_CONSISTENCY_SERIAL
       CASS_CONSISTENCY_LOCAL_SERIAL
       CASS_CONSISTENCY_LOCAL_ONE
    */
    //CassConsistency consist( CASS_CONSISTENCY_UNKNOWN );
    CassConsistency const consist( CASS_CONSISTENCY_QUORUM );

    //if     ( CONSISTENCY_LEVEL_ONE          == f_consistencyLevel ) consist = CASS_CONSISTENCY_ONE;          
    //else if( CONSISTENCY_LEVEL_QUORUM       == f_consistencyLevel ) consist = CASS_CONSISTENCY_QUORUM;       
    //else if( CONSISTENCY_LEVEL_LOCAL_QUORUM == f_consistencyLevel ) consist = CASS_CONSISTENCY_LOCAL_QUORUM; 
    //else if( CONSISTENCY_LEVEL_EACH_QUORUM  == f_consistencyLevel ) consist = CASS_CONSISTENCY_EACH_QUORUM;  
    //else if( CONSISTENCY_LEVEL_ALL          == f_consistencyLevel ) consist = CASS_CONSISTENCY_ALL;          
    //else if( CONSISTENCY_LEVEL_ANY          == f_consistencyLevel ) consist = CASS_CONSISTENCY_ANY;          
    //else if( CONSISTENCY_LEVEL_TWO          == f_consistencyLevel ) consist = CASS_CONSISTENCY_TWO;          
    //else if( CONSISTENCY_LEVEL_THREE        == f_consistencyLevel ) consist = CASS_CONSISTENCY_THREE;        
    //else throw exception_t( "Unsupported consistency level!" );

    //if( consist == CASS_CONSISTENCY_UNKNOWN )
    //{
    //    throw exception_t( "This should never happen! Consistency has not been set!" );
    //}

    f_queryStmt->set_consistency( consist );
}


/** \brief Internal method which sets the timestamp in the query statement.
 */
void query::setStatementTimestamp()
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

    f_queryStmt->set_timestamp( f_timestamp );
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
void query::query( const QString &query_string, int bind_count )
{
    if( bind_count == -1 )
    {
        bind_count = query_string.count('?');
    }

    f_queryStmt = std::make_unique<statement>( query_string, bind_count );

    setStatementConsistency();
    setStatementTimestamp();

    f_queryString = query_string;
}


int query::pagingSize() const
{
    return f_pagingSize;
}

/** \brief Set the paging size for the current query.
 *
 * Call this method after you have called the query() method, but before
 * calling the start() method. If you do not go in order, then your query
 * will not be paged properly (it will default to a LIMIT of 10000 records.
 * See the cassandra-cpp docs).
 *
 * \param[in] size  The number of rows or cells to read per page.
 *
 * \sa query()
 */
void query::setPagingSize( const int size )
{
    f_pagingSize = size;
    f_queryStmt->set_paging_size( size );
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
void query::bindBool( const size_t num, const bool value )
{
   f_queryStmt->bind_bool( num, value );
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
void query::bindInt32( const size_t num, const int32_t value )
{
   f_queryStmt->bind_int32( num, value );
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
void query::bindInt64( const size_t num, const int64_t value )
{
   f_queryStmt->bind_int64( num, value );
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
void query::bindFloat( const size_t num, const float value )
{
   f_queryStmt->bind_float( num, value );
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
void query::bindDouble( const size_t num, const double value )
{
   f_queryStmt->bind_double( num, value );
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
void query::bindString( const size_t num, const QString &value )
{
    f_queryStmt->bind_string( num, value );
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
void query::bindByteArray( const size_t num, const QByteArray& value )
{
    f_queryStmt->bind_blob( num, value );
}


void query::bindJsonMap( const size_t num, const string_map_t& value )
{
    std::string data;
    getDataFromJsonMap( value, data );
    f_queryStmt->bind_string( num, data );
}


void query::bindMap( const size_t num, const string_map_t& value )
{
    collection coll( CASS_COLLECTION_TYPE_MAP, value.size() );
    for( const auto& pair : value )
    {
        coll.append_string( pair.first  );
        coll.append_string( pair.second );
    }
    //
    f_queryStmt->bind_collection( num, coll );
}


void query::queryCallbackFunc( void* f, void *data )
{
    future*          this_future( reinterpret_cast<future*>(f) );
    query* this_query( reinterpret_cast<query*>(data) );
    if( *(this_query->f_sessionFuture.get()) != *this_future )
    {
        //throw exception_t( "Unexpected future!" );
        // Do nothing with this future, because this belongs to a different query
        return;
    }

    // This comes from the background thread created by the Cassandra driver
    // However, when Qt5 emits it, it is properly marshalled into the
    // main thread (when using the GUI, this would be the GUI thread).
    // Therefore, this is completely thread safe, so there is no need to
    // serialize access to any shared data members.
    //
    // However, one of the side effects is that I have to send the bare pointer
    // through the signal into the main thread. Qt does not let me marshall
    // the shared_ptr<> for some reason. However, once in the main thread, the
    // emmissions from that point on will only be shared_ptr.
    //
    emit this_query->threadQueryFinished( this_query );
}


void query::onThreadQueryFinished( query* q )
{
    if( q != this )
    {
        throw exception_t("query::onThreadQueryFinished(): Query objects are not the same!");
    }

    emit queryFinished( shared_from_this() );
}


#if 0
void query::testMetrics()
{
// The following loops are a couple of attempts to get things to work when we
// send loads of data to Cassandra all at once. All failed though. The cluster
// still crashes if too much data gets there all at once.

    CassMetrics metrics;
    cass_session_get_metrics(f_session->getSession().get(), &metrics);

    double load_avg(0.0);
    {
        std::ifstream load_avg_file;
        load_avg_file.open("/proc/loadavg", std::ios_base::in);
        if(load_avg_file.is_open())
        {
            char buf[256];
            load_avg_file.getline(buf, sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
            load_avg = atof(buf);
        }
        else
        {
            load_avg = 0.0;
        }
    }

std::cerr << "*** ongoing metrics info: " << metrics.requests.mean_rate << " and " << load_avg << "\n";

    if(metrics.stats.total_connections != 0
    && (metrics.stats.available_connections <= 0 || metrics.requests.mean_rate >= 60 || load_avg > 2.5))
    {
std::cerr << "*** pausing, waiting for a connection to be available! ***\n";
        do
        {
            // no connection currently available, wait for one to
            // become available before sending more data
            sleep(1);

            cass_session_get_metrics(f_session->getSession().get(), &metrics);
            std::ifstream load_avg_file;
            load_avg_file.open("/proc/loadavg", std::ios_base::in);
            if(load_avg_file.is_open())
            {
                char buf[256];
                load_avg_file.getline(buf, sizeof(buf));
                buf[sizeof(buf) - 1] = '\0';
                load_avg = atof(buf);
            }
            else
            {
                load_avg = 0.0;
            }
std::cerr << "  pause with " << metrics.requests.mean_rate << " and " << load_avg << "\n";

//    "          total_connections: " << metrics.stats.total_connections      << "\n"
// << "      available_connections: " << metrics.stats.available_connections  << "\n"
// << "                  mean_rate: " << metrics.requests.mean_rate           << "\n"
// << "            one_minute_rate: " << metrics.requests.one_minute_rate     << "\n"
// << "           five_minute_rate: " << metrics.requests.five_minute_rate    << "\n"
// << "        fifteen_minute_rate: " << metrics.requests.fifteen_minute_rate << "\n"
// << "                OS load_avg: " << load_avg << "\n"
//
//;
        }
        while(metrics.requests.mean_rate >= 10.0 || load_avg > 0.9);
std::cerr << "*** ...pause is over... ***\n";
    }

//std::cerr << "Executing query=[" << f_queryString.toUtf8().data() << "]" << std::endl;
    // we repeat the request when it times out
    int max_repeat(10);
    do
    {
        if(max_repeat == 0)
        {
            // TODO: this error does not automatically use the correct error
            //       code (i.e. the throwIfError() function checks various
            //       codes as "timed out")
            //
            const char * message = 0;
            size_t length        = 0;
            cass_future_error_message( f_sessionFuture.get(), &message, &length );
            QByteArray errmsg( message, length );
            std::stringstream ss;
            ss << "Query could not execute (timed out)! Cassandra error: code=" << static_cast<unsigned int>(CASS_ERROR_LIB_REQUEST_TIMED_OUT)
               << ", error={" << cass_error_desc(CASS_ERROR_LIB_REQUEST_TIMED_OUT )
               << "}, message={" << errmsg.data()
               << "} aborting operation!";
            throw exception_t( ss.str().c_str() );
        }
        --max_repeat;
        f_sessionFuture.reset( cass_session_execute( f_session->getSession().get(), f_queryStmt.get() ) , futureDeleter() );
    }
    while(throwIfError( QString("Error in query string [%1]!").arg(f_queryString) ));
}
#endif


/** \brief Start the query
 *
 * This method assumes that you have called the query() method already, and
 * optionally specified the paging size and any binding values to the query.
 *
 * \param block[in]	if true, then the method blocks while waiting for completion. It does not block if false.
 *
 * \sa query(), setPagingSize(), bindInt32(), bindInt64(), bindString(), bindByteArray()
 */
void query::start( const bool block )
{
    //testMetrics();
    //int64_t const now(QCassandra::timeofday());
    //std::cerr << now << " -- Executing query=[" << f_queryString.toUtf8().data() << "]" << std::endl;

    if( !f_queryStmt )
    {
        throw exception_t( "query::start() called with an unconnected session or no query statement." );
    }

    f_sessionFuture = std::make_unique<future>();
    *f_sessionFuture = f_session->getSession().execute( *f_queryStmt );
    if( block )
    {
        getQueryResult();
    }
    else
    {
        // This will call back on a background thread
        //
        f_sessionFuture->set_callback
            ( reinterpret_cast<void*>(&query::queryCallbackFunc)
            , reinterpret_cast<void*>(this)
            );
    }
}


/** \brief Non-blocking call to see if query has completed.
 *
 * If query has not yet completed (Cassandra future is not ready), then
 * the method immediately returns false.
 *
 * If it has completed, then the result is checked and throws on error.
 * If it was a valid result (CASS_OK), then true is returned
 *
 * /sa nextRow(), nextPage(), query(), getQueryResult()
 *
 * /return false if not ready, true otherwise
 */
bool query::isReady() const
{
    return f_sessionFuture->is_ready();
}


bool query::queryActive() const
{
    return (f_queryResult && f_rowsIterator);
}


/** \brief Get the query result. This method blocks if the result is not ready yet.
 *
 * \note Throws exception_t if query failed.
 *
 * /sa isReady(), query()
 */
void query::getQueryResult()
{
    throwIfError( QString("Error in query string:\n%1").arg(f_queryString) );

    f_queryResult   = std::make_unique<result>   ( f_sessionFuture->get_result() );
    f_rowsIterator  = std::make_unique<iterator> ( f_queryResult->get_iterator() );
}


/** \brief End the query and reset all of the pointers
 *
 * Call this to reset the query and destroy all of the cassandra-cpp object.
 *
 * \sa start()
 */
void query::end()
{
    f_queryString.clear();
    f_rowsIterator.reset();
    f_queryResult.reset();
    f_sessionFuture.reset();
    f_queryStmt.reset();
}


size_t query::rowCount() const
{
    return f_queryResult->get_row_count();
}


/** \brief Get the next row in the result set
 *
 * After you start your query, call this method to get the first/next row
 * in the result set. When you reach the end of the result set (or the current page),
 * it will return false.
 *
 * \sa query(), start(), nextPage()
 */
bool query::nextRow()
{
    return f_rowsIterator->next();
}


/** \brief Get the next page in the result set
 *
 * Once nextRow() returns false, and you have paging turned on, then call this
 * method to get the next page of results. When there are no more pages, this
 * will return false.
 *
 * \sa query(), start(), setPagingSize(), nextRow()
 */
bool query::nextPage( const bool block )
{
    if( !f_queryResult->has_more_pages() )
    {
        return false;
    }

    f_queryStmt->set_paging_state( *f_queryResult );

    // Reset the current query session, and run the next page
    //
    start( block );

    return true;
}


/** \brief Internal method for throwing after the query fails.
 *
 * \return true if a timeout error occurred and we want the query repeated.
 *         (always return false in current implementation!)
 *
 * \sa start()
 */
void query::throwIfError( const QString& msg )
{
    if( !f_sessionFuture )
    {
        std::stringstream ss;
        ss << "There is no active session for query [" << f_queryString.toUtf8().data() << "], msg=["
           << msg.toUtf8().data() << "]";
        throw exception_t( ss.str().c_str() );
    }

    const CassError code( f_sessionFuture->get_error_code() );
    if( code != CASS_OK )
    {
        throw query_exception_t( *f_sessionFuture, msg );
    }
}


#if 0
{
    const CassError code( cass_future_error_code( f_sessionFuture.get() ) );
    switch( code )
    {
    case CASS_OK:
        // everything was okay
        return false;

    //case CASS_ERROR_LIB_REQUEST_TIMED_OUT:
    //case CASS_ERROR_SERVER_WRITE_TIMEOUT:
    //case CASS_ERROR_SERVER_READ_TIMEOUT:
    //    sleep(1);
    //    return true;

    default:
        // some error occurred and we just throw on any others that
        // we do not handle here
        //
        {
            const char * message = 0;
            size_t length        = 0;
            cass_future_error_message( f_sessionFuture.get(), &message, &length );
            QByteArray errmsg( message, length );
            std::stringstream ss;
            ss << msg.toUtf8().data() << "! Cassandra error: code=" << static_cast<unsigned int>(code)
               << ", error={" << cass_error_desc(code)
               << "}, message={" << errmsg.data()
               << "} aborting operation!";
            throw exception_t( ss.str().c_str() );
        }

    }
}
#endif


/** \brief Get named Boolean column value
 *
 * \param name[in] name of column
 */
bool query::getBoolColumn( const QString &name ) const
{
    return f_rowsIterator->get_row().get_column_by_name( name ).get_bool();
}


/** \brief Get Boolean column value by number
 *
 * \param num[in] position of column in the result set
 */
bool query::getBoolColumn( const int num ) const
{
    return f_rowsIterator->get_row().get_column( num ).get_bool();
}


/** \brief Get named integer column value
 *
 * \param name[in] name of column
 */
int32_t query::getInt32Column( const QString& name ) const
{
    return f_rowsIterator->get_row().get_column_by_name( name ).get_int32();
}


/** \brief Get integer column value by position
 *
 * \param num[in] position of column in the result set
 */
int32_t query::getInt32Column( const int num ) const
{
    return f_rowsIterator->get_row().get_column( num ).get_int32();
}


/** \brief Get named counter column value
 *
 * \param name[in] name of column
 */
int64_t query::getInt64Column( const QString& name ) const
{
    return f_rowsIterator->get_row().get_column_by_name( name ).get_int64();
}


/** \brief Get counter column value by position
 *
 * \param num[in] position of column in the result set
 */
int64_t query::getInt64Column( const int num ) const
{
    return f_rowsIterator->get_row().get_column( num ).get_int64();
}


/** \brief Get named float column value
 *
 * \param name[in] name of column
 */
float query::getFloatColumn( const QString& name ) const
{
    return f_rowsIterator->get_row().get_column_by_name( name ).get_float();
}


/** \brief Get float column value by position
 *
 * \param num[in] position of column in the result set
 */
float query::getFloatColumn( const int num ) const
{
    return f_rowsIterator->get_row().get_column( num ).get_float();
}


/** \brief Get named double column value
 *
 * \param name[in] name of column
 */
double query::getDoubleColumn( const QString &name ) const
{
    return f_rowsIterator->get_row().get_column_by_name( name ).get_double();
}


/** \brief Get double column value by position
 *
 * \param num[in] position of column in the result set
 */
double query::getDoubleColumn( const int num ) const
{
    return f_rowsIterator->get_row().get_column( num ).get_double();
}


/** \brief Get named string column value
 *
 * \param name[in] name of column
 */
QString query::getStringColumn( const QString& name ) const
{
    return f_rowsIterator->get_row().get_column_by_name( name ).get_string();
}


/** \brief Get string column value by position
 *
 * \param num[in] position of column in the result set
 */
QString query::getStringColumn( const int num ) const
{
    return f_rowsIterator->get_row().get_column( num ).get_string();
}


/** \brief Get named byte array column value
 *
 * \param name[in] name of column
 */
QByteArray query::getByteArrayColumn( const char * name ) const
{
    return f_rowsIterator->get_row().get_column_by_name( name ).get_blob();
}


/** \brief Get named byte array column value
 *
 * \param name[in] name of column
 */
QByteArray query::getByteArrayColumn( const QString& name ) const
{
    return f_rowsIterator->get_row().get_column_by_name( name ).get_blob();
}


/** \brief Get byte array column value by position
 *
 * \param num[in] position of column in the result set
 */
QByteArray query::getByteArrayColumn( const int num ) const
{
    return f_rowsIterator->get_row().get_column( num ).get_blob();
}


/** \brief Get named JSON map column value
 *
 * \param name[in] name of column
 */
query::string_map_t query::getJsonMapColumn ( const QString& name ) const
{
    string_map_t json_map;
    getMapFromJsonObject( json_map, getStringColumn( name ) );
    return json_map;
}


/** \brief Get JSON map column value by position
 *
 * \param num[in] position of column in the result set
 */
query::string_map_t query::getJsonMapColumn ( const int num ) const
{
    string_map_t json_map;
    getMapFromJsonObject( json_map, getStringColumn( num ) );
    return json_map;
}


/** \brief Get Cassandra map column value from CassValue
 *
 * \param value[in] pointer to Cassandra value
 */
query::string_map_t query::getMapFromValue( const casswrapper::value& value ) const
{
    string_map_t ret_map;
    iterator map_iter( value.get_iterator_from_map() );
    while( map_iter.next() )
    {
        casswrapper::value const key( map_iter.get_map_key   () );
        casswrapper::value const val( map_iter.get_map_value () );
        std::string const key_str(key.get_string().toUtf8().data());
        std::string const val_str(val.get_string().toUtf8().data());
        //
        ret_map[key_str] = val_str;
    }

    return ret_map;
}


/** \brief Get named Cassandra map column value
 *
 * \param name[in] name of column
 */
query::string_map_t query::getMapColumn ( const QString& name ) const
{
    return getMapFromValue( f_rowsIterator->get_row().get_column_by_name( name ) );
}


/** \brief Get Cassandra map column value by position
 *
 * \param num[in] position of column in the result set
 */
query::string_map_t query::getMapColumn ( const int num ) const
{
    return getMapFromValue( f_rowsIterator->get_row().get_column( num ) );
}


} // namespace casswrapper

// vim: ts=4 sw=4 et
