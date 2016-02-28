/*
 * Text:
 *      sql_backup_restore.cpp
 *
 * Description:
 *      Reads and describes a Snap database. This ease checking out the
 *      current content of the database as the cassandra-cli tends to
 *      show everything in hexadecimal number which is quite unpractical.
 *      Now we do it that way for runtime speed which is much more important
 *      than readability by humans, but we still want to see the data in an
 *      easy practical way which this tool offers.
 *
 * License:
 *      Copyright (c) 2012-2016 Made to Order Software Corp.
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

// our lib
//
#include "sql_backup_restore.h"
#include "snap_table_list.h"
#include "qstring_stream.h"
#include "dbutils.h"

// 3rd party libs
//
#include <QtCore>
#include <QtSql>

// system
//
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace QtCassandra;

namespace
{
    void do_query( const QString& q_str )
    {
        QSqlQuery q;
        if( !q.exec( q_str ) )
        {
            std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
            throw std::runtime_error( q.lastError().text().toUtf8().data() );
        }
    }
}


sqlBackupRestore::sqlBackupRestore( const QString& host_name, const QString& sqlDbFile )
    : f_cluster( cass_cluster_new() )
    , f_session( cass_session_new() )
{
	QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );
	db.setDatabaseName( sqlDbFile );
	if( !db.open() )
    {
        const QString error( QString("Cannot open SQLite database [%1]!").arg(sqlDbFile) );
        throw std::runtime_error( error.toUtf8().data() );
    }

    cass_cluster_set_contact_points( f_cluster, host_name.toUtf8().data() );
    f_connection = cass_session_connect( f_session, f_cluster );

    /* This operation will block until the result is ready */
    CassError rc = cass_future_error_code(f_connection);
    std::cout << "Connect result: [" << cass_error_desc(rc) << "]" << std::endl;

    if( rc != CASS_OK )
    {
        const char* message;
        size_t message_length;
        cass_future_error_message(f_connection, &message, &message_length);
        std::cerr << "Cannot connect to cassandra server! " << message << std::endl;
        throw std::runtime_error( message );
    }
}


#if 0
void sqlBackupRestore::createSchema( const QString& sqlDbFile )
{
	QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );
	db.setDatabaseName( sqlDbFile );
	if( !db.open() )
    {
        const QString error( QString("Cannot open SQLite database [%1]!").arg(sqlDbFile) );
        throw std::runtime_error( error.toUtf8().data() );
    }

    do_query( "CREATE TABLE IF NOT EXISTS snap_context   ("
                "id INTEGER PRIMARY KEY, "
                "context_name TEXT UNIQUE, "
                "strategy_class TEXT, "
                "replication_factor INTEGER, "
                "durable_writes BOOLEAN, "
                "host_name TEXT, "
                "last_update TEXT"
                ");"
                );
    do_query( "CREATE TABLE IF NOT EXISTS snap_context_desc_options   ("
                "id INTEGER PRIMARY KEY, "
                "context_id INTEGER, "
                "option TEXT UNIQUE, "
                "value TEXT"
                ");"
                );
    do_query( "CREATE TABLE IF NOT EXISTS snap_tables ("
                "id INTEGER PRIMARY KEY, "
                "table_name TEXT UNIQUE, "
                "identifier INTEGER, "
                "column_type TEXT, "
                "comment TEXT, "
                "key_validation_class TEXT, "
                "default_validation_class TEXT, "
                "comparator_type TEXT, "
                "key_cache_save_period_secs INTEGER, "
                "memtable_flush_period_mins INTEGER, "
                "gc_grace_secs INTEGER, "
                "min_compaction_threshold INTEGER, "
                "max_compaction_threshold INTEGER, "
                "replicate_on_write BOOLEAN"
                ");"
                );
    do_query( "CREATE TABLE IF NOT EXISTS snap_rows   (id INTEGER PRIMARY KEY, row_name   TEXT, table_id    INTEGER);" );
    do_query( "CREATE TABLE IF NOT EXISTS snap_cells  (id INTEGER PRIMARY KEY, cell_name  TEXT, row_id      INTEGER, "
        "ttl INTEGER, consistency_level INTEGER, timestamp INTEGER, cell_value LONGBLOB);" ); // TODO: Do we need to have timestamp_mode? There are two timestamps; one for the cell, and one for the value--are they both needed?
}


void sqlBackupRestore::storeContext()
{
    QSqlDatabase db( QSqlDatabase::database() );
    db.transaction();

    // CONTEXT
    //
    writeContext();

    // TABLES
    //
    storeTables();

    // ROWS
    //
    for( auto table : f_tableList )
    {
        storeRowsByTable( table );
    }

    // CELLS
    //
    for( auto row : f_rowList )
    {
        storeCellsByRow( row );
    }

    db.commit();
}


int sqlBackupRestore::getTableId( QCassandraTable::pointer_t table )
{
    QSqlQuery q;
    q.prepare( "SELECT id FROM snap_tables WHERE table_name = :table_name;" );
    q.bindValue( ":table_name", table->tableName() );
    if( !q.exec() )
    {
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }
    //
    q.first();
    if( !q.isValid() )
    {
        throw std::runtime_error( "database is inconsistent!" );
    }
    return q.value(0).toInt();
}


int sqlBackupRestore::getRowId( QCassandraRow::pointer_t row )
{
    QSqlQuery q;
    q.prepare( "SELECT snap_rows.id FROM snap_rows,snap_tables "
        "WHERE snap_rows.row_name = :row_name "
            "AND snap_rows.table_id = snap_tables.id "
            "AND snap_tables.table_name = :table_name;"
        );
    const QString tableName( row->parentTable()->tableName() );
    snap::dbutils du( tableName, row->rowName() );
    q.bindValue( ":row_name",   du.get_row_name(row) );
    q.bindValue( ":table_name", tableName            );
    if( !q.exec() )
    {
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }
    //
    q.first();
    if( !q.isValid() )
    {
        throw std::runtime_error( "database is inconsistent!" );
    }
    return q.value(0).toInt();
}


void sqlBackupRestore::writeContext()
{
    QSqlQuery q;

    q.prepare( "INSERT OR REPLACE INTO snap_context "
        "(context_name,strategy_class,replication_factor,durable_writes,host_name,last_update) VALUES "
        "(:context_name,:strategy_class,:replication_factor,:durable_writes,:host_name,:last_update);"
        );
    q.bindValue( ":context_name:",      f_context->contextName()                );
    q.bindValue( ":strategy_class",     f_context->strategyClass()              );
    q.bindValue( ":replication_factor", f_context->replicationFactor()          );
    q.bindValue( ":durable_writes",     f_context->durableWrites()              );
    q.bindValue( ":host_name",          f_context->hostName()                   );
    q.bindValue( ":last_update",        QDateTime::currentDateTime().toString() );
    if( !q.exec() )
    {
        std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }

    q.clear();
    q.prepare( "SELECT id FROM snap_context WHERE context_name = :context_name;" );
    q.bindValue( ":context_name", f_context->contextName() );
    if( !q.exec() )
    {
        std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }
    //
    q.first();
    const int context_id = q.value(0).toInt();

    for( auto option : f_context->descriptionOptions().keys() )
    {
        q.clear();
        q.prepare( "INSERT OR REPLACE INTO snap_context_desc_options (context_id,option,value) VALUES (:context_id,:option,:value);" );
        q.bindValue( ":context_id", context_id                           );
        q.bindValue( ":option",     option                               );
        q.bindValue( ":value",      f_context->descriptionOption(option) );
        if( !q.exec() )
        {
            std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
            throw std::runtime_error( q.lastError().text().toUtf8().data() );
        }
    }
}
#endif


void sqlBackupRestore::storeContext()
{
    QSqlDatabase db( QSqlDatabase::database() );
    db.transaction();

    // TABLES
    //
    storeTables();

    db.commit();
}


void sqlBackupRestore::restoreContext()
{
    std::cerr << "--restore-context has not been implemented yet for SQLite. Exiting..." << std::endl;
}

void sqlBackupRestore::storeTables()
{
    snapTableList   dump_list;

    for( auto table_name : dump_list.tablesToDump() ) // f_context->tables() )
    {
        QString q_str = QString( "CREATE TABLE IF NOT EXISTS %1 ("
                "id INTEGER PRIMARY KEY, "
                "key LONGBLOB, "
                "column1 LONGBLOB, "
                "value LONGBLOB "
                ");"
                ).arg(table_name);
        QSqlQuery q;
        q.prepare( q_str );
        if( !q.exec() )
        {
            std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
            std::cerr << "query error=[" << q.lastError().text() << "]" << std::endl;
            throw std::runtime_error( q.lastError().text().toUtf8().data() );
        }

        std::cout << "Processing table [" << table_name << "]" << std::endl;

        q_str = QString("SELECT key,column1,value FROM snap_websites.%1").arg(table_name);
        CassStatement* query_stmt    = cass_statement_new( q_str.toUtf8().data(), 0 );
        CassFuture*    result_future = cass_session_execute( f_session, query_stmt );

        if( cass_future_error_code( result_future ) != CASS_OK )
        {
            const char* message;
            size_t message_length;
            cass_future_error_message( result_future, &message, &message_length );
            std::cerr << "Cassandra query error: " << message << std::endl;
            throw std::runtime_error( message );
        }

        const CassResult* result = cass_future_get_result(result_future);
        CassIterator*     rows   = cass_iterator_from_result(result);

        while( cass_iterator_next(rows) )
        {
            const CassRow*   row           = cass_iterator_get_row( rows );
            const CassValue* key_value     = cass_row_get_column_by_name( row, "key"     );
            const CassValue* column1_value = cass_row_get_column_by_name( row, "column1" );
            const CassValue* value_value   = cass_row_get_column_by_name( row, "value"   );

            const char* value;
            size_t value_len;

            q_str = QString( "INSERT OR REPLACE INTO %1 "
                    "(key, column1, value) "
                    "VALUES "
                    "(:key,:column1,:value);"
                    ).arg(table_name);
            q.clear();
            q.prepare( q_str );
            //
            cass_value_get_string( key_value, &value, &value_len );
            q.bindValue( ":key",     value  );
            //
            cass_value_get_string( column1_value, &value, &value_len );
            q.bindValue( ":column1", value );
            //
            cass_value_get_string( value_value, &value, &value_len );
            q.bindValue( ":value",   value );
            //
            if( !q.exec() )
            {
                std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
                throw std::runtime_error( q.lastError().text().toUtf8().data() );
            }
        }

        cass_result_free(result);
        cass_iterator_free(rows);
    }
}


#if 0
void sqlBackupRestore::storeRowsByTable( QCassandraTable::pointer_t table )
{
    f_colPred.reset( new QCassandraColumnRangePredicate );
    f_colPred->setCount(1000);
    //
    f_rowPred.setStartRowName("");
    f_rowPred.setEndRowName("");
    f_rowPred.setCount(100);
    f_rowPred.setColumnPredicate( f_colPred );

    uint32_t rowsRemaining = table->readRows( f_rowPred );

    if( table->rows().isEmpty() )
    {
        std::cout << "Table [" << table->tableName() << "] has no rows, so skipping..." << std::endl;
        return;
    }

    const int       table_id = getTableId( table );
    snapTableList   dump_list;

    while( true )
    {
        for( auto row : table->rows() )
        {
            if( !dump_list.canDumpRow( table->tableName(), row->rowName() ) )
            {
                std::cout << "Skipping the '"
                          << row->rowName()
                          << "' row of the '"
                          << table->tableName()
                          << "' table."
                          << std::endl;
                continue;
            }

            snap::dbutils	du( table->tableName(), row->rowName() );
            const QString	row_name( du.get_row_name(row) );
            std::cout << "Processing table [" << table->tableName()
                      << "], row [" << row_name << "]"
                      << std::endl;
            f_rowList << row;

            QSqlQuery q;
            q.prepare( "INSERT OR REPLACE INTO snap_rows (row_name,table_id) VALUES (:row_name,:table_id);" );
            q.bindValue( ":row_name", row_name );
            q.bindValue( ":table_id", table_id );
            if( !q.exec() )
            {
                std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
                throw std::runtime_error( q.lastError().text().toUtf8().data() );
            }
        }
        //
        rowsRemaining = table->readRows( f_rowPred ); // Next 100 records
        //
        if( rowsRemaining == 0 )
        {
            break;
        }
    }
}


void sqlBackupRestore::storeCellsByRow( QCassandraRow::pointer_t row )
{
    f_colPred.reset( new QCassandraColumnRangePredicate );
    f_colPred->setCount(1000);

    // This seems to be a bug. The colsRemaining return value never changes with each read.
    //
    /*uint32_t colsRemaining =*/ row->readCells( *f_colPred );
#if 0
    while( true )
    {
#endif
        const QString tableName( row->parentTable()->tableName() );
        snap::dbutils du( tableName, row->rowName() );
        const QString rowName( du.get_row_name(row) );

        if( row->cells().isEmpty() )
        {
            std::cout << "Row [" << rowName << "] has no cells, so skipping..." << std::endl;
            return;
        }

        std::cout << "Processing cells for row [" << rowName << "] in table [" << tableName << "]:" << std::endl;
        const int row_id = getRowId( row );

        for( auto col : row->cells() )
        {
            const QString cell_name( du.get_column_name(col) );
            std::cout << "Processing cell [" << cell_name << "]" << std::endl;

            auto& val( col->value() );
            QSqlQuery q;
            q.prepare(
                    "INSERT OR REPLACE INTO snap_cells "
                    "(cell_name,row_id,ttl,consistency_level,timestamp,cell_value) "
                    "VALUES (:cell_name,:row_id,:ttl,:consistency_level,:timestamp,:cell_value);"
                    );
            q.bindValue( ":cell_name",        cell_name                                 );
            q.bindValue( ":row_id",           row_id                                    );
            q.bindValue( ":ttl",              val.ttl()                                 );
            q.bindValue( "consistency_level", static_cast<int>(col->consistencyLevel()) );
            q.bindValue( ":timestamp",        static_cast<qlonglong>(col->timestamp())  );
            q.bindValue( ":cell_value",       val.binaryValue()                         );
            if( !q.exec() )
            {
                std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
                throw std::runtime_error( q.lastError().text().toUtf8().data() );
            }
        }

#if 0
        if( colsRemaining == 0 )
        {
            break;
        }
        //
        colsRemaining = row->readCells( *f_colPred ); // Next 100 columns
    }
#endif

    std::cout << "Done." << std::endl << std::endl;
}

#if 0
bool snapdb::restore_context()
{
    f_cassandra->connect(f_host, f_port);
    const QString infile( f_opt->get_string( "restore-context" ).c_str() );

    sqlBackupRestore backup( f_cassandra, f_context, infile );
    backup.restoreContext();

    //std::cerr << "--restore-context has not been implemented yet for SQLite. Exiting..." << std::endl;
    return true;
#if 0
    f_cassandra->connect(f_host, f_port);

    if( f_cassandra->findContext( f_context ) )
    {
        std::cerr << "The " << f_context.toUtf8().data() << " context already exists! This feature will not overwrite existing data. Please drop the context first." << std::endl;
        return false;
    }

    QCassandraContext::pointer_t context( f_cassandra->context( f_context ) );

    const QString infile( f_opt->get_string( "restore-context" ).c_str() );
    std::shared_ptr<QFile> qif;
    if( infile.isEmpty() )
    {
        qif.reset( new QFile );
        qif->open( stdin, QIODevice::ReadOnly );
    }
    else
    {
        QFileInfo fi( infile );
        if( !fi.exists() )
        {
            std::cerr << "Input file does not exist!" << std::endl;
            return false;
        }

        qif.reset( new QFile( infile ) );
        qif->open( QIODevice::ReadOnly );
    }

    QCassandraTable::pointer_t table;
    QCassandraRow::pointer_t   row;
    QCassandraCell::pointer_t  cell;

    QXmlStreamReader stream( qif.get() );
    while( !stream.atEnd() )
    {
        QXmlStreamReader::TokenType type = stream.readNext();
        if( type == QXmlStreamReader::StartElement )
        {
            QXmlStreamAttributes attribs( stream.attributes() );
            if( stream.name() == "table" )
            {
                Q_ASSERT(context);
                table = context->table( attribs.value("name").toString() );
            }
            else if( stream.name() == "row" )
            {
                Q_ASSERT(table);
                row = table->row( attribs.value("name").toString() );
            }
            else if( stream.name() == "col" )
            {
                Q_ASSERT(row);
                cell = row->cell( attribs.value("name").toString() );
            }
        }
        else if( type == QXmlStreamReader::Characters )
        {
            if( !stream.isWhitespace() )
            {
                Q_ASSERT(cell);
                cell->setValue( stream.text().toString() );
            }
        }
    }

    if( stream.hasError() )
    {
        std::cerr << "Error in XML: [" << stream.errorString().toUtf8().data() << "]!" << std::endl;
        std::cerr << "Failed at line "
                  << stream.lineNumber() << ", token=["
                  << stream.tokenString().toUtf8().data() << "], name=["
                  << stream.name().toUtf8().data() << "]" << std::endl;
        return false;
    }

    return true;
#endif
}
#endif
#endif


// vim: ts=4 sw=4 et
