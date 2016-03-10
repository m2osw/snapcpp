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
    : f_context(context)
    , f_tableName(table_name)
    //f_rows() -- auto-init
    , f_currentPredicate(0)
{
    // verify the name here (faster than waiting for the server and good documentation)
    QRegExp re("[A-Za-z][A-Za-z0-9_]*");
    if(!re.exactMatch(table_name)) {
        throw std::runtime_error("invalid table name (does not match [A-Za-z][A-Za-z0-9_]*)");
    }

    // we save the context name (keyspace) and since it's forbidden to change it
    // in the context, we know it won't change here either
    const QString keyspace = context->contextName();
    //f_private->__set_keyspace(keyspace.toUtf8().data());

    // we save the name and at this point we prevent it from being changed.
    //f_private->__set_name(table_name.toUtf8().data());
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
    return f_context->contextName();
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
    return f_tableName;
}


QString QCassandraTable::option( const QString& option_type, const QString& option_name) const
{
    return f_options[option_type][option_name];
}


QString& QCassandraTable::option( const QString& option_type, const QString& option_name)
{
    return f_options[option_type][option_name];
}


void QCassandraTable::unsetOption( const QString& option_type, const QString& option_name)
{
    f_options[option_type].remove( option_name );
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
    //f_from_cassandra = true;

    // TBD: Should we then call describe_keyspace() on our Context
    //      to make sure we have got the right data (defaults) in this
    //      object and column definitions?

    QString query( QString( "CREATE TABLE IF NOT EXISTS %1.%2 \n"
        "(key BLOB, column1 BLOB, value BLOB, PRIMARY KEY (key, column1) ) \n"
        "WITH COMPACT STORAGE\n"
        "AND CLUSTERING ORDER BY (column1 ASC)\n" )
            .arg(f_context->contextName())
            .arg(f_tableName)
            );
    for( const auto& type : f_options.keys() )
    {
        if( type.toLower() == "general" )
        {
            for( const auto& key : f_options[type] )
            {
                query += QString("AND %1 = %2\n").arg(key.toLower()).arg(f_options[type][key]);
            }
        }
        else
        {
            auto& option( f_options[type] );
            query += QString("AND %1 = '{").arg(type.toLower());
            QString comma;
            for( const auto& key : option.keys() )
            {
                query += QString("%1'%2':'%3'").arg(comma).arg(key.toLower()).arg(option[key]);
                comma = ", ";
            }
            query += "}'\n";
        }
    }

    // Create the table using the query string
    //
    f_context->parentCassandra()->executeQuery( query );

#if 0
CREATE TABLE snap_websites.firewall (
    key blob,
    column1 blob,
    value blob,
    PRIMARY KEY (key, column1)
) WITH COMPACT STORAGE
    AND CLUSTERING ORDER BY (column1 ASC)
    AND bloom_filter_fp_chance = 0.01
    AND caching = '{"keys":"ALL", "rows_per_partition":"NONE"}'
    AND comment = 'List of IP addresses we want blocked'
    AND compaction = {'class': 'org.apache.cassandra.db.compaction.SizeTieredCompactionStrategy'}
    AND compression = {'sstable_compression': 'org.apache.cassandra.io.compress.LZ4Compressor'}
    AND dclocal_read_repair_chance = 0.0
    AND default_time_to_live = 0
    AND gc_grace_seconds = 864000
    AND max_index_interval = 2048
    AND memtable_flush_period_in_ms = 0
    AND min_index_interval = 128
    AND read_repair_chance = 0.0
    AND speculative_retry = 'NONE';
#endif

}

#if 0
/** \brief Update a Cassandra table.
 *
 * This function is used to update a table with changes that
 * you made in memory (i.e. calling functions such as
 * setDefaultValidationClass().)
 *
 * Note that you must synchronize your Cassandra schema in your
 * entire cluster with a call to the QCassandra::synchornizeSchemaVersions()
 * function before you use an updated table.
 *
 * This function is most often used to define new column types or
 * changing existing columns (although changing existing columns
 * may not work as expected, from what I understand, TBD. I
 * suggest you create new columns and copy the data if you run
 * into problems.)
 *
 * \sa setDefaultValidationClass()
 * \sa QCassandra::synchronizeSchemaVersions()
 */
void QCassandraTable::update()
{
    auto context( f_context.lock() );
    if(!context) {
        throw std::runtime_error("table was dropped and is not attached to a context anymore");
    }
    if(!f_from_cassandra) {
        // you cannot update a table that's not from Cassandra
        throw std::runtime_error("table is not defined in Cassandra, it cannot be updated there");
    }

    context->updateTable(this);

    // TBD: Should we then call describe_keyspace() on our Context
    //      to make sure we've got the right data (defaults) in this
    //      object and column definitions?
}
#endif


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
    const QString query(
        QString("TRUNCATE %1.%2;")
            .arg(f_context->contextName())
            .arg(f_tableName)
            );

    f_context->parentCassandra()->executeQuery( query );

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
    f_rows.clear();
}


/// \brief Return the full count of the entire table.
//
int32_t QCassandraTable::rowCount( const QByteArray& row_key ) const
{
    const QString query( QString("SELECT COUNT(*) AS count FROM %1.%2 WHERE key = ?").arg(f_context->contextName()).arg(f_tableName) );
    //
    statement_pointer_t query_stmt( cass_statement_new( query.toUtf8().data(), 1 ), statementDeleter() );
    cass_statement_bind_string_n( query_stmt.get(), 0, row_key.constData(), row_key.size() );
    cass_statement_set_paging_size( query_stmt.get(), 100 );

    future_pointer_t result_future( cass_session_execute( f_context->parentCassandra()->session().get(), query_stmt.get() ) , futureDeleter()    );
    throwIfError( result_future, QString("Cannot select from table '%1'!").arg(f_tableName) );

    result_pointer_t   result( cass_future_get_result(result_future.get()), resultDeleter() );
    iterator_pointer_t rows  ( cass_iterator_from_result(result.get()), iteratorDeleter()   );

    cass_iterator_next(rows.get());
    const CassRow*   row         = cass_iterator_get_row( rows.get() );
    const CassValue* count_value = cass_row_get_column_by_name( row, "count" );
    int32_t count;
    cass_value_get_int32( count_value, &count );

    auto session( f_context->parentCassandra()->session() );

    while( cass_result_has_more_pages(result.get()) )
    {
        cass_statement_set_paging_state( query_stmt.get(), result.get() );
        result_future.reset ( cass_session_execute( session.get(), query_stmt.get() ) , futureDeleter()    );
        result.reset        ( cass_future_get_result(result_future.get()), resultDeleter() );

        rows.reset( cass_iterator_from_result(result.get()), iteratorDeleter() );
        cass_iterator_next(rows.get());
        row         = cass_iterator_get_row( rows.get() );
        count_value = cass_row_get_column_by_name( row, "count" );
        cass_value_get_int32( count_value, &count );
    }

    return count > 0;
}


int32_t QCassandraTable::getCurrentCount()
{
    if( !f_currentQueryResult )
    {
        throw std::runtime_error( "There is no query active!" );
    }

    iterator_pointer_t rows( cass_iterator_from_result(f_currentQueryResult.get()), iteratorDeleter()   );
    cass_iterator_next(rows.get());
    const CassRow*   row         = cass_iterator_get_row( rows.get() );
    const CassValue* count_value = cass_row_get_column_by_name( row, "count" );
    int32_t count;
    cass_value_get_int32( count_value, &count );

    return count;
}


/** \brief Read a set of rows as defined by the row predicate.
 *
 * This function reads a set of rows as defined by the row predicate.
 *
 * To change the consistency for this read, check out the
 * QCassandraColumnPredicate::setConsistencyLevel() function.
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
 * \sa QCassandraColumnPredicate::setConsistencyLevel()
 * \sa dropRow()
 */
uint32_t QCassandraTable::readRows( QCassandraRowPredicate& row_predicate )
{
    f_rows.clear();

    if( &row_predicate != f_currentPredicate )
    {
        f_queryStmt.reset();
    }

    f_currentPredicate = &row_predicate;

    if( f_queryStmt )
    {
        if( !cass_result_has_more_pages(f_currentQueryResult.get()) )
        {
            f_queryStmt.reset();
            return 0;
        }

        cass_statement_set_paging_state( f_queryStmt.get(), f_currentQueryResult.get() );
    }
    else
    {
        const QString query( QString("SELECT COUNT(*) AS count,key,column1,value FROM %1.%2").arg(f_context->contextName()).arg(f_tableName) );
        //
        f_queryStmt.reset( cass_statement_new( query.toUtf8().data(), 0 ), statementDeleter() );
        cass_statement_set_paging_size( f_queryStmt.get(), row_predicate.count() );
    }

    f_sessionExecute.reset( cass_session_execute( f_context->parentCassandra()->session().get(), f_queryStmt.get() ) , futureDeleter()    );
    throwIfError( f_sessionExecute, QString("Cannot select from table '%1'!").arg(f_tableName) );

    f_currentQueryResult.reset( cass_future_get_result(f_sessionExecute.get()), resultDeleter() );

    return getCurrentCount();
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
 * \return A shared pointer to the matching row or a null pointer.
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
 * \return A constant reference to a map of rows.
 */
const QCassandraRows& QCassandraTable::rows()
{
    if( !f_queryStmt )
    {
        throw std::runtime_error( "No query is in effect!" );
    }

    if( !f_rows.empty() )
    {
        // We already read these, so we don't need to do it again.
        //
        // TODO:
        // Not sure how the cpp driver works--if it caches this stuff up, so this might be
        // a premature optimization.
        return f_rows;
    }

    iterator_pointer_t  rows( cass_iterator_from_result(f_currentQueryResult.get()), iteratorDeleter() );
    while( cass_iterator_next(rows.get()) )
    {
        const CassRow*   iter_row        = cass_iterator_get_row( rows.get() );
        const CassValue* key_value       = cass_row_get_column_by_name( iter_row, "key"       );
        const CassValue* column1_value   = cass_row_get_column_by_name( iter_row, "column1"   );
        const CassValue* value_value     = cass_row_get_column_by_name( iter_row, "value"     );

        const char *    byte_value;
        size_t          value_len;
        QByteArray		ba( byte_value, value_len );
        //
        cass_value_get_string( key_value, &byte_value, &value_len );
        QCassandraRow::pointer_t new_row( row( ba ) );

        cass_value_get_string( column1_value, &byte_value, &value_len );
        QCassandraCell::pointer_t new_cell( new_row->cell( QByteArray(byte_value,value_len) ) );

        cass_value_get_string( value_value, &byte_value, &value_len );
        QCassandraValue val( QByteArray(byte_value,value_len) );
        new_cell->assignValue( val );
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
 */
QCassandraRow::pointer_t QCassandraTable::findRow(const QString& row_name)
{
    return findRow(row_name.toUtf8());
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
bool QCassandraTable::exists(const QString& row_name)
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
bool QCassandraTable::exists(const QByteArray& row_key)
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

    return rowCount( row_key ) > 0;
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

#if 0
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
    const QCassandraRow::pointer_t p_row( findRow(row_name) );
    if( !p_row )
    {
        throw std::runtime_error("row does not exist so it cannot be read from");
    }
    return *p_row;
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
#endif


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
 * was defined in a cell.
 *
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[in] value  The new value of the cell.
 */
void QCassandraTable::insertValue(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value)
{
#if 0
    if(f_from_cassandra)
    {
        if(f_private->default_validation_class == "CounterColumnType") {
            // we cannot "set" a counter, but we can simulate that function
            QCassandraValue v;
            f_context->getCounter(tableName(), row_key, column_key, v);
            // new value = user value - current value
            int64_t add(-v.int64Value());
            switch(value.size()) {
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
            f_context->addValue(tableName(), row_key, column_key, add);
        }
        else {
            f_context->insertValue(tableName(), row_key, column_key, value);
        }
    }
#endif

    const QString query_string(QString("INSERT OR UPDATE %1.%2 (key,column1,value) VALUES (?,?,?);")
            .arg(f_context->contextName())
            .arg(f_tableName)
            );

    statement_pointer_t query_stmt( cass_statement_new( query_string.toUtf8().data(), 3 ), statementDeleter() );

    auto binary_val( value.binaryValue() );
    cass_statement_bind_string_n( query_stmt.get(), 0, row_key.constData(),    row_key.size()    );
    cass_statement_bind_string_n( query_stmt.get(), 1, column_key.constData(), column_key.size() );
    cass_statement_bind_string_n( query_stmt.get(), 2, binary_val.constData(), binary_val.size() );

    future_pointer_t result_future( cass_session_execute( f_context->parentCassandra()->session().get(), query_stmt.get() ), futureDeleter() );
    cass_future_wait( result_future.get() );

    throwIfError( result_future, QString("Cannot insert into table '%1.%2'").arg(f_context->contextName()).arg(f_tableName) );
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
#if 0
    if(f_from_cassandra)
    {
        if(f_private->default_validation_class == "CounterColumnType") {
            return f_context->getCounter(tableName(), row_key, column_key, value);
        }
        else {
            return f_context->getValue(tableName(), row_key, column_key, value);
        }
    }
    return false;
#endif
    const QString query_string(QString("SELECT value FROM %1.%2 WHERE key = ? AND column1 = ?;")
            .arg(f_context->contextName())
            .arg(f_tableName)
            );

    statement_pointer_t query_stmt( cass_statement_new( query_string.toUtf8().data(), 2 ), statementDeleter() );

    auto binary_val( value.binaryValue() );
    cass_statement_bind_string_n( query_stmt.get(), 0, row_key.constData(),    row_key.size()    );
    cass_statement_bind_string_n( query_stmt.get(), 1, column_key.constData(), column_key.size() );

    std::shared_ptr<CassFuture> result_future( cass_session_execute( f_context->parentCassandra()->session().get(), query_stmt.get() ), futureDeleter() );
    cass_future_wait( result_future.get() );

    //throwIfError( result_future, QString("Cannot select key/column from table '%1.%2'").arg(f_contextName).arg(f_tableName) );

    const CassError code( cass_future_error_code( result_future.get() ) );
    if( code == CASS_OK )
    {
        result_pointer_t    result ( cass_future_get_result(result_future.get()), resultDeleter()   );
        //
        if( cass_result_row_count( result.get() ) > 1 )
        {
            std::stringstream ss;
            ss << "Query [" << query_stmt << "] returned more than one row!";
            throw std::runtime_error( ss.str().c_str() );
        }

        iterator_pointer_t  rows   ( cass_iterator_from_result(result.get())    , iteratorDeleter() );
        cass_iterator_next(rows.get());

        const CassRow*   row     = cass_iterator_get_row       ( rows.get()   );
        const CassValue* q_value = cass_row_get_column_by_name ( row, "value" );
        //
        const char *    byte_value;
        size_t          value_len;
        cass_value_get_string( q_value, &byte_value, &value_len );
        value.setBinaryValue( QByteArray(byte_value, value_len) );

        return true;
    }

    return false;
}

#if 0
/** \brief Add a value to a Cassandra counter.
 *
 * This function calls the context addValue() function to add a value
 * to a Cassandra counter.
 *
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[in] value  The value to add to the counter. It may be negative.
 *
 * \return false when the value was not found in the database, true otherwise
 */
void QCassandraTable::addValue(const QByteArray& row_key, const QByteArray& column_key, int64_t value)
{
    if(f_private->default_validation_class != "CounterColumnType") {
        throw std::runtime_error("the add() function and operators cannot be used on a standard table, only on tables defined as counters");
    }
    if( f_from_cassandra )
    {
        //f_context->addValue(tableName(), row_key, column_key, value);
    }
    // else -- we don't currently handle in memory counters!?
}
#endif

/** \brief Assign a value to a cell in a row of this table.
 *
 * This function assigns the specified value to a cell in a row of this table.
 * It is an internal function used to save the results of a get_slice()
 * or get_range_slices() function calls.
 *
 * It also marks the data as coming from Cassandra so it remains cached as
 * expected (i.e. we avoid further reads until the cache gets cleared.)
 *
 * \param[in] row_key  The binary key of the row.
 * \param[in] column_key  The binary key of the cell.
 * \param[in] value  The value to assign to the named cell.
 */
void QCassandraTable::assignRow(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value)
{
    QCassandraRow::pointer_t r(row(row_key));
    QCassandraCell::pointer_t c(r->cell(column_key));
    c->assignValue(value); // assign the value to the cell so we avoid re-reading it
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
int32_t QCassandraTable::getCellCount(const QByteArray& row_key, const QCassandraColumnPredicate& /*column_predicate*/)
{
#if 0
    auto context( f_context.lock() );
    if(!context)
    {
        throw std::runtime_error("table was dropped and is not attached to a context anymore");
    }
    if(f_from_cassandra) {
        return context->getCellCount(tableName(), row_key, column_predicate);
    }
#endif
    if( f_rows.find( row_key ) == f_rows.end() )
    {
        throw std::runtime_error("Key not found--you need to call readRows()!");
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
void QCassandraTable::remove( const QByteArray& row_key, const QByteArray& column_key )
{
    const QString query(
        QString("DELETE FROM %1.%2 WHERE key=? AND column1=?;")
            .arg(f_context->contextName())
            .arg(f_tableName)
            );

    statement_pointer_t query_stmt( cass_statement_new( query.toUtf8().data(), 2 ), statementDeleter() );
    cass_statement_bind_string_n( query_stmt.get(), 0, row_key.constData(),    row_key.size()    );
    cass_statement_bind_string_n( query_stmt.get(), 1, column_key.constData(), column_key.size() );
    future_pointer_t result_future( cass_session_execute( f_context->parentCassandra()->session().get(), query_stmt.get() ) , futureDeleter()    );
    throwIfError( result_future, QString("Cannot select from table '%1'!").arg(f_tableName) );
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
    const QString query(
        QString("DELETE FROM %1.%2 WHERE key=?;")
            .arg(f_context->contextName())
            .arg(f_tableName)
            );

    statement_pointer_t query_stmt( cass_statement_new( query.toUtf8().data(), 1 ), statementDeleter() );
    cass_statement_bind_string_n( query_stmt.get(), 0, row_key.constData(),    row_key.size()    );
    future_pointer_t result_future( cass_session_execute( f_context->parentCassandra()->session().get(), query_stmt.get() ) , futureDeleter()    );
    throwIfError( result_future, QString("Cannot select from table '%1'!").arg(f_tableName) );
}

} // namespace QtCassandra

// vim: ts=4 sw=4 et
