/*
 * Text:
 *      QCassandraContext.cpp
 *
 * Description:
 *      Handling of the cassandra::KsDef which is a context.
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
#include <stdexcept>
#include <unistd.h>

#include <QRegExp>
#include <QDebug>


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

/** \var QCassandraContext::f_private
 * \brief The pointer to the QCassandraContextPrivate object.
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

/** \var QCassandraContext::f_options
 * \brief List of tables.
 *
 * A map of name and value pairs representing options of the context.
 * Each context can have many options.
 *
 * The libQtCassandra doesn't make use of these options. It's only used
 * by the Cassandra server.
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
 * By default this name is set to: "libQtCassandraLockTable". You should not
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

/** \brief Overload the KsDef to handle details.
 *
 * This class hides the KsDef from the outside. It will give a copy of the
 * KsDef to the QCassandraContext.
 *
 * Having this definition allows us to avoid the \#include of thrift
 * generated headers (which in turn \#include thrift headers) that your
 * applications would otherwise need to have access to.
 */
class QCassandraContextPrivate : public org::apache::cassandra::KsDef {};







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
 * "libQtCassandraLockTable". See the QCassandraLock object for more
 * information about locks in a Cassandra environment.
 *
 * \note
 * A context can be created, updated, and dropped. In all those cases, the
 * functions return once the Cassandra instance with which you are
 * connected is ready. However, that is not enough if you are working with
 * a cluster because the other nodes do not get updated instantaneously.
 * Instead, you have to call the QCassandra::synchronizeSchemaVersions()
 * function of the QCassandra object to make sure that the context is fully
 * available across your cluster.
 *
 * \param[in] cassandra  The QCassandra object owning this context.
 * \param[in] context_name  The name of the Cassandra context.
 *
 * \sa contextName()
 * \sa setLockTableName()
 * \sa setLockHostName()
 * \sa QCassandra::context()
 * \sa QCassandra::synchronizeSchemaVersions()
 */
QCassandraContext::QCassandraContext(QCassandra::pointer_t cassandra, const QString& context_name)
    : f_private(new QCassandraContextPrivate),
      f_cassandra(cassandra),
      //f_options() -- auto-init
      //f_tables() -- auto-init
      //f_host_name() -- auto-init
      f_lock_table_name("libQtCassandraLockTable")
      //f_lock_accessed(false) -- auto-init
      //f_lock_timeout(5) -- auto-init
      //f_lock_ttl(60) -- auto-init
{
    // verify the name here (faster than waiting for the server and good documentation)
    QRegExp re("[A-Za-z][A-Za-z0-9_]*");
    if(!re.exactMatch(context_name)) {
        throw std::runtime_error("invalid context name (does not match [A-Za-z][A-Za-z0-9_]*)");
    }

    // we save the name and at this point we prevent it from being changed.
    f_private->__set_name(context_name.toUtf8().data());

    // get the computer name as the host name
    char hostname[HOST_NAME_MAX + 1];
    if(gethostname(hostname, sizeof(hostname)) == 0) {
        f_host_name = hostname;
    }
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

/** \brief Retrieve the name of this context.
 *
 * This function returns the name of this context.
 *
 * Note that the name cannot be modified. It is set by the constructor as
 * you create a QCassandraContext.
 *
 * \return A QString with the context name.
 */
QString QCassandraContext::contextName() const
{
    return f_private->name.c_str();
}

/** \brief Set the context strategy class.
 *
 * This function is used to change the strategy class of the context to a
 * new strategy. The strategy class (called the placement_strategy in
 * CLI/CQL) is used to handle reads and writes according to the use of the
 * corresponding context.
 *
 * \note
 * If not set, this library automatically assigns the local strategy
 * by default:
 *
 * \code
 *      ks->strategy_class = "org.apache.cassandra.locator.LocalStrategy";
 * \endcode
 *
 * \param[in] strategy_class  The new strategy class.
 *
 * \sa strategyClass()
 */
void QCassandraContext::setStrategyClass(const QString& strategy_class)
{
    f_private->__set_strategy_class(strategy_class.toUtf8().data());
}

/** \brief Retrieve the name of the strategy class of this context.
 *
 * This function returns the strategy class of this context. This is
 * a name defining how the data in this context is duplicated accross
 * the nodes of the Cassandra system.
 *
 * \return The strategy class.
 *
 * \sa descriptionOption()
 * \sa descriptionOptions()
 * \sa setStrategyClass()
 */
QString QCassandraContext::strategyClass() const
{
    return f_private->strategy_class.c_str();
}

/** \brief Replace all the context description options.
 *
 * This function overwrites all the "description" options (this is called
 * the strategy_options in the CLI or CQL) with the ones specified in the
 * input parameter.
 *
 * This function can be used to clear all the options by passing an
 * empty \p options parameter. Note that any existing options get overwritten
 * and that includes the replication factor.
 *
 * To avoid overwriting existing options, you may want to consider using
 * the setDescriptionOption() function instead.
 *
 * \warning
 * Since Cassandra version 1.1, the replication_factor has become a full
 * option and the definition found directly in the KsDef structure is
 * ignored. This means overwriting all the options may have
 * the unwanted side effect of deleting the replication_factor
 * under your feet (i.e. it should be defined in your \p options
 * parameter to make sure it will not get deleted.)
 *
 * \param[in] options  The replacing options
 *
 * \sa setReplicationFactor()
 * \sa descriptionOptions()
 * \sa setDescriptionOption()
 */
void QCassandraContext::setDescriptionOptions(const QCassandraContextOptions& options)
{
    f_options = options;

    if(f_options.contains("replication_factor")) {
        f_private->__set_replication_factor(f_options["replication_factor"].toInt());
    }
    else {
        f_private->__isset.replication_factor = false;
    }
}

/** \brief Get the map of all description options.
 *
 * The context maintains a map indexed by option name of all the description
 * options of the context. This function retreives a constant reference to that
 * list.
 *
 * If you keep the return reference as such (i.e. a reference) then make sure
 * you do not modify the options (calling one of the setDescriptionOptions()
 * or setDescriptionOption() functions.) Otherwise the reference may become
 * invalid. If you are going to modify the options, make a copy of the map.
 *
 * \return A reference to the map of context options.
 *
 * \sa setDescriptionOptions()
 * \sa setDescriptionOption()
 * \sa descriptionOption()
 */
const QCassandraContext::QCassandraContextOptions& QCassandraContext::descriptionOptions() const
{
    return f_options;
}

/** \brief Add or replace one of the context description options.
 *
 * This function sets the specified \p option to the specified \p value.
 *
 * \warning
 * The CQL language shows a field named "class" as part of the strategy
 * options. Setting that parameter actually prevents the creation of
 * context. You want to limit your setup to "replication_factor" and
 * valid data center names.
 *
 * \param[in] option  The option to set.
 * \param[in] value  The new value the option to set.
 *
 * \sa descriptionOption()
 * \sa descriptionOptions()
 * \sa setDescriptionOptions()
 */
void QCassandraContext::setDescriptionOption(const QString& option, const QString& value)
{
    f_options[option] = value;

    if(option == "replication_factor") {
        f_private->__set_replication_factor(value.toInt());
    }
}

/** \brief Retrieve a description option.
 *
 * This function retrieves the description option named by the \p option
 * parameter.
 *
 * \note
 * Versions of this function in libQtCassandra before 0.4.2 would add
 * an empty version of the option if it wasn't defined yet.
 *
 * \param[in] option  The name of the option to retrieve.
 *
 * \return The value of the named option or an empty string.
 */
QString QCassandraContext::descriptionOption(const QString& option) const
{
    // avoid creating an entry if it is not defined
    if(!f_options.contains(option)) {
        return "";
    }
    return f_options[option];
}

/** \brief Delete an option from the current list of options.
 *
 * This function deletes the specified option from the list of description
 * options in memory. If the option was not defined, the function has no
 * effect.
 *
 * \note
 * This is useful to manage the list of options, however, erasing an
 * option here only tells the system to use the default value. It does
 * not prevent the system from having that option defined.
 *
 * \param[in] option  The name of the option to delete.
 */
void QCassandraContext::eraseDescriptionOption(const QString& option)
{
    f_options.erase(f_options.find(option));

    if(option == "replication_factor") {
        f_private->__isset.replication_factor = false;
    }
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
    // table already exists?
    QCassandraTables::iterator ti(f_tables.find(table_name));
    if(ti != f_tables.end()) {
        return ti.value();
    }

    // this is a new table, allocate it
    QCassandraTable::pointer_t t(new QCassandraTable(shared_from_this(), table_name));
    f_tables.insert(table_name, t);
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
const QCassandraTables& QCassandraContext::tables() const
{
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
QCassandraTable::pointer_t QCassandraContext::findTable(const QString& table_name) const
{
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

/** \brief Retrieve a constant table reference.
 *
 * This array operator is the same as the other one, just this one deals
 * with constant tables. It can be used to retrieve values from the
 * Cassandra cluster you're connected to:
 *
 * \code
 * value = context[table_name][column_name];
 * \endcode
 *
 * \exception std::runtime_error
 * If the table doesn't exist, this function raises an exception
 * since otherwise the reference would be a NULL pointer.
 *
 * \param[in] table_name  The name of the table to retrieve.
 *
 * \return A constant reference to the named table.
 */
const QCassandraTable& QCassandraContext::operator [] (const QString& table_name) const
{
    const QCassandraTable::pointer_t ptable( findTable(table_name) );
    if( !ptable ) {
        throw std::runtime_error("named table was not found, cannot return a reference");
    }

    return *ptable;
}

/** \brief Set the replication factor.
 *
 * This function sets the replication factor of the context.
 *
 * The replication factor is only used if you set the replication
 * strategy to "SimpleStrategy". In all other cases, it is ignored.
 *
 * When setting up the replication factor for a strategy set to
 * "NetworkTopologyStrategy", then you need to set replication
 * factors using data center names as in:
 *
 * \code
 *    // get a new context (assuming "my_context" does not exist in Cassandra)
 *    // (use findContext() to know whether a context exists)
 *    QtCassandra::QCassandraContext::pointer_t context = cassandra->context("my_context");
 *
 *    context->setStrategyClass("org.apache.cassandra.locator.NetworkTopologyStrategy");
 *
 *    QtCassandra::QCassandraContextOptions options;
 *    options["dc1"] = 3;
 *    options["dc2"] = 2;
 *    options["dc3"] = 5;
 *    context->setDescriptionOptions(options);
 *
 *    context->createContext();
 * \endcode
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
 *
 * \sa replicationFactor()
 * \sa unsetReplicationFactor()
 * \sa hasReplicationFactor()
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
 *
 * \sa setReplicationFactor()
 * \sa replicationFactor()
 * \sa hasReplicationFactor()
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
 *
 * \sa setReplicationFactor()
 * \sa replicationFactor()
 * \sa unsetReplicationFactor()
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
 *
 * \sa setReplicationFactor()
 * \sa hasReplicationFactor()
 * \sa unsetReplicationFactor()
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
 *
 * \sa durableWrites()
 * \sa unsetDurableWrites()
 * \sa hasDurableWrites()
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
 *
 * \sa durableWrites()
 * \sa setDurableWrites()
 * \sa hasDurableWrites()
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
 *
 * \sa durableWrites()
 * \sa setDurableWrites()
 * \sa unsetDurableWrites()
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
 *
 * \sa hasDurableWrites()
 * \sa setDurableWrites()
 * \sa unsetDurableWrites()
 */
bool QCassandraContext::durableWrites() const
{
    if(f_private->__isset.durable_writes) {
        return f_private->durable_writes;
    }
    return false;
}

/** \brief This is an internal function used to parse a KsDef structure.
 *
 * This function is called internally to parse a KsDef object.
 *
 * \param[in] data  The pointer to the KsDef object.
 *
 * \sa prepareContextDefinition()
 */
void QCassandraContext::parseContextDefinition(const void *data)
{
    const org::apache::cassandra::KsDef *ks = reinterpret_cast<const org::apache::cassandra::KsDef *>(data);

    // name
    if(ks->name != f_private->name) {
        // what do we do here?
        throw std::logic_error("KsDef and QCassandraContextPrivate names don't match");
    }

    // strategy class
    f_private->__set_strategy_class(ks->strategy_class);

    // replication factor
    if(ks->__isset.replication_factor) {
        f_private->__set_replication_factor(ks->replication_factor);
    }
    else {
        f_private->__isset.replication_factor = false;
    }

    // durable writes
    if(ks->__isset.durable_writes) {
        f_private->__set_durable_writes(ks->durable_writes);
    }
    else {
        f_private->__isset.durable_writes = false;
    }

    // the options is an array that we keep on our end
    f_options.clear();
    if(ks->__isset.strategy_options) {
        for(std::map<std::string, std::string>::const_iterator
                    o = ks->strategy_options.begin();
                    o != ks->strategy_options.end();
                    ++o) {
            // TBD: can option strings include binary data?
            f_options.insert(o->first.c_str(), o->second.c_str());
        }
    }

    // table definitions (CfDef, column family definitions)
    f_tables.clear();
    for(std::vector<org::apache::cassandra::CfDef>::const_iterator
                cf = ks->cf_defs.begin(); cf != ks->cf_defs.end(); ++cf) {
        QCassandraTable::pointer_t t(table(cf->name.c_str()));
        const org::apache::cassandra::CfDef& cf_def = *cf;
        t->parseTableDefinition(&cf_def);
    }
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
void QCassandraContext::prepareContextDefinition(void *data) const
{
    org::apache::cassandra::KsDef *ks(reinterpret_cast<org::apache::cassandra::KsDef *>(data));
    *ks = *f_private;

    if(ks->strategy_class == "") {
        ks->strategy_class = "org.apache.cassandra.locator.LocalStrategy";
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
    for(QtCassandra::QCassandraTables::const_iterator
            t = f_tables.begin();
            t != f_tables.end();
            ++t)
    {
        org::apache::cassandra::CfDef cf;
        (*t)->prepareTableDefinition(&cf);
        ks->cf_defs.push_back(cf);
    }
    //if(ks->cf_defs.empty()) ... problem? it's not optional...
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
 * \note
 * If you just created a context, you want to call the
 * QCassandra::synchronizeSchemaVersions() function before calling this
 * function or you may get an exception saying that the context is not
 * availabe across your Cassandra cluster.
 *
 * \sa QCassandra::setContext()
 * \sa QCassandra::synchronizeSchemaVersions()
 */
void QCassandraContext::makeCurrent()
{
    if(!f_cassandra) {
        throw std::runtime_error("this context was dropped and is not attached to a cassandra cluster anymore");
    }

    // we need a shared pointer to the context and the only way to
    // get that is to retrieve it using our name... (somewhat slow
    // but I don't see a cleaner way to do it without generating a
    // pointer reference loop.)
    QCassandraContext::pointer_t me(f_cassandra->context(f_private->name.c_str()));
    f_cassandra->setCurrentContext(me);
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
 * \warning
 * After this call, if you are to use the context immediately, you want to
 * first call the synchronization function,
 * QCassandra::synchronizeSchemaVersions(), to make sure that all the nodes
 * are ready to use the new context. Otherwise you are likely to get errors
 * about things not being compatible or up to date.
 *
 * \sa QCassandraTable::create()
 * \sa QCassandra::synchronizeSchemaVersions()
 */
void QCassandraContext::create()
{
    if(!f_cassandra) {
        throw std::runtime_error("this context was dropped and is not attached to a cassandra cluster anymore");
    }

    f_cassandra->getPrivate()->createContext(*this);

    // If the user defined tables, we must mark them as loaded from Cassandra
    // which in this case would not otherwise happen!
    for(QtCassandra::QCassandraTables::const_iterator
            t = f_tables.begin();
            t != f_tables.end();
            ++t)
    {
        (*t)->setFromCassandra();
    }

    // TBD: Should we then call describe_keyspace() to make sure we've
    //      got the right data (defaults) in this object, tables, and
    //      column definitions?
    //
    //      Actually the describe_schema_versions() needs to be called
    //      to make sure that all the nodes are synchronized properly.
    //      This is done with the QCassandra::synchronizeSchemaVersions()
    //      function.
}

/** \brief Update a context with new properties.
 *
 * This function defines a new set of properties in the specified context.
 * In general, the context will be searched in the cluster definitions,
 * updated in memory then this function called.
 */
void QCassandraContext::update()
{
    if(!f_cassandra) {
        throw std::runtime_error("this context was dropped and is not attached to a cassandra cluster anymore");
    }

    f_cassandra->getPrivate()->updateContext(*this);
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
    if( !f_cassandra ) {
        throw std::runtime_error("this context was dropped and is not attached to a cassandra cluster anymore");
    }

    f_cassandra->getPrivate()->dropContext(*this);
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
 * (contrary to rows.) However, it can be a slow operation since all the
 * nodes need to be notified (i.e. consistency of ALL.) If you need to
 * know when the table is dropped from the entire cluster, call the
 * QCassandra::synchronizeSchemaVersions() function.
 *
 * \param[in] table_name  The name of the table to drop.
 *
 * \sa QCassandra::synchronizeSchemaVersions()
 */
void QCassandraContext::dropTable(const QString& table_name)
{
    if(f_tables.find(table_name) != f_tables.end()) {
        // keep a shared pointer on the table
        QCassandraTable::pointer_t t(table(table_name));

        // remove from the Cassandra database
        makeCurrent();
        f_cassandra->getPrivate()->dropTable(table_name);

        // disconnect all the cached data from this table
        t->unparent();
        f_tables.remove(table_name);
    }
}

/** \brief Create a Cassandra table.
 *
 * This function creates a Cassandra table by sending the corresponding
 * order to the connected server.
 *
 * The creation of a table is not instantaneous in all the nodes of your
 * cluster. When this function returns, the table was created in the node
 * you're connected with. To make sure that all the nodes are up to date
 * (and before using this newly created table) you must call the
 * QCassandra::synchronizeSchemaVersions() function. You may create any
 * number of tables at once, then call the synchronization function to
 * make sure that your entire cluster is ready.
 *
 * \param[in] table  The table definition used to create the Cassandra table.
 *
 * \sa QCassandra::synchronizeSchemaVersions()
 */
void QCassandraContext::createTable(const QCassandraTable *ptable)
{
    makeCurrent();
    f_cassandra->getPrivate()->createTable(ptable);
}

/** \brief Update a Cassandra table.
 *
 * This function updates a Cassandra table by sending the corresponding
 * order to the connected server.
 *
 * It can be used to define new column types or changing existing
 * columns (although changing existing columns may not work as
 * expected, from what I understand.)
 *
 * This function is not instantaneous and will slowly replicate to all
 * your nodes. In other words, you cannot access the table until the schema
 * is full propagated throughout your cluster. If you need to access the
 * table immediately after an update, make sure to call the
 * QCassandra::synchronizeSchemaVersions()
 *
 * \param[in] table  The table to update in the Cassandra server.
 *
 * \sa QCassandra::synchronizeSchemaVersions()
 */
void QCassandraContext::updateTable(const QCassandraTable *ptable)
{
    makeCurrent();
    f_cassandra->getPrivate()->updateTable(ptable);
}

/** \brief Truncate a Cassandra table.
 *
 * This function truncates a Cassandra table by sending the corresponding
 * order to the connected server.
 *
 * \param[in] table  The table to drop from the Cassandra server.
 */
void QCassandraContext::truncateTable(const QCassandraTable *ptable)
{
    makeCurrent();
    f_cassandra->getPrivate()->truncateTable(ptable);
}

/** \brief Insert a new value in the Cassandra database.
 *
 * This function inserts a new \p value in this Context of the Cassandra
 * database referenced by the row_key and column_key.
 *
 * \param[in] table_name  Name of the table where the value is inserted.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[in] value  The new value of the cell.
 */
void QCassandraContext::insertValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value)
{
    makeCurrent();
    f_cassandra->getPrivate()->insertValue(table_name, row_key, column_key, value);
}

/** \brief Retrieve a value from the Cassandra database.
 *
 * This function gets a \p value from the Cassandra database
 * referenced by the \p row_key and \p column_key.
 *
 * If the column is not found, then the \p value parameter is
 * set to the null value and the function returns false.
 *
 * \warning
 * This function does not work for counters, use getCounter() instead.
 *
 * \param[in] table_name  Name of the table where the value is inserted.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[out] value  The new value of the cell.
 *
 * \return false when the value was not found in the database, true otherwise
 *
 * \sa getCounter()
 */
bool QCassandraContext::getValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value)
{
    makeCurrent();
    try {
        f_cassandra->getPrivate()->getValue(table_name, row_key, column_key, value);
    }
    catch(const org::apache::cassandra::NotFoundException&) {
        value.setNullValue();
        return false;
    }
    return true;
}

/** \brief Retrieve a counter from the Cassandra database.
 *
 * This function gets a counter \p value from the Cassandra database
 * referenced by the \p table_name, \p row_key, and \p column_key.
 *
 * If the column is not found, then the \p value parameter is
 * set to zero and the function returns false.
 *
 * \warning
 * This function only works on counters, see getValue() for any other
 * type of data.
 *
 * \param[in] table_name  Name of the table where the value is inserted.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[out] value  The new value of the cell.
 *
 * \return false when the value was not found in the database, true otherwise
 *
 * \sa getValue()
 */
bool QCassandraContext::getCounter(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value)
{
    makeCurrent();
    try {
        f_cassandra->getPrivate()->getCounter(table_name, row_key, column_key, value);
    }
    catch(const org::apache::cassandra::NotFoundException&) {
        value.setInt64Value(0);
        return false;
    }
    return true;
}

/** \brief Add a value to a Cassandra counter.
 *
 * This function adds \p value to a Cassandra counter as
 * referenced by the \p row_key and \p column_key.
 *
 * \param[in] table_name  Name of the table where the value is inserted.
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[out] value  The value to add to this counter.
 */
void QCassandraContext::addValue(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t value)
{
    makeCurrent();
    f_cassandra->getPrivate()->addValue(table_name, row_key, column_key, value);
}

/** \brief Get a table slice.
 *
 * This function reads a table slice from the Cassandra database.
 *
 * \param[in] table_name  The name of the table where the row is check is defined.
 * \param[in] row_key  The row for which this data is being counted.
 * \param[in] column_predicate  The predicate to use to count the cells.
 *
 * \return The number of columns defined in the specified row.
 */
int32_t QCassandraContext::getCellCount(const QString& table_name, const QByteArray& row_key, const QCassandraColumnPredicate& column_predicate)
{
    makeCurrent();
    return f_cassandra->getPrivate()->getCellCount(table_name, row_key, column_predicate);
}

/** \brief Get a table slice.
 *
 * This function reads a table slice from the Cassandra database.
 *
 * \note
 * The column_predicate is an [in,out] parameter because the start column
 * name is set to the name of the last column read. This allows for reading
 * all the columns of a row used as an index.
 *
 * \param[in] table  The table where the results is saved.
 * \param[in] row_key  The row for which this data is being counted.
 * \param[in,out] column_predicate  The predicate to use to count the cells.
 *
 * \return The number of columns read from Cassandra.
 */
uint32_t QCassandraContext::getColumnSlice(QCassandraTable& rtable, const QByteArray& row_key, QCassandraColumnPredicate& column_predicate)
{
    makeCurrent();
    return f_cassandra->getPrivate()->getColumnSlice(rtable, row_key, column_predicate);
}

/** \brief Remove a cell from the Cassandra database.
 *
 * This function calls the Cassandra server to remove a cell in the Cassandra
 * database.
 *
 * \param[in] table_name  The name of the column where the row is defined.
 * \param[in] row_key  The row in which the cell is to be removed, if empty all the rows.
 * \param[in] column_key  The cell to be removed, if empty all the cells.
 * \param[in] timestamp  The time when the key to be removed was created.
 * \param[in] consistency_level  The consistency level to use to remove this cell.
 */
void QCassandraContext::remove(const QString& table_name, const QByteArray& row_key, const QByteArray& column_key, int64_t timestamp, consistency_level_t consistency_level)
{
    makeCurrent();
    f_cassandra->getPrivate()->remove(table_name, row_key, column_key, timestamp, consistency_level);
}

/** \brief Retrieve a slice of rows from Cassandra.
 *
 * This function calls the Cassandra server to retrieve a set of rows as
 * defined by the row predicate. These rows get cells as defined by the
 * column predicate defined inside the row predicate object.
 *
 * Note that the function updates the predicate so the next call returns
 * the following rows as expected.
 *
 * \param[in] table  The table which is emitting this call.
 * \param[in,out] row_predicate  The predicate used to select the rows.
 *
 * \return The number of rows read on this call.
 */
uint32_t QCassandraContext::getRowSlices(QCassandraTable& rtable, QCassandraRowPredicate& row_predicate)
{
    makeCurrent();
    return f_cassandra->getPrivate()->getRowSlices(rtable, row_predicate);
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
    // lose all the tables
    for(QCassandraTables::iterator ti(f_tables.begin()); ti != f_tables.end(); ++ti) {
        (*ti)->unparent();
    }
    f_tables.clear();

    // then reload those tables that still exist in this context
    f_cassandra->getPrivate()->retrieve_context(f_private->name.c_str());
}

/** \brief Synchronize the schema versions.
 *
 * This function calls the cassandra synchronizeSchemaVersions(). This can be
 * called by the context children as required.
 *
 * If the context was already unparented, then nothing happens.
 *
 * \sa unparent()
 * \sa QCassandra::synchronizeSchemaVersions()
 */
void QCassandraContext::synchronizeSchemaVersions()
{
    if(f_cassandra) {
        f_cassandra->synchronizeSchemaVersions();
    }
}

/** \brief Unparent the context.
 *
 * This function is called internally to mark the context as unparented.
 * This means you cannot use it anymore. This happens whenever you
 * call the dropContext() funtion on a QCassandra object.
 *
 * \sa QCassandra::dropContext()
 */
void QCassandraContext::unparent()
{
    f_cassandra.reset();
    clearCache();
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
 * This function retrieves the "libQtCassandraLockTable" table to use
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
    lock_table->setColumnType("Standard");
    lock_table->setKeyValidationClass("BytesType");
    lock_table->setDefaultValidationClass("BytesType");
    lock_table->setComparatorType("BytesType");
    lock_table->setKeyCacheSavePeriodInSeconds(14400);
    lock_table->setMemtableFlushAfterMins(60);
    lock_table->setGcGraceSeconds(3600);
    lock_table->setMinCompactionThreshold(4);
    lock_table->setMaxCompactionThreshold(22);
    lock_table->setReplicateOnWrite(1);
    lock_table->create();

    // we create the table when needed and then use it ASAP so we need
    // to synchronize; since that happens just once per context the hit
    // isn't that bad
    synchronizeSchemaVersions();

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
    QCassandraColumnRangePredicate hosts_predicate;
    hosts_predicate.setConsistencyLevel(CONSISTENCY_LEVEL_QUORUM);
    hosts_predicate.setCount(hosts_count);
    hosts_row->readCells(hosts_predicate);
    const QCassandraCells& hosts(hosts_row->cells());
    // note: there is an interesting way to find one or more missing numbers
    //       in a list which involves polynomials, however, at this point we
    //       do not know the largest number and thus how many numbers are
    //       missing in the list (if any); plus it becomes very expensive
    //       when many numbers are missing; so instead we use a vector of
    //       booleans which anyway is a lot simpler to implement and maintain
    //       See: http://stackoverflow.com/questions/3492302/easy-interview-question-got-harder-given-numbers-1-100-find-the-missing-numbe
    std::vector<controlled_vars::flbool_t> set;
    std::vector<controlled_vars::flbool_t>::size_type size(0);
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
        std::vector<controlled_vars::flbool_t>::const_iterator it(std::find(set.begin() + 1, set.end(), false));
        new_id = it - set.begin();
    }
    QCassandraValue value(new_id);
    value.setConsistencyLevel(CONSISTENCY_LEVEL_QUORUM);
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
    c->setConsistencyLevel(CONSISTENCY_LEVEL_QUORUM);
    row->dropCell(host_name, QCassandraValue::TIMESTAMP_MODE_DEFINED, QCassandra::timeofday());
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
    return f_cassandra;
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
 * The lock table name is set to "libQtCassandraLockTable"
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
