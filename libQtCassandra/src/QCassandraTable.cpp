/*
 * Text:
 *      QCassandraTable.cpp
 *
 * Description:
 *      Handling of the cassandra::CfDef (Column Family Definition).
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

#include "QtCassandra/QCassandra.h"
#include "QtCassandra/QCassandraTable.h"
#include "QtCassandra/QCassandraContext.h"

#include "legacy/cassandra_types.h"

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <QtCore>


namespace QtCassandra
{


/** \class QCassandraTable
 * \brief Defines a table and may hold a Cassandra column family definition.
 *
 * In Cassandra, a table is called a column family. Mainly because
 * each row in a Cassandra table can have a different set of columns
 * whereas a table is usually viewed as a set of rows that all have
 * the exact same number of columns (but really, even in SQL the set
 * of columns can be viewed as varying since columns can be set to
 * NULL and that has [nearly] the same effect as not defining a column
 * in Cassandra.)
 *
 * This class defines objects that can hold all the column family
 * information so as to create new ones and read existing ones.
 *
 * The name of a table is very limited (i.e. letters, digits, and
 * underscores, and the name must start with a letter.) The maximum
 * length is not specified, but it is relatively short as it is
 * used as a filename that will hold the data of the table.
 *
 * Whenever trying to get a row, the default behavior is to create
 * a new row if it doesn't exist yet. If you need to know whether a
 * row exists, make sure you use the exists() function.
 *
 * \note
 * A table can be created, updated, and dropped. In all those cases, the
 * functions return once the Cassandra instance with which you are
 * connected is ready. However, that is not enough if you are working with
 * a cluster because the other nodes do not get updated instantaneously.
 * Instead, you have to call the QCassandra::synchronizeSchemaVersions()
 * function of the QCassandra object to make sure that the table is fully
 * available across your cluster.
 *
 * \sa exists()
 * \sa QCassandra::synchronizeSchemaVersions()
 */


/** \var QCassandraTable::f_from_cassandra
 * \brief Whether the table is a memory table or a server table.
 *
 * A table read from the Cassandra server or created with the
 * create() function is marked as being from Cassandra.
 * All other tables are considered memory tables.
 *
 * A memory table can be used as a set of global variables
 * with a format similar to a Cassandra table.
 *
 * If you define a new table with the intend to call the
 * create() function, avoid saving data in the new table
 * as it won't make it to the database. (This may change
 * in the future though.)
 */

/** \var QCassandraTable::f_private
 * \brief The table private data: CfDef
 *
 * A table is always part of a specific context. You can only create a
 * new table using a function from your context objects.
 *
 * This is a bare pointer since you cannot delete the context and hope
 * the table remains (i.e. when the context goes, the table goes!)
 */

/** \var QCassandraTable::f_context
 * \brief The context that created this table.
 *
 * A table is always part of a specific context. You can only create a
 * new table using a function from your context objects.
 *
 * This is a bare pointer since you cannot delete the context and hope
 * the table remains (i.e. when the context goes, the table goes!)
 * However, you may keep a shared pointer to a table after the table
 * was deleted. In that case, the f_context pointer is set to NULL
 * and calling functions on that table may result in an exception
 * being raised.
 */

/** \var QCassandraTable::f_rows
 * \brief Set of rows.
 *
 * The system caches rows in this map. The map index is the row key. You can
 * clear the table using the clearCache() function.
 */

/** \brief Initialize a QCassandraTable object.
 *
 * This function initializes a QCassandraTable object.
 *
 * All the parameters are set to the defaults as defined in the Cassandra
 * definition of the CfDef message. You can use the different functions to
 * change the default values.
 *
 * Note that the context and table name cannot be changed later. These
 * are fixed values that follow the Cassandra behavior.
 *
 * A table name must be composed of letters (A-Za-z), digits (0-9) and
 * underscores (_). It must start with a letter. The corresponding lexical
 * expression is: /^[A-Za-z][A-Za-z0-9_]*$\/
 *
 * The length of a table name is also restricted, however it is not specified
 * by Cassandra. I suggest you never use a name of more than 200 letters.
 * The length is apparently limited by the number of characters that can be
 * used to name a file on the file system you are using.
 *
 * \param[in] context  The context where this table definition is created.
 * \param[in] table_name  The name of the table definition being created.
 */
QCassandraTable::QCassandraTable(QCassandraContext::pointer_t context, const QString& table_name)
    //f_from_cassandra(false) -- auto-init
    : f_private(new CfDef)
    , f_context(context)
    //f_rows() -- auto-init
{
    // verify the name here (faster than waiting for the server and good documentation)
    QRegExp re("[A-Za-z][A-Za-z0-9_]*");
    if(!re.exactMatch(table_name)) {
        throw std::runtime_error("invalid table name (does not match [A-Za-z][A-Za-z0-9_]*)");
    }

    // we save the context name (keyspace) and since it's forbidden to change it
    // in the context, we know it won't change here either
    const QString keyspace = context->contextName();
    f_private->__set_keyspace(keyspace.toStdString());

    // we save the name and at this point we prevent it from being changed.
    f_private->__set_name(table_name.toStdString());

    f_session = f_context->parentCassandra()->session();
}

/** \brief Clean up the QCassandraTable object.
 *
 * This function ensures that all resources allocated by the
 * QCassandraTable are released.
 *
 * Note that does not in any way destroy the table in the
 * Cassandra cluster.
 */
QCassandraTable::~QCassandraTable()
{
}

/** \brief Return the name of the context attached to this table definition.
 *
 * This function returns the name of the context attached to this table
 * definition.
 *
 * Note that it is not possible to rename a context and therefore this
 * name will never change.
 *
 * To get a pointer to the context, use the cluster function context()
 * with this name. Since each context is unique, it will always return
 * the correct pointer.
 *
 * \return The name of the context this table definition is defined in.
 */
QString QCassandraTable::contextName() const
{
    return f_private->keyspace.c_str();
}

/** \brief Retrieve the name of this table.
 *
 * This function returns the name of this table. Note that the
 * name cannot be changed.
 *
 * \return The table name.
 */
QString QCassandraTable::tableName() const
{
    return f_private->name.c_str();
}


/** \brief Define the table identifier.
 *
 * This function is provided to let the user choose an identifier
 * for his tables. Identifiers must be unique and in general it
 * is better to let the system define an identifier for you.
 *
 * \note
 * From my understanding the identifier generated by the Cassandra
 * system helps in properly manage the content of the table within
 * your cluster. This is why it is safer not to temper with this
 * information. However, if you enlarge or shrink your cluster, it
 * may be necessary to do so.
 *
 * \param[in] identifier  The identifier to use for this table.
 */
void QCassandraTable::setIdentifier(int32_t val)
{
    f_private->__set_id(val);
}

/** \brief Unset the table definition identifier.
 *
 * This function marks the table identifier as unset. This
 * is the default when creating a new table as you expect
 * the Cassandra system to generate the necessary identifier.
 */
void QCassandraTable::unsetIdentifier()
{
    f_private->__isset.id = false;
}

/** \brief Check whether the identifier is defined.
 *
 * This function retrieves the current status of the identifier parameter.
 *
 * \return True if the identifier parameter is defined.
 */
bool QCassandraTable::hasIdentifier() const
{
    return f_private->__isset.id;
}

/** \brief Retrieve the table definition identifier.
 *
 * Each table is identified by a unique number. This function
 * returns that unique number.
 *
 * If somehow undefined, the function returns zero.
 *
 * \return The table identifier.
 */
int32_t QCassandraTable::identifier() const
{
    if(f_private->__isset.id) {
        return f_private->id;
    }
    return 0;
}

/** \brief Set the table comment.
 *
 * This function saves a new comment in the table definition.
 *
 * This is any human readable comment that you want to attach with
 * this table. It is mainly useful for documentation purpose.
 *
 * \param[in] comment  The new table comment.
 */
void QCassandraTable::setComment(QString val)
{
    f_private->__set_comment(val.toStdString());
}

/** \brief Unset a comment.
 *
 * This function marks the comment as not set.
 */
void QCassandraTable::unsetComment()
{
    f_private->__isset.comment = false;
}

/** \brief Check whether the comment is defined.
 *
 * This function retrieves the current status of the comment parameter.
 *
 * \return True if the comment parameter is defined.
 */
bool QCassandraTable::hasComment() const
{
    return f_private->__isset.comment;
}

/** \brief Retrieve the table comment.
 *
 * This function retrieves the comment assigned to this table.
 *
 * \return A string with the table definition comment.
 *
 * \sa unsetComment()
 * \sa setComment()
 */
QString QCassandraTable::comment() const
{
    if(f_private->__isset.comment) {
        return f_private->comment.c_str();
    }
    return "";
}

/** \brief Set the column type on this table.
 *
 * The default column type is "Standard". It may also be set to "Super".
 *
 * \exception std::runtime_error
 * This error is generated whenever the column_type parameter is not set
 * to either "Standard" or "Super".
 *
 * \param[in] column_type  The named type of this table definition column.
 */
void QCassandraTable::setColumnType(const QString& column_type)
{
    if(column_type != "Standard" && column_type != "Super") {
        throw std::runtime_error("the type of a column can be either \"Standard\" or \"Super\".");
    }

    f_private->__set_column_type(column_type.toStdString());
}

/** \brief Unset the column type on this table.
 *
 * This function clears the set of the column type.
 */
void QCassandraTable::unsetColumnType()
{
    f_private->__isset.column_type = false;
}

/** \brief Check whether the comlumn type is defined.
 *
 * This function retrieves the current status of the comlumn type parameter.
 *
 * \return True if the comlumn type parameter is defined.
 */
bool QCassandraTable::hasColumnType() const
{
    return f_private->__isset.column_type;
}

/** \brief Retrieve the current column type.
 *
 * This function returns the current column type of this table definition.
 *
 * \return The named type of the table definition column.
 */
QString QCassandraTable::columnType() const
{
    if(f_private->__isset.column_type) {
        return f_private->column_type.c_str();
    }
    return "";
}

/** \brief Set the default validation class to create a counters table.
 *
 * This function is a specialized version of the setDefaultValidationClass()
 * with the name of the class necessary to create a table of counters. Remember
 * that once in this state a table cannot be converted.
 *
 * This is equivalent to setDefaultValidationClass("CounterColumnType").
 */
void QCassandraTable::setDefaultValidationClassForCounters()
{
    setDefaultValidationClass("CounterColumnType");
}

/** \brief Set the default validation class.
 *
 * This function defines the default validation class for the table columns.
 * By default it is set to binary (BytesType), which is similar to saying
 * no validation is required.
 *
 * The CLI documentation says that the following are valid as a default
 * validation class:
 *
 * AsciiType, BytesType, CounterColumnType, IntegerType, LexicalUUIDType,
 * LongType, UTF8Type
 *
 * \param[in] validation_class  The default validation class for columns data.
 */
void QCassandraTable::setDefaultValidationClass(const QString& validation_class)
{
    f_private->__set_default_validation_class(validation_class.toStdString());
}

/** \brief Unset the default validation class.
 *
 * This function removes the effects of setDefaultValidationClass() calls.
 */
void QCassandraTable::unsetDefaultValidationClass()
{
    f_private->__isset.default_validation_class = false;
}

/** \brief Check whether the default validation class is defined.
 *
 * This function retrieves the current status of the default validation class parameter.
 *
 * \return True if the default validation class parameter is defined.
 */
bool QCassandraTable::hasDefaultValidationClass() const
{
    return f_private->__isset.default_validation_class;
}

/** \brief Retrieve the default validation class.
 *
 * This function retrieves the default validation class for the columns
 * of this table.
 *
 * \return The validation class name.
 */
QString QCassandraTable::defaultValidationClass() const
{
    if(f_private->__isset.default_validation_class) {
        return f_private->default_validation_class.c_str();
    }
    return "";
}

/** \brief Set the table name validation class.
 *
 * The table name is called a key. This key can be used in queries and as
 * such may need to be of a very specific type. This function can be used to
 * define that type.
 *
 * The supported types for a table key are:
 *
 * AsciiType, BytesType, IntegerType, LexicalUUIDType, LongType, UTF8Type
 *
 * The default is BytesType, which means not validation is applied to the key.
 *
 * \param[in] validation_class  The new key validation class for this table name.
 */
void QCassandraTable::setKeyValidationClass(const QString& validation_class)
{
    f_private->__set_key_validation_class(validation_class.toStdString());
}

/** \brief Unset the table name validation class.
 *
 * This function marks the key validation class parameter as unset so it
 * doesn't get sent to the Cassandra serer.
 */
void QCassandraTable::unsetKeyValidationClass()
{
    f_private->__isset.key_validation_class = false;
}

/** \brief Check whether the key validation class is defined.
 *
 * This function retrieves the current status of the key validation class parameter.
 *
 * \return True if the key validation class parameter is defined.
 */
bool QCassandraTable::hasKeyValidationClass() const
{
    return f_private->__isset.key_validation_class;
}

/** \brief Retrieve the current validation class for the table name.
 *
 * This function returns the current validation class for the table name.
 *
 * \return The current validation class for the table name.
 */
QString QCassandraTable::keyValidationClass() const
{
    if(f_private->__isset.key_validation_class) {
        return f_private->key_validation_class.c_str();
    }
    return "";
}

/** \brief Set the alias for the key.
 *
 * Each table name can make use of one key alias as defined by this entry.
 *
 * This is used by CQL at this point so the key can be represented by a
 * name other than KEY. This way it looks a little more like a column
 * name. Note that the alias cannot match the name of any one column.
 *
 * \param[in] key_alias  The new key alias to use with the table definition.
 */
void QCassandraTable::setKeyAlias(const QString& key_alias)
{
    f_private->__set_key_alias(key_alias.toStdString());
}

/** \brief Unset the alias for the key.
 *
 * Cancel the effect of a previous setKeyAlias() call.
 */
void QCassandraTable::unsetKeyAlias()
{
    f_private->__isset.key_alias = false;
}

/** \brief Check whether the key alias is defined.
 *
 * This function retrieves the current status of the key alias parameter.
 *
 * \return True if the key alias parameter is defined.
 */
bool QCassandraTable::hasKeyAlias() const
{
    return f_private->__isset.key_alias;
}

/** \brief Retrieve the key alias for this table.
 *
 * This function returns the alias of this table name.
 *
 * This is only used by CQL at this point.
 *
 * \return The key alias.
 *
 * \sa unsetKeyAlias()
 * \sa setKeyAlias()
 */
QString QCassandraTable::keyAlias() const
{
    if(f_private->__isset.key_alias) {
        return f_private->key_alias.c_str();
    }
    return "";
}

/** \brief Define the column comparator type.
 *
 * Define the type of comparator to use to compare columns in queries.
 *
 * The default comparator is "BytesType".
 *
 * The type must be one of the supported Cassandra types (\sa setColumnType())
 * or the fully qualified name of a class you added.
 *
 * A common column type is BytesType for binary data. In most cases this will
 * be used as the default.
 *
 * The internal types available are:
 *
 * (those marked with an asterisk (*) are defined in the CLI documentation
 * for CREATE COLUMN FAMILY function; if they are not defined there, it is
 * either not available or that documentation was not up to date)
 *
 * \li AsciiType (*)
 * \li BooleanType
 * \li BytesType (*)
 * \li CounterColumnType (*)
 * \li DateType
 * \li DoubleType
 * \li FloatType
 * \li IntegerType (*)
 * \li LexicalUUIDType (*)
 * \li LongType (*)
 * \li TimeUUIDType
 * \li UTF8Type (*)
 * \li UUIDType
 *
 * Note that all these types are not available for all the different keys.
 *
 * The composite types can be defined using the following two names. These are
 * used with a list of parameters which are basic types. This list defines the
 * type of each key defined in columns. Since you define a composite type at
 * the level of a table, all the columns are assigned that exact same composite
 * type. To access the columns, you must use the compositeCell() functions on
 * the QCassandraRow. The default column access will fail unless you know how
 * to create the QArrayByte key.
 *
 * \li CompositeType
 * \li DynamicCompositeType
 *
 * A composite type may define three columns as follow:
 *
 * \code
 * CompositeType(UTF8Type, IntegerType, UUIDType);
 * \endcode
 *
 * Some types that are defined but I have no clue what they mean:
 *
 * \li LocalByPartitionerType (TBD?)
 *
 * \note
 * This function does not check the validity of the parameter since it has no
 * way of knowing whether the parameter is a valid type.
 *
 * \param[in] comparator_type  The type of the column comparator.
 */
void QCassandraTable::setComparatorType(const QString& comparator_type)
{
    f_private->__set_comparator_type(comparator_type.toStdString());
}

/** \brief Cancel calls to the setComparatorType() function.
 *
 * This function resets the comparator flag to false so it looks like the
 * comparator wasn't set. The default or current value will be used instead.
 */
void QCassandraTable::unsetComparatorType()
{
    f_private->__isset.comparator_type = false;
}

/** \brief Check whether the comparator type is defined.
 *
 * This function retrieves the current status of the comparator type parameter.
 *
 * \return True if the comparator type parameter is defined.
 */
bool QCassandraTable::hasComparatorType() const
{
    return f_private->__isset.comparator_type;
}

/** \brief Retrieve the comparator type for this table name.
 *
 * This function returns a copy of the comparator type.
 *
 * \return The comparator type.
 */
QString QCassandraTable::comparatorType() const
{
    if(f_private->__isset.comparator_type) {
        return f_private->comparator_type.c_str();
    }
    return "";
}

/** \brief Set the sub-comparator type.
 *
 * This function sets the sub-comparator type of this table definition.
 * The sub-comparator is used against super columns data.
 *
 * You cannot define the sub-comparator type on a Standard table. Only
 * Super tables accepts this parameter.
 *
 * The default is undefined.
 *
 * \param[in] subcomparator_type  The new sub-comparator type.
 */
void QCassandraTable::setSubcomparatorType(const QString& subcomparator_type)
{
    f_private->__set_subcomparator_type(subcomparator_type.toStdString());
}

/** \brief Unset the sub-comparator type specification.
 *
 * This function marks the sub-comparator type as unset.
 */
void QCassandraTable::unsetSubcomparatorType()
{
    f_private->__isset.subcomparator_type = false;
}

/** \brief Check whether the sub-comparator type is defined.
 *
 * This function retrieves the current status of the sub-comparator type parameter.
 *
 * \return True if the sub-comparator type parameter is defined.
 */
bool QCassandraTable::hasSubcomparatorType() const
{
    return f_private->__isset.subcomparator_type;
}

/** \brief Return the current sub-comparator type.
 *
 * This function retrieves the sub-comparator type and return its
 * name. If no sub-comparator is defined, an empty string is
 * returned ("").
 *
 * \return The sub-comparator or an empty string.
 */
QString QCassandraTable::subcomparatorType() const
{
    if(f_private->__isset.subcomparator_type) {
        return f_private->subcomparator_type.c_str();
    }
    return "";
}

/** \brief Retrieve a column definition by name.
 *
 * This function is used to retrieve a column definition by name.
 * If the column doesn't exist, it gets created.
 *
 * You can test whether the result is null with the operator bool() function
 * of the std::shared_ptr<> class.
 *
 * \param[in] column_name  The name of the column definition to retrieve.
 *
 * \return A shared pointer to the table found or a null shared pointer.
 */
QCassandraColumnDefinition::pointer_t QCassandraTable::columnDefinition(const QString& column_name)
{
    // column already exists?
    QCassandraColumnDefinitions::iterator ci(f_column_definitions.find(column_name));
    if(ci != f_column_definitions.end()) {
        return ci.value();
    }

    // this is a new column, allocate it
    QCassandraColumnDefinition::pointer_t c(new QCassandraColumnDefinition(shared_from_this(), column_name));
    f_column_definitions.insert(column_name, c);
    return c;
}

/** \brief Return the a reference to the column definitions.
 *
 * This function gives you direct access to the internal map of column
 * definitions to avoid a copy of the map. Note that if you change
 * the definitions in any way, this map may become invalid. To be
 * safe, it is wise to make a copy.
 *
 * The map is indexed by column names.
 *
 * \return A reference to the internal map of column definitions.
 */
const QCassandraColumnDefinitions& QCassandraTable::columnDefinitions() const
{
    return f_column_definitions;
}

/** \brief Set the caching mode.
 *
 * This function sets the caching mode of this table definition.
 * The caching is used to know what is kept in memory.
 *
 * \since Cassandra 1.1.0
 *
 * \param[in] caching  The name of the caching mode to use for this table.
 */
void QCassandraTable::setCaching(const QString& val)
{
    f_private->__set_caching(val.toStdString());
}

/** \brief Unset the caching mode.
 *
 * This function marks the caching mode as unset.
 *
 * \since Cassandra 1.1.0
 */
void QCassandraTable::unsetCaching()
{
    f_private->__isset.caching = false;
}

/** \brief Check whether the caching mode is defined.
 *
 * This function retrieves the current status of the caching parameter.
 *
 * \return True if the caching parameter is defined.
 */
bool QCassandraTable::hasCaching() const
{
    return f_private->__isset.caching;
}

/** \brief Return the current caching mode.
 *
 * This function retrieves the caching mode and return its
 * name. If no caching is defined, an empty string is
 * returned ("").
 *
 * \since Cassandra 1.1.0
 *
 * \return The caching or an empty string.
 */
QString QCassandraTable::caching() const
{
    if(f_private->__isset.caching) {
        return f_private->caching.c_str();
    }
    return "";
}

/** \brief Define the number of rows of this type to cache.
 *
 * This function defines the number of rows from this table
 * that should be cached.
 *
 * By default this is set to zero as by default no rows are
 * being cached. A very large table may benefit from a cache
 * but really only if you know that the same row(s) will be
 * hit over and over again.
 *
 * The size can either be expressed in a percent (0.01 to 0.99)
 * or as an absolute number of rows (200000).
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] size  The size required for this table row cache.
 */
void QCassandraTable::setRowCacheSize(double size)
{
    f_private->__set_row_cache_size(size);
}

/** \brief Unset the row cache size.
 *
 * This function marks the row cache size as unset.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 */
void QCassandraTable::unsetRowCacheSize()
{
    f_private->__isset.row_cache_size = false;
}

/** \brief Check whether the row cache size is defined.
 *
 * This function retrieves the current status of the row cache size parameter.
 *
 * \return True if the row cache size parameter is defined.
 */
bool QCassandraTable::hasRowCacheSize() const
{
    return f_private->__isset.row_cache_size;
}

/** \brief Retrieve the current row cache size.
 *
 * This function returns the current size of the row cache.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return The current row cache size.
 */
double QCassandraTable::rowCacheSize() const
{
    if(f_private->__isset.row_cache_size) {
        return f_private->row_cache_size;
    }
    return 0.0;
}

/** \brief Set the cache save period for rows.
 *
 * It is possible, with Cassandra, to save the cache to disk.
 * This allows you to restart Cassandra with a non-empty cache.
 * Whether this is useful will very much depend on your data.
 *
 * If you rarely restart Cassandra, then a larger number is better.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] seconds  The number of seconds the cache is retained for.
 */
void QCassandraTable::setRowCacheSavePeriodInSeconds(int32_t seconds)
{
    f_private->__set_row_cache_save_period_in_seconds(seconds);
}

/** \brief Mark the row cache save period as unset.
 *
 * This function marks the row cache save period in seconds as
 * not set at all.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 */
void QCassandraTable::unsetRowCacheSavePeriodInSeconds()
{
    f_private->__isset.row_cache_save_period_in_seconds = false;
}

/** \brief Check whether the row cache save periond in seconds is defined.
 *
 * This function retrieves the current status of the row save periond in seconds size parameter.
 *
 * \return True if the row cache save periond in seconds parameter is defined.
 */
bool QCassandraTable::hasRowCacheSavePeriodInSeconds() const
{
    return f_private->__isset.row_cache_save_period_in_seconds;
}

/** \brief Retreive the row cache save period.
 *
 * This function retrieves the row cache save period in seconds.
 *
 * The value may be 0.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return The cache save period in seconds.
 */
int32_t QCassandraTable::rowCacheSavePeriodInSeconds() const
{
    if(f_private->__isset.row_cache_save_period_in_seconds) {
        return f_private->row_cache_save_period_in_seconds;
    }
    return 0;
}

/** \brief Define the size of the cache key.
 *
 * Set the size of the cache where keys are saved. This cache is used to quickly
 * search the location of rows. This is similar to an in memory index.
 *
 * The size can either be expressed in a percent (0.01 to 0.99) or as an
 * absolute number of keys (200,000).
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] size  The size of the key cache.
 */
void QCassandraTable::setKeyCacheSize(double size)
{
    f_private->__set_key_cache_size(size);
}

/** \brief Unset the key cache size.
 *
 * This function removes the effect of previous calls to the setKeyCacheSize()
 * function.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 */
void QCassandraTable::unsetKeyCacheSize()
{
    f_private->__isset.key_cache_size = false;
}

/** \brief Check whether the key cache size is defined.
 *
 * This function retrieves the current status of the key cache size parameter.
 *
 * \return True if the key cache size parameter is defined.
 */
bool QCassandraTable::hasKeyCacheSize() const
{
    return f_private->__isset.key_cache_size;
}

/** \brief Get the size of the key cache.
 *
 * This function returns the definition of the key cache.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return Cache size either as a % value (1.0 or less) or a Mb value (over 1.0)
 */
double QCassandraTable::keyCacheSize() const
{
    if(f_private->__isset.key_cache_size) {
        return f_private->key_cache_size;
    }
    return 0.0;
}

/** \brief Define the amount of time the key cache is kept in memory.
 *
 * This function defines the number of seconds to wait before saving the
 * cached keys on disk. This is useful if you want to restart Cassandra
 * with a non empty cache.
 *
 * If you rarely restart Cassandra, a larger number is better.
 *
 * The default is 0.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] seconds  The number of seconds the data should be kept in memory.
 */
void QCassandraTable::setKeyCacheSavePeriodInSeconds(int32_t seconds)
{
    f_private->__set_key_cache_save_period_in_seconds(seconds);
}

/** \brief Unset the effect of calling setKeyCacheSavePeriodInSeconds().
 *
 * This function marks the key cache save period in seconds parameter
 * as not set.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 */
void QCassandraTable::unsetKeyCacheSavePeriodInSeconds()
{
    f_private->__isset.key_cache_save_period_in_seconds = false;
}

/** \brief Check whether the key cache save period in seconds is defined.
 *
 * This function retrieves the current status of the key cache save period in seconds parameter.
 *
 * \return True if the key cache save period in seconds parameter is defined.
 */
bool QCassandraTable::hasKeyCacheSavePeriodInSeconds() const
{
    return f_private->__isset.key_cache_save_period_in_seconds;
}

/** \brief Retrieve the key cache save period.
 *
 * This function returns the current number of seconds the cache should be
 * retained in memory.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return The number of seconds before saving the cache.
 */
int32_t QCassandraTable::keyCacheSavePeriodInSeconds() const
{
    if(f_private->__isset.key_cache_save_period_in_seconds) {
        return f_private->key_cache_save_period_in_seconds;
    }
    return 0;
}

/** \brief Define the row keys that should be cached.
 *
 * This function defines the number of row keys that should be cached.
 *
 * \since Cassandra version 1.0.0
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] count  The number of seconds the data should be kept in memory.
 */
void QCassandraTable::setRowCacheKeysToSave(int32_t count)
{
    f_private->__set_row_cache_keys_to_save(count);
}

/** \brief Mark the row cache keys to save as unset.
 *
 * This function marks the row cache keys to save parameter as not set.
 *
 * \since Cassandra version 1.0.0.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 */
void QCassandraTable::unsetRowCacheKeysToSave()
{
    f_private->__isset.row_cache_keys_to_save = false;
}

/** \brief Check whether the row cache keys to save is defined.
 *
 * This function retrieves the current status of the row cache keys to save parameter.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return True if the row cache keys to save parameter is defined.
 */
bool QCassandraTable::hasRowCacheKeysToSave() const
{
    return f_private->__isset.row_cache_keys_to_save;
}

/** \brief Retrieve the row cache keys to save value.
 *
 * This function returns the current value of the row cache keys to save.
 *
 * \since Cassandra version 1.0.0.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return The number of seconds before saving the cache.
 */
int32_t QCassandraTable::keyRowCacheKeysToSave() const
{
    if(f_private->__isset.row_cache_keys_to_save) {
        return f_private->row_cache_keys_to_save;
    }
    return 0;
}

/** \brief Define the probability of the bloom filter.
 *
 * This function defines the probability of the bloom filter.
 *
 * Bloom filtering is used to save I/O access.
 *
 * Note that this value is a double.
 *
 * \since Cassandra version 1.0.0
 *
 * \sa http://wiki.apache.org/cassandra/ArchitectureOverview#Write_path
 *
 * \param[in] chance  The probability used with the bloom filter.
 */
void QCassandraTable::setBloomFilterFPChance(double chance)
{
    f_private->__set_bloom_filter_fp_chance(chance);
}

/** \brief Mark the bloom filter probability as unset.
 *
 * This function marks the bloom filter probability parameter as not set.
 *
 * \since Cassandra version 1.0.0.
 *
 * \sa http://wiki.apache.org/cassandra/ArchitectureOverview#Write_path
 */
void QCassandraTable::unsetBloomFilterFPChance()
{
    f_private->__isset.bloom_filter_fp_chance = false;
}

/** \brief Check whether the bloom filter FP chance is defined.
 *
 * This function retrieves the current status of the bloom filter FP chance parameter.
 *
 * \return True if the bloom filter FP chance parameter is defined.
 */
bool QCassandraTable::hasBloomFilterFPChance() const
{
    return f_private->__isset.bloom_filter_fp_chance;
}

/** \brief Retrieve the bloom filter probability value.
 *
 * This function returns the current value of the bloom filter probability.
 * Note that this value is a double.
 *
 * \since Cassandra version 1.0.0.
 *
 * \sa http://wiki.apache.org/cassandra/ArchitectureOverview#Write_path
 *
 * \return The number of seconds before saving the cache.
 */
double QCassandraTable::bloomFilterFPChance() const
{
    if(f_private->__isset.bloom_filter_fp_chance) {
        return f_private->bloom_filter_fp_chance;
    }
    return 0.0;
}

/** \brief Set the read repair change value.
 *
 * This function can be used to change the read repair chance value.
 *
 * \param[in] repair_chance  The chance to repair data for a read.
 */
void QCassandraTable::setReadRepairChance(double repair_chance)
{
    f_private->__set_read_repair_chance(repair_chance);
}

/** \brief Unset the read repair chance.
 *
 * This function marks the read repair chance as unset.
 */
void QCassandraTable::unsetReadRepairChance()
{
    f_private->__isset.read_repair_chance = false;
}

/** \brief Check whether the read repair chance is defined.
 *
 * This function retrieves the current status of the read repair chance parameter.
 *
 * \return True if the read repair chance parameter is defined.
 */
bool QCassandraTable::hasReadRepairChance() const
{
    return f_private->__isset.read_repair_chance;
}

/** \brief Retrieve the current value of the read repair chance.
 *
 * This function returns the read repair chance value.
 *
 * \return The read repair chance value.
 */
double QCassandraTable::readRepairChance() const
{
    if(f_private->__isset.read_repair_chance) {
        return f_private->read_repair_chance;
    }
    return 0.0;
}

/** \brief Set the DC local read repair change value.
 *
 * This function can be used to change the DC local read repair chance value.
 *
 * \since Cassandra version 1.1.0.
 *
 * \param[in] repair_chance  The chance to repair local data for a read.
 */
void QCassandraTable::setDCLocalReadRepairChance(double repair_chance)
{
    f_private->__set_dclocal_read_repair_chance(repair_chance);
}

/** \brief Unset the read repair chance.
 *
 * This function marks the read repair chance as unset.
 *
 * \since Cassandra version 1.1.0.
 */
void QCassandraTable::unsetDCLocalReadRepairChance()
{
    f_private->__isset.dclocal_read_repair_chance = false;
}

/** \brief Check whether the read repair chance is defined.
 *
 * This function retrieves the current status of the read repair chance parameter.
 *
 * \since Cassandra version 1.1.0.
 *
 * \return True if the read repair chance parameter is defined.
 */
bool QCassandraTable::hasDCLocalReadRepairChance() const
{
    return f_private->__isset.dclocal_read_repair_chance;
}

/** \brief Retrieve the current value of the read repair chance.
 *
 * This function returns the read repair chance value.
 *
 * \since Cassandra version 1.1.0.
 *
 * \return The read repair chance value.
 */
double QCassandraTable::dcLocalReadRepairChance() const
{
    if(f_private->__isset.dclocal_read_repair_chance) {
        return f_private->dclocal_read_repair_chance;
    }
    return 0.0;
}

/** \brief Set whether replication occurs on write.
 *
 * This function defines the replication flag. On write you can
 * ask for counters to be replicated (the default) on other
 * nodes.
 *
 * \param[in] replicate_on_write  Whether counters are replicated on different nodes.
 */
void QCassandraTable::setReplicateOnWrite(bool replicate_on_write)
{
    f_private->__set_replicate_on_write(replicate_on_write);
}

/** \brief Unset the replicate on write flag.
 *
 * This function cancels the setReplicateOnWrite() function call by marking
 * the flag as unset.
 */
void QCassandraTable::unsetReplicateOnWrite()
{
    f_private->__isset.replicate_on_write = false;
}

/** \brief Check whether the replicate on write is defined.
 *
 * This function retrieves the current status of the replicate on write parameter.
 *
 * \return True if the replicate on write parameter is defined.
 */
bool QCassandraTable::hasReplicateOnWrite() const
{
    return f_private->__isset.replicate_on_write;
}

/** \brief Retrieve the current status of the replicate on write flag.
 *
 * This function retrieves the replicate on write flag for this table.
 * If false, the data is only saved on the node you're connected to.
 * If true, it will be duplicated as specified by the replication
 * level.
 *
 * \return The replicate on write flag status.
 */
bool QCassandraTable::replicateOnWrite() const
{
    if(f_private->__isset.replicate_on_write) {
        return f_private->replicate_on_write;
    }
    return false;
}

/** \brief Set the amount of chance that counters get merged.
 *
 * Counters may generate a contention. To break that contention, it is
 * possible to break them up into shards (counter sharding). This value
 * defines the chance that the system will reverse the process.
 *
 * The default is 0.0
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] merge_shards_chance  The chance that shards get merged.
 */
void QCassandraTable::setMergeShardsChance(double merge_shards_chance)
{
    f_private->__set_merge_shards_chance(merge_shards_chance);
}

/** \brief Unset the merge shards value.
 *
 * Mark the merge shards chance value as unset.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 */
void QCassandraTable::unsetMergeShardsChance()
{
    f_private->__isset.merge_shards_chance = false;
}

/** \brief Check whether the merge shards chance is defined.
 *
 * This function retrieves the current status of the merge shards chance parameter.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return True if the merge shards chance parameter is defined.
 */
bool QCassandraTable::hasMergeShardsChance() const
{
    return f_private->__isset.merge_shards_chance;
}

/** \brief Return the current merge shards chance value.
 *
 * This function returns the chance the system will merge counter
 * shards.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return The chance the system will merge counter shards.
 */
double QCassandraTable::mergeShardsChance() const
{
    if(f_private->__isset.merge_shards_chance) {
        return f_private->merge_shards_chance;
    }
    return 0.0;
}

/** \brief Set the row cache provider class name.
 *
 * The class to be used to as the row cache provider.
 *
 * It defaults to: org.apache.cassandra.cache.ConcurrentLinkedHashCacheProvider.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] provider  The row cache provider class name.
 */
void QCassandraTable::setRowCacheProvider(const QString& provider)
{
    f_private->__set_row_cache_provider(provider.toStdString());
}

/** \brief Mark the cache provider class name as unset.
 *
 * This function cancels the call to the setRowCacheProvider() function.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 */
void QCassandraTable::unsetRowCacheProvider()
{
    f_private->__isset.row_cache_provider = false;
}

/** \brief Check whether the row cache provider is defined.
 *
 * This function retrieves the current status of the row cache provider parameter.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return True if the row cache provider parameter is defined.
 */
bool QCassandraTable::hasRowCacheProvider() const
{
    return f_private->__isset.row_cache_provider;
}

/** \brief Retrieve the row cache provider.
 *
 * This function retrieves the row cache provider class name.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return The row cache provider.
 */
QString QCassandraTable::rowCacheProvider() const
{
    if(f_private->__isset.row_cache_provider) {
        return f_private->row_cache_provider.c_str();
    }
    return "";
}

/** \brief Defines the number of seconds to wait before forced garbage collection.
 *
 * Cassandra uses a garbage collection mechanism to manage its data.
 *
 * The default value for this parameter is 10 days (864000.) This value needs
 * to be large enough for your entire ring of nodes to clear their garbage
 * collection before a new cycle starts (don't ask me! I would think they
 * should have a flag to prevent two such collections... but there are
 * problems difficult to circumvent with potential hardware failures.)
 *
 * \param[in] seconds  How often tombstones are searched for.
 */
void QCassandraTable::setGcGraceSeconds(int32_t seconds)
{
    f_private->__set_gc_grace_seconds(seconds);
}

/** \brief Unset the garbage collection grace period value.
 *
 * This function marks the Garbage Collection Grace period as
 * unset.
 */
void QCassandraTable::unsetGcGraceSeconds()
{
    f_private->__isset.gc_grace_seconds = false;
}

/** \brief Check whether the gc grace seconds is defined.
 *
 * This function retrieves the current status of the gc grace seconds parameter.
 *
 * \return True if the gc grace seconds parameter is defined.
 */
bool QCassandraTable::hasGcGraceSeconds() const
{
    return f_private->__isset.gc_grace_seconds;
}

/** \brief Get the current garbage collect grace period.
 *
 * This value is used by the Cassandra system to force garbage collection
 * tombstones removals.
 *
 * \return The number of seconds to wait before each garbage collection recollections.
 */
int32_t QCassandraTable::gcGraceSeconds() const
{
    if(f_private->__isset.gc_grace_seconds) {
        return f_private->gc_grace_seconds;
    }
    return 0;
}

/** \brief Set the table flashing period.
 *
 * This function defines how often tables cached in memory get
 * flushed to disk (i.e. when they are that old, write a copy.)
 *
 * This flush function forces the flush whether or not the number of
 * operations or size limits were reached.
 *
 * It should be larger on production systems. Cassandra recommends
 * you to use 1 day (1,440 minutes.) The default is 60 (1h.)
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] minutes  The number of minutes a table will be kept in the memory cache.
 */
void QCassandraTable::setMemtableFlushAfterMins(int32_t minutes)
{
    f_private->__set_memtable_flush_after_mins(minutes);
}

/** \brief Unset the memtable flush period parameter.
 *
 * This function cancels the effects of the setMemtableFlushAfterMins() call.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \sa setMemtableFlushAfterMins()
 */
void QCassandraTable::unsetMemtableFlushAfterMins()
{
    f_private->__isset.memtable_flush_after_mins = false;
}

/** \brief Check whether the memory table flush after minutes is defined.
 *
 * This function retrieves the current status of the memory table flush after minutes parameter.
 *
 * \return True if the memory table flush after minutes parameter is defined.
 */
bool QCassandraTable::hasMemtableFlushAfterMins() const
{
    return f_private->__isset.memtable_flush_after_mins;
}

/** \brief Retrieve the memory table flush period.
 *
 * This function reads the number of minutes to wait before forcing
 * a flush of a table from memory. A table that is not being accessed
 * for that long gets removed.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return The number of minutes a cached table will be kept.
 */
int32_t QCassandraTable::memtableFlushAfterMins() const
{
    if(f_private->__isset.memtable_flush_after_mins) {
        return f_private->memtable_flush_after_mins;
    }
    return 0;
}

/** \brief Set the memtable throughput.
 *
 * This function sets the memtable throughput in megabytes. This means the
 * tables are flushed after that much data was written to them.
 *
 * A larger number is better especially if you send many writes to Cassandra
 * in random order (i.e. not sorted.) However, this represents a lot of
 * memory buffers and you want to avoid swapping.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] megabytes  The number of megabytes.
 */
void QCassandraTable::setMemtableThroughputInMb(int32_t megabytes)
{
    f_private->__set_memtable_throughput_in_mb(megabytes);
}

/** \brief Unset the memtable throughput value.
 *
 * This function marks the table definition as not having a memtable
 * throughput value.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 */
void QCassandraTable::unsetMemtableThroughputInMb()
{
    f_private->__isset.memtable_throughput_in_mb = false;
}

/** \brief Check whether the memory table throughput in Mb is defined.
 *
 * This function retrieves the current status of the memory table throughput in Mb parameter.
 *
 * \return True if the memory table throughput in Mb parameter is defined.
 */
bool QCassandraTable::hasMemtableThroughputInMb() const
{
    return f_private->__isset.memtable_throughput_in_mb;
}

/** \brief Get the memtable throughput.
 *
 * This function retrieve the current memtable throughput in megabytes.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return The memtable throughput in megabytes.
 */
int32_t QCassandraTable::memtableThroughputInMb() const
{
    if(f_private->__isset.memtable_throughput_in_mb) {
        return f_private->memtable_throughput_in_mb;
    }
    return 0;
}

/** \brief Operations limit flush.
 *
 * This function defines the number of operations that can be executed against
 * a memory table before it gets flushed.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \param[in] operations  The number of operations in millions.
 */
void QCassandraTable::setMemtableOperationsInMillions(int32_t operations)
{
    f_private->__set_memtable_operations_in_millions(operations);
}

/** \brief Unset the operations limit parameter.
 *
 * This function marks the memory table operations in millions as unset.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 */
void QCassandraTable::unsetMemtableOperationsInMillions()
{
    f_private->__isset.memtable_operations_in_millions = false;
}

/** \brief Check whether the memory table operations in millions is defined.
 *
 * This function retrieves the current status of the memory table operations in millions parameter.
 *
 * \return True if the memory table operations in millions parameter is defined.
 */
bool QCassandraTable::hasMemtableOperationsInMillions() const
{
    return f_private->__isset.memtable_operations_in_millions;
}

/** \brief Retrieve the current number of operations before flushing memory tables.
 *
 * This function returns the current number of operations to perform
 * on memory tables for this table before they get flushed.
 *
 * \deprecated
 * This function is ignored in Cassandra version 1.1+.
 *
 * \return The number of operations in millions before a flush occurs.
 */
int32_t QCassandraTable::memtableOperationsInMillions() const
{
    if(f_private->__isset.memtable_operations_in_millions) {
        return f_private->memtable_operations_in_millions;
    }
    return 0;
}

/** \brief Set the minimum compaction threshold.
 *
 * This function is used to change the minimum size to reach before
 * compacting data on disk.
 *
 * Data is saved in separate files. Reading multiple files is slow.
 * The compaction mechanism is used to concatenate multiple files
 * one after another. This makes the read of older data much faster.
 *
 * Data that constantly changes should have a larger threshold since
 * it otherwise would often break up the compaction anyway.
 *
 * Setting the compaction to zero disables this feature.
 *
 * \param[in] threshold  The threshold before compressing data.
 */
void QCassandraTable::setMinCompactionThreshold(int32_t threshold)
{
    f_private->__set_min_compaction_threshold(threshold);
}

/** \brief Mark the minimum compaction as unset.
 *
 * This function cancels calls to the setMinCompactionThreshold() function.
 */
void QCassandraTable::unsetMinCompactionThreshold()
{
    f_private->__isset.min_compaction_threshold = false;
}

/** \brief Check whether the minimum compaction threshold is defined.
 *
 * This function retrieves the current status of the minimum compaction threshold parameter.
 *
 * \return True if the minimum compaction threshold parameter is defined.
 */
bool QCassandraTable::hasMinCompactionThreshold() const
{
    return f_private->__isset.min_compaction_threshold;
}

/** \brief Retrieve the current minimum compaction threshold.
 *
 * This function reads the minimum compaction threshold.
 *
 * \return The minimum compaction threshold of this table.
 */
double QCassandraTable::minCompactionThreshold() const
{
    if(f_private->__isset.min_compaction_threshold) {
        return f_private->min_compaction_threshold;
    }
    return 0.0;
}

/** \brief Set the maximum compaction threshold.
 *
 * This function sets the maximum compaction threshold of
 * a table. When this limit is reached, then files are
 * concatenated to ensure better read access times.
 *
 * Setting the compaction to zero disables this feature.
 *
 * \param[in] threshold  The maximum threshold before compaction occurs.
 */
void QCassandraTable::setMaxCompactionThreshold(int32_t threshold)
{
    f_private->__set_max_compaction_threshold(threshold);
}

/** \brief Mark the maximum compaction as unset.
 *
 * This function marks the maximum compaction parameter as
 * unset.
 */
void QCassandraTable::unsetMaxCompactionThreshold()
{
    f_private->__isset.max_compaction_threshold = false;
}

/** \brief Check whether the maximum compaction threshold is defined.
 *
 * This function retrieves the current status of the maximum compaction threshold parameter.
 *
 * \return True if the maximum compaction threshold parameter is defined.
 */
bool QCassandraTable::hasMaxCompactionThreshold() const
{
    return f_private->__isset.max_compaction_threshold;
}

/** \brief Retrieve the current maximum compaction threshold.
 *
 * This function retrieves the current maximum compaction threshold.
 *
 * \return The maximum compaction threshold for this table.
 *
 * \sa setMaxCompactionThreshold()
 * \sa unsetMaxCompactionThreshold()
 */
double QCassandraTable::maxCompactionThreshold() const
{
    if(f_private->__isset.max_compaction_threshold) {
        return f_private->max_compaction_threshold;
    }
    return 0.0;
}

/** \brief Define the compaction strategy.
 *
 * This function defines the compaction strategy for your data.
 *
 * \since Cassandra version 1.0.0.
 *
 * \param[in] compaction_strategy  The name of the compaction strategy.
 */
void QCassandraTable::setCompactionStrategy(const QString& compaction_strategy)
{
    f_private->__set_compaction_strategy(compaction_strategy.toStdString());
}

/** \brief Unset the operations limit parameter.
 *
 * This function marks the compaction strategy as unset.
 *
 * \since Cassandra version 1.0.0.
 */
void QCassandraTable::unsetCompactionStrategy()
{
    f_private->__isset.compaction_strategy = false;
}

/** \brief Check whether the compaction strategy is defined.
 *
 * This function retrieves the current status of the compaction strategy parameter.
 *
 * \return True if the compaction strategy parameter is defined.
 */
bool QCassandraTable::hasCompactionStrategy() const
{
    return f_private->__isset.compaction_strategy;
}

/** \brief Retrieve the current compaction strategy.
 *
 * This function returns the current compaction strategy used for this table.
 *
 * \since Cassandra version 1.0.0.
 *
 * \return The compaction strategy.
 */
QString QCassandraTable::compactionStrategy() const
{
    if(f_private->__isset.compaction_strategy) {
        return f_private->compaction_strategy.c_str();
    }
    return "";
}

/** \brief Define a compaction strategy option.
 *
 * This function let you define a strategy option. You specify the name and the
 * value of the option. If the option was already defined, the new value is saved
 * and overwrites the existing value.
 *
 * \since Cassandra version 1.0.0.
 *
 * \param[in] option_name  The name of the compaction strategy option to set.
 * \param[in] value  The new value for this compaction strategy option.
 */
void QCassandraTable::setCompactionStrategyOption(const QString& option_name, const QString& value)
{
    // TBD: can any option make use of binary data?
    f_private->compaction_strategy_options[option_name.toStdString()] = value.toStdString();
}

/** \brief Unset the named compaction strategy option.
 *
 * This function removes the specified option from the array of options. When the
 * array is used, if no options are defined, then the compaction strategy options
 * is marked as not set at all.
 *
 * \since Cassandra version 1.0.0.
 *
 * \param[in] option_name  The name of the compaction strategy option to unset.
 */
void QCassandraTable::unsetCompactionStrategyOption(const QString& option_name)
{
    f_private->compaction_strategy_options.erase(f_private->compaction_strategy_options.find(option_name.toStdString()));
}

/** \brief Check whether the compaction strategy option is defined.
 *
 * This function retrieves the current status of the compaction strategy option parameter.
 *
 * \param[in] option_name  The name of the compaction strategy option to check for.
 *
 * \return True if the compaction strategy option parameter is defined.
 */
bool QCassandraTable::hasCompactionStrategyOption(const QString& option_name) const
{
    return f_private->compaction_strategy_options.find(option_name.toStdString()) != f_private->compaction_strategy_options.end();
}

/** \brief Check whether the compaction strategy has any option defined.
 *
 * This function returns true if there is at least one compaction strategy
 * option defined.
 *
 * \return True if there is at least one compaction strategy option defined.
 */
bool QCassandraTable::hasCompactionStrategyOptions() const
{
    return !f_private->compaction_strategy_options.empty();
}

/** \brief Retrieve the value of the specified compaction strategy option.
 *
 * This function returns the value of the named compaction strategy option.
 *
 * If the option is still undefined, the function returns an empty string ("").
 *
 * \since Cassandra version 1.0.0.
 *
 * \param[in] option_name  The name of the compaction strategy option to retrieve.
 *
 * \return The value of the specfied compaction strategy option.
 */
QString QCassandraTable::compactionStrategyOption(const QString& option_name) const
{
    if(f_private->compaction_strategy_options.find(option_name.toStdString()) == f_private->compaction_strategy_options.end()) {
        return "";
    }
    return f_private->compaction_strategy_options[option_name.toStdString()].c_str();
}

/** \brief Define a compression option.
 *
 * This function let you define a compression option. You specify the name and the
 * value of the option. If the option was already defined, the new value is saved
 * and overwrites the existing value.
 *
 * \since Cassandra version 1.0.0.
 *
 * \param[in] option_name  The name of the compression option to set.
 * \param[in] value  The new value for this compression option.
 */
void QCassandraTable::setCompressionOption(const QString& option_name, const QString& value)
{
    // TBD: can any option make use of binary data?
    f_private->compression_options[option_name.toStdString()] = value.toStdString();
}

/** \brief Unset the named compression option.
 *
 * This function removes the specified option from the array of options. When the
 * array is used, if no options are defined, then the compression options
 * is marked as not set at all.
 *
 * \since Cassandra version 1.0.0.
 *
 * \param[in] option_name  The name of the compression option to unset.
 */
void QCassandraTable::unsetCompressionOption(const QString& option_name)
{
    f_private->compression_options.erase(f_private->compression_options.find(option_name.toStdString()));
}

/** \brief Check whether the compression option is defined.
 *
 * This function retrieves the current status of the specified compression option parameter.
 *
 * \param[in] option_name  Check whether the named compression option is defined.
 *
 * \return true if the specified compression option is defined.
 */
bool QCassandraTable::hasCompressionOption(const QString& option_name) const
{
    return f_private->compression_options.find(option_name.toStdString()) != f_private->compression_options.end();
}

/** \brief Check whether any compression options are defined.
 *
 * This function checks whether the array of compression options is empty or not.
 *
 * \return true if at least one compression parameter is defined.
 */
bool QCassandraTable::hasCompressionOptions() const
{
    return !f_private->compression_options.empty();
}

/** \brief Retrieve the value of the specified compression option.
 *
 * This function returns the value of the named compression option.
 *
 * If the option is still undefined, the function returns an empty string ("").
 *
 * \since Cassandra version 1.0.0.
 *
 * \param[in] option_name  The name of the compression option to retrieve.
 *
 * \return The value of the specfied compression option.
 */
QString QCassandraTable::compressionOption(const QString& option_name) const
{
    if(f_private->compression_options.find(option_name.toStdString()) == f_private->compression_options.end()) {
        return "";
    }
    return f_private->compression_options[option_name.toStdString()].c_str();
}

/** \brief Mark this table as from Cassandra.
 *
 * This very case happens when the user creates a new context that,
 * at the time of calling QCassandraContext::create(), includes
 * a list of table definitions.
 *
 * In that case we know that the context is being created, but not
 * the tables because the server does it transparently in one go.
 */
void QCassandraTable::setFromCassandra()
{
    f_from_cassandra = true;
}

/** \brief This is an internal function used to parse a CfDef structure.
 *
 * This function is called internally to parse a CfDef object.
 * The data is saved in this QCassandraTabel.
 *
 * \param[in] data  The pointer to the CfDef object.
 */
void QCassandraTable::parseTableDefinition( const CfDef* cf )
{
    if(cf->keyspace != f_private->keyspace) {
        // what do we do here?
        throw std::logic_error("CfDef and QCassandraTablePrivate context names don't match");
    }

    // table name
    if(cf->name != f_private->name) {
        // what do we do here?
        throw std::logic_error("CfDef and QCassandraTablePrivate table names don't match");
    }

    // column type
    if(cf->__isset.column_type) {
        f_private->__set_column_type(cf->column_type);
    }
    else {
        f_private->__isset.column_type = false;
    }

    // comparator type
    if(cf->__isset.comparator_type) {
        f_private->__set_comparator_type(cf->comparator_type);
    }
    else {
        f_private->__isset.comparator_type = false;
    }

    // sub-comparator type
    if(cf->__isset.subcomparator_type) {
        f_private->__set_subcomparator_type(cf->subcomparator_type);
    }
    else {
        f_private->__isset.subcomparator_type = false;
    }

    // comment
    if(cf->__isset.comment) {
        f_private->__set_comment(cf->comment);
    }
    else {
        f_private->__isset.comment = false;
    }

    // row cache size
    if(cf->__isset.row_cache_size) {
        f_private->__set_row_cache_size(cf->row_cache_size);
    }
    else {
        f_private->__isset.row_cache_size = false;
    }

    // key cache size
    if(cf->__isset.key_cache_size) {
        f_private->__set_key_cache_size(cf->key_cache_size);
    }
    else {
        f_private->__isset.key_cache_size = false;
    }

    // read repair chance
    if(cf->__isset.read_repair_chance) {
        f_private->__set_read_repair_chance(cf->read_repair_chance);
    }
    else {
        f_private->__isset.read_repair_chance = false;
    }

    // gc grace seconds
    if(cf->__isset.gc_grace_seconds) {
        f_private->__set_gc_grace_seconds(cf->gc_grace_seconds);
    }
    else {
        f_private->__isset.gc_grace_seconds = false;
    }

    // default validation class
    if(cf->__isset.default_validation_class) {
        f_private->__set_default_validation_class(cf->default_validation_class);
    }
    else {
        f_private->__isset.default_validation_class = false;
    }

    // identifier
    if(cf->__isset.id) {
        f_private->__set_id(cf->id);
    }
    else {
        f_private->__isset.id = false;
    }

    // min compaction threshold
    if(cf->__isset.min_compaction_threshold) {
        f_private->__set_min_compaction_threshold(cf->min_compaction_threshold);
    }
    else {
        f_private->__isset.min_compaction_threshold = false;
    }

    // max compaction threshold
    if(cf->__isset.max_compaction_threshold) {
        f_private->__set_max_compaction_threshold(cf->max_compaction_threshold);
    }
    else {
        f_private->__isset.max_compaction_threshold = false;
    }

    // row cache save period in seconds
    if(cf->__isset.row_cache_save_period_in_seconds) {
        f_private->__set_row_cache_save_period_in_seconds(cf->row_cache_save_period_in_seconds);
    }
    else {
        f_private->__isset.row_cache_save_period_in_seconds = false;
    }

    // key cache save period in seconds
    if(cf->__isset.key_cache_save_period_in_seconds) {
        f_private->__set_key_cache_save_period_in_seconds(cf->key_cache_save_period_in_seconds);
    }
    else {
        f_private->__isset.key_cache_save_period_in_seconds = false;
    }

    // memtable flush after mins
    if(cf->__isset.memtable_flush_after_mins) {
        f_private->__set_memtable_flush_after_mins(cf->memtable_flush_after_mins);
    }
    else {
        f_private->__isset.memtable_flush_after_mins = false;
    }

    // memtable_throughput_in_mb
    if(cf->__isset.memtable_throughput_in_mb) {
        f_private->__set_memtable_throughput_in_mb(cf->memtable_throughput_in_mb);
    }
    else {
        f_private->__isset.memtable_throughput_in_mb = false;
    }

    // memtable_operations_in_millions
    if(cf->__isset.memtable_operations_in_millions) {
        f_private->__set_memtable_operations_in_millions(cf->memtable_operations_in_millions);
    }
    else {
        f_private->__isset.memtable_operations_in_millions = false;
    }

    // replicate on write
    if(cf->__isset.default_validation_class
    && cf->default_validation_class == "CounterColumnType") {
        // we must force the replicate on write to true for counters
        // (although it is not really necessary after 0.8.1, it may
        // cause problems to have false in this very case.
        f_private->__set_replicate_on_write(true);
    }
    else if(cf->__isset.replicate_on_write) {
        f_private->__set_replicate_on_write(cf->replicate_on_write);
    }
    else {
        f_private->__isset.replicate_on_write = false;
    }

    // merge shards chance
    if(cf->__isset.merge_shards_chance) {
        f_private->__set_merge_shards_chance(cf->merge_shards_chance);
    }
    else {
        f_private->__isset.merge_shards_chance = false;
    }

    // key validation class
    if(cf->__isset.key_validation_class) {
        f_private->__set_key_validation_class(cf->key_validation_class);
    }
    else {
        f_private->__isset.key_validation_class = false;
    }

    // row_cache_provider
    if(cf->__isset.row_cache_provider) {
        f_private->__set_row_cache_provider(cf->row_cache_provider);
    }
    else {
        f_private->__isset.row_cache_provider = false;
    }

    // key alias
    if(cf->__isset.key_alias) {
        f_private->__set_key_alias(cf->key_alias);
    }
    else {
        f_private->__isset.key_alias = false;
    }

    // table definitions (CfDef, column family definitions)
    f_column_definitions.clear();
    for( auto col : cf->column_metadata )
    {
        QCassandraColumnDefinition::pointer_t column_definition(columnDefinition(col.name.c_str()));
        column_definition->parseColumnDefinition(&col);
    }

    f_from_cassandra = true;
}

/** \brief Prepare a table definition.
 *
 * This function transforms a QCassandra table definition into
 * a Cassandra CfDef structure.
 *
 * The parameter is passed as a void * because we do not want to define
 * the thrift types in our public headers.
 *
 * \param[in] data  The CfDef were the table is to be saved.
 */
void QCassandraTable::prepareTableDefinition( CfDef* cf ) const
{
    *cf = *f_private;

    // copy the columns
    cf->column_metadata.clear();
    for( auto c : f_column_definitions )
    {
        ColumnDef col;
        c->prepareColumnDefinition( &col );
        cf->column_metadata.push_back(col);
    }
    cf->__isset.column_metadata             = !cf->column_metadata.empty();
    cf->__isset.compaction_strategy_options = !cf->compaction_strategy_options.empty();
    cf->__isset.compression_options         = !cf->compression_options.empty();
}


QString QCassandraTable::getTableOptions( const CfDef& cf ) const
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
        q_str += QString("AND caching = '%1'\n").arg(cf.caching.c_str());
    }
    if( cf.__isset.comment )
    {
        q_str += QString("AND comment = '%1'\n").arg(cf.comment.c_str());
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


/** \brief Create a Cassandra table.
 *
 * This function creates a Cassandra table in the context as specified
 * when you created the QCassandraTable object.
 *
 * If you want to declare a set of columns, this is a good time to do
 * it too (there is not QColumnDefinition::create() function!) By
 * default, columns use the default validation type as defined using
 * the setComparatorType() for their name and the
 * setDefaultValidationClass() for their data. It is not required to
 * define any column. In that case they all make use of the exact
 * same data.
 *
 * The table cannot already exist or an error will be thrown by the
 * Cassandra server. If the table is being updated, use the update()
 * function instead.
 *
 * Note that when you create a new context, you can create its tables
 * at once by defining tables before calling the QCassandraContext::create()
 * function.
 *
 * Creating a new QCassandraTable:
 *
 * \code
 * QtCassandra::QCassandraTable::pointer_t table(context->table("qt_cassandra_test_table"));
 * table->setComment("Our test table.");
 * table->setColumnType("Standard"); // Standard or Super
 * table->setKeyValidationClass("BytesType");
 * table->setDefaultValidationClass("BytesType");
 * table->setComparatorType("BytesType");
 * table->setKeyCacheSavePeriodInSeconds(14400); // unused in 1.1+
 * table->setMemtableFlushAfterMins(60); // unused in 1.1+
 * // Memtable defaults are dynamic and usually a better bet
 * //table->setMemtableThroughputInMb(247); // unused in 1.1+
 * //table->setMemtableOperationsInMillions(1.1578125); // unused in 1.1+
 * table->setGcGraceSeconds(864000);
 * table->setMinCompactionThreshold(4);
 * table->setMaxCompactionThreshold(22);
 * table->setReplicateOnWrite(1);
 * table->create();
 *
 * // you also may want to call this function (see note below):
 * QCassandra::synchronizeSchemaVersions()
 * \endcode
 *
 * \note
 * Once the table->create(); function returns, the table was created in the
 * Cassandra node you are connect with, but it was not yet replicated. In
 * order to use the table, the replication needs to be complete. To know
 * once it is complete, call the QCassandra::synchronizeSchemaVersions()
 * function. If you are to create multiple tables, you can create all the
 * tables at once, then synchronize them all at once which should give
 * time for the Cassandra nodes to replicate the first few tables while
 * you create the last few and thus saving you time.
 *
 * \sa update()
 * \sa QCassandraContext::create()
 * \sa QCassandra::synchronizeSchemaVersions()
 */
void QCassandraTable::create()
{
    CfDef cf;
    prepareTableDefinition( &cf );

    QString value_type("BLOB");

    if( isCounterClass() )
    {
        value_type = "COUNTER";
    }

    QString query( QString( "CREATE TABLE IF NOT EXISTS %1.%2 \n"
        "(key BLOB, column1 BLOB, value %3, PRIMARY KEY (key, column1) ) \n"
        "WITH COMPACT STORAGE\n"
        "AND CLUSTERING ORDER BY (column1 ASC)\n" )
            .arg(f_context->contextName())
            .arg(f_tableName)
            .arg(value_type)
            );
    query += getTableOptions( cf );

    // 1) Load exiting tables from the database,
    // 2) Create the table using the query string,
    // 3) Add this object into the list.
    //
    QCassandraQuery q( f_session );
    q.query( query );
    q.start();

    f_from_cassandra = true;
}


/** \brief Truncate a Cassandra table.
 *
 * The truncate() function removes all the rows from a Cassandra table
 * and clear out the cached data (rows and cells.)
 *
 * If the table is not connected to Cassandra, then nothing happens with
 * the Cassandra server.
 *
 * If you want to keep a copy of the cache, you will have to retrieve a
 * copy of the rows map using the rows() function.
 *
 * \sa rows()
 * \sa clearCache()
 */
void QCassandraTable::truncate()
{
    if( !f_from_cassandra )
    {
        return;
    }

    const QString query(
        QString("TRUNCATE %1.%2;")
            .arg(f_context->contextName())
            .arg(f_tableName)
            );

    QCassandraQuery q;
    q.query( query );
    q.start();
    q.end();

    clearCache();
}


/** \brief Clear the memory cache.
 *
 * This function clears the memory cache. This means all the rows and
 * their cells will be deleted from this table. The memory cache doesn't
 * affect the Cassandra database.
 *
 * After a clear, you can retrieve fresh data (i.e. by directly loading the
 * data from the Cassandra database.)
 *
 * Note that if you kept shared pointers to rows and cells defined in
 * this table, accessing those is likely going to generate an exception.
 */
void QCassandraTable::clearCache()
{
    f_query.clear();
    f_rows.clear();
}


void QCassandraTable::addRow( const QByteArray& row_key, const QByteArray& column_key, const QByteArray& data )
{
    QCassandraRow::pointer_t  new_row  ( new QCassandraRow( shared_from_this(), row_key ) );
    QCassandraCell::pointer_t new_cell ( new_row->cell( column_key ) );
    new_cell->assignValue( QCassandraValue(data) );

    // Now add to the map.
    //
    f_rows[row_key] = new_row;
}


/** \brief Read a set of rows as defined by the row predicate.
 *
 * This function reads a set of rows as defined by the row predicate.
 *
 * To change the consistency for this read, check out the
 * QCassandraCellPredicate::setConsistencyLevel() function.
 *
 * If the table is not connected to Cassandra (i.e. the table is
 * a memory table) then nothing happens.
 *
 * Remember that if you are querying without checking for any column
 * you will get "empty" rows in your results (see dropRow() function
 * for more information and search for TombStones in Cassandra.)
 * This was true in version 0.8.0 to 1.1.5. It may get fixed at some
 * point.
 *
 * Note that the function updates the predicate so the next call
 * returns the following rows as expected.
 *
 * \warning
 * This function MAY NOT "WORK RIGHT" if your cluster was defined using
 * the RandomPartitioner. Rows are not sorted by key when the
 * RandomPartitioner is used. Instead, the rows are sorted by their
 * MD5 sum. Also the system may add additional data before or
 * after that MD5 and the slice range cannot anyway provide that
 * MD5 to the system. If you want to query sorted slices of your
 * rows, you must create your cluster with another partitioner.
 * Search for partitioner in conf/cassandra.yaml in the
 * Cassandra tarball.
 * See also: http://ria101.wordpress.com/2010/02/22/cassandra-randompartitioner-vs-orderpreservingpartitioner/
 *
 * \param[in,out] row_predicate  The row predicate.
 *
 * \return The number of rows read.
 *
 * \sa QCassandraRowPredicate (see detailed description of row predicate for an example)
 * \sa QCassandraCellPredicate::setConsistencyLevel()
 * \sa dropRow()
 */
uint32_t QCassandraTable::readRows( QCassandraRowPredicate::pointer_t row_predicate )
{
    if( f_query )
    {
        if( !f_query->nextPage() )
        {
            f_query.reset();
            return 0;
        }
    }
    else
    {
        QString query( QString("SELECT key,column1,value FROM %1.%2").arg(f_context->contextName()).arg(f_tableName) );
        int bind_count = 0;
        if( row_predicate )
        {
            row_predicate->appendQuery( query, bind_count );
        }
        query += " ALLOW FILTERING";
        //
        std::cout << "query=[" << query.toStdString() << "]" << std::endl;
        f_query = std::make_shared<QCassandraQuery>(f_session);
        f_query->query( query );
        //
        if( row_predicate )
        {
            int bind_num = 0;
            row_predicate->bindQuery( f_query, bind_num );
            f_query->setPagingSize( row_predicate->count() );
        }

        f_query->start();
    }

    size_t result_size = 0;
    while( f_query->nextRow() )
    {
        const QByteArray row_key   ( f_query->getByteArrayColumn( "key"     ) );
        const QByteArray column_key( f_query->getByteArrayColumn( "column1" ) );
        const QByteArray data      ( f_query->getByteArrayColumn( "value"   ) );
        addRow( row_key, column_key, data );
        ++result_size;
    }

    return result_size;
}


QCassandraRow::pointer_t QCassandraTable::row(const char* row_name)
{
    return row( QString(row_name) );
}


/** \brief Search for a row or create a new one.
 *
 * This function searches for a row or, if it doesn't exist, create
 * a new row.
 *
 * Note that unless you set the value of a column in this row, the
 * row will never appear in the Cassandra cluster.
 *
 * This function accepts a name for the row. The name is a UTF-8 string.
 *
 * \param[in] row_name  The name of the row to search or create.
 *
 * \return A shared pointer to the matching row or a null pointer.
 */
QCassandraRow::pointer_t QCassandraTable::row(const QString& row_name)
{
    return row(row_name.toUtf8());
}

/** \brief Search for a row or create a new one.
 *
 * This function searches for a row or, if it doesn't exist, create
 * a new row.
 *
 * Note that unless you set the value of a column in this row, the
 * row will never appear in the Cassandra cluster.
 *
 * This function assigns the row a binary key.
 *
 * \param[in] row_key  The name of the row to search or create.
 *
 * \return A shared pointer to the matching row. Throws if not found in the map.
 *
 * \sa readRows()
 */
QCassandraRow::pointer_t QCassandraTable::row(const QByteArray& row_key)
{
    // row already exists?
    QCassandraRows::iterator ri(f_rows.find(row_key));
    if(ri != f_rows.end())
    {
        return ri.value();
    }

    // this is a new row, allocate it
    QCassandraRow::pointer_t c(new QCassandraRow(shared_from_this(), row_key));
    f_rows.insert(row_key, c);
    return c;
}


/** \brief Retrieve the entire set of rows defined in this table.
 *
 * This function returns a constant reference to the map listing all
 * the rows currently defined in memory for this table.
 *
 * This can be used to determine how many rows are defined in memory
 * and to scan all the data.
 *
 * \return A constant reference to a map of rows. Throws if readRows() has not first been called.
 */
const QCassandraRows& QCassandraTable::rows()
{
    if( !f_queryStmt )
    {
        throw std::runtime_error( "No query is in effect!" );
    }

    if( f_rows.empty() )
    {
        std::stringstream msg;
        msg << "You must first call readRows() on table " << f_tableName.toStdString() << " before trying to access the rows!";
        throw std::runtime_error( msg.str().c_str() );
    }

    return f_rows;
}


/** \brief Search for a row.
 *
 * This function searches for a row. If it doesn't exist, then a NULL
 * pointer is returned (use the operator bool() function on the shared pointer.)
 *
 * The function can be used to check whether a given row was already created
 * in memory without actually creating it.
 *
 * This function accepts a row name viewed as a UTF-8 string.
 *
 * \warning
 * This function does NOT attempt to read the row from the Cassandra database
 * system. It only checks whether the row already exists in memory. To check
 * whether the row exists in the database, use the exists() function instead.
 *
 * \param[in] row_name  The name of the row to check for.
 *
 * \return A shared pointer to the row, may be NULL (operator bool() returning true)
 *
 * \sa exists()
 * \sa row()
 */QCassandraRow::pointer_t QCassandraTable::findRow(const char* row_name)
{
    return findRow( QByteArray::fromRawData(row_name, qstrlen(row_name)) );
}


/** \brief Search for a row.
 *
 * This function searches for a row. If it doesn't exist, then a NULL
 * pointer is returned (use the operator bool() function on the shared pointer.)
 *
 * The function can be used to check whether a given row was already created
 * in memory without actually creating it.
 *
 * This function accepts a row name viewed as a UTF-8 string.
 *
 * \warning
 * This function does NOT attempt to read the row from the Cassandra database
 * system. It only checks whether the row already exists in memory. To check
 * whether the row exists in the database, use the exists() function instead.
 *
 * \param[in] row_name  The name of the row to check for.
 *
 * \return A shared pointer to the row, may be NULL (operator bool() returning true)
 *
 * \sa exists()
 * \sa row()
 */
QCassandraRow::pointer_t QCassandraTable::findRow(const QString& row_name)
{
    return findRow( row_name.toUtf8() );
}


/** \brief Search for a row.
 *
 * This function searches for a row. If it doesn't exist, then a NULL
 * pointer is returned (use the operator bool() function on the shared pointer.)
 *
 * The function can be used to check whether a given row was already created
 * in memory without actually creating it.
 *
 * This function accepts a row key which is a binary buffer.
 *
 * \warning
 * This function does NOT attempt to read the row from the Cassandra database
 * system. It only checks whether the row already exists in memory. To check
 * whether the row exists in the database, use the exists() function instead.
 *
 * \param[in] row_key  The binary key of the row to search for.
 *
 * \return A shared pointer to the row, may be NULL (operator bool() returning true)
 *
 * \sa exists()
 * \sa row()
 */
QCassandraRow::pointer_t QCassandraTable::findRow(const QByteArray& row_key)
{
    QCassandraRows::iterator ri(f_rows.find(row_key));
    if(ri == f_rows.end())
    {
        QCassandraRow::pointer_t null;
        return null;
    }
    return *ri;
}


/** \brief Check whether a row exists.
 *
 * This function checks whether the named row exists.
 *
 * \param[in] row_name  The row name to transform to UTF-8.
 *
 * \return true if the row exists in memory or the Cassandra database.
 */
bool QCassandraTable::exists(const char *row_name) const
{
    return exists(QByteArray(row_name, qstrlen(row_name)));
}


/** \brief Check whether a row exists.
 *
 * This function checks whether the named row exists.
 *
 * \param[in] row_name  The row name to transform to UTF-8.
 *
 * \return true if the row exists in memory or the Cassandra database.
 */
bool QCassandraTable::exists(const QString& row_name) const
{
    return exists(row_name.toUtf8());
}


/** \brief Check whether a row exists.
 *
 * This function checks whether a row exists. First it checks whether
 * it exists in memory. If not, then it checks in the Cassandra database.
 *
 * Empty keys are always viewed as non-existant and this function
 * returns false in that case.
 *
 * \warning
 * If you dropped the row recently, IT STILL EXISTS. This is a "bug" in
 * Cassandra and there isn't really a way around it except by testing
 * whether a specific cell exists in the row. We cannot really test for
 * a cell here since we cannot know what cell exists here. So this test
 * really only tells you that (1) the row was never created; or (2) the
 * row was drop "a while back" (the amount of time it takes for a row
 * to completely disappear is not specified and it looks like it can take
 * days.)
 *
 * \todo
 * At this time there isn't a way to specify the consistency level of the
 * calls used by this function. The QCassandra default is used.
 *
 * \param[in] row_key  The binary key of the row to check for.
 *
 * \return true if the row exists in memory or in Cassandra.
 */
bool QCassandraTable::exists(const QByteArray& row_key) const
{
    // an empty key cannot represent a valid row
    if(row_key.size() == 0)
    {
        return false;
    }

    QCassandraRows::iterator ri(f_rows.find(row_key));
    if(ri != f_rows.end())
    {
        // row exists in memory
        return true;
    }

    QCassandraRowPredicate::pointer_t row_predicate( new QCassandraRowPredicate );
    row_predicate.setrowKey(row_key);

    // define a key range that is quite unlikely to match any column
    QCassandraCellRangePredicate::pointer_t cell_pred( new QCassandraCellRangePredicate );
    QByteArray key;
    setInt32Value(key, 0x00000000);
    cell_pred->setStartCellKey(key);
    setInt32Value(key, 0x00000001);
    cell_pred->setEndCellKey(key);
    row_predicate->setCellPredicate( std::static_pointer_cast<QCassandraCellPredicate>( cell_pred ) );

    return const_cast<QCassandraTable *>(this)->readRows( row_predicate ) != 0;
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then it is created first.
 *
 * The reference is writable so you make write to a cell in this row.
 *
 * This function accepts a UTF-8 name for this row reference.
 *
 * \param[in] row_name  The name of the row to retrieve.
 *
 * \return A reference to a QCassandraRow.
 */
QCassandraRow& QCassandraTable::operator [] (const char *row_name)
{
    // in this case we may create the row and that's fine!
    return *row(row_name);
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then it is created first.
 *
 * The reference is writable so you make write to a cell in this row.
 *
 * This function accepts a UTF-8 name for this row reference.
 *
 * \param[in] row_name  The name of the row to retrieve.
 *
 * \return A reference to a QCassandraRow.
 */
QCassandraRow& QCassandraTable::operator [] (const QString& row_name)
{
    // in this case we may create the row and that's fine!
    return *row(row_name);
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the keyed row doesn't exist yet,
 * then it is created first.
 *
 * The reference is writable so you make write to a cell in this row.
 *
 * This function accepts a binary key for this row reference.
 *
 * \param[in] row_key  The binary key of the row to retrieve.
 *
 * \return A reference to a QCassandraRow.
 */
QCassandraRow& QCassandraTable::operator[] (const QByteArray& row_key)
{
    // in this case we may create the row and that's fine!
    return *row(row_key);
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then the function raises an error.
 *
 * The reference is read-only (constant) so you may retrieve a cell value
 * from it, but not modify the cell.
 *
 * This function accepts a name as the row reference. The name is viewed as
 * a UTF-8 string.
 *
 * \exception std::runtime_error
 * The function checks whether the named row exists. If not, then this error
 * is raised because the function is constant and cannot create a new row.
 *
 * \param[in] row_name  The name of the row to retrieve.
 *
 * \return A constant reference to a QCassandraRow.
 */
const QCassandraRow& QCassandraTable::operator[] (const char *row_name) const
{
    const QCassandraRow::pointer_t p_row(findRow(row_name));
    if(!p_row) {
        throw std::runtime_error("row does not exist so it cannot be read from");
    }
    return *p_row;
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then the function raises an error.
 *
 * The reference is read-only (constant) so you may retrieve a cell value
 * from it, but not modify the cell.
 *
 * This function accepts a name as the row reference.
 *
 * \exception std::runtime_error
 * The function checks whether the named row exists. If not, then this error
 * is raised because the function is constant and cannot create a new row.
 *
 * \param[in] row_name  The name of the row to retrieve.
 *
 * \return A constant reference to a QCassandraRow.
 */
const QCassandraRow& QCassandraTable::operator[] (const QString& row_name) const
{
    const QCassandraRow::pointer_t p_row(findRow(row_name));
    if( !p_row ) {
        throw std::runtime_error("row does not exist so it cannot be read from");
    }
    return *p_row;
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then the function raises an error.
 *
 * The reference is read-only (constant) so you may retrieve a cell value
 * from it, but not modify the cell.
 *
 * This function accepts a binary key as the row reference.
 *
 * \exception std::runtime_error
 * The function checks whether the named row exists. If not, then this error
 * is raised because the function is constant and cannot create a new row.
 *
 * \param[in] row_key  The binary key of the row to retrieve.
 *
 * \return A constant reference to a QCassandraRow.
 */
const QCassandraRow& QCassandraTable::operator[] (const QByteArray& row_key) const
{
    const QCassandraRow::pointer_t p_row(findRow(row_key));
    if(!p_row) {
        throw std::runtime_error("row does not exist so it cannot be read from");
    }
    return *p_row;
}


/** \brief Drop the named row.
 *
 * This function is the same as the dropRow() that takes a row_key parameter.
 * It simply transforms the row name into a row key and calls that other
 * function.
 *
 * \param[in] row_name  Specify the name of the row to drop.
 * \param[in] mode  Specify the timestamp mode.
 * \param[in] timestamp  Specify the timestamp to remove only rows that are have that timestamp or are older.
 * \param[in] consistency_level  Specify the timestamp to remove only rows that are have that timestamp or are older.
 */
void QCassandraTable::dropRow(const char *row_name, QCassandraValue::timestamp_mode_t mode, int64_t timestamp, consistency_level_t consistency_level)
{
    dropRow(QByteArray::fromRawData(row_name, qstrlen(row_name)), mode, timestamp, consistency_level);
}


/** \brief Drop the named row.
 *
 * This function is the same as the dropRow() that takes a row_key parameter.
 * It simply transforms the row name into a row key and calls that other
 * function.
 *
 * \param[in] row_name  Specify the name of the row to drop.
 * \param[in] mode  Specify the timestamp mode.
 * \param[in] timestamp  Specify the timestamp to remove only rows that are have that timestamp or are older.
 */
void QCassandraTable::dropRow(const QString& row_name, QCassandraValue::timestamp_mode_t mode, int64_t timestamp)
{
    dropRow(row_name.toUtf8(), mode, timestamp);
}

/** \brief Drop the row from the Cassandra database.
 *
 * This function deletes the specified row and its data from the Cassandra
 * database and from memory.
 *
 * In regard to getting the row deleted from memory, you are expected to
 * use a weak pointer as follow:
 *
 * \code
 * ...
 * {
 *     QWeakPointer<QtCassandra::QCassandraRow> row(table.row(row_key)));
 *     ...
 *     table.dropRow(row_key);
 * }
 * ...
 * \endcode
 *
 * Note that Cassandra doesn't actually remove the row from its database until
 * the next time it does a garbage collection. Still, if there is a row you do
 * not need, drop it.
 *
 * The timestamp \p mode can be set to QCassandraValue::TIMESTAMP_MODE_DEFINED
 * in which case the value defined in the \p timestamp parameter is used by the
 * Cassandra remove() function.
 *
 * By default the \p mode parameter is set to
 * QCassandraValue::TIMESTAMP_MODE_AUTO which means that we'll make use of
 * the current time (i.e. only a row created after this call will exist.)
 *
 * The consistency level is set to CONSISTENCY_LEVEL_ALL since you are likely
 * willing to delete the row on all the nodes. However, I'm not certain this
 * is the best choice here. So the default may change in the future. You
 * may specify CONSISTENCY_LEVEL_DEFAULT in which case the QCassandra object
 * default is used.
 *
 * \warning
 * Remember that a row doesn't actually get removed from the Cassandra database
 * until the next Garbage Collection runs. This is important for all your data
 * centers to be properly refreshed. This also means a readRows() will continue
 * to return the deleted row unless you check for a specific column that has
 * to exist. In that case, the row is not returned since all the columns ARE
 * deleted (or at least hidden in some way.) This function could be called
 * truncate(), however, all empty rows are really removed at some point.
 *
 * \warning
 * After a row was dropped, you cannot use the row object anymore, even if you
 * kept a shared pointer to it. Calling functions of a dropped row is likely
 * to get you a run-time exception. Note that all the cells defined in the
 * row are also dropped and are also likely to generate a run-time exception
 * if you kept a shared pointer on any one of them.
 *
 * \param[in] row_key  Specify the key of the row.
 * \param[in] mode  Specify the timestamp mode.
 * \param[in] timestamp  Specify the timestamp to remove only rows that are have that timestamp or are older. Ignored.
 */
void QCassandraTable::dropRow( const QByteArray& row_key, QCassandraValue::timestamp_mode_t /*mode*/, int64_t /*timestamp*/)
{
    remove( row_key );
    f_rows.remove( row_key );
}


/** \brief Get the pointer to the parent object.
 *
 * \return Shared pointer to the cassandra object.
 */
QCassandraContext::pointer_t QCassandraTable::parentContext() const
{
    return f_context;
}


/** \brief Save a cell value that changed.
 *
 * This function calls the context insertValue() function to save the new value that
 * was defined in a cell. The idea is so that when the user alters the value of a cell,
 * it directly updates the database. If the row does not exist, an exception is thrown.
 *
 * \param[in] row_key     The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[in] value       The new value of the cell.
 */
void QCassandraTable::insertValue( const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& p_value )
{
    if( !f_from_cassandra )
    {
        return;
    }

    QCassandraValue value( p_value );
    int row_id    = 0;
    int column_id = 1;
    int value_id  = 2;
    QString query_string;
    if( isCounterClass() )
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

    // Insert or update the row values.
    //
    QCassandraQuery q( f_session );
    q.query( query_string, 3 );
    q.bindByteArray( row_id,    row_key.constData(),    row_key.size()    );
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


bool QCassandraTable::isCounterClass()
{
    if( !f_private->__isset.default_validation_class )
    {
        QCassandraQuery q( f_session );
        q.query(
            QString("SELECT validator FROM system.schema_columns WHERE keyspace_name = '%1' AND columnfamily_name = '%2'" )
            .arg(f_context->contextName())
            .arg(f_tableName)
            );
        q.start();
        if( !q.nextRow() )
        {
            throw std::runtime_error( "Critical database error! Cannot read system.schema_columns!" );
        }
        f_private->set_default_validation_class( q.getStringColumn( 0 ).toStdString() );
        q.end();
    }

    const auto& the_class( f_private->default_validation_class );
    return (the_class == "org.apache.cassandra.db.marshal.CounterColumnType")
        || (the_class == "CounterColumnType");
}


/** \brief Get a cell value from Cassandra.
 *
 * This function calls the context getValue() function to retrieve a value
 * from Cassandra.
 *
 * The \p value parameter is not modified unless some data can be retrieved
 * from Cassandra.
 *
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[out] value  The new value of the cell.
 *
 * \return false when the value was not found in the database, true otherwise
 */
bool QCassandraTable::getValue(const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value)
{
    const QString q_str( QString("SELECT value FROM %1.%2 WHERE key = ? AND column1 = ?")
                         .arg(f_context->contextName()).arg(f_tableName) );

    QCassandraQuery q( f_session );
    q.query( q_str, 2 );

    size_t bind_num = 0;
    q.bindByteArray( bind_num++, row_key    );
    q.bindByteArray( bind_num++, column_key );

    q.start();

    if( !q.nextRow() )
    {
        if( isCounterClass() )
        {
            value.setInt64Value(0);
        }
        else
        {
            value.setNullValue();
        }
        return false;
    }

    const CassRow* row( cass_iterator_get_row(rows.get()));
    if( isCounterClass() )
    {
        value = q.getInt64Column( "value" );
    }
    else
    {
        value = q.getByteArray( "value" );
    }

    return true;
}


/** \brief Count columns.
 *
 * This function counts a the number of columns that match a specified
 * column_predicate.
 *
 * \param[in] row_key  The row for which this data is being counted.
 * \param[in] column_predicate  The predicate to use to count the cells.
 *
 * \return The number of columns in this row.
 */
int32_t QCassandraTable::getCellCount
    ( const QByteArray& row_key
    , QCassandraCellPredicate::pointer_t column_predicate
    )
{
    if( f_rows.find( row_key ) == f_rows.end() )
    {
        query_string = QString("SELECT COUNT(*) AS count FROM %1.%2")
            .arg(f_context->contextName())
            .arg(f_tableName)
            ;

        QCassandraQuery q( f_session );
        q.query( query_string, 0 );
        q.setPagingSize( column_predicate->count() );
        q.start();
        q.nextRow();
        return getInt32Column( "count" );
    }

    // return the count from the memory cache
    return f_rows[row_key]->cells().size();
}

/** \brief Delete a Cell from a table row.
 *
 * This function removes a cell from the Cassandra database as specified
 * by the parameters.
 *
 * \param[in] row_key  The row in which the cell is to be removed.
 * \param[in] column_key  The cell to be removed.
 */
void QCassandraTable::remove
    ( const QByteArray& row_key
    , const QByteArray& column_key
    )
{
    if( !f_from_cassandra )
    {
        return;
    }

    const QString query(
        QString("DELETE FROM %1.%2 WHERE key=? AND column1=?;")
            .arg(f_context->contextName())
            .arg(f_tableName)
            );

    QCassandraQuery q( f_session );
    q.query( query_string, 2 );
    q.bindByteArray( row_id    , row_key.constData()    , row_key.size()    );
    q.bindByteArray( column_id , column_key.constData() , column_key.size() );
    q.start();
}

/** \brief Delete a Cell from a table row.
 *
 * This function removes a cell from the Cassandra database as specified
 * by the parameters.
 *
 * \param[in] row_key  The row in which the cell is to be removed.
 * \param[in] column_key  The cell to be removed.
 * \param[in] timestamp  The time when the key to be removed was created.
 */
void QCassandraTable::remove( const QByteArray& row_key )
{
    if( !f_from_cassandra )
    {
        return;
    }

    const QString query(
        QString("DELETE FROM %1.%2 WHERE key=?;")
            .arg(f_context->contextName())
            .arg(f_tableName)
            );

    QCassandraQuery q( f_session );
    q.query( query_string, 2 );
    q.bindByteArray( row_id, row_key.constData(), row_key.size() );
    q.start();
}

/** \brief Set the default validation class to create a counters table.
 *
 * This function is a specialized version of the setDefaultValidationClass()
 * with the name of the class necessary to create a table of counters. Remember
 * that once in this state a table cannot be converted.
 *
 * This is equivalent to setDefaultValidationClass("CounterColumnType").
 */
void QCassandraTable::setDefaultValidationClassForCounters()
{
    setDefaultValidationClass("CounterColumnType");
}

/** \brief Set the default validation class.
 *
 * This function defines the default validation class for the table columns.
 * By default it is set to binary (BytesType), which is similar to saying
 * no validation is required.
 *
 * The CLI documentation says that the following are valid as a default
 * validation class:
 *
 * AsciiType, BytesType, CounterColumnType, IntegerType, LexicalUUIDType,
 * LongType, UTF8Type
 *
 * \param[in] validation_class  The default validation class for columns data.
 */
void QCassandraTable::setDefaultValidationClass(const QString& validation_class)
{
    f_defaultValidationClass = validation_class;
}

/** \brief Unset the default validation class.
 *
 * This function removes the effects of setDefaultValidationClass() calls.
 */
void QCassandraTable::unsetDefaultValidationClass()
{
    f_defaultValidationClass = QString("BytesType");
}

} // namespace QtCassandra

// vim: ts=4 sw=4 et
