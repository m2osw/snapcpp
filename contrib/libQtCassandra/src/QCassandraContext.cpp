/*
 * Text:
 *      QCassandraContext.cpp
 *
 * Description:
 *      Handling of Cassandra Keyspace which is a context.
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

#include "QtCassandra/QCassandraContext.h"
#include "QtCassandra/QCassandra.h"

#include <casswrapper/schema.h>

#include <stdexcept>
#include <unistd.h>

#include <sstream>

#include <QtCore>
//#include <QDebug>


namespace QtCassandra
{

/** \class QCassandraContext
 * \brief Hold a Cassandra keyspace definition.
 *
 * This class defines objects that can hold all the necessary information
 * for a Cassandra keyspace definition.
 *
 * A keyspace is similar to a context in the sense that to work on a keyspace
 * you need to make it the current context. Whenever you use a context, this
 * class automatically makes it the current context. This works well in a non
 * threaded environment. In a threaded environment, you want to either make
 * sure that only one thread makes use of the Cassandra objects or that you
 * protect all the calls. This library does not.
 *
 * You may think of this context as one database of an SQL environment. If
 * you have used OpenGL, this is very similar to the OpenGL context.
 */

/** \typedef QCassandraContext::QCassandraContextOptions
 * \brief A map of context options.
 *
 * This map defines options as name / value pairs.
 *
 * Only known otion names should be used or a protocol error may result.
 */

/** \typedef QCassandraContext::host_identifier_t
 * \brief Define host identifiers.
 *
 * This type is used to hold host identifiers. It is used when transforming
 * a host name to a host identifier. The identifier is unique and represents
 * an order in which each client are sorted.
 *
 * See the QCassandraLock object for more information about how this information
 * is used to create an inter-client lock.
 */

/** \var QCassandraContext::NULL_HOST_ID
 * \brief The NULL host identifier.
 *
 * When querying for a host that was not yet defined in the cluster, this
 * value is returned. It tells you that the host identifier wasn't defined
 * yet.
 *
 * This is used by the QCassandraLock to determine whether the host identifier
 * was defined.
 */

/** \var QCassandraContext::LARGEST_HOST_ID
 * \brief The largest acceptable host identifier.
 *
 * This value represents the largest accepted host identifier. If you try to
 * define more identifiers, then you get an error.
 *
 * This limit was <i>randomly</i> chosen. However, it represents the number of
 * computers that could ever connect to your Cassandra cluster. If you really
 * have more than that, update the value!
 */

/** \typedef QCassandraContext::lock_timeout_t
 * \brief The internal type for the lock timeout value.
 *
 * This type is internally used to hold the lock timeout. It is initialized
 * to 5 by default. It is defined in seconds.
 *
 * \sa setLockTimeout()
 * \sa lockTimeout()
 */

/** \typedef QCassandraContext::lock_ttl_t
 * \brief The internal type for the time to live of a lock.
 *
 * This type is internally used to hold the lock time to live value. It is
 * initialized to 60 by default. It is defined in seconds.
 *
 * \sa setLockTtl()
 * \sa lockTtl()
 */

/** \var QCassandraContext::f_schema
 * \brief The pointer to the casswrapper::schema meta object.
 *
 * This pointer is a shared pointer to the private definition of
 * the Cassandra context (i.e. a keyspace definition.)
 *
 * The pointer is created at the time the context is created.
 */

/** \var QCassandraContext::f_cassandra
 * \brief A pointer back to the QCassandra object.
 *
 * The bare pointer is used by the context to access the cassandra
 * private object and make the context the current context. It is
 * a bare pointer because the QCassandra object cannot be deleted
 * without the context getting deleted first.
 *
 * Note that when deleting a QCassandraContext object, you may still
 * have a shared pointer referencing the object. This means the
 * object itself will not be deleted. In that case, the f_cassandra
 * parameter becomes NULL and calling functions that make use of
 * it throw an error.
 *
 * \note
 * If you look at the implementation, many functions call the
 * makeCurrent() which checks the f_cassandra pointer, thus these
 * functions don't actually test the pointer.
 */

/** \var QCassandraContext::f_tables
 * \brief List of tables.
 *
 * A list of the tables defined in this context. The tables may be created
 * in memory only.
 *
 * The list is a map using the table binary key as the its own key.
 */

/** \var QCassandraContext::f_host_name
 * \brief The name of the host running this QCassandra instance.
 *
 * This variable holds the name of the host running this instance. This
 * is most often the name of the computer (what the hostname() function
 * returns, which is taken as the default value.) You may use a different
 * name by calling the setHostName() function.
 *
 * \sa setHostName()
 * \sa hostName()
 */

/** \var QCassandraContext::f_lock_table_name
 * \brief The name of the table used to create locks.
 *
 * This variable holds the name of the table used by the QCassandraLock
 * implementation. This name should be set once early on when creating the
 * context. It cannot be changed once a lock was created.
 *
 * By default this name is set to: "lock_table". You should not
 * have to ever change it.
 */

/** \var QCassandraContext::f_lock_accessed
 * \brief Internal flag to know that a lock was accessed.
 *
 * This variable is set to true as soon as a value used by the QCassandraLock
 * is read. This tells the context that some parameters such as the
 * host name and the table name used to create locks cannot be changed
 * anymore.
 */

/** \var QCassandraContext::f_lock_timeout
 * \brief The lock timeout value.
 *
 * This value is used by the QCassandraLock implementation to know how long to
 * wait before timing out when trying to obtain a lock.
 *
 * Note that the minimum is 1 second. Remember that if you are using a cluster
 * with computers in multiple centers throughout the world, 1 second is not very
 * much to ensure QUORUM consistency.
 */

/** \var QCassandraContext::f_lock_ttl
 * \brief The lock time to live value.
 *
 * This variable holds locks time to live (TTL).
 *
 * The QCassandraLock object saves all the lock values using this TTL so as to
 * ensure that the lock data does not live forever (otherwise it would lock things
 * for that long: forever.)
 *
 * In most cases the TTL is not necessary since the lock is released (and thus
 * deleted from the database) way before the TTL enters in action. However, if
 * your application crashes and the intended RAII implementation does not run
 * (i.e. SEGV that aborts the process immediately?) then at least the lock system
 * will recover after a little while.
 */


/** \brief Initialize a QCassandraContext object.
 *
 * This function initializes a QCassandraContext object.
 *
 * Note that the constructor is private. To create a new context, you must
 * use the QCassandra::context() function.
 *
 * All the parameters are set to the defaults as defined in the Cassandra
 * definition of the KsDef message. You can use the different functions to
 * change the default values.
 *
 * A context name must be composed of letters (A-Za-z), digits (0-9) and
 * underscore (_). It must start with a letter. The corresponding lexical
 * expression is: /[A-Za-z][A-Za-z0-9_]*\/
 *
 * The name of the lock table is defined at the time a cassandra context
 * object is created in memory. You must change it immediately with the
 * setLockTableName() if you do not want to use the default which is:
 * "lock_table". See the QCassandraLock object for more
 * information about locks in a Cassandra environment.
 *
 * \note
 * A context can be created, updated, and dropped. In all those cases, the
 * functions return once the Cassandra instance with which you are
 * connected is ready.
 *
 * \param[in] cassandra  The QCassandra object owning this context.
 * \param[in] context_name  The name of the Cassandra context.
 *
 * \sa contextName()
 * \sa setLockTableName()
 * \sa setLockHostName()
 * \sa QCassandra::context()
 */
QCassandraContext::QCassandraContext(QCassandra::pointer_t cassandra, const QString& context_name)
    //: f_schema(std::make_shared<casswrapper::schema::SessionMeta::KeyspaceMeta>())
    : f_cassandra(cassandra)
    , f_context_name(context_name)
      //f_tables() -- auto-init
      //f_host_name() -- auto-init
    , f_lock_table_name("lock_table")
      //f_lock_accessed(false) -- auto-init
      //f_lock_timeout(5) -- auto-init
      //f_lock_ttl(60) -- auto-init
{
    // verify the name here (faster than waiting for the server and good documentation)
    QRegExp re("[A-Za-z][A-Za-z0-9_]*");
    if(!re.exactMatch(context_name))
    {
        throw std::runtime_error("invalid context name (does not match [A-Za-z][A-Za-z0-9_]*)");
    }

    // get the computer name as the host name
    char hostname[HOST_NAME_MAX + 1];
    if(gethostname(hostname, sizeof(hostname)) == 0)
    {
        f_host_name = hostname;
    }

    resetSchema();
}

/** \brief Clean up the QCassandraContext object.
 *
 * This function ensures that all resources allocated by the
 * QCassandraContext are released.
 *
 * Note that does not in any way destroy the context in the
 * Cassandra cluster.
 */
QCassandraContext::~QCassandraContext()
{
}


void QCassandraContext::resetSchema()
{
    f_schema = std::make_shared<casswrapper::schema::SessionMeta::KeyspaceMeta>();

    casswrapper::schema::Value replication;
    auto& replication_map(replication.map());
    replication_map["class"]              = QVariant("SimpleStrategy");
    replication_map["replication_factor"] = QVariant(1);

    auto& field_map(f_schema->getFields());
    field_map["replication"]    = replication;
    field_map["durable_writes"] = QVariant(true);
}


/** \brief Retrieve the name of this context.
 *
 * This function returns the name of this context.
 *
 * Note that the name cannot be modified. It is set by the constructor as
 * you create a QCassandraContext.
 *
 * \return A QString with the context name.
 */
const QString& QCassandraContext::contextName() const
{
    return f_context_name;
}


const casswrapper::schema::Value::map_t& QCassandraContext::fields() const
{
    return f_schema->getFields();
}


casswrapper::schema::Value::map_t& QCassandraContext::fields()
{
    return f_schema->getFields();
}


/** \brief Retrieve a table definition by name.
 *
 * This function is used to retrieve a table definition by name.
 * If the table doesn't exist, it gets created.
 *
 * Note that if the context is just a memory context (i.e. it does not yet
 * exist in the Cassandra cluster,) then the table is just created in memory.
 * This is useful to create a new context with all of its tables all at
 * once. The process is to call the QCassandra::context() function to get
 * an in memory context, and then call this table() function for each one of
 * the table you want to create. Finally, to call the create() function to
 * actually create the context and its table in the Cassandra cluster.
 *
 * You can test whether the result is null with the isNull() function
 * of the std::shared_ptr<> class.
 *
 * \param[in] table_name  The name of the table to retrieve.
 *
 * \return A shared pointer to the table definition found or a null shared pointer.
 */
QCassandraTable::pointer_t QCassandraContext::table(const QString& table_name)
{
    QCassandraTable::pointer_t t( findTable( table_name ) );
    if( t != QCassandraTable::pointer_t() )
    {
        return t;
    }

    // this is a new table, allocate it
    t.reset( new QCassandraTable(shared_from_this(), table_name) );
    f_tables.insert( table_name, t );
    return t;
}


/** \brief Retrieve a reference to the tables.
 *
 * This function retrieves a constant reference to the map of table definitions.
 * The list is read-only, however, it is strongly suggested that you make a copy
 * if your code is going to modifiy tables later (i.e. calling table() may
 * affect the result of this call if you did not first copy the map.)
 *
 * \return A reference to the table definitions of this context.
 */
const QCassandraTables& QCassandraContext::tables()
{
#if 0
    if( f_tables.empty() )
    {
        loadTables();
    }
#endif

    return f_tables;
}

/** \brief Search for a table.
 *
 * This function searches for a table. If it exists, its shared pointer is
 * returned. Otherwise, it returns a NULL pointer (i.e. the
 * std::shared_ptr<>::operator bool() function returns true.)
 *
 * \note
 * Since the system reads the list of existing tables when it starts, this
 * function returns tables that exist in the database and in memory only.
 *
 * \param[in] table_name  The name of the table to retrieve.
 *
 * \return A shared pointer to the table.
 */
QCassandraTable::pointer_t QCassandraContext::findTable(const QString& table_name)
{
#if 0
    if( f_tables.empty() )
    {
        loadTables();
    }
#endif

    QCassandraTables::const_iterator it(f_tables.find(table_name));
    if(it == f_tables.end()) {
        QCassandraTable::pointer_t null;
        return null;
    }
    return *it;
}

/** \brief Retrieve a table reference from a context.
 *
 * The array operator searches for a table by name and returns
 * its reference. This is useful to access data with array like
 * syntax as in:
 *
 * \code
 * context[table_name][column_name] = value;
 * \endcode
 *
 * \exception std::runtime_error
 * If the table doesn't exist, this function raises an exception
 * since otherwise the reference would be a NULL pointer.
 *
 * \param[in] table_name  The name of the table to retrieve.
 *
 * \return A reference to the named table.
 */
QCassandraTable& QCassandraContext::operator [] (const QString& table_name)
{
    QCassandraTable::pointer_t ptable( findTable(table_name) );
    if( !ptable ) {
        throw std::runtime_error("named table was not found, cannot return a reference");
    }

    return *ptable;
}


#if 0
/** \brief Set the replication factor.
 *
 * This function sets the replication factor of the context.
 *
 * \deprecated
 * Since version 1.1 of Cassandra, the context replication
 * factor is viewed as a full option. This function automatically
 * sets the factor using the setDescriptionOption() function.
 * This means calling the setDescriptionOptions()
 * and overwriting all the options has the side effect of
 * cancelling this call. Note that may not work right with
 * older version of Cassandra. Let me know if that's the case.
 *
 * \param[in] factor  The new replication factor.
 */
void QCassandraContext::setReplicationFactor(int32_t factor)
{
    // since version 1.1 of Cassandra, the replication factor
    // defined in the structure is ignored
    QString value(QString("%1").arg(factor));
    setDescriptionOption("replication_factor", value);
}

/** \brief Unset the replication factor.
 *
 * This function unsets the replication factor in case it was set.
 * In general it is not necessary to call this function unless you
 * are initializing a new context and you want to make sure that
 * the default replication factor is used.
 */
void QCassandraContext::unsetReplicationFactor()
{
    eraseDescriptionOption("replication_factor");
}

/** \brief Check whether the replication factor is defined.
 *
 * This function retrieves the current status of the replication factor parameter.
 *
 * \return True if the replication factor parameter is defined.
 */
bool QCassandraContext::hasReplicationFactor() const
{
    return f_private->__isset.replication_factor;
}

/** \brief Retrieve the current replication factor.
 *
 * This function reads and return the current replication factor of
 * the context.
 *
 * If the replication factor is not defined, zero is returned.
 *
 * \return The current replication factor.
 */
int32_t QCassandraContext::replicationFactor() const
{
    if(f_private->__isset.replication_factor) {
        return f_private->replication_factor;
    }
    return 0;
}

/** \brief Set whether the writes are durable.
 *
 * Temporary and permanent contexts can be created. This option defines
 * whether it is one of the other. Set to true to create a permanent
 * context (this is the default.)
 *
 * \param[in] durable_writes  Set whether writes are durable.
 */
void QCassandraContext::setDurableWrites(bool durable_writes)
{
    f_private->__set_durable_writes(durable_writes);
}

/** \brief Unset the durable writes flag.
 *
 * This function marks the durable write flag as not set. This does
 * not otherwise change the flag. It will just not be sent over the
 * network and the default will be used when required.
 */
void QCassandraContext::unsetDurableWrites()
{
    f_private->__isset.durable_writes = false;
}

/** \brief Check whether the durable writes is defined.
 *
 * This function retrieves the current status of the durable writes parameter.
 *
 * \return True if the durable writes parameter is defined.
 */
bool QCassandraContext::hasDurableWrites() const
{
    return f_private->__isset.durable_writes;
}

/** \brief Retrieve the durable write flag.
 *
 * This function returns the durable flag that determines whether a
 * context is temporary (false) or permanent (true).
 *
 * \return The current durable writes flag status.
 */
bool QCassandraContext::durableWrites() const
{
    if(f_private->__isset.durable_writes) {
        return f_private->durable_writes;
    }
    return false;
}


/** \brief Prepare the context.
 *
 * This function prepares the context so it can be copied in a
 * keyspace definition later used to create a keyspace or to
 * update an existing keyspace.
 *
 * \todo
 * Verify that the strategy options are properly defined for the strategy
 * class (i.e. the "replication_factor" parameter is only for SimpleStrategy
 * and a list of at least one data center for other strategies.) However,
 * I'm not 100% sure that this is good idea since a user may add strategies
 * that we do not know anything about!
 *
 * \param[out] data  The output keyspace definition.
 *
 * \sa parseContextDefinition()
 */
void QCassandraContext::prepareContextDefinition(KsDef *ks) const
{
    *ks = *f_private;

    if(ks->strategy_class == "") {
        ks->strategy_class = "LocalStrategy";
    }

    // copy the options
    ks->strategy_options.clear();
    for(QCassandraContextOptions::const_iterator
                o = f_options.begin(); o != f_options.end(); ++o)
    {
        ks->strategy_options.insert(
                std::pair<std::string, std::string>(o.key().toUtf8().data(),
                                                    o.value().toUtf8().data()));
    }
    ks->__isset.strategy_options = !ks->strategy_options.empty();

    // copy the tables -- apparently we cannot do that here!
    // instead we have to loop through the table in the previous
    // level and update each column family separately
    ks->cf_defs.clear();
    for( auto t : f_tables )
    {
        CfDef cf;
        t->prepareTableDefinition(&cf);
        ks->cf_defs.push_back(cf);
    }
    //if(ks->cf_defs.empty()) ... problem? it's not optional...
}


/** \brief Generate the replication stanza for the CQL keyspace schema.
 */
QString QCassandraContext::generateReplicationStanza() const
{
    QString replication_stanza;
    if( f_private->strategy_class == "SimpleStrategy" )
    {
        replication_stanza = QString("'class': 'SimpleStrategy', 'replication_factor' : %1")
                             .arg(f_options["replication_factor"]);
    }
    else if( f_private->strategy_class == "NetworkTopologyStrategy" )
    {
        QString datacenters;
        for( QString key : f_options.keys() )
        {
            if( !datacenters.isEmpty() )
            {
                datacenters += ", ";
            }
            datacenters += QString("'%1': %2").arg(key).arg(f_options[key]);
        }
        //
        replication_stanza = QString("'class': 'NetworkTopologyStrategy', %1")
                             .arg(datacenters);
    }
    else
    {
        std::stringstream ss;
        ss << "This strategy class, '" << f_private->strategy_class << "', is not currently supported!";
        throw std::runtime_error( ss.str().c_str() );
    }

    return replication_stanza;
}
#endif


/** \brief This is an internal function used to parse a KsDef structure.
 *
 * This function is called internally to parse a KsDef object.
 *
 * \param[in] data  The pointer to the KsDef object.
 *
 * \sa prepareContextDefinition()
 */
void QCassandraContext::parseContextDefinition( casswrapper::schema::SessionMeta::KeyspaceMeta::pointer_t keyspace_meta )
{
    f_schema = keyspace_meta;
    for( const auto pair : keyspace_meta->getTables() )
    {
        QCassandraTable::pointer_t t(table(pair.first));
        t->parseTableDefinition(pair.second);
    }
}


/** \brief Make this context the current context.
 *
 * This function marks this context as the current context where further
 * function calls will be made (i.e. table and cell editing.)
 *
 * Note that whenever you call a function that requires this context to
 * be current, this function is called automatically. If the context is
 * already the current context, then no message is sent to the Cassandra
 * server.
 *
 * \sa QCassandra::setContext()
 */
void QCassandraContext::makeCurrent()
{
    parentCassandra()->setCurrentContext( shared_from_this() );
}


QString QCassandraContext::getKeyspaceOptions()
{
    QString q_str;
    for( const auto& pair : f_schema->getFields() )
    {
        if( q_str.isEmpty() )
        {
            q_str = "WITH ";
        }
        else
        {
            q_str += "AND ";
        }
        q_str += QString("%1 = %2\n")
                .arg(pair.first)
                .arg(pair.second.output())
                ;
    }

    return q_str;
}


/** \brief Create a new context.
 *
 * This function is used to create a new context (keyspace) in the current
 * Cassandra cluster. Once created, you can make use of it whether it is
 * attached to the Cassandra cluster or not.
 *
 * If you want to include tables in your new context, then create them before
 * calling this function. It will be faster since you'll end up with one
 * single request.
 *
 * There is an example on how to create a new context with this library:
 *
 * \code
 * QtCassandra::QCassandraContext context("qt_cassandra_test_context");
 * // default strategy is LocalStrategy which you usually do not want
 * context.setStrategyClass("org.apache.cassandra.locator.SimpleStrategy");
 * context.setDurableWrites(true); // by default this is 'true'
 * context.setReplicationFactor(1); // by default this is undefined
 * ...  // add tables before calling create() if you also want tables
 * context.create();
 * \endcode
 *
 * With newer versions of Cassandra (since 1.1) and a network or local
 * strategy you have to define the replication factors using your data
 * center names (the "replication_factor" parameter is ignored in that
 * case):
 *
 * \code
 * context.setDescriptionOption("strategy_class", "org.apache.cassandra.locator.NetworkTopologyStrategy");
 * context.setDescriptionOption("data_center1", "3");
 * context.setDescriptionOption("data_center2", "3");
 * context.setDurableWrites(true);
 * context.create();
 * \endcode
 *
 * Note that the replication factor is not set by default, yet it is a required
 * parameter.
 *
 * Also, the replication factor can be set to 1, although if you have more
 * than one node it is probably a poor choice. You probably want a minimum
 * of 3 for the replication factor, and you probably want a minimum of
 * 3 nodes in any live cluster.
 *
 * \sa QCassandraTable::create()
 */
void QCassandraContext::create()
{
    QString q_str( QString("CREATE KEYSPACE IF NOT EXISTS %1").arg(f_context_name) );
    q_str += getKeyspaceOptions();

    QCassandraOrder create_keyspace;
    create_keyspace.setCql( q_str, QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_SUCCESS );
    create_keyspace.setClearClusterDescription(true);
    QCassandraOrderResult const create_keyspace_result(parentCassandra()->proxy()->sendOrder(create_keyspace));
    if(!create_keyspace_result.succeeded())
    {
        throw std::runtime_error("keyspace creation failed");
    }

    for( auto t : f_tables )
    {
        t->create();
    }
}

/** \brief Update a context with new properties.
 *
 * This function defines a new set of properties in the specified context.
 * In general, the context will be searched in the cluster definitions,
 * updated in memory then this function called.
 */
void QCassandraContext::update()
{
    QString q_str( QString("ALTER KEYSPACE %1").arg(f_context_name) );
    q_str += getKeyspaceOptions();

    QCassandraOrder alter_keyspace;
    alter_keyspace.setCql( q_str, QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_SUCCESS );
    alter_keyspace.setClearClusterDescription(true);
    QCassandraOrderResult const alter_keyspace_result(parentCassandra()->proxy()->sendOrder(alter_keyspace));
    if(alter_keyspace_result.succeeded())
    {
        throw std::runtime_error("keyspace creation failed");
    }
}


/** \brief Drop this context.
 *
 * This function drops this context in the Cassandra database.
 *
 * Note that contexts are dropped by name so we really only use the name of
 * the context in this case.
 *
 * The QCassandraContext object is still valid afterward, although, obviously
 * no data can be read from or written to the Cassandra server since the
 * context is gone from the cluster.
 *
 * You may change the parameters of the context and call create() to create
 * a new context with the same name.
 *
 * \warning
 * If the context does not exist in Cassandra, this function call
 * raises an exception in newer versions of the Cassandra system
 * (in version 0.8 it would just return silently.) You may want to
 * call the QCassandra::findContext() function first to know whether
 * the context exists before calling this function.
 *
 * \sa QCassandra::dropContext()
 * \sa QCassandra::findContext()
 */
void QCassandraContext::drop()
{
    QString q_str(QString("DROP KEYSPACE IF EXISTS %1").arg(f_context_name));

    QCassandraOrder drop_keyspace;
    drop_keyspace.setCql( q_str, QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_SUCCESS );
    drop_keyspace.setClearClusterDescription(true);
    QCassandraOrderResult const drop_keyspace_result(parentCassandra()->proxy()->sendOrder(drop_keyspace));
    if(drop_keyspace_result.succeeded())
    {
        throw std::runtime_error("drop keyspace failed");
    }

    resetSchema();
    f_tables.clear();
}


/** \brief Drop the specified table from the Cassandra database.
 *
 * This function sends a message to the Cassandra server so the named table
 * gets droped from it.
 *
 * The function also deletes the table from memory (which means all its
 * rows and cells are also deleted.) Do not use the table after this call,
 * even if you kept a shared pointer to it. You may create a new one
 * with the same name though.
 *
 * Note that tables get dropped immediately from the Cassandra database
 * (contrary to rows).
 *
 * \param[in] table_name  The name of the table to drop.
 */
void QCassandraContext::dropTable(const QString& table_name)
{
    if(f_tables.find(table_name) == f_tables.end())
    {
        return;
    }

    // keep a shared pointer on the table
    QCassandraTable::pointer_t t(table(table_name));

    // remove from the Cassandra database
    makeCurrent();

    QString q_str(QString("DROP TABLE IF EXISTS %1.%2").arg(f_context_name).arg(table_name));

    QCassandraOrder drop_table;
    drop_table.setCql( q_str, QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_SUCCESS );
    drop_table.setClearClusterDescription(true);
    QCassandraOrderResult const drop_table_result(parentCassandra()->proxy()->sendOrder(drop_table));
    if(drop_table_result.succeeded())
    {
        throw std::runtime_error("drop table failed");
    }

    // disconnect all the cached data from this table
    f_tables.remove(table_name);
}


/** \brief Clear the context cache.
 *
 * This function clears the context cache. This means all the tables, their
 * rows, and the cells of those rows all get cleared. None of these can be
 * used after this call even if you kept a shared pointer to any of these
 * objects.
 */
void QCassandraContext::clearCache()
{
    f_tables.clear();
    parentCassandra()->retrieveContextMeta( shared_from_this(), f_context_name );
}

/** \brief The hosts are listed in the locks table under this name.
 *
 * The lock table uses rows named after the objects that you want to
 * lock. It also includes the list of all the hosts that access your
 * Cassandra system.
 *
 * \return The name used for row holding the list of host names and
 *         their identifiers.
 */
QString QCassandraContext::lockHostsKey() const
{
    return "hosts";
}

/** \brief Retrieve the table used by the Lock implementation.
 *
 * This function retrieves the "lock_table" table to use
 * with the different functions that handle the Cassandra interprocess
 * locking implementation.
 *
 * If the table doesn't exist yet, then it gets created. The function
 * also synchronize the cluster so as to allow other functions to
 * instantly make use of the table on return.
 *
 * \note
 * Remember that the lock is inter-PROCESS and not inter-threads. This
 * library is not thread safe.
 *
 * \return The function returns a pointer to the Cassandra table.
 */
QCassandraTable::pointer_t QCassandraContext::lockTable()
{
    // check whether the table exists
    const QString& table_name(lockTableName());
    if(findTable(table_name)) {
        return table(table_name);
    }

    // TODO: determine what the best parameters are for a session table
    QCassandraTable::pointer_t lock_table(table(table_name));

    casswrapper::schema::Value compaction_value;
    auto& compaction_value_map(compaction_value.map());
    compaction_value_map["class"]         = casswrapper::schema::Value("SizeTieredCompactionStrategy");
    compaction_value_map["max_threshold"] = casswrapper::schema::Value(22);
    compaction_value_map["min_threshold"] = casswrapper::schema::Value(4);

    casswrapper::schema::Value caching_value;
    auto& caching_value_map(caching_value.map());
    caching_value_map["keys"]               = casswrapper::schema::Value("ALL");
    caching_value_map["rows_per_partition"] = casswrapper::schema::Value("NONE");

    auto& fields_map( lock_table->fields() );
    fields_map["gc_grace_seconds"]            = casswrapper::schema::Value(3600);
    fields_map["memtable_flush_period_in_ms"] = casswrapper::schema::Value(3600000); // 1 hour
    fields_map["compaction"]                  = compaction_value;
    fields_map["caching"]                     = caching_value;

    lock_table->create();

    return lock_table;
}


/** \brief Add a new host to the existing list of hosts.
 *
 * This function adds the name of a host and assign it an identifier
 * between 1 and LARGEST_HOST_ID. If you have more hosts than
 * LARGEST_HOST_ID then you will have to recompile Snap with a larger
 * number (really? more than 10,000 computers?)
 *
 * The addition of hosts in this way is safe on a running system as
 * long as:
 *
 * \li (1) the host being added is not already running;
 * \li (2) only one instance of the process calling this function
 *         runs at a time.
 *
 * The new identifier is added after looking at all the existing
 * identifiers (i.e. if one is available, it is used, if none are
 * available, a new one is created.)
 *
 * \note
 * If the named host already exists in the list of hosts, then
 * it is not added a second time. Be careful because the remove
 * removes at once (i.e. if you added the same host name 10 times,
 * only 1 remove and it is gone.)
 *
 * \param[in] host_name  The name of the host to be added.
 */
void QCassandraContext::addLockHost(const QString& host_name)
{
    QCassandraTable::pointer_t locks_table(lockTable());
    QCassandraRow::pointer_t hosts_row(locks_table->row(lockHostsKey()));
    hosts_row->clearCache(); // make sure we have a clean slate
    const int hosts_count(hosts_row->cellCount());
    auto hosts_predicate( std::make_shared<QCassandraCellRangePredicate>() );
    hosts_predicate->setCount(hosts_count);
    hosts_row->readCells(hosts_predicate);
    const QCassandraCells& hosts(hosts_row->cells());
    // note: there is an interesting way to find one or more missing numbers
    //       in a list which involves polynomials, however, at this point we
    //       do not know the largest number and thus how many numbers are
    //       missing in the list (if any); plus it becomes very expensive
    //       when many numbers are missing; so instead we use a vector of
    //       booleans which anyway is a lot simpler to implement and maintain
    //       See: http://stackoverflow.com/questions/3492302/easy-interview-question-got-harder-given-numbers-1-100-find-the-missing-numbe
    std::vector<bool> set;
    std::vector<bool>::size_type size(0);
    for(QCassandraCells::const_iterator j(hosts.begin()); j != hosts.end(); ++j) {
        if((*j)->columnName() == host_name) {
            // we already have it there, don't touch it
            return;
        }
        uint32_t id((*j)->value().uint32Value());
        if(id > size) {
            // make sure to resize or we'll crash
            set.resize(id + 1);
            size = id;
        }
        set[id] = true;
    }
    uint32_t new_id(0);
    if(size == 0) {
        // first entry is 1
        new_id = 1;
    }
    else {
        // ignore 0 in the search (which is why we need a special case...)
        std::vector<bool>::const_iterator it(std::find(set.begin() + 1, set.end(), false));
        new_id = it - set.begin();
    }
    QCassandraValue value(new_id);
    locks_table->row(lockHostsKey())->cell(host_name)->setValue(value);
}


/** \brief Remove a lock host name from the database.
 *
 * This function removes the specified host name from the database.
 * The identifier of the host is then released, but all existing
 * identifiers are not modified. It will be reused next time a
 * host is added to the database.
 *
 * It is safe to remove a host on a running system as long as the
 * host being removed does not run anymore at the time it is removed.
 *
 * The removal makes use of a consistency level of QUORUM to make sure
 * it happens in the entire cluster.
 *
 * \param[in] host_name  The name of the host to be removed from the database.
 */
void QCassandraContext::removeLockHost(const QString& host_name)
{
    QCassandraTable::pointer_t locks_table(table(f_lock_table_name));
    QCassandraRow::pointer_t row(locks_table->row(lockHostsKey()));
    QCassandraCell::pointer_t c(row->cell(host_name));
    row->dropCell(host_name);
}


/** \brief Set the name of the host using this instance.
 *
 * Each host must have a unique name which the libQtCassandra system can
 * transform in an identifier (a number from 1 to 10000).
 *
 * For locks to function (see QCassandraLock), it is a requirement to
 * call this function because each host must be given a unique identifier
 * used in the lock implementation to know who gets the lock next.
 *
 * \exception std::logic_error
 * This exception is raised if the lock was already accessed.
 *
 * \param[in] host_name  The name of the host running this instance of QtCassandra.
 *
 * \sa QCassandraLock
 */
void QCassandraContext::setHostName(const QString& host_name)
{
    if(f_lock_accessed) {
        // TBD: should we accept a set if the table name is anyway the same?
        throw std::logic_error("setLockHostName() called after a lock was created");
    }
    f_host_name = host_name;
}


/** \brief Get the name of the host using this instance.
 *
 * This function returns the name of the host using this QCassandraContext.
 * The name is defined by calling hostname() by default. However, it can be
 * modified by calling the setHostName() function.
 *
 * \return The name of the host as set by setHostName().
 */
QString QCassandraContext::hostName() const
{
    f_lock_accessed = true;
    return f_host_name;
}


/** \brief Get the pointer to the parent object.
 *
 * \return Shared pointer to the cassandra object.
 */
QCassandra::pointer_t QCassandraContext::parentCassandra() const
{
    QCassandra::pointer_t cassandra(f_cassandra.lock());
    if(cassandra == nullptr)
    {
        throw std::runtime_error("this context was dropped and is not attached to a cassandra cluster anymore");
    }

    return cassandra;
}


/** \brief Set the name of the lock table in this context.
 *
 * The QCassandraContext uses a default name to lock tables, rows, cells.
 * This name can be changed by calling this function until a lock was
 * used. Once a lock was used, the name cannot be changed anymore. Note
 * that you MUST always have exactly the same name for all your application
 * processes or the lock won't work properly. This function should be called
 * very early on to ensure consistency.
 *
 * \exception std::logic_error
 * This exception is raised if the lock was already accessed.
 *
 * \param[in] lock_table_name  The name to use as the lock table.
 */
void QCassandraContext::setLockTableName(const QString& lock_table_name)
{
    if(f_lock_accessed) {
        // TBD: should we accept a set if the table name is anyway the same?
        throw std::logic_error("setLockTableName() called after a lock was created");
    }
    f_lock_table_name = lock_table_name;
}

/** \brief Retrieve the current lock table name.
 *
 * The lock table name is set to "lock_table"
 *
 * \return The name of the table used to create locks.
 */
const QString& QCassandraContext::lockTableName() const
{
    f_lock_accessed = true;
    return f_lock_table_name;
}

/** \brief Set the lock timeout.
 *
 * Set the time out of the lock function in seconds. This amount is used to
 * time the function while acquiring a lock. If more than this number of
 * seconds, then the function fails and returns false.
 *
 * The default lock timeout is 5 seconds. It cannot be set to less than 1
 * second. There is no option to set a lock that never times out, although
 * if you use a very large number, it is pretty much the same.
 *
 * \param[in] timeout  The new timeout in seconds.
 */
void QCassandraContext::setLockTimeout(int timeout)
{
    if(timeout < 1) {
        timeout = 1;
    }
    f_lock_timeout = timeout;
}

/** \brief Retrieve the lock time out.
 *
 * By default retrieving a lock times out in 5 seconds. In other words, if
 * the acquisition of the lock takes more than 5 seconds, the function times
 * out and returns false.
 *
 * \return The current lock time out.
 */
int QCassandraContext::lockTimeout() const
{
    return f_lock_timeout;
}

/** \brief Set a different TTL for lock variables.
 *
 * Whenever creating lock variables in the database, this TTL is used.
 * This ensures that the lock variables are not permanent in the
 * database. This will automatically unlock everything.
 *
 * The value is defined in seconds although a lock should be returns
 * in milliseconds, it may take a little bit of time if you have
 * several data centers accross the Internet.
 *
 * The default is 1 minute (60 seconds.)
 *
 * \param[in] ttl  The new time to live value in seconds.
 */
void QCassandraContext::setLockTtl(int ttl)
{
    if(ttl < 0) {
        throw std::runtime_error("the TTL value cannot be negative");
    }
    f_lock_ttl = ttl;
}

/** \brief Retrieve the lock TTL.
 *
 * This function returns the lock TTL as defined by the setLockTtl().
 * By default the TTL value is set to 1 minute.
 *
 * The value is specified in seconds.
 *
 * \return The current TTL value.
 */
int QCassandraContext::lockTtl() const
{
    return f_lock_ttl;
}


} // namespace QtCassandra
// vim: ts=4 sw=4 et
