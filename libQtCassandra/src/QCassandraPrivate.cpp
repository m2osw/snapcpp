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
 *      Copyright (c) 2011-2013 Made to Order Software Corp.
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
#include "QThriftHeaders.h"

namespace QtCassandra
{


namespace {

/** \brief Value representing a node that's disconnected.
 *
 * Nodes that are up and down can be checked using the
 * describe_schema_versions() function. If a node is
 * disconnected, its version is set to UNREACHABLE.
 */
const char g_unreachable[] = "UNREACHABLE";

/** \brief Factory used to allow our library to specify a password
 * 
 * This class is used to specify a password when conneting to the
 * Cassandra server. This is used to work between servers that
 * are connected over the Internet using SSL.
 */
class QCassandraSocketFactory : public apache::thrift::transport::TSSLSocketFactory
{
public:
    QCassandraSocketFactory(const char *password);
    virtual ~QCassandraSocketFactory();

    virtual void getPassword(std::string& password, int max_size);

private:
    std::string     f_password;
};

/** \var QCassandraSocketFactory::f_password
 * \brief The Cassandra password to connect with an SSL socket.
 *
 * The password passed to the SSL socket implementation of thrift which then
 * passes it to OpenSSL. This variable remains defined until the
 * QCassandraSocketFactory destructor is called. At that point it gets
 * cleared for security reasons.
 */

/** \brief Initialize the QCassandraSocketFactory object.
 *
 * This function saves the cassandra_private pointer as the parent of this
 * object.
 *
 * \param[in] password  The password used to authenticate with an SSL connection.
 */
QCassandraSocketFactory::QCassandraSocketFactory(const char *password)
    : f_password(password)
{
}

/** \brief Clean up the QCassandraSocketFactory object.
 *
 * This function clears the password so we do not keep a copy in memory.
 */
QCassandraSocketFactory::~QCassandraSocketFactory()
{
    for(int i(f_password.length() - 1); i >= 0; --i) {
        f_password[i] = '*';
    }
}

/** \brief Retrieve the Cassandra password.
 *
 * This function copies the password as passed to us via the
 * QCassandraPrivate::connect() function.
 *
 * \note
 * We make a copy of the password in the string passed by thrift,
 * whether thrift properly clears the buffer is not our responsibility,
 * but know that as of thrift 0.8.0 does NOT currently clear its
 * password buffers.
 *
 * \param[out] password  The string where the password is returned.
 * \param[in] max_size  The maximum size for the password.
 */
void QCassandraSocketFactory::getPassword(std::string& password, int /*max_size*/)
{
    password = f_password;
}

} // no name namespace

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
QCassandraPrivate::QCassandraPrivate(QCassandra *parent)
    : f_parent(parent)
      //f_socket(NULL) -- auto-initialized
      //f_transport(NULL) -- auto-initialized
      //f_protocol(NULL) -- auto-initialized
      //f_client(NULL) -- auto-initialized
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
 * \param[in] password  The password to use on an SSL connection if not empty.
 *
 * \return true when the server connected, false otherwise
 *
 * \sa isConnected()
 * \sa disconnect()
 * \sa QCassandraSocketFactory
 */
bool QCassandraPrivate::connect(const QString& host, const int port, const QString& password)
{
    // disconnect any existing connection
    disconnect();

    bool worked(true);
    try {
        // create a socket, transportation, protocol, and client
        // the client is what we use to communicate with the Cassandra server
        if(password.isEmpty()) {
            f_socket.reset(new apache::thrift::transport::TSocket(host.toUtf8().data(), port));
        }
        else {
            // make sure we keep a reference to the copy so we can
            // clear it once we're done with it (for security reasons)
            QByteArray pwd(password.toUtf8());
            QCassandraSocketFactory socket_factory(pwd.data());
            for(int i(pwd.size() - 1); i >= 0; --i) {
                pwd[i] = '*';
            }
            socket_factory.overrideDefaultPasswordCallback();
            socket_factory.authenticate(password != "ignore");
            boost::shared_ptr<apache::thrift::transport::TSSLSocket> socket(socket_factory.createSocket(host.toUtf8().data(), port));
            f_socket = boost::static_pointer_cast<apache::thrift::transport::TTransport>(socket);
        }

        f_transport.reset(new apache::thrift::transport::TFramedTransport(f_socket));
        f_protocol.reset(new apache::thrift::protocol::TBinaryProtocol(f_transport));
        f_client.reset(new org::apache::cassandra::CassandraClient(f_protocol));

        // once everything is connected as it should, open the transport link
        // NB: you may get an error here because it tries to open with IPv6
        //     first, but if we do not catch the error, IPv4 worked
        f_transport->open();
    }
    catch(...) {
        // TBD: should we check for some specific error here?
        worked = false;
    }

    // if it failed, make sure to clear all the pointers
    if(!worked) {
        disconnect();
    }

    return worked;
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
    if(f_client.get()) {
        // this is probably not necessary (it should anyway happen as required)
        f_transport->close();
    }
    f_client.reset();
    f_protocol.reset();
    f_transport.reset();
    f_socket.reset();
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
    return f_client.get() != NULL;
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
 */
void QCassandraPrivate::synchronizeSchemaVersions(int timeout)
{
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
    if(!isConnected()) {
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
    mustBeConnected();
    std::string cluster_name;
    f_client->describe_cluster_name(cluster_name);
    return cluster_name.c_str();
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
    mustBeConnected();
    std::string protocol_version;
    f_client->describe_version(protocol_version);
    return protocol_version.c_str();
}

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
    mustBeConnected();
    std::string partitioner_str;
    f_client->describe_partitioner(partitioner_str);
    return partitioner_str.c_str();
}

/** \brief Retrieve the snitch of the cluster.
 *
 * This function sends a message to the Cassandra server to
 * determine the snitch defined for the cluster.
 *
 * \return The snitch used by the cluster.
 */
QString QCassandraPrivate::snitch() const
{
    mustBeConnected();
    std::string snitch_str;
    f_client->describe_snitch(snitch_str);
    return snitch_str.c_str();
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
    mustBeConnected();
    f_client->set_keyspace(context_name.toUtf8().data());
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

    // retrieve the key spaces from Cassandra
    std::vector<org::apache::cassandra::KsDef> keyspaces;
    f_client->describe_keyspaces(keyspaces);

    for(std::vector<org::apache::cassandra::KsDef>::const_iterator
                    ks(keyspaces.begin()); ks != keyspaces.end(); ++ks) {
        QSharedPointer<QCassandraContext> c(f_parent->context(ks->name.c_str()));
        const org::apache::cassandra::KsDef& ks_def = *ks;
        c->parseContextDefinition(&ks_def);
    }
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

    f_client->insert(rkey, column_parent, column, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
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
    //try {
        f_client->get(column_result, rkey, column_path, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
    //}
    //catch(org::apache::cassandra::NotFoundException& /*e*/) {
    //    // this happens when a column is missing and thus we cannot
    //    // read it; in this case we return NULL in value.
    //    value.setNullValue();
    //    return;
    //}

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
 * \return The number of rows read from the Cassandra server.
 */
uint32_t QCassandraPrivate::getRowSlices(QCassandraTable& table, QCassandraRowPredicate& row_predicate)
{
    mustBeConnected();

    org::apache::cassandra::ColumnParent column_parent;
    column_parent.__set_column_family(table.tableName().toUtf8().data());
    // no super column support here

    QSharedPointer<QCassandraColumnPredicate> column_predicate(row_predicate.columnPredicate());
    org::apache::cassandra::SlicePredicate slice_predicate;
    column_predicate->toPredicate(&slice_predicate);

    org::apache::cassandra::KeyRange key_range;
    row_predicate.toPredicate(&key_range);

    typedef std::vector<org::apache::cassandra::KeySlice> key_slice_vector_t;
    key_slice_vector_t results;

    // our consistency level is 100% based on the Thrift consistency level
    // a cast is enough to get the value we want to get
    // (see the QCassandraValue.cpp file)

    consistency_level_t consistency_level = column_predicate->consistencyLevel();
    if(consistency_level == CONSISTENCY_LEVEL_DEFAULT) {
        consistency_level = f_parent->defaultConsistencyLevel();
    }

    try {
//printf("start/end [%s]/[%s] OR [%s]/[%s]\n",
//                key_range.start_key.c_str(), key_range.end_key.c_str(),
//                key_range.start_token.c_str(), key_range.end_token.c_str());
        f_client->get_range_slices(results, column_parent, slice_predicate, key_range, static_cast<org::apache::cassandra::ConsistencyLevel::type>(static_cast<cassandra_consistency_level_t>(consistency_level)));
    }
    // Wondering whether the invalid request exception is what 0.8.0 was
    // returning instead of the transport exception.
    //catch(const org::apache::cassandra::InvalidRequestException& /*e*/) {
    //    // this happens when the predicate is not liked by Cassandra
    //    // (it's probably some form of a bug though)
    //    return 0;
    //}
    catch(const apache::thrift::transport::TTransportException& e) {
        if(e.what() == std::string("No more data to read.")) {
            return 0;
        }
        throw;
    }

    // we got results, copy the data to the table cache
    int adjust(0);
    for(key_slice_vector_t::iterator it = results.begin(); it != results.end(); ++it) {
        if(it == results.begin() && row_predicate.excludeFirst()) {
            adjust = -1;
            continue;
        }
        QByteArray row_key(it->key.c_str(), it->key.size());
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
        if(it + 1 == results.end()) {
            row_predicate.setLastKey(row_key);
        }
    }

    return results.size() + adjust;
}

} // namespace QtCassandra
// vim: ts=4 sw=4 et
