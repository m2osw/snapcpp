/*
 * Text:
 *      src/casswrapper_impl.cpp
 *
 * Description:
 *      Handling of the CQL interface.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2017 Made to Order Software Corp.
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

#include "casswrapper_impl.h"
#include "exception_impl.h"

#include <sstream>

/** \mainpage
 * \brief libcasswrapper C++ Documentation
 *
 * \section introduction Introduction
 *
 * The libcasswrapper library provides a light wrapper around the Cassandra CPP
 * Driver API. 
 *
 * Datastax's library is sadly NOT really C++, but consistent straight C, in
 * terms of design. All of the resources handles returned are not managed
 * with constructors/destructors to deal with object lifetime. The developer is
 * responsible to make sure resources are properly destroyed once he/she is
 * finished with them. This opens up the door to memory and resource leaks.
 *
 * In order to avoid those pitfalls, we've written a thin C++ "wrapper" around
 * the driver API itself. The resource handles are preserved in std smart
 * pointers, which automatically manage the lifetimes and prevent resource
 * leaks.  This makes the library a lot easier to use in an object oriented
 * situation, consistent with the tried and true design principle of RAII.
 *
 * \section core Core Classes
 *
 * At the core of this library is the Query class. Since Cassandra migrated to
 * CQL and away from their traditional binary driver (which required Thrift), a
 * method is needed to deal with sending CQL queries to the database server.
 * Also, the unique design of this distributed database means that you should
 * not code your app to sit around and wait for a response. One needs to think
 * in a distributed, threaded way. You can "block" until the operation
 * completes, but a more effective way is to use the non-block switch and take
 * advantage of the callback. For example, here is a traditional "blocking" way
 * to make a database query:
 *
 * \code
 *    Query::pointer_t my_query( Query::create(session) );
 *    my_query->query( "SELECT foo,bar FROM my_database.foobar" );
 *    my_query->setPagingSize( 100 );     // listen for 100 records at a time
 *    my_query->start();                  // defaults to blocking
 *    //
 *    do
 *    {
 *        while( q->nextRow() )
 *        {
 *            const int32_t foo(my_query->getInt32Column("foo"));
 *            const QString bar(my_query->getStringColumn("bar"));
 *            // Do stuff
 *        }
 *    }
 *    while( q->nextPage() );
 * \endcode
 *
 * However, if you attach to the Qt5 signal queryFinished(), then instead of
 * calling, you can return processing to other critical areas of your code and
 * process the returned rows when they are ready.
 *
 * \section whats_next What's Next?
 *
 * There are certain elements of Datastax's API which we have not yet
 * implemented, however we have captured the most critical aspects. You can
 * peruse the database schema without having to directly query the system tables
 * (which are subject to change between versions of the database), you can take
 * advantage of batch processing, all data types are supported, and you even can
 * create an SSL connection to your database.
 *
 * And the huge advantage is that we put all resources into smart pointer
 * containers, so this will help you keep resource and memory leaks to a minimum
 * in your software.
 *
 * \sa casswrapper::Query::queryFinished(), casswrapper::Query, casswrapper::Session
 */


namespace casswrapper
{


//===============================================================================
// batch
//
void batch::deleter_t::operator()(CassBatch* p) const
{
    cass_batch_free( p );
}


batch::batch( CassBatchType type )
    : f_ptr(cass_batch_new(type),deleter_t())
{
}


void batch::set_consistency ( CassConsistency const c ) const
{
    CassError const rc = cass_batch_set_consistency( f_ptr.get(), c );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl("Cannot set batch consistency!",rc);
    }
}


void batch::set_serial_consistency ( CassConsistency const c ) const
{
    CassError const rc = cass_batch_set_serial_consistency( f_ptr.get(), c );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl("Cannot set batch serial consistency!",rc);
    }
}


void batch::set_timestamp ( int64_t const timestamp ) const
{
    CassError rc = cass_batch_set_timestamp( f_ptr.get(), timestamp );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl("Cannot set batch timestamp!",rc);
    }
}


void batch::set_request_timeout ( int64_t const timeout ) const
{
    CassError rc = cass_batch_set_request_timeout( f_ptr.get(), timeout );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl("Cannot set batch request timeout!",rc);
    }
}


void batch::set_is_idempotent ( bool const val ) const
{
    CassError rc = cass_batch_set_is_idempotent( f_ptr.get(), val? cass_true: cass_false );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl("Cannot set batch idempotent status!",rc);
    }
}


void batch::set_retry_policy ( retry_policy const& p ) const
{
    CassError rc = cass_batch_set_retry_policy( f_ptr.get(), p.f_ptr.get() );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl("Cannot set batch retry policy!",rc);
    }
}


void batch::set_custom_payload ( custom_payload const& p ) const
{
    CassError rc = cass_batch_set_custom_payload( f_ptr.get(), p.f_ptr.get() );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl("Cannot set batch custom payload!",rc);
    }
}


void batch::add_statement ( statement const& p ) const
{
    CassError rc = cass_batch_add_statement( f_ptr.get(), p.f_ptr.get() );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl("Cannot add statement to batch!",rc);
    }
}


//===============================================================================
// collection
//
collection::collection( CassCollectionType type, size_t item_count )
    : f_ptr( cass_collection_new( type, item_count ), deleter_t() )
{
}


void collection::deleter_t::operator()(CassCollection* p) const
{
    cass_collection_free(p);
}


void collection::append_string( const std::string& value ) const
{
    const CassError rc = cass_collection_append_string( f_ptr.get(), value.c_str() );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl(
            QString("Cannot append string '%1' to collection!")
                .arg(value.c_str())
            , rc
            );
    }
}


//===============================================================================
// column_meta
//
column_meta::column_meta( CassColumnMeta* p )
    : f_ptr( p, deleter_t() )
{
}


void column_meta::deleter_t::operator()(const CassColumnMeta*) const
{
    // No need to delete anything
    //cass_column_meta_free(p);
}


QString column_meta::get_name() const
{
    const char * name;
    size_t len;
    cass_column_meta_name( f_ptr.get(), &name, &len );
    return QString::fromUtf8(name,len);
}


CassColumnType column_meta::get_column_type() const
{
    return cass_column_meta_type( f_ptr.get() );
}


CassValueType  column_meta::get_value_type()  const
{
    return cass_data_type_type( cass_column_meta_data_type(f_ptr.get()) );
}


iterator column_meta::get_fields() const
{
    return iterator( cass_iterator_fields_from_column_meta( f_ptr.get() ) );
}


//===============================================================================
// cluster
//
cluster::cluster()
    : f_ptr(cass_cluster_new(), deleter_t())
{
}


void cluster::deleter_t::operator()( CassCluster * p) const
{
    cass_cluster_free(p);
}


void cluster::set_contact_points( const QString& host_list )
{
    cass_cluster_set_contact_points( f_ptr.get(), host_list.toUtf8().data() );
}


void cluster::set_port( const int port )
{
    cass_cluster_set_port( f_ptr.get(), port );
}


void cluster::set_request_timeout( const timeout_t timeout )
{
    cass_cluster_set_request_timeout( f_ptr.get(), static_cast<unsigned>(timeout) );
}


void cluster::set_write_bytes_low_water_mark( const uint32_t low )
{
    cass_cluster_set_write_bytes_low_water_mark( f_ptr.get(), low  );
}


void cluster::set_write_bytes_high_water_mark( const uint32_t high )
{
    cass_cluster_set_write_bytes_high_water_mark( f_ptr.get(), high );
}


void cluster::reset_ssl() const
{
    cass_cluster_set_ssl( f_ptr.get(), nullptr );
}


void cluster::set_ssl( const ssl& ssl ) const
{
    cass_cluster_set_ssl( f_ptr.get(), ssl.f_ptr.get() );
}


//===============================================================================
// custom_payload
//
custom_payload::custom_payload()
    : f_ptr(cass_custom_payload_new(),deleter_t())
{
}


void custom_payload::deleter_t::operator()(CassCustomPayload* p) const
{
    cass_custom_payload_free( p );
}


void custom_payload::payload_set( QString const& name, QByteArray const& value ) const
{
    cass_custom_payload_set_n
        ( f_ptr.get()
        , name.toUtf8().data()
        , name.size()
        , reinterpret_cast<const cass_byte_t*>(value.data())
        , value.size()
        );
}


void custom_payload::payload_remove( QString const& name ) const
{
    cass_custom_payload_remove_n
        ( f_ptr.get()
        , name.toUtf8().data()
        , name.size()
        );
}


//===============================================================================
// future
//
future::future()
{
}


future::future( CassFuture* p )
    : f_ptr( p, deleter_t() )
{
}


void future::deleter_t::operator()(CassFuture * p) const
{
    cass_future_free(p);
}


future::future( const session& sess, const cluster& cl )
    : f_ptr( cass_session_connect( sess.f_ptr.get(), cl.f_ptr.get() ), deleter_t() )
{
}


CassError future::get_error_code() const
{
    return cass_future_error_code( f_ptr.get() );
}


QString future::get_error_message() const
{
    const char* message = 0;
    size_t length       = 0;
    cass_future_error_message( f_ptr.get(), &message, &length );
    return QString::fromUtf8(message,length);
}


result future::get_result() const
{
    return result( const_cast<CassResult*>(cass_future_get_result(f_ptr.get())) );
}


bool future::is_ready() const
{
    return cass_future_ready( f_ptr.get() );
}


void future::set_callback( void* callback, void* data )
{
    cass_future_set_callback( f_ptr.get(), reinterpret_cast<CassFutureCallback>(callback), data );
}


void future::wait() const
{
    cass_future_wait( f_ptr.get() );
}


bool future::operator ==( const future& f )
{
    return f_ptr == f.f_ptr;
}


bool future::operator !=( const future& f )
{
    return !(*this == f);
}


//===============================================================================
// iterator
//
iterator::iterator( CassIterator* p )
    : f_ptr(p, deleter_t())
{
}


iterator::iterator( const iterator& iter )
    : f_ptr( iter.f_ptr )
{
}


void iterator::deleter_t::operator()(CassIterator* p) const
{
    cass_iterator_free( p );
}


bool iterator::next() const
{
    return cass_iterator_next( f_ptr.get() ) == cass_true? true: false;
}


value iterator::get_map_key() const
{
    return value( const_cast<CassValue*>(cass_iterator_get_map_key(f_ptr.get())) );
}


value iterator::get_map_value() const
{
    return value( const_cast<CassValue*>(cass_iterator_get_map_value(f_ptr.get())) );
}


value iterator::get_value() const
{
    return value( const_cast<CassValue*>(cass_iterator_get_value(f_ptr.get())) );
}


QString iterator::get_meta_field_name() const
{
    const char * name;
    size_t len;
    const CassError rc = cass_iterator_get_meta_field_name( f_ptr.get(), &name, &len );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Cannot get field name from iterator!", rc );
    }

    return QString::fromUtf8( name, len );
}


value iterator::get_meta_field_value() const
{
    return value( const_cast<CassValue*>(cass_iterator_get_meta_field_value( f_ptr.get() )) );
}


row iterator::get_row() const
{
    return row( const_cast<CassRow*>(cass_iterator_get_row(f_ptr.get())) );
}


keyspace_meta iterator::get_keyspace_meta() const
{
    return keyspace_meta( const_cast<CassKeyspaceMeta*>(cass_iterator_get_keyspace_meta(f_ptr.get())) );
}


table_meta iterator::get_table_meta() const
{
    return table_meta( const_cast<CassTableMeta*>(cass_iterator_get_table_meta(f_ptr.get())) );
}


column_meta iterator::get_column_meta() const
{
    return column_meta( const_cast<CassColumnMeta*>(cass_iterator_get_column_meta(f_ptr.get())) );
}


//===============================================================================
// keyspace_meta
//
keyspace_meta::keyspace_meta( CassKeyspaceMeta* p )
    : f_ptr( p, deleter_t() )
{
    //cass_keyspace_meta_deleter(p);
}


void keyspace_meta::deleter_t::operator()( CassKeyspaceMeta* ) const
{
    // No need to do this...
    //
    //cass_keyspace_meta_deleter(p);
}


iterator keyspace_meta::get_fields() const
{
    return iterator( cass_iterator_fields_from_keyspace_meta( f_ptr.get() ) );
}


iterator keyspace_meta::get_tables() const
{
    return iterator( cass_iterator_tables_from_keyspace_meta( f_ptr.get() ) );
}


QString keyspace_meta::get_name() const
{
    const char* name = 0;
    size_t length    = 0;
    cass_keyspace_meta_name( f_ptr.get(), &name, &length );
    return QString::fromUtf8( name, length );
}


//===============================================================================
// retry_policy
//
retry_policy::retry_policy( type_t const t )
{
    switch( t )
    {
    case type_t::Default                : f_ptr.reset( cass_retry_policy_default_new()                 , deleter_t() ); break;
    case type_t::DowngradingConsistency : f_ptr.reset( cass_retry_policy_downgrading_consistency_new() , deleter_t() ); break;
    case type_t::FallThrough            : f_ptr.reset( cass_retry_policy_fallthrough_new()             , deleter_t() ); break;
    case type_t::Logging                : throw libexcept::exception_t("You must use the other constructor for retry_policy. We need a child policy."); break;
    }
}


retry_policy::retry_policy( retry_policy const& child_policy )
    : f_ptr( cass_retry_policy_logging_new(child_policy.f_ptr.get()), deleter_t() )
{
}


void retry_policy::deleter_t::operator()( CassRetryPolicy* p ) const
{
    cass_retry_policy_free(p);
}


//===============================================================================
// result
//
result::result( CassResult* p )
    : f_ptr(p, deleter_t() )
{
}


result::result( const result& res )
    : f_ptr( res.f_ptr )
{
}


void result::deleter_t::operator()(const CassResult* p) const
{
    cass_result_free(p);
}


iterator result::get_iterator() const
{
    return cass_iterator_from_result( f_ptr.get() );
}


size_t result::get_row_count() const
{
    return cass_result_row_count( f_ptr.get() );
}


size_t result::get_column_count() const
{
    return cass_result_column_count( f_ptr.get() );
}


bool result::has_more_pages() const
{
    return cass_result_has_more_pages( f_ptr.get() ) == cass_true? true: false;
}


QString result::get_column_name( size_t const index ) const
{
    const char * name;
    size_t len;
    CassError rc = cass_result_column_name( f_ptr.get(), index, &name, &len );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( QString("Error fetching column name from column %1").arg(index), rc );
    }
    return QString::fromUtf8( name, len );
}


CassValueType result::get_column_type( size_t const index ) const
{
    return cass_result_column_type( f_ptr.get(), index );
}


row result::get_first_row() const
{
    return cass_result_first_row(f_ptr.get());
}


//===============================================================================
// row
//
row::row( CassRow* p )
    : f_ptr( p, deleter_t() )
{
}


row::row( CassRow const* p )
    : f_ptr( const_cast<CassRow*>(p), deleter_t() )
{
}


void row::deleter_t::operator()( CassRow* ) const
{
    // Not needed, API deletes it on its own.
    //cass_row_free(p);
}


value row::get_column_by_name( const QString& name ) const
{
    return value(
            const_cast<CassValue*>(cass_row_get_column_by_name( f_ptr.get(), name.toUtf8().data() ))
            );
}


value row::get_column( const int num ) const
{
    return value(
            const_cast<CassValue*>(cass_row_get_column( f_ptr.get(), num ))
            );
}


iterator row::get_iterator() const
{
    return iterator(
            cass_iterator_from_row( f_ptr.get() )
            );
}


//===============================================================================
// schema_meta
//
schema_meta::schema_meta( const session& s )
    : f_ptr( const_cast<CassSchemaMeta*>(cass_session_get_schema_meta(s.f_ptr.get())), deleter_t() )
{
}


void schema_meta::deleter_t::operator()(const CassSchemaMeta* p) const
{
    cass_schema_meta_free(p);
}


iterator schema_meta::get_keyspaces() const
{
    return( cass_iterator_keyspaces_from_schema_meta( f_ptr.get() ) );
}


//===============================================================================
// session
//
session::session()
    : f_ptr(cass_session_new(), deleter_t())
{
}


void session::deleter_t::operator()(CassSession* p) const
{
    cass_session_free(p);
}


future session::execute( const statement& s ) const
{
    return future( 
            cass_session_execute( f_ptr.get(), s.f_ptr.get() )
            );
}


future session::execute_batch( const batch& b ) const
{
    return future(
            cass_session_execute_batch( f_ptr.get(), b.f_ptr.get() )
            );
}


future session::close() const
{
    return future( cass_session_close(f_ptr.get()) );
}


//===============================================================================
// ssl
//
ssl::ssl()
    : f_ptr( cass_ssl_new(), deleter_t() )
{
    cass_ssl_set_verify_flags( f_ptr.get(), CASS_SSL_VERIFY_PEER_CERT | CASS_SSL_VERIFY_PEER_IDENTITY );
}


void ssl::deleter_t::operator()(CassSsl* p) const
{
    cass_ssl_free(p);
}


void ssl::add_trusted_cert( const QString& cert )
{
    CassError rc = cass_ssl_add_trusted_cert_n
        ( f_ptr.get()
        , cert.toUtf8().data()
        , cert.size()
        );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Error loading SSL certificate", rc );
    }
}


//===============================================================================
// statement
//
statement::statement( const QString& query, const int bind_count  )
    : f_ptr(const_cast<CassStatement*>(cass_statement_new(query.toUtf8().data(), bind_count)), deleter_t())
    , f_query(query)
{
}


void statement::deleter_t::operator()(CassStatement* p) const
{
    cass_statement_free(p);
}


void statement::set_consistency( const CassConsistency consist ) const
{
    cass_statement_set_consistency( f_ptr.get(), consist );
}


void statement::set_timestamp( const int64_t timestamp ) const
{
    cass_int64_t const cass_time( static_cast<cass_int64_t>(timestamp) );
    cass_statement_set_timestamp( f_ptr.get(), cass_time );
}


void statement::set_paging_size( const int size ) const
{
    cass_statement_set_paging_size( f_ptr.get(), size );
}


void statement::set_paging_state( const result& res ) const
{
    cass_statement_set_paging_state( f_ptr.get(), res.f_ptr.get() );
}


void statement::bind_bool( const size_t id, const bool value ) const
{
   cass_statement_bind_bool( f_ptr.get(), id, value? cass_true: cass_false );
}


void statement::bind_bool( const QString& id, const bool value ) const
{
   cass_statement_bind_bool_by_name( f_ptr.get(), id.toUtf8().data(), value? cass_true: cass_false );
}


void statement::bind_int32( const size_t id, const int32_t value ) const
{
   cass_statement_bind_int32( f_ptr.get(), id, value );
}


void statement::bind_int32( const QString& id, const int32_t value ) const
{
   cass_statement_bind_int32_by_name( f_ptr.get(), id.toUtf8().data(), value );
}


void statement::bind_int64( const size_t id, const int64_t value ) const
{
   cass_statement_bind_int64( f_ptr.get(), id, value );
}


void statement::bind_int64( const QString& id, const int64_t value ) const
{
   cass_statement_bind_int64_by_name( f_ptr.get(), id.toUtf8().data(), value );
}


void statement::bind_float( const size_t id, const float value ) const
{
   cass_statement_bind_float( f_ptr.get(), id, value );
}


void statement::bind_float( const QString& id, const float value ) const
{
   cass_statement_bind_float_by_name( f_ptr.get(), id.toUtf8().data(), value );
}


void statement::bind_double( const size_t id, const double value ) const
{
   cass_statement_bind_double( f_ptr.get(), id, value );
}


void statement::bind_double( const QString& id, const double value ) const
{
   cass_statement_bind_double_by_name( f_ptr.get(), id.toUtf8().data(), value );
}


void statement::bind_string( const size_t id, const std::string& value ) const
{
    bind_blob( id, value.c_str() );
}


void statement::bind_string( const QString& id, const std::string& value ) const
{
    bind_blob( id, value.c_str() );
}


void statement::bind_string( const size_t id, const QString& value ) const
{
    bind_blob( id, value.toUtf8() );
}


void statement::bind_string( const QString& id, const QString& value ) const
{
    bind_blob( id, value.toUtf8() );
}


void statement::bind_blob( const size_t id, const QByteArray& value ) const
{
    cass_statement_bind_string_n( f_ptr.get(), id, value.constData(), value.size() );
}


void statement::bind_blob( const QString& id, const QByteArray& value ) const
{
    cass_statement_bind_string_by_name_n( f_ptr.get(), id.toUtf8().data(), id.size(), value.constData(), value.size() );
}


void statement::bind_collection ( const size_t id, const collection& value ) const
{
    cass_statement_bind_collection( f_ptr.get(), id, value.f_ptr.get() );
}


void statement::bind_collection ( const QString& id, const collection& value ) const
{
    cass_statement_bind_collection_by_name( f_ptr.get(), id.toUtf8().data(), value.f_ptr.get() );
}


//===============================================================================
// table_meta
//
table_meta::table_meta( CassTableMeta* p )
    : f_ptr( p, deleter_t() )
{
}


void table_meta::deleter_t::operator()( CassTableMeta* ) const
{
    //cass_table_meta_free(p);
}


iterator table_meta::get_fields() const
{
    return iterator( cass_iterator_fields_from_table_meta( f_ptr.get() ) );
}


iterator table_meta::get_columns() const
{
    return iterator( cass_iterator_columns_from_table_meta( f_ptr.get() ) );
}


QString table_meta::get_name() const
{
    const char * name;
    size_t len;
    cass_table_meta_name( f_ptr.get(), &name, &len );
    return QString::fromUtf8( name, len );
}



//===============================================================================
// value
//
value::value( CassValue* p )
    : f_ptr(p, deleter_t() )
{
}


void value::deleter_t::operator()(CassValue*) const
{
    // no deletion necessary
}


iterator value::get_iterator_from_map() const
{
    return iterator( cass_iterator_from_map(f_ptr.get()) );
}


iterator value::get_iterator_from_collection() const
{
    return iterator( cass_iterator_from_collection(f_ptr.get()) );
}


iterator value::get_iterator_from_tuple() const
{
    return iterator( cass_iterator_from_tuple(f_ptr.get()) );
}


CassValueType value::get_type() const
{
    return cass_value_type( f_ptr.get() );
}


QString value::get_string() const
{
    const char* str;
    size_t len = 0;
    CassError rc = cass_value_get_string( f_ptr.get(), &str, &len );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Can't extract value string!", rc );
    }
    return QString::fromUtf8( str, len );
}


QByteArray value::get_blob() const
{
    const cass_byte_t* buff;
    size_t len = 0;
    CassError rc = cass_value_get_bytes( f_ptr.get(), &buff, &len );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Cannot extract value blob!", rc );
    }
    return QByteArray( reinterpret_cast<const char *>(buff), len );
}


bool value::get_bool() const
{
    cass_bool_t b;
    CassError rc = cass_value_get_bool( f_ptr.get(), &b );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Cannot extract value!", rc );
    }
    return b == cass_true;
}


float value::get_float() const
{
    cass_float_t f;
    CassError rc = cass_value_get_float( f_ptr.get(), &f );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Cannot extract value!", rc );
    }
    return static_cast<float>(f);
}


double value::get_double() const
{
    cass_double_t d;
    CassError rc = cass_value_get_double( f_ptr.get(), &d );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Cannot extract value!", rc );
    }
    return static_cast<double>(d);
}


int8_t value::get_int8() const
{
    cass_int8_t i;
    CassError rc = cass_value_get_int8( f_ptr.get(), &i );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Cannot extract value!", rc );
    }
    return static_cast<int8_t>(i);
}


int16_t value::get_int16() const
{
    cass_int16_t i;
    CassError rc = cass_value_get_int16( f_ptr.get(), &i );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Cannot extract value!", rc );
    }
    return static_cast<int16_t>(i);
}


int32_t value::get_int32() const
{
    cass_int32_t i;
    CassError rc = cass_value_get_int32( f_ptr.get(), &i );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Cannot extract value!", rc );
    }
    return static_cast<int32_t>(i);
}


int64_t value::get_int64() const
{
    cass_int64_t i;
    CassError rc = cass_value_get_int64( f_ptr.get(), &i );
    if( rc != CASS_OK )
    {
        throw cassandra_exception_impl( "Cannot extract value!", rc );
    }
    return static_cast<qlonglong>(i);
}


QString value::get_uuid() const
{
    CassUuid uuid;
    CassError rc = cass_value_get_uuid( f_ptr.get(), &uuid );
    if( rc == CASS_OK )
    {
        char str[CASS_UUID_STRING_LENGTH+1];
        cass_uuid_string( uuid, str );
        return QString(str);
    }

    return QString();
}


qulonglong value::get_uuid_timestamp() const
{
    CassUuid uuid;
    CassError rc = cass_value_get_uuid( f_ptr.get(), &uuid );
    if( rc == CASS_OK )
    {
        return static_cast<qulonglong>(cass_uuid_timestamp( uuid ));
    }

    return 0;
}


QString value::get_inet() const
{
    CassInet inet;
    CassError rc = cass_value_get_inet( f_ptr.get(), &inet );
    if( rc == CASS_OK )
    {
        char str[CASS_UUID_STRING_LENGTH+1];
        cass_inet_string( inet, str );
        return QString(str);
    }
   
    return QString();
}




}
// namespace casswrapper

// vim: ts=4 sw=4 et
