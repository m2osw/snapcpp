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


void sqlBackupRestore::storeContext()
{
    QSqlDatabase db( QSqlDatabase::database() );
    db.transaction();
    storeTables();
    db.commit();
}


void sqlBackupRestore::restoreContext()
{
    restoreTables();
}


/// \brief Backup snap_websites tables.
//
// This does not dump the Cassandra schema. In order to obtain this, run the following command on a
// Cassandra node:
//
//      cqlsh -e "DESCRIBE snap_websites" > schema.sql
// 
// The above command creates an SQL file that can be reimported into your Cassandra node.
//
// Then you can call this method.
//
void sqlBackupRestore::storeTables()
{
    snapTableList   dump_list;

    for( auto table_name : dump_list.tablesToDump() ) // f_context->tables() )
    {
        QString q_str = QString( "CREATE TABLE IF NOT EXISTS %1 ("
                "id INTEGER PRIMARY KEY, "
                "key LONGBLOB, "
                "column1 LONGBLOB, "
                "value LONGBLOB, "
                "ttl INTEGER, "
                "writetime INTEGER "
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

        std::cout << "Dumping table [" << table_name << "]" << std::endl;

        const QString cql_select_string("SELECT key,column1,value,ttl(value),writetime(value) FROM snap_websites.%1 LIMIT 10000000");
        if( table_name == "libQtCassandraLockTable" )
        {
            // TODO: ugly hack! We need to correct this in the cassandra table itself.
            q_str = cql_select_string.arg("\""+table_name+"\"");
        }
        else
        {
            q_str = cql_select_string.arg(table_name);
        }
        CassStatement* query_stmt    = cass_statement_new( q_str.toUtf8().data(), 0 );
        CassFuture*    result_future = cass_session_execute( f_session, query_stmt );

        if( cass_future_error_code( result_future ) != CASS_OK )
        {
            const char* message;
            size_t message_length;
            cass_future_error_message( result_future, &message, &message_length );
            std::cerr << "Cassandra query error: " << message << std::endl;
            cass_future_free(result_future);
            cass_statement_free(query_stmt);
            throw std::runtime_error( message );
        }

        const CassResult* result = cass_future_get_result(result_future);
        CassIterator*     rows   = cass_iterator_from_result(result);

        while( cass_iterator_next(rows) )
        {
            const CassRow*   row             = cass_iterator_get_row( rows );
            const CassValue* key_value       = cass_row_get_column_by_name( row, "key"       );
            const CassValue* column1_value   = cass_row_get_column_by_name( row, "column1"   );
            const CassValue* value_value     = cass_row_get_column_by_name( row, "value"     );
            const CassValue* ttl_value       = cass_row_get_column_by_name( row, "ttl"       );
            const CassValue* writetime_value = cass_row_get_column_by_name( row, "writetime" );

            const char *    byte_value;
            cass_uint32_t   uint32_value;
            size_t          value_len;

            q_str = QString( "INSERT OR REPLACE INTO %1 "
                    "(key, column1, value, ttl, writetime) "
                    "VALUES "
                    "(:key, :column1, :value, :ttl, :writetime);"
                    ).arg(table_name);
            q.clear();
            q.prepare( q_str );
            //
            cass_value_get_string( key_value, &byte_value, &value_len );
            q.bindValue( ":key", QByteArray(byte_value, value_len) );
            //
            cass_value_get_string( column1_value, &byte_value, &value_len );
            q.bindValue( ":column1", QByteArray(byte_value,value_len) );
            //
            cass_value_get_string( value_value, &byte_value, &value_len );
            q.bindValue( ":value", QByteArray(byte_value,value_len) );
            //
            cass_value_get_uint32( ttl_value, &uint32_value );
            q.bindValue( ":ttl", uint32_value );
            //
            cass_value_get_uint32( writetime_value, &uint32_value );
            q.bindValue( ":writetime", uint32_value );
            //
            if( !q.exec() )
            {
                std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
                throw std::runtime_error( q.lastError().text().toUtf8().data() );
            }
        }

        cass_result_free(result);
        cass_iterator_free(rows);
        cass_future_free(result_future);
        cass_statement_free(query_stmt);
    }
}


/// \brief Restore snap_websites tables.
//
// This assumes that the Cassandra schema has been created already. On backup, follow the instructions
// above sqlBackupRestore::storeTable() to create your schema.sql file. Then dump the database.
//
// In order to restore, drop the "snap_websites" context on the Cassandra node you wish to restore.
// Then run the following commands:
//
//      snapdb --drop-context
//      cqlsh -f schema.sql
//
// Then call this method.
//
void sqlBackupRestore::restoreTables()
{
    snapTableList   dump_list;

    for( auto table_name : dump_list.tablesToDump() ) // f_context->tables() )
    {
        std::cout << "Restoring table [" << table_name << "]" << std::endl;

        // TODO: Ugly hack!
        //
        const QString target_table_name(
            table_name == "libQtCassandraLockTable"
                ? "\"libQtCassandraLockTable\""
                : table_name
            );

        const QString sql_select_string ("SELECT key,column1,value,ttl,writetime FROM %1");
        QString q_str                   ( sql_select_string.arg(table_name) );
        QSqlQuery q;
        q.prepare( q_str );
        if( !q.exec() )
        {
            std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
            std::cerr << "query error=[" << q.lastError().text() << "]" << std::endl;
            throw std::runtime_error( q.lastError().text().toUtf8().data() );
        }

        // TODO: have to import TTLs, but they are too big for the CQL interface to accept!
        const int key_idx       = q.record().indexOf("key");
        const int column1_idx   = q.record().indexOf("column1");
        const int value_idx     = q.record().indexOf("value");
        const int ttl_idx       = q.record().indexOf("ttl");
        const int writetime_idx = q.record().indexOf("writetime");
        for( q.first(); q.isValid(); q.next() )
        {
            const QByteArray key       ( q.value( key_idx       ).toByteArray() );
            const QByteArray column1   ( q.value( column1_idx   ).toByteArray() );
            const QByteArray value     ( q.value( value_idx     ).toByteArray() );
            const uint32_t   ttl       ( q.value( ttl_idx       ).toUInt()      );
            const uint32_t   writetime ( q.value( writetime_idx ).toUInt()      );

            const QString cql_insert_string("INSERT INTO snap_websites.%1 (key,column1,value) VALUES (?,?,?) USING TTL ? AND TIMESTAMP ?;");
            //const QString cql_insert_string("INSERT INTO snap_websites.%1 (key,column1,value) VALUES (?,?,?);");
            const QString qstr( cql_insert_string.arg(target_table_name) );

            // TODO: put this into RAII class
            //
            std::cout << "TTL = " << ttl << ", TIMESTAMP = " << writetime << std::endl;
            CassStatement* query_stmt    = cass_statement_new( qstr.toUtf8().data(), 5 );

            cass_statement_bind_string_n( query_stmt, 0, key.constData(),     key.size()     );
            cass_statement_bind_string_n( query_stmt, 1, column1.constData(), column1.size() );
            cass_statement_bind_string_n( query_stmt, 2, value.constData(),   value.size()   );
            cass_statement_bind_uint32  ( query_stmt, 3, ttl       );
            cass_statement_bind_uint32  ( query_stmt, 4, writetime );

            CassFuture*    result_future = cass_session_execute( f_session, query_stmt );
            cass_future_wait( result_future );
            //
            if( cass_future_error_code( result_future ) != CASS_OK )
            {
                const char* message;
                size_t message_length;
                cass_future_error_message( result_future, &message, &message_length );
                std::cerr << "Cassandra query error: " << message << ", query=" << qstr << std::endl;
                cass_future_free(result_future);
                cass_statement_free(query_stmt);
                throw std::runtime_error( message );
            }
            //
            cass_future_free(result_future);
            cass_statement_free(query_stmt);
        }
    }
}


// vim: ts=4 sw=4 et
