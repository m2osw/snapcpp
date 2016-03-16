/*
 * Text:
 *      QCassandraPrivate.cpp
 *
 * Description:
 *      Handling of the cassandra::CassandraClient and corresponding transports,
 *      protocols, sockets, etc.
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

#include "QCassandraPrivate.h"
#include "legacy/cassandra_types.h"

#include <as2js/json.h>

#include <iostream>
#include <stdexcept>

#include <QtCore>

namespace QtCassandra
{


namespace
{

/** \brief Value representing a node that's disconnected.
 *
 * Nodes that are up and down can be checked using the
 * describe_schema_versions() function. If a node is
 * disconnected, its version is set to UNREACHABLE.
 */
const char g_unreachable[] = "UNREACHABLE";

} // unnamed namespace


/** \class QCassandraPrivate
 * \brief Private data for the QCassandra class.
 *
 * This class handles such things as the connection to the Cassandra
 * server and hiding all the thrift definitions.
 */

/** \var QCassandraPrivate::f_parent
 * \brief The pointer to the QCassandra object.
 *
 * This variable member holds the QCassandra object pointer. This
 * QCassandra object owns the QCassandraPrivate object (it's a 1 to 1.)
 * This is why this pointer is a bare pointer. When the QCassandra
 * goes, so does the QCassandraPriavet. And if the QCassandraPrivate
 * goes, that's because the QCassandra is going.
 */

/** \var QCassandraPrivate::f_socket
 * \brief The socket used to connect to the Cassandra server.
 *
 * This pointer holds a Thrift socket used to communicate with the
 * Cassandra server.
 *
 * At our level we just provide the socket to the framed transport.
 * How it is being used is Thrift responsibility.
 *
 * It is setup with the IP address and port that one can use to
 * access the Cassandra server.
 */

/** \var QCassandraPrivate::f_transport
 * \brief The transport handler.
 *
 * The transport object of the Thrift system to manage the data
 * being send and received from the Cassandra server.
 *
 * The socket is attached to the transport system.
 */

/** \var QCassandraPrivate::f_protocol
 * \brief The protocol between us and the Cassandra server.
 *
 * This object defines the protocol. In other words, it knows how
 * to send and receive messages (i.e. header, footer, etc.)
 */

/** \var QCassandraPrivate::f_client
 * \brief Define a client object.
 *
 * The client object is what we use everywhere to communicate
 * with the Cassandra server. It is a Cassandra Client object
 * that knows how to convert the Cassandra specific messages
 * and send them over the transport object.
 *
 * To create a client, we need to create a socket, a transport
 * and a protocol. If any one of those steps fails, then you
 * cannot communicate with the Cassandra server.
 *
 * The main raison for failure is an invalid IP/port combinaison
 * or your Cassandra server isn't running.
 */

/** \brief Initialize the private QCassandra object.
 *
 * This function initialize the QCassandra private object.
 *
 * \param[in] parent  The parent pointer (i.e. QCassandra object)
 */
QCassandraPrivate::QCassandraPrivate( QCassandra::pointer_t parent )
    : f_parent(parent)
    //, f_cluster
    //, f_session
    //, f_connection
{
}

/** \fn QCassandraPrivate::QCassandraPrivate(const QCassandraPrivate &)
 * \brief Forbid the copy operator.
 *
 * By default we forbid the copy operator of the QCassandraPrivate class.
 *
 * The class includes a network connection and we really do not want to
 * have to clone such a thing.
 *
 * Note that most of the objects do not define the copy operator since
 * they are QObject's and these are already not copyable.
 */

/** \brief Clean up the object.
 *
 * This function makes sure we clean up after ourselves.
 */
QCassandraPrivate::~QCassandraPrivate()
{
    disconnect();
}


/** \brief Execute a full query string.
 *
 */
future_pointer_t QCassandraPrivate::executeQuery( const QString &query ) const
{
    statement_pointer_t statement(
        cass_statement_new( query.toUtf8().data(), 0 ), statementDeleter() );
    future_pointer_t future(
        cass_session_execute( f_session.get(), statement.get() ),
        futureDeleter() );

    cass_future_wait( future.get() );

    throwIfError( future, QString( "Query [%1] failed" ).arg( query ) );

    return future;
}

/// \brief Execute a query based on a table and a single column within. No
/// keyspace is assumed.
//
/// \note This assumes a single column query
//
void QCassandraPrivate::executeQuery( const QString &query, QStringList &values ) const
{
    future_pointer_t future( executeQuery( query ) );
    result_pointer_t result( cass_future_get_result( future.get() ),
                             resultDeleter() );

    values.clear();
    CassIterator *rows = cass_iterator_from_result( result.get() );
    while ( cass_iterator_next( rows ) )
    {
        const CassRow *row = cass_iterator_get_row( rows );
        values << QString( getByteArrayFromRow( row, 0 ).data() );
    }
}

/// \brief Execute a query based on a table and a single column within. No
/// keyspace is assumed.
//
/// \note This assumes a single column query
//
void QCassandraPrivate::executeQuery( const QString &table, const QString &column,
                               QStringList &values ) const
{
    const QString query(
        QString( "SELECT %1 FROM %2" ).arg( column ).arg( table ) );
    executeQuery( query, values );
}


/// \brief Execute a query based on a table and a single column within. No
/// keyspace is assumed.
//
/// \note This assumes a single column query
//
void QCassandraPrivate::executeQuery( const QString &query, QStringList &values ) const
{
    future_pointer_t future( executeQuery( query ) );
    result_pointer_t result( cass_future_get_result( future.get() ),
                             resultDeleter() );

    values.clear();
    CassIterator *rows = cass_iterator_from_result( result.get() );
    while ( cass_iterator_next( rows ) )
    {
        const CassRow *row = cass_iterator_get_row( rows );
        values << QString( getByteArrayFromRow( row, 0 ).data() );
    }
}


/** \brief Connect to a Cassandra Cluster.
 *
 * This function connects to a Cassandra Cluster. Which cluster is determined
 * by the host and port parameters.
 *
 * One cluster may include many database contexts (i.e. keyspaces.) Each database
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
 * \param[in] host      The host, defaults to "localhost" (an IP address, computer
 *                      hostname, domain name, etc.)
 * \param[in] port      The connection port, defaults to 9042.
 *
 * \return true if the connection succeeds, throws otherwise
 */
bool QCassandraPrivate::connect( const QString& host, const int port )
{
    QStringList host_list;
    host_list << host;
    return connect( host_list, port );
}

/** \brief Connect to the Cassandra server.
 *
 * This function needs to be called before most of the others.
 *
 * It is not mentioned in each other function that if the connection is not
 * up then an exception is generated. It will be noted, however, that a
 * connection is required for the function to work.
 *
 * Note that the main QCassandra object is responsible to test whether the
 * connection is valid (isConnected()) and to capture exceptions that should
 * be replaced by a boolean return values and similar approaches.
 *
 * In the early days expect to see exceptions coming up until we take care
 * of them all.
 *
 * Note that this function first disconnects the existing connection if there
 * is one.
 *
 * When passing a password, the function attempts an SSL connection instead
 * of a standard direct connection. These should be used when connecting over
 * the Internet. If you do not have a password but still wanted to use SSL,
 * then set the password to the special value "ignore".
 *
 * \warning
 * Although there is a password parameter, it has not yet been successfully
 * tested by us. If it works for you, great! You may have to tweak the code
 * though (in which case we'd really appreciate a patch!)
 *
 * \todo
 * Add means for users to be able to define SSL parameters such as the ciphers
 * accepted, certificates, public/private keys, etc.
 *
 * \param[in] host  The host of the Cassandra server.
 * \param[in] port  The port of the Cassandra server.
 *
 * \return true when the server connected, false otherwise
 *
 * \sa isConnected()
 * \sa disconnect()
 * \sa QCassandraSocketFactory
 */
bool QCassandraPrivate::connect(const QStringList& host_list, const int port )
{
    // disconnect any existing connection
    disconnect();

    std::stringstream contact_points;
    for( QString host : host_list )
    {
        if( contact_points.str() != "" )
        {
            contact_points << ",";
        }
        contact_points << host.toUtf8().data();
    }

    f_cluster.reset( cass_cluster_new(), clusterDeleter() );
    cass_cluster_set_contact_points( f_cluster.get(), contact_points.str().c_str() );

    std::stringstream port_str;
    port_str << port;
    cass_cluster_set_contact_points( f_cluster.get(), port_str.str().c_str() );
    //
    f_session.reset( cass_session_new(), sessionDeleter() );
    f_connection.reset( cass_session_connect(f_session.get(), f_cluster.get()), futureDeleter() );

    /* This operation will block until the result is ready */
    CassError rc = cass_future_error_code(f_connection.get());
    if( rc != CASS_OK )
    {
        const char* message;
        size_t message_length;
        cass_future_error_message( f_connection.get(), &message, &message_length );
        std::stringstream msg;
        msg << "Cannot connect to cassandra server! Reason=[" << std::string(message) << "]";

        f_connection.reset();
        f_session.reset();
        f_cluster.reset();
        throw std::runtime_error( msg.str().c_str() );
    }

    future_pointer_t    future( executeQuery( "SELECT cluster_name, native_protocol_version, partitioner FROM system.local" ) );
    result_pointer_t	result( cass_future_get_result(future.get()), resultDeleter() );

    CassIterator* rows = cass_iterator_from_result( result.get() );
    if( !cass_iterator_next( rows ) )
    {
        throw std::runtime_error( "Error in database table system.local!" );
    }

    const char *    byte_value = 0;
    size_t          value_len  = 0;
    const CassRow*  row        = cass_iterator_get_row( rows );
    //
    const CassValue* value  = cass_row_get_column( row, 0 );
    cass_value_get_string( value, &byte_value, &value_len );
    f_cluster_name = byte_value;
    //
    value  = cass_row_get_column( row, 1 );
    cass_value_get_string( value, &byte_value, &value_len );
    f_protocol_version = byte_value;
    //
    value  = cass_row_get_column( row, 2 );
    cass_value_get_string( value, &byte_value, &value_len );
    f_partitioner = byte_value;

    // I have no idea how to get this from the new CQL-based c++ interface.
    //
    f_snitch = "TODO!";

    return true;
}

/** \brief Disconnect from the Cassandra server.
 *
 * This function destroys the connection of the Cassandra server.
 *
 * After calling this function, any other function that require a
 * connection will fail.
 */
void QCassandraPrivate::disconnect()
{
    f_connection.reset();
    //
    if( f_session )
    {
        future_pointer_t result( cass_session_close( f_session.get() ), futureDeleter() );
        cass_future_wait( result.get() );
    }
    //
    f_session.reset();
    f_cluster.reset();

    f_current_context.reset();
    f_cluster_name     = "";
    f_protocol_version = "";
    f_partitioner      = "";
    f_snitch           = "";
}

/** \brief Check whether we're connected.
 *
 * This checks whether an f_client point exists.
 *
 * \note
 * To test whether the actual TCP/IP connection is up we'll want to
 * have some NOOP function (i.e. describe_cluster_name()).
 *
 * \return true if the server is connected
 */
bool QCassandraPrivate::isConnected() const
{
    return f_connection && f_session && f_cluster;
}

/** \brief Synchronize the version of schemas on all nodes.
 *
 * This function waits until the schema on all nodes that are currently up
 * agree on their version. Nodes that are down are ignored, although it can
 * be a problem to change a schema when a node is down...
 *
 * This function should be called any time a schema is changed and multiple
 * nodes are in use. This is why in most cases you do not want to create,
 * update, drop schemas (contexts or column families) on a live system. If
 * you do so, be sure to do it from your backend to avoid potential slow
 * downs of your front end.
 *
 * When you make multiple changes, you are free to do all the changes at
 * once and then call this function. It will generally be faster since the
 * first changes are likely through by the time you check the schema versions.
 *
 * The following functions affect the schema version:
 *
 * \li createContext()
 * \li createTable()
 * \li dropContext()
 * \li dropTable()
 * \li updateContext()
 * \li updateTable()
 *
 * You are required to make a call to the synchronization function:
 *
 * \li If you create or update a context/table and intend to use
 *     it immediately;
 * \li If you drop a context/table and then intend to re-created
 *     it immediately;
 *
 * You'll need two synchronizations if you first drop a context/table
 * then create it again. Also, to create a table in a context, you
 * want to synchronize the context first, then create the table (and
 * synchronize the table if you want to create rows.)
 *
 * Note that if you have many clients and any one of them may create, drop, or
 * update a context or a table, then all your clients need to call the
 * synchronization function to make sure all the nodes are in agreement.
 * This is a rather faster process, but it sure will slow down your service.
 * It would be better to have a backend tool used to create contexts and
 * tables, that backend tool can then call this synchronization function and
 * once it returns, you are ready to start using those new contexts and
 * tables. Of course, you could also make use of the Cassandra console
 * (cassandra-cli) to do that work.
 *
 * \exception std::runtime_error()
 * This exception is raised if the synchronization does not
 * happen in the specified amount of time
 *
 * \param[in] timeout  The number of seconds to wait for the synchronization to happen
 *
 * \todo I have no idea if this is even required with the newer CQL interface...
 */
void QCassandraPrivate::synchronizeSchemaVersions(int /*timeout*/)
{
#if 0
    uint64_t limit(QCassandra::timeofday()
                 + static_cast<uint64_t>(timeout) * 1000000ULL);

    for(;;) {
        // the versions is composed of a UUID (the version)
        // and an array of IP addresses in text representing the nodes
        //
        // IMPORTANT NOTE: the describe_schema_versions() may take a long time
        //                 before it returns!
        std::map<std::string, std::vector<std::string> > versions;
        f_client->describe_schema_versions(versions);
        if(versions.size() == 1) {
            // if all the nodes are in agreement we have one entry
            return;
        }
        std::map<std::string, std::vector<std::string> >::const_iterator it(versions.begin());
        std::string version;
        for(; it != versions.end(); ++it) {
            // unreachable nodes are ignored at this point; after all they are
            // down and would slow us down very much if we had to wait on them
            if(it->first != g_unreachable) {
                if(version.empty()) {
                    // we don't have our schema version
                    version = it->first;
                }
                else if(version != it->first) {
                    // versions are not all equal yet
                    break;
                }
            }
        }
        if(it == versions.end()) {
            // all versions are equal!
            // (this should not happen unless we have unreachable nodes)
            return;
        }
        if( static_cast<uint32_t>(QCassandra::timeofday()) > limit ) {
            throw std::runtime_error("schema versions synchronization did not happen in 'timeout' seconds");
        }
        // The Cassandra CLI has a tight loop instead!
        struct timespec pause;
        pause.tv_sec = 0;
        pause.tv_nsec = 10000000; // 10ms
        nanosleep(&pause, NULL);
    }
    /*NOTREACHED*/
#endif
}

/** \brief Check that we're connected.
 *
 * This function verifies that the cassandra server is connected (as in
 * the connect() function was called ans succeeded. If not connected,
 * then an exception is raised.
 *
 * \exception std::runtime_error
 * This function raises an exception if the QCassandraPrivate object
 * is not currently connected to a Cassandra server.
 */
void QCassandraPrivate::mustBeConnected() const throw(std::runtime_error)
{
    if(!isConnected())
    {
        throw std::runtime_error("not connected to the Cassandra server.");
    }
}

/** \brief Retrieve the name of the cluster.
 *
 * This function sends a message to the Cassandra server to
 * determine the name of the cluster.
 *
 * \return The name of the cluster.
 */
QString QCassandraPrivate::clusterName() const
{
    return f_cluster_name;
}

/** \brief Retrieve the version of the protocol.
 *
 * This function sends a message to the Cassandra server to
 * determine the version of the protocol.
 *
 * \return The version of the protocol.
 */
QString QCassandraPrivate::protocolVersion() const
{
    return f_protocol_version;
}


///** \brief Retrive the Ring information.
// *
// * This function calls the describe_ring() function and saves that
// * information ina cluster information class.
// *
// * \param[out] cluster_information  The object where the cluster information
// *             gathered by this call is saved.
// */
//void QCassandraPrivate::clusterInformation(QCassandraClusterInformation& cluster_information) const
//{
//    cluster_information.f_start_token.clear();
//    cluster_information.f_end_token.clear();
//    cluster_information.f_nodes.clear();
//
//    mustBeConnected();
//    std::vector<org::apache::cassandra::TokenRange> result;
//    -- this call fails without a "valid" context which defies the
//    -- concept of getting the number of nodes in a cluster in the
//    -- first place (i.e. so we can use that number to create the
//    -- new context)
//    f_client->describe_ring(result, "system");
//
//    -- I had not planned for an array of arrays... so I am not too
//    -- sure that I got the correct loops below. TBD if we ever make
//    -- this work
//    const size_t ranges(result.size());
//    for(size_t r(0); r < ranges; ++r)
//    {
//        const size_t size(result[r].endpoints.size());
//std::cerr << "***\n*** sizes are " << size
//                           << ", " << result[r].rpc_endpoints.size()
//                           << ", " << result[r].endpoint_details.size()
//                           << "\n***\n";
//        if(size != result[r].rpc_endpoints.size()
//        || size != result[r].endpoint_details.size())
//        {
//            throw std::runtime_error("endpoint vectors of described ring do not all have the same size");
//        }
//
//        cluster_information.f_start_token = QString::fromUtf8(result[r].start_token.c_str());
//        cluster_information.f_end_token = QString::fromUtf8(result[r].end_token.c_str());
//
//        for(size_t idx(0); idx < size; ++idx)
//        {
//            QCassandraNode node;
//            node.f_node_host = QString::fromUtf8(result[r].endpoint_details[idx].host.c_str());
//            node.f_node_rack = QString::fromUtf8(result[r].endpoint_details[idx].rack.c_str());
//            node.f_node_data_center = QString::fromUtf8(result[r].endpoint_details[idx].datacenter.c_str());
//            node.f_start_token = QString::fromUtf8(result[r].endpoints[idx].c_str());
//            node.f_end_token = QString::fromUtf8(result[r].endpoints[idx].c_str());
//            node.f_start_rpc_token = QString::fromUtf8(result[r].rpc_endpoints[idx].c_str());
//            node.f_end_rpc_token = QString::fromUtf8(result[r].rpc_endpoints[idx].c_str());
//        }
//    }
//}


/** \brief Retrieve the partitioner of the cluster.
 *
 * This function sends a message to the Cassandra server to
 * determine the partitioner defined for the cluster.
 *
 * The partitioner determines how data is distributed between
 * nodes in your Cassandra environment. Like with SQL data,
 * if bad paritioning of the data creates side effects when
 * handling the data. In case of Cassandra, a partitioner
 * that ends up sending all of its data to one or two nodes
 * will end up not making much use of all your other nodes.
 *
 * This information is defined in the cassandra.yaml
 * configuration file. It cannot be changed once you created
 * your cluster.
 *
 * \return The name of the partitioner.
 */
QString QCassandraPrivate::partitioner() const
{
    return f_partitioner;
}

/** \brief Retrieve the snitch of the cluster.
 *
 * This function sends a message to the Cassandra server to
 * determine the snitch defined for the cluster.
 *
 * \return The snitch used by the cluster.
 *
 * \note CQL does not support querying the snitch
 */
const QString& QCassandraPrivate::snitch() const
{
    return f_snitch;
}

/** \brief Set the context keyspace name.
 *
 * The Cassandra database system reacts to commands in a specific keyspace
 * which is managed like a context. This context must be specified before
 * other functions are called or those other functions will fail.
 *
 * \param[in] context_name  The name of the context to use as the current context.
 */
void QCassandraPrivate::setContext(const QString& context_name)
{
    f_contextName = context_name;
}

/** \brief Go through the list of contexts and build a list of such.
 *
 * This function creates a list of QCassandraContexts and returns
 * the result.
 *
 * The function retrieves all the keyspaces from Cassandra, transforms them
 * in QCassandraContext objects, and save the result in a
 * QCassandraContexts map indexed by name and returns that object.
 *
 * The QCassandra object is responsible for caching the result. The result
 * should not change until we create a new table although if another process
 * on another machine changes the Cassandra cluster structure, it will not
 * be seen until the cache is cleared.
 */
void QCassandraPrivate::contexts() const
{
    mustBeConnected();

    QStringList keyspaces;
    executeQuery( "SELECT keyspace_name FROM system.schema_keyspaces;", keyspaces );
    for( auto keyspace : keyspaces )
    {
        f_parent->context( keyspace );
    }
}


/** \brief Retrieve the description of all tables.
 *
 * \param[in]  context_name  The name of the context in which the tables are located.
 * \param[out] cf_def_list   Definition of all of the tables
 */
void QCassandraPrivate::retrieve_tables( const QString& context_name, std::vector<org::apache::cassandra::CfDef>& cf_def_list ) const
{
    using ::org::apache::cassandra;

#if 0
    string name;
    std::string column_type;
    std::string comparator_type;
    std::string subcomparator_type;
    std::string comment;
    double read_repair_chance;
    std::vector<ColumnDef> column_metadata;
    int32_t gc_grace_seconds;
    std::string default_validation_class;
    int32_t id;
    int32_t min_compaction_threshold;
    int32_t max_compaction_threshold;
    bool replicate_on_write;
    std::string key_validation_class;
    std::string key_alias;
    std::string compaction_strategy;
    std::map<std::string, std::string> compaction_strategy_options;
    std::map<std::string, std::string> compression_options;
    double bloom_filter_fp_chance;
    std::string caching;
    double dclocal_read_repair_chance;
    bool populate_io_cache_on_flush;
    int32_t memtable_flush_period_in_ms;
    int32_t default_time_to_live;
    int32_t index_interval;
    std::string speculative_retry;
    std::vector<TriggerDef> triggers;
    double row_cache_size;
    double key_cache_size;
    int32_t row_cache_save_period_in_seconds;
    int32_t key_cache_save_period_in_seconds;
    int32_t memtable_flush_after_mins;
    int32_t memtable_throughput_in_mb;
    double memtable_operations_in_millions;
    double merge_shards_chance;
    std::string row_cache_provider;
    int32_t row_cache_keys_to_save;
#endif

    const QString query( QString("SELECT columnfamily_name, type, comparator, subcomparator, "
                                 "comment, read_repair_chance, gc_grace_seconds, default_validator, "
                                 "cf_id, min_compaction_threshold, max_compaction_threshold, "
                                 "key_validator, key_aliases, compaction_strategy_class, "
                                 "compaction_strategy_options, bloom_filter_fp_chance, caching, "
                                 "read_repair_chance, memtable_flush_period_in_ms, default_time_to_live, "
                                 "max_index_interval, speculative_retry "
                                 "FROM system.schema_columnfamilies "
                                 "WHERE keyspace_name = '%1'")
                         .arg(f_contextName)
                         );
    //
    statement_pointer_t query_stmt( cass_statement_new( query.toUtf8().data(), 0 ), statementDeleter() );
    future_pointer_t session( cass_session_execute( f_session.get(), query_stmt.get() ) , futureDeleter()    );
    throwIfError( session, "Cannot select from system.schema_columnfamilies!" );

    result_pointer_t query_result( cass_future_get_result(session.get()), resultDeleter() );
    iterator_pointer_t rows( cass_iterator_from_result(query_result.get()), iteratorDeleter()   );
    while( cass_iterator_next(rows.get()) )
    {
        const CassRow* row( cass_iterator_get_row(rows.get()));
        const bool     durable_writes   ( getBoolFromRow   ( row, "durable_writes"   ) );
        const QString  strategy_class   ( getStringFromRow ( row, "strategy_class"   ) );
        const QString  strategy_options ( getStringFromRow ( row, "strategy_options" ) );

        ks_def.__set_name ( f_contextName.toUtf8().data() );
        ks_def.__set_strategy_class ( strategy_class.toUtf8().data() );

        as2js::JSON::pointer_t load_json( std::make_shared<as2js::JSON>() );
        as2js::StringInput::pointer_t in( std::make_shared<as2js::StringInput>(strategy_options.toUtf8().data()) );
        as2js::JSON::JSONValue::pointer_t opts( load_json->parse(in) );

        auto options( opts->get_object() );
        std::map<std::string,std::string> the_map;
        for( const auto& elm : options )
        {
            the_map[*elm.first] = *elm.second;
        }
        ks_def.__set_strategy_options( the_map );

        auto iter = options.find( "replication_factor" );
        if( iter != options.end() )
        {
            ks_def.__set_replication_factor( static_cast<int32_t>( iter->first->get_int64() ) );
        }

        retrieve_tables( context_name, ks_def.cf_defs );

        ks_def.__set_durable_writes( durable_writes );
    }

}


/** \brief Retrieve the description of a keyspace.
 *
 * This function requests for the descriptions of a specific keyspace
 * (context). It is used to rebuild the list of tables after a clearCache()
 * call on a context object.
 *
 * The QCassandra object is responsible for caching the result. The result
 * should not change until we create a new table although if another process
 * on another machine changes the Cassandra cluster structure, it will not
 * be seen until the cache gets cleared.
 *
 * \param[in] context_name  The name of the context to re-describe.
 */
void QCassandraPrivate::retrieve_context(const QString& context_name) const
{
    using ::org::apache::cassandra;

    mustBeConnected();

    // retrieve this keyspace from Cassandra
    KsDef ks_def;
    //f_client->describe_keyspace(ks_def, context_name.toUtf8().data());

    const QString query( QString("SELECT durable_writes, strategy_class, strategy_options "
                                 "FROM system.schema_keyspaces "
                                 "WHERE keyspace_name = '%1'")
                         .arg(f_contextName)
                         );
    //
    statement_pointer_t query_stmt( cass_statement_new( query.toUtf8().data(), 0 ), statementDeleter() );
    future_pointer_t session( cass_session_execute( f_session.get(), query_stmt.get() ) , futureDeleter()    );
    throwIfError( session, "Cannot select from system.schema_keyspaces!" );

    result_pointer_t query_result( cass_future_get_result(session.get()), resultDeleter() );
    iterator_pointer_t rows( cass_iterator_from_result(query_result.get()), iteratorDeleter()   );
    while( cass_iterator_next(rows.get()) )
    {
        const CassRow* row( cass_iterator_get_row(rows.get()));
        const bool     durable_writes   ( getBoolFromRow   ( row, "durable_writes"   ) );
        const QString  strategy_class   ( getStringFromRow ( row, "strategy_class"   ) );
        const QString  strategy_options ( getStringFromRow ( row, "strategy_options" ) );

        ks_def.__set_name ( f_contextName.toUtf8().data() );
        ks_def.__set_strategy_class ( strategy_class.toUtf8().data() );

        as2js::JSON::pointer_t load_json( std::make_shared<as2js::JSON>() );
        as2js::StringInput::pointer_t in( std::make_shared<as2js::StringInput>(strategy_options.toUtf8().data()) );
        as2js::JSON::JSONValue::pointer_t opts( load_json->parse(in) );

        auto options( opts->get_object() );
        std::map<std::string,std::string> the_map;
        for( const auto& elm : options )
        {
            the_map[*elm.first] = *elm.second;
        }
        ks_def.__set_strategy_options( the_map );

        auto iter = options.find( "replication_factor" );
        if( iter != options.end() )
        {
            ks_def.__set_replication_factor( static_cast<int32_t>( iter->first->get_int64() ) );
        }

        retrieve_tables( context_name, ks_def.cf_defs );

        ks_def.__set_durable_writes( durable_writes );
    }

    QCassandraContext::pointer_t c(f_parent->context(context_name));
    c->parseContextDefinition( &ks_def );
}


/** \brief Create a new context.
 *
 * This function creates a new context. Trying to create a context with
 * the name of an existing context will fail. Use the update function
 * instead.
 *
 * The new context identifier is not returned. I'm not too sure what
 * you could do with it anyway since no function make use of it.
 *
 * At this time, it looks like you cannot include an array of tables
 * in the context or the create function fails.
 *
 * \param[in] context  The context definition used to create the new context.
 *
 * \sa updateContext()
 */
void QCassandraPrivate::createContext(const QCassandraContext& context)
{
    mustBeConnected();
    org::apache::cassandra::KsDef ks;
    context.prepareContextDefinition(&ks);
    std::string id_ignore;
    f_client->system_add_keyspace(id_ignore, ks);
}

/** \brief Update an existing context.
 *
 * This function updates an existing context. Some options may not be
 * updateable.
 *
 * The context should be one that you loaded from the cluster to be sure
 * that you start with the right data.
 *
 * \param[in] context  The context to update in the attached Cassandra cluster.
 *
 * \sa createContext()
 */
void QCassandraPrivate::updateContext(const QCassandraContext& context)
{
    mustBeConnected();
    org::apache::cassandra::KsDef ks;
    context.prepareContextDefinition(&ks);
    std::string id_ignore;
    f_client->system_update_keyspace(id_ignore, ks);
}

/** \brief Drop an existing context.
 *
 * This function drops an existing context. After this call, do not try to
 * access the context again until you re-create it.
 *
 * \param[in] context  The context to drop from the attached Cassandra cluster.
 *
 * \sa createContext()
 */
void QCassandraPrivate::dropContext(const QCassandraContext& context)
{
    mustBeConnected();
    f_parent->clearCurrentContextIf(context);
    std::string id_ignore;
    f_client->system_drop_keyspace(id_ignore, context.contextName().toUtf8().data());
}

/** \brief Create a table in the Cassandra server.
 *
 * This function creates a table in the cassandra server transforming a
 * QCassandraTable in a table definition (CfDef) and then calling the
 * system_add_column_family() function.
 *
 * \param[in] table  The table to be created.
 */
void QCassandraPrivate::createTable(const QCassandraTable *table)
{
    mustBeConnected();
    org::apache::cassandra::CfDef cf;
    table->prepareTableDefinition(&cf);
    std::string id_ignore;
    f_client->system_add_column_family(id_ignore, cf);
}

/** \brief Update a table in the Cassandra server.
 *
 * This function updates a table int the cassandra server.
 *
 * \param[in] table  The table to be updated.
 */
void QCassandraPrivate::updateTable(const QCassandraTable *table)
{
    mustBeConnected();
    org::apache::cassandra::CfDef cf;
    table->prepareTableDefinition(&cf);
    std::string id_ignore;
    f_client->system_update_column_family(id_ignore, cf);
}

/** \brief Drop a table from the Cassandra server.
 *
 * This function drops the named table from the cassandra server.
 *
 * \param[in] table_name  The name of the table to be dropped.
 */
void QCassandraPrivate::dropTable(const QString& table_name)
{
    mustBeConnected();
    std::string id_ignore;
    f_client->system_drop_column_family(id_ignore, table_name.toUtf8().data());
}

/** \brief Truncate a table in the Cassandra server.
 *
 * This function truncates (i.e. removes all the rows and their data) a table
 * from the cassandra server.
 *
 * \param[in] table  The table to be created.
 */
void QCassandraPrivate::truncateTable(const QCassandraTable *table)
{
    mustBeConnected();
    f_client->truncate(table->tableName().toUtf8().data());
}

/** \brief Insert a value in the Cassandra database.
 *
 * This function insert the specified \p value in the Cassandra database.
 *
 * It is saved in the current context, \p table_name, \p row_key, and
 * \p column_key.
 *
 * \param[in] table_name  Name of the table where the value is inserted.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[in] value  The new value of the cell.
 */
void QCassandraPrivate::insertValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value)
{
    mustBeConnected();

    std::string rkey(row_key.data(), row_key.size());

    org::apache::cassandra::ColumnParent column_parent;
    column_parent.__set_column_family(table_name.toUtf8().data());
    // no super column support here

    org::apache::cassandra::Column column;
    std::string ckey(column_key.data(), column_key.size());
    column.__set_name(ckey);

    const QByteArray& data(value.binaryValue());
    column.__set_value(std::string(data.data(), data.size())); // "unavoidable" copy of the data

    const QCassandraValue::timestamp_mode_t mode = value.timestampMode();
    // some older version of gcc require a cast here
    switch(static_cast<QCassandraValue::def_timestamp_mode_t>(mode)) {
    case QCassandraValue::TIMESTAMP_MODE_AUTO:
        // libQtCassandra default
        column.__set_timestamp(QCassandra::timeofday());
        break;

    case QCassandraValue::TIMESTAMP_MODE_DEFINED:
        // user defined
        column.__set_timestamp(value.timestamp());
        break;

    case QCassandraValue::TIMESTAMP_MODE_CASSANDRA:
        // let Cassandra use its own default
        break;

    }

    if(value.ttl() != QCassandraValue::TTL_PERMANENT) {
        column.__set_ttl(value.ttl());
    }

    // our consistency level is 100% based on the Thrift consistency level
    // a cast is enough to get the value we want to get
    // (see the QCassandraValue.cpp file)

    consistency_level_t consistency_level = value.consistencyLevel();
    if(consistency_level == CONSISTENCY_LEVEL_DEFAULT) {
        consistency_level = f_parent->defaultConsistencyLevel();
    }

    for(int retry(0);; ++retry)
    {
        try
        {
            f_client->insert(rkey, column_parent, column, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
            return;
        }
        catch(::apache::thrift::TException& e)
        {
            if(retry >= 5)
            {
//std::cerr << "insertValue() failed.\n";
                throw;
            }
            struct timeval to;
            to.tv_sec = 0;
            to.tv_usec = 100000; // 100ms
            select(0, nullptr, nullptr, nullptr, &to);
        }
    }
}

/** \brief Get a value from the Cassandra database.
 *
 * This function retrieves a \p value from the Cassandra database.
 *
 * It is retrieved from the current context, \p table_name, \p row_key, and
 * \p column_key.
 *
 * \param[in] table_name  Name of the table where the value is inserted.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[out] value  The new value of the cell.
 */
void QCassandraPrivate::getValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value)
{
    mustBeConnected();

    std::string rkey(row_key.data(), row_key.size());

    org::apache::cassandra::ColumnPath column_path;
    column_path.__set_column_family(table_name.toUtf8().data());
    std::string ckey(column_key.data(), column_key.size());
    column_path.__set_column(ckey);
    // no super column support here

    org::apache::cassandra::ColumnOrSuperColumn column_result;

    // our consistency level is 100% based on the Thrift consistency level
    // a cast is enough to get the value we want to get
    // (see the QCassandraValue.cpp file)

    consistency_level_t consistency_level = value.consistencyLevel();
    if(consistency_level == CONSISTENCY_LEVEL_DEFAULT) {
        consistency_level = f_parent->defaultConsistencyLevel();
    }

    // We cannot have a try/catch at this level, instead it's done at
    // higher levels as required (doing it here would prevent many
    // features from working without having to transmit a shit load
    // of information from this level)
    for(int retry(0);; ++retry)
    {
        try {
          f_client->get(column_result, rkey, column_path, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
          break;
        }
        catch(org::apache::cassandra::NotFoundException& /*e*/) {
//std::cerr << "getValue() not found exception.\n";
            throw;
        }
        catch(::apache::thrift::TException const& e)
        {
            if(retry >= 5)
            {
//std::cerr << "getValue() failed.\n";
                throw;
            }
            struct timeval to;
            to.tv_sec = 0;
            to.tv_usec = 100000; // 100ms
            select(0, nullptr, nullptr, nullptr, &to);
        }
    }

    if(!column_result.__isset.column) {
        throw std::runtime_error("attempt to retrieve a cell failed");
    }

    // we got a column, copy the data to the value parameter
    if(column_result.column.__isset.value) {
        value.setBinaryValue(QByteArray(column_result.column.value.c_str(), column_result.column.value.length()));
    }
    else {
        // undefined we assume empty...
        value.setNullValue();
    }
    if(column_result.column.__isset.timestamp) {
        value.assignTimestamp(column_result.column.timestamp);
    }
    if(column_result.column.__isset.ttl) {
        value.setTtl(column_result.column.ttl);
    }
}

/** \brief Get the value of a counter from the Cassandra database.
 *
 * This function retrieves a counter \p value from the Cassandra database.
 *
 * It is retrieved from the current context, \p table_name, \p row_key, and
 * \p column_key.
 *
 * \param[in] table_name  Name of the table where the value is inserted.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[out] value  The new value of the cell.
 */
void QCassandraPrivate::getCounter(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value)
{
    mustBeConnected();

    std::string rkey(row_key.data(), row_key.size());

    org::apache::cassandra::ColumnPath column_path;
    column_path.__set_column_family(table_name.toUtf8().data());
    std::string ckey(column_key.data(), column_key.size());
    column_path.__set_column(ckey);
    // no super column support here

    org::apache::cassandra::ColumnOrSuperColumn column_result;

    // our consistency level is 100% based on the Thrift consistency level
    // a cast is enough to get the value we want to get
    // (see the QCassandraValue.cpp file)

    consistency_level_t consistency_level = value.consistencyLevel();
    if(consistency_level == CONSISTENCY_LEVEL_DEFAULT) {
        consistency_level = f_parent->defaultConsistencyLevel();
    }

    // We cannot have a try/catch at this level, instead it's done at
    // higher levels as required (doing it here would prevent many
    // features from working without having to transmit a shit load
    // of information from this level)
    //try {
        f_client->get(column_result, rkey, column_path, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
    //}
    //catch(org::apache::cassandra::NotFoundException& /*e*/) {
    //    // this happens when a column is missing and thus we cannot
    //    // read it; in this case we return NULL in value.
    //    value.setNullValue();
    //    return;
    //}

    if(!column_result.__isset.counter_column) {
        throw std::runtime_error("attempt to retrieve a cell failed");
    }

    // we got a column, copy the data to the value parameter
    value.setInt64Value(column_result.counter_column.value);

    // no TTL or timestamp for counters
}

/** \brief Add value to a Cassandra counter.
 *
 * This function adds \p value to the specified Cassandra counter.
 *
 * The counter is defined in the current context with the \p table_name,
 * \p row_key, and \p column_key.
 *
 * \param[in] table_name  Name of the table where the value is inserted.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[in] value  The value to add to the counter.
 */
void QCassandraPrivate::addValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t value)
{
    mustBeConnected();

    std::string rkey(row_key.data(), row_key.size());

    org::apache::cassandra::ColumnParent column_parent;
    column_parent.__set_column_family(table_name.toUtf8().data());
    // no super column support here

    org::apache::cassandra::CounterColumn counter_column;
    std::string ckey(column_key.data(), column_key.size());
    counter_column.__set_name(ckey);
    counter_column.__set_value(value);

    // our consistency level is 100% based on the Thrift consistency level
    // a cast is enough to get the value we want to get
    // (see the QCassandraValue.cpp file)

    // IMPORTANT NOTE: In version 0.8.x the consistency level was limited to
    //                 one (CONSITENCY_LEVEL_ONE) which is expected to be
    //                 the value used here for safety; we still use the
    //                 default consistency defined in the context
    consistency_level_t consistency_level = f_parent->defaultConsistencyLevel();
 
    f_client->add(rkey, column_parent, counter_column, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
}

/** \brief Get a slice of columns from the Cassandra database.
 *
 * This function retrieves the number of columns as defined by a \p slice
 * of the Cassandra database. A slice is an array of column from a specific
 * row.
 *
 * Remember that this number represents the number of cells in a specific row.
 * Each row may have a different number of cells.
 *
 * \param[in] table_name  Name of the table where the cell counting is processed.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_predicate  The predicate defining the name of the columns to count.
 *
 * \return The number of cells (Columns) found using the specified predicate.
 */
int32_t QCassandraPrivate::getCellCount(const QString& table_name, const QByteArray& row_key, const QCassandraColumnPredicate& column_predicate)
{
    mustBeConnected();

    std::string rkey(row_key.data(), row_key.size());

    org::apache::cassandra::ColumnParent column_parent;
    column_parent.__set_column_family(table_name.toUtf8().data());
    // no super column support here

    org::apache::cassandra::SlicePredicate slice_predicate;
    column_predicate.toPredicate(&slice_predicate);
    slice_predicate.slice_range.__set_count(0x7FFFFFFF);

    // our consistency level is 100% based on the Thrift consistency level
    // a cast is enough to get the value we want to get
    // (see the QCassandraValue.cpp file)

    consistency_level_t consistency_level = column_predicate.consistencyLevel();
    if(consistency_level == CONSISTENCY_LEVEL_DEFAULT) {
        consistency_level = f_parent->defaultConsistencyLevel();
    }

    return f_client->get_count(rkey, column_parent, slice_predicate, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
}

/** \brief Get a slice of columns from the Cassandra database.
 *
 * This function retrieves a \p slice from the Cassandra database. A slice is an
 * array of column from a specific row.
 *
 * The result is passed to a QCassandraTable which is expected to save it
 * as a set of rows, cells, and values.
 *
 * The function returns the number of cells read from Cassandra. If the predicate
 * is used as an index, the returned number may be the number of cells read minus
 * one (extra one that is not returned an thus not counted.)
 *
 * \param[in,out] table  The table where values found get inserted.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_predicate  The predicate defining the name of the columns to return.
 *
 * \return The number of columns read from Cassandra.
 */
uint32_t QCassandraPrivate::getColumnSlice(QCassandraTable& table, const QByteArray& row_key, QCassandraColumnPredicate& column_predicate)
{
    mustBeConnected();

    std::string key(row_key.data(), row_key.size());

    org::apache::cassandra::ColumnParent column_parent;
    column_parent.__set_column_family(table.tableName().toUtf8().data());
    // no super column support here

    org::apache::cassandra::SlicePredicate slice_predicate;
    column_predicate.toPredicate(&slice_predicate);

    typedef std::vector<org::apache::cassandra::ColumnOrSuperColumn> column_vector_t;
    column_vector_t results;

    // our consistency level is 100% based on the Thrift consistency level
    // a cast is enough to get the value we want to get
    // (see the QCassandraValue.cpp file)

    consistency_level_t consistency_level = column_predicate.consistencyLevel();
    if(consistency_level == CONSISTENCY_LEVEL_DEFAULT) {
        consistency_level = f_parent->defaultConsistencyLevel();
    }

    f_client->get_slice(results, key, column_parent, slice_predicate, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));

    // we got results, copy the data to the table cache
    QCassandraColumnRangePredicate *range(dynamic_cast<QCassandraColumnRangePredicate *>(&column_predicate));
    bool has_range(range != NULL && range->index());
    int adjust(0);
    for(column_vector_t::iterator it = results.begin(); it != results.end(); ++it) {
        if(has_range && it == results.begin() && range->excludeFirst()) {
            adjust = -1;
            continue;
        }
        // transform the value of the cell to a QCassandraValue
        QCassandraValue value;
        if(it->column.__isset.value) {
            value.setBinaryValue(QByteArray(it->column.value.c_str(), it->column.value.length()));
        }
        if(it->column.__isset.timestamp) {
            value.assignTimestamp(it->column.timestamp);
        }
        if(it->column.__isset.ttl) {
            value.setTtl(it->column.ttl);
        }

        // save the cell in the corresponding table, row, cell
        QByteArray cell_key(it->column.name.c_str(), it->column.name.size());
        table.assignRow(row_key, cell_key, value);

        if(has_range && it + 1 == results.end()) {
            range->setLastKey(cell_key);
        }
    }

    return results.size() + adjust;
}

/** \brief Remove a cell from the Cassandra database.
 *
 * This function calls the Cassandra server to remove a cell in the Cassandra
 * database.
 *
 * \param[in] table_name  The name of the column where the row is defined.
 * \param[in] row_key  The row in which the cell is to be removed.
 * \param[in] column_key  The cell to be removed, may be empty to remove all the cells.
 * \param[in] timestamp  The time when the key to be removed was created.
 * \param[in] consistency_level  The consistency level to use to remove this cell.
 */
void QCassandraPrivate::remove(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t timestamp, consistency_level_t consistency_level)
{
    mustBeConnected();

    std::string rkey(row_key.data(), row_key.size());

    org::apache::cassandra::ColumnPath column_path;
    column_path.__set_column_family(table_name.toUtf8().data());
    // no super column support here
    if(column_key.size() > 0) {
        // when column_key is empty we want to remove all the columns!
        std::string ckey(column_key.data(), column_key.size());
        column_path.__set_column(ckey);
    }

    if(consistency_level == CONSISTENCY_LEVEL_DEFAULT) {
        consistency_level = f_parent->defaultConsistencyLevel();
    }

    f_client->remove(rkey, column_path, timestamp, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
}

/** \brief This function retrieves a set of rows.
 *
 * This function retrieves a set of rows filtered by the specified predicate
 * and return the result in the corresponding table.
 *
 * \param[in,out] table  The table requesting the row slices.
 * \param[in] row_predicate  The predicate to select rows and columns.
 *
 * \return The number of rows read from the Cassandra server and added to
 *         the table.
 */
uint32_t QCassandraPrivate::getRowSlices(QCassandraTable& table, QCassandraRowPredicate& row_predicate)
{
    mustBeConnected();

    org::apache::cassandra::ColumnParent column_parent;
    column_parent.__set_column_family(table.tableName().toUtf8().data());
    // no super column support here

    QCassandraColumnPredicate::pointer_t column_predicate(row_predicate.columnPredicate());
    org::apache::cassandra::SlicePredicate slice_predicate;
    column_predicate->toPredicate(&slice_predicate);

    typedef std::vector<org::apache::cassandra::KeySlice> key_slice_vector_t;
    key_slice_vector_t results;

    // our consistency level is 100% based on the Thrift consistency level
    // a cast is enough to get the value we want to get
    // (see the QCassandraValue.cpp file)

    consistency_level_t consistency_level = column_predicate->consistencyLevel();
    if(consistency_level == CONSISTENCY_LEVEL_DEFAULT) {
        consistency_level = f_parent->defaultConsistencyLevel();
    }

    // get a copy of the row predicate name match regular expression
    auto re(row_predicate.rowNameMatch());

    uint32_t size(0);
    int32_t max(row_predicate.count());
    while(max > 0)
    {
        QCassandraRowPredicate current_row_predicate(row_predicate);
        if(re.isEmpty())
        {
            current_row_predicate.setCount(max);
        }
        else
        {
            // the regular expression may remove many (most, all) of the
            // entries so if max is very small, we will be going at a real
            // snail pace here... so instead use 100 as the minimum number
            // of rows to fetch
            current_row_predicate.setCount(std::min(max, 100));
        }
        org::apache::cassandra::KeyRange key_range;
        current_row_predicate.toPredicate(&key_range);

        try {
//std::cerr << "start/end ["<< key_range.start_key<< "]/[" << key_range.end_key<< "] OR ["<< key_range.start_token<<"]/[" << key_range.end_token<< "]\n";
            f_client->get_range_slices(results, column_parent, slice_predicate, key_range, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
        }
        // Wondering whether the invalid request exception is what 0.8.0 was
        // returning instead of the transport exception.
        //catch(const org::apache::cassandra::InvalidRequestException& /*e*/) {
        //    // this happens when the predicate is not liked by Cassandra
        //    // (it's probably some form of a bug though)
        //    return size;
        //}
        catch(apache::thrift::transport::TTransportException const & e) {
            if(e.what() == std::string("No more data to read.")) {
                return size;
            }
            throw;
        }
        // Too many nodes are down to continue processing.
        // This can also happen if the initialization of a context includes a
        // data center name which is invalid (because if Cassandra cannot find
        // that data center it will not be happy about it!)
        // It can also happen if the replication factor is larger than the
        // number of nodes available.
        //catch(const org::apache::cassandra::UnavailableException& e)
        //{
        //    throw;
        //}

        // if no results, exit the loop immediately
        // (in case of a long list that uses excludeFirst(), we may end up
        // with 1 entry which is going to be ignored and thus that's an
        // equivalent to emptiness)
        if(results.empty()
        || (row_predicate.excludeFirst() && results.size() == 1))
        {
            break;
        }

        // we got results, copy the data to the table cache
        QByteArray last_key;
        for(key_slice_vector_t::iterator it = results.begin(); it != results.end(); ++it) {
            if(it == results.begin() && row_predicate.excludeFirst()) {
                continue;
            }
            QByteArray row_key(it->key.c_str(), it->key.size());
            last_key = row_key;
            if(!re.isEmpty()) {
                QString row_name(QString::fromUtf8(row_key.data()));
                if(re.indexIn(row_name) == -1) {
                    continue;
                }
            }
            typedef std::vector<org::apache::cassandra::ColumnOrSuperColumn> column_vector_t;
            for(column_vector_t::iterator jt = it->columns.begin(); jt != it->columns.end(); ++jt) {
                // transform the value of the cell to a QCassandraValue
                QCassandraValue value;
                if(jt->column.__isset.value) {
                    value.setBinaryValue(QByteArray(jt->column.value.c_str(), jt->column.value.length()));
                }
                if(jt->column.__isset.timestamp) {
                    value.assignTimestamp(jt->column.timestamp);
                }
                if(jt->column.__isset.ttl) {
                    value.setTtl(jt->column.ttl);
                }

                // save the cell in the corresponding table, row, cell
                QByteArray cell_key(jt->column.name.c_str(), jt->column.name.size());
                table.assignRow(row_key, cell_key, value);
            }
            ++size;
            --max;
        }

        if(!last_key.isNull()) {
            row_predicate.setLastKey(last_key);
        }
    }

    return size;
}

} // namespace QtCassandra
// vim: ts=4 sw=4 et
