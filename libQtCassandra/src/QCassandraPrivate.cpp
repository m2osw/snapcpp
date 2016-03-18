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

/** \var QCassandraPrivate::f_session
 * \brief Define a CQL client object.
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


QCassandraSession::pointer_t QCassandraPrivate::getCassandraSession() const
{
    return f_session;
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

    f_session = std::make_shared<QCassandraSession>();
    f_session->connect( host_list, port ); // throws on failure!

    QCassandraQuery local_table( f_session );
    local_table.query( "SELECT cluster_name, native_protocol_version, partitioner FROM system.local" );
    local_table.start();
    //
    if( !local_table.nextRow() )
    {
        throw std::runtime_error( "Error in database table system.local!" );
    }

    f_cluster_name     = local_table.getStringColumn( "cluster_name"            );
    f_protocol_version = local_table.getStringColumn( "native_protocol_version" );
    f_partitioner      = local_table.getStringColumn( "partitioner"             );
    //
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
    if( f_session )
    {
        f_session->disconnect();
    }
    f_session.reset();
}


/** \brief Check whether we're connected.
 *
 * This checks whether an f_session point exists.
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
const QString& QCassandraPrivate::clusterName() const
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
const QString& QCassandraPrivate::protocolVersion() const
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
const QString& QCassandraPrivate::partitioner() const
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

    f_parent->clearCache();

    QCassandraQuery keyspace_query( f_session );
    keyspace_query.query( "SELECT keyspace_name FROM system.schema_keyspaces;" );
    keyspace_query.start();
    while( keyspace_query.nextRow() )
    {
        f_parent->context( keyspace_query.getStringColumn("keyspace_name") );
    }
}


/** \brief Retrieve the description of all columns for each table
 *
 * \param[in,out]  cf_def  The "columnfamily" (i.e. table) information structure that we will populate from the query.
 */
void QCassandraPrivate::retrieve_columns( org::apache::cassandra::CfDef& cf_def ) const
{
    using ::org::apache::cassandra;

    const QString query( QString("SELECT column_name, index_name, "
                                 "index_options, index_type, type, validator "
                                 "FROM system.schema_columns "
                                 "WHERE keyspace_name = '%1' "
                                 "AND columnfamily_name = '%2'")
                         .arg(cf_def.keyspace)
                         .arg(cf_def.name)
                         );

    QCassandraQuery the_query( f_session );
    the_query.query( query );
    the_query.start();

    std::vector<ColumnDef> col_def_list;
    while( the_query.nextRow() )
    {
        ColumnDef col_def;
        col_def.__set_name             ( the_query.getStringColumn  ("column_name").toStdString() );
        col_def.__set_index_name       ( the_query.getStringColumn  ("index_name").toStdString()  );
        col_def.__set_validation_class ( the_query.getStringColumn  ("validator").toStdString()   );
        col_def.__set_index_options    ( the_query.getJsonMapColumn ("index_options")             );

        const QString index_type( the_query.getStringColumn( "index_type" ).toLower() );
        if( index_type == "keys" )
        {
            col_def.__set_index_type( IndexType::KEYS );
        }
        else if( index_type == "custom" )
        {
            col_def.__set_index_type( IndexType::CUSTOM );
        }
        else if( index_type == "composites" )
        {
            col_def.__set_index_type( IndexType::COMPOSITES );
        }

        col_def_list.push_back( cf_def );
    }

    cf_def.__set_columns( col_def_list );
}


/** \brief Retrieve the description of all triggers for each table
 *
 * \param[in,out]  cf_def  The "columnfamily" (i.e. table) information structure that we will populate from the query.
 */
void QCassandraPrivate::retrieve_triggers( org::apache::cassandra::CfDef& cf_def ) const
{
    using ::org::apache::cassandra;

    const QString query( QString("SELECT trigger_name, trigger_options "
                                 "FROM system.schema_triggers "
                                 "WHERE keyspace_name = '%1' "
                                 "AND columnfamily_name = '%2'")
                         .arg(cf_def.keyspace)
                         .arg(cf_def.name)
                         );

    QCassandraQuery the_query( f_session );
    the_query.query( query );
    the_query.start();

    std::vector<TriggerDef> trig_def_list;
    while( the_query.nextRow() )
    {
        TriggerDef trig_def;
        trig_def.__set_name    ( the_query.getStringColumn ("trigger_name").toStdString() );
        trig_def.__set_options ( the_query.getMapColumn    ("trigger_options")            );
        trig_def_list.push_back( cf_def );
    }

    cf_def.__set_triggers( trig_def_list );
}


/** \brief Retrieve the description of all tables.
 *
 * \param[in,out]  ks_def  The keyspace information structure that we will populate from the query.
 */
void QCassandraPrivate::retrieve_tables( org::apache::cassandra::KsDef& ks_def ) const
{
    using ::org::apache::cassandra;

    const QString query( QString("SELECT columnfamily_name, type, comparator, subcomparator, "
                                 "comment, read_repair_chance, gc_grace_seconds, default_validator, "
                                 "cf_id, min_compaction_threshold, max_compaction_threshold, "
                                 "key_validator, key_aliases, compaction_strategy_class, "
                                 "compaction_strategy_options, compression_parameters, bloom_filter_fp_chance, caching, "
                                 "memtable_flush_period_in_ms, default_time_to_live, "
                                 "speculative_retry "
                                 "FROM system.schema_columnfamilies "
                                 "WHERE keyspace_name = '%1'")
                         .arg(ks_def.name)
                         );

    QCassandraQuery the_query( f_session );
    the_query.query( query );
    the_query.start();

    std::vector<CfDef> cf_def_list;
    while( the_query.nextRow() )
    {
        CfDef cf_def;
        cf_def.__set_keyspace                    ( ks_def.name                );
        cf_def.__set_name                        ( the_query.getStringColumn  ("columnfamily_name")                    .toStdString() );
        cf_def.__set_column_type                 ( the_query.getStringColumn  ("type")                                 .toStdString() );
        cf_def.__set_comparator_type             ( the_query.getStringColumn  ("comparator")                           .toStdString() );
        cf_def.__set_subcomparator_type          ( the_query.getStringColumn  ("subcomparator")                        .toStdString() );
        cf_def.__set_comment                     ( the_query.getStringColumn  ("comment")                              .toStdString() );
        cf_def.__set_read_repair_chance          ( the_query.getDoubleColumn  ("read_repair_chance")                   );
        cf_def.__set_gc_grace_seconds            ( the_query.getIntColumn     ("gc_grace_seconds")                     );
        cf_def.__set_default_validation_class    ( the_query.getStringColumn  ("default_validator")                    .toStdString() );
        cf_def.__set_id                          ( the_query.getIntColumn     ("cf_id")                                );
        cf_def.__set_min_compaction_threshold    ( the_query.getIntColumn     ("min_compaction_threshold")             );
        cf_def.__set_max_compaction_threshold    ( the_query.getIntColumn     ("max_compaction_threshold")             );
        cf_def.__set_key_validation_class        ( the_query.getStringColumn  ("key_validator")                        .toStdString() );
        cf_def.__set_key_alias                   ( the_query.getStringColumn  ("key_aliases")                          .toStdString() );
        cf_def.__set_compaction_strategy         ( the_query.getStringColumn  ("compaction_strategy_class")            .toStdString() );
        cf_def.__set_compaction_strategy_options ( the_query.getJsonMapColumn ("compaction_strategy_options")          );
        cf_def.__set_compression_options         ( the_query.getJsonMapColumn ("compression_parameters")               );
        cf_def.__set_bloom_filter_fp_chance      ( the_query.getDoubleColumn  ("bloom_filter_fp_chance")               );
        cf_def.__set_caching                     ( the_query.getStringColumn  ("bloom_filter_fp_chance").toStdString() );
        cf_def.__set_caching                     ( the_query.getStringColumn  ("bloom_filter_fp_chance").toStdString() );
        cf_def.__set_memtable_flush_period_in_ms ( the_query.getIntColumn     ("memtable_flush_period_in_ms")          );
        cf_def.__set_default_time_to_live        ( the_query.getIntColumn     ("default_time_to_live")                 );
        cf_def.__set_speculative_retry           ( the_query.getStringColumn  ("speculative_retry")                    .toStdString() );
        //
        retrieve_columns  ( cf_def );
        retrieve_triggers ( cf_def );
        //
        cf_def_list.push_back( cf_def );
    }

    ks_def.__set_cf_defs( cf_def_list );
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

    const QString query( QString("SELECT durable_writes, strategy_class, strategy_options "
                                 "FROM system.schema_keyspaces "
                                 "WHERE keyspace_name = '%1'")
                         .arg(f_contextName)
                         );

    QCassandraQuery the_query( f_session );
    the_query.query( query );
    the_query.start();
    if( !the_query.nextRow() )
    {
        throw std::runtime_error("database is inconsistent!");
    }

    const bool    durable_writes   ( the_query.getBoolColumn   ( "durable_writes"   ) );
    const QString strategy_class   ( the_query.getStringColumn ( "strategy_class"   ) );
    const QString strategy_options ( the_query.getStringColumn ( "strategy_options" ) );

    ks_def.__set_name             ( context_name.toStdString() );
    ks_def.__set_strategy_class   ( the_query.getStringColumn  (  "strategy_class"   ).toStdString() );
    ks_def.__set_strategy_options ( the_query.getJsonMapColumn (  "strategy_options" )               );

    auto iter = options.find( "replication_factor" );
    if( iter != options.end() )
    {
        ks_def.__set_replication_factor( static_cast<int32_t>( iter->first->get_int64() ) );
    }

    retrieve_tables( ks_def );

    ks_def.__set_durable_writes( durable_writes );

    QCassandraContext::pointer_t c(f_parent->context(context_name));
    c->parseContextDefinition( &ks_def );
}


QString QCassandraPrivate::getKeyspaceOptions( org::apache::cassandra::KsDef& ks )
{
    QString q_str = QString("WITH replication = {'class': '%1'").arg(ks.strategy_class);
    for( const auto& pair : ks.strategy_options )
    {
        q_str += QString(", '%1': '%2'").arg(pair.first.c_str()).arg(pair.second.c_str());
    }
    q_str += "}\n";

    if( ks.__isset.durable_writes )
    {
        q_str += QString("AND durable_writes = %1\n").arg(ks.durable_writes? 'true': 'false');
    }
}


namespace
{
    QString map_to_json( const std::map<std::string,std::string>& map )
    {
        QString ret;
        for( const auto& pair : map )
        {
            if( !ret.isEmpty() )
            {
                ret += ", ";
            }
            ret += QString("'%1': '%2'").arg(pair.first.c_str()).arg(pair.second.c_str());
        }
        return ret;
    }
}


QString QCassandraPrivate::getTableOptions( org::apache::cassandra::CfDef& cf )
{
    QString q_str;

    // TODO: We might want to add more options, but for now, this is what Snap! uses in each of their tables.
    //
    if( cf.__isset.bloom_filter_fp_chance )
    {
        q_str += QString("AND bloom_filter_fp_chance = %1\n").arg(cf.bloom_filter_fp_chance);
    }
    if( cf.__isset.caching )
    {
        q_str += QString("AND caching = '%1'\n").arg(cf.caching);
    }
    if( cf.__isset.comment )
    {
        q_str += QString("AND comment = '%1'\n").arg(cf.comment);
    }
    if( cf.__isset.compaction_strategy_options )
    {
        q_str += QString("AND compaction = '%1'\n").arg(map_to_json(cf.compaction_strategy_options));
    }
    if( cf.__isset.compression_options )
    {
        q_str += QString("AND compression = '%1'\n").arg(map_to_json(cf.compression_options));
    }
    if( cf.__isset.dclocal_read_repair_chance )
    {
        q_str += QString("AND dclocal_read_repair_chance = %1\n").arg(cf.dclocal_read_repair_chance);
    }
    if( cf.__isset.default_time_to_live )
    {
        q_str += QString("AND default_time_to_live = %1\n").arg(cf.default_time_to_live);
    }
    if( cf.__isset.gc_grace_seconds )
    {
        q_str += QString("AND gc_grace_seconds = %1\n").arg(cf.gc_grace_seconds);
    }
    if( cf.__isset.memtable_flush_period_in_ms )
    {
        q_str += QString("AND memtable_flush_period_in_ms = %1\n").arg(cf.memtable_flush_period_in_ms);
    }
    if( cf.__isset.read_repair_chance )
    {
        q_str += QString("AND read_repair_chance = %1\n").arg(cf.read_repair_chance);
    }
    if( cf.__isset.speculative_retry )
    {
        q_str += QString("AND speculative_retry = %1\n").arg(cf.speculative_retry);
    }

    return q_str;
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
    //std::string id_ignore;
    //f_client->system_add_keyspace(id_ignore, ks);

    QString q_str( QString("CREATE KEYSPACE IF NOT EXISTS %1\n").arg(context->contextName()) );
    q_str += getKeyspaceOptions( ks );

    QCassandraQuery q( f_session );
    q.query( q_str );
    q.start();
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
    //std::string id_ignore;
    //f_client->system_update_keyspace(id_ignore, ks);

    QString q_str( QString("ALTER KEYSPACE %1\n").arg(context->contextName()) );
    q_str += getKeyspaceOptions( ks );

    QCassandraQuery q( f_session );
    q.query( q_str );
    q.start();
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
    //std::string id_ignore;
    //f_client->system_drop_keyspace(id_ignore, context.contextName().toUtf8().data());

    QCassandraQuery q( f_session );
    q.query( QString("DROP KEYSPACE %1").arg(context.contextName()) );
    q.start();
}

/** \brief Create a table in the Cassandra server.
 *
 * This function creates a table in the cassandra server transforming a
 * QCassandraTable in a table definition (CfDef) and then calling the
 * system_add_column_family() function.
 *
 * \param[in] table  The table to be created.
 */
void QCassandraPrivate::createTable(QCassandraTable::pointer_t table)
{
    mustBeConnected();
    org::apache::cassandra::CfDef cf;
    table->prepareTableDefinition(&cf);
    //std::string id_ignore;
    //f_client->system_add_column_family(id_ignore, cf);

    QString q_str(
            QString("CREATE TABLE IF NOT EXISTS %1.%2 "
                    "(key BLOB, column1 BLOB, value BLOB, PRIMARY KEY (key,column1))"
                    "WITH COMPACT STORAGE AND CLUSTERING ORDER BY (column1 ASC)\n")
                .arg(cf.keyspace)
                .arg(cf.name)
                );
    q_str = getTableOptions( cf );

    QCassandraQuery q( f_session );
    q.query( q_str );
    q.start();
}


/** \brief Update a table in the Cassandra server.
 *
 * This function updates a table int the cassandra server.
 *
 * \param[in] table  The table to be updated.
 */
void QCassandraPrivate::updateTable(QCassandraTable::pointer_t table)
{
    mustBeConnected();
    org::apache::cassandra::CfDef cf;
#if 0
    std::string id_ignore;
    f_client->system_update_column_family(id_ignore, cf);
#endif
    // NOT SUPPORTED
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
    //std::string id_ignore;
    //f_client->system_drop_column_family(id_ignore, table_name.toUtf8().data());
    QCassandraQuery q( f_session );
    q.query( QString("DROP TABLE IF EXISTS %1.%2")
             .arg(f_parent->currentContext().contextName())
             .arg(table_name) );
    q.start();
}


/** \brief Truncate a table in the Cassandra server.
 *
 * This function truncates (i.e. removes all the rows and their data) a table
 * from the cassandra server.
 *
 * \param[in] table  The table to be created.
 */
void QCassandraPrivate::truncateTable(QCassandraTable::pointer_t table)
{
    mustBeConnected();
    //f_client->truncate(table->tableName().toUtf8().data());
    QCassandraQuery q( f_session );
    q.query( QString("TRUNCATE TABLE IF EXISTS %1.%2")
             .arg(f_parent->currentContext().contextName())
             .arg(table->tableName()) );
    q.start();
}


namespace
{
    bool isCounterClass( const QString& validation_class )
    {
        return (validation_class == "org.apache.cassandra.db.marshal.CounterColumnType")
                || (validation_class == "CounterColumnType");
    }
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
void QCassandraPrivate::insertValue
    ( const QString&         table_name
    , const QByteArray&      row_key
    , const QByteArray&      column_key
    , const QCassandraValue& p_value
    , const QString&		 validation_class
    )
{
    mustBeConnected();

#if 0
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
#endif

    QCassandraValue value( p_value );
    int row_id    = 0;
    int column_id = 1;
    int value_id  = 2;
    QString query_string;
    if( isCounterClass(validation_class) )
    {
        QCassandraValue v;
        getValue( row_key, column_key, v );
        // new value = user value - current value
        int64_t add(-v.int64Value());
        switch( value.size() )
        {
            case 0:
                // accept NULL as zero
                break;

            case 1:
                add += value.signedCharValue();
                break;

            case 2:
                add += value.int16Value();
                break;

            case 4:
                add += value.int32Value();
                break;

            case 8:
                add += value.int64Value();
                break;

            default:
                throw std::runtime_error("value has an invalid size for a counter value");
        }
        value.setInt64Value( add );

        query_string = QString("UPDATE %1.%2 SET value = value + ? WHERE key = ? AND column1 = ?;")
            .arg(f_context->contextName())
            .arg(f_tableName)
            ;
        value_id  = 0;
        row_id    = 1;
        column_id = 2;
    }
    else
    {
        query_string = QString("INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?);")
            .arg(f_context->contextName())
            .arg(f_tableName)
            ;
    }

    QCassandraQuery q( f_session );
    q.query( query_string, 3 );
    q.bindByteArray( row_id, row_key.constData(), row_key.size() );
    q.bindByteArray( column_id, column_key.constData(), column_key.size() );

    if( isCounterClass(validation_class) )
    {
        q.bindInt64( value_id, value.int64Value() );
    }
    else
    {
        auto binary_val( value.binaryValue() );
        q.bindByteArray( value_id, binary_val.constData(), binary_val.size() );
    }

    q.start();
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
