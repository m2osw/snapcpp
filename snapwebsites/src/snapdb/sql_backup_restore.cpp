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

    struct cluster_deleter
    { 
        void operator()(CassCluster* p) const
        {
            cass_cluster_free(p);
        }
    };

    struct iterator_deleter
    { 
        void operator()(CassIterator* p) const
        {
            cass_iterator_free(p);
        }
    };

    struct future_deleter
    { 
        void operator()(CassFuture* p) const
        {
            cass_future_free(p);
        }
    };

    struct result_deleter
    {
        void operator()(const CassResult* p) const
        {
            cass_result_free(p);
        }
    };

    struct session_deleter
    { 
        void operator()(CassSession* p) const
        {
            cass_session_free(p);
        }
    };

    struct statement_deleter
    { 
        void operator()(CassStatement* p) const
        {
            cass_statement_free(p);
        }
    };

    void throw_if_error( std::shared_ptr<CassFuture> result_future, const QString& msg = "Cassandra error" )
    {
        const CassError code( cass_future_error_code( result_future.get() ) );
        if( code != CASS_OK )
        {
            std::stringstream ss;
            ss << msg << "! Cassandra error: code=" << static_cast<unsigned int>(code) << ", message={" << cass_error_desc(code) << "}, aborting operation!";
            throw std::runtime_error( ss.str().c_str() );
        }
    }
}
// unnamed namespace


sqlBackupRestore::sqlBackupRestore( const QString& host_name, const QString& sqlDbFile )
    : f_cluster( cass_cluster_new(), cluster_deleter() )
    , f_session( cass_session_new(), session_deleter() )
{
	QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );
	db.setDatabaseName( sqlDbFile );
	if( !db.open() )
    {
        const QString error( QString("Cannot open SQLite database [%1]!").arg(sqlDbFile) );
        throw std::runtime_error( error.toUtf8().data() );
    }

    cass_cluster_set_contact_points( f_cluster.get(), host_name.toUtf8().data() );
    f_connection.reset( cass_session_connect(f_session.get(), f_cluster.get()), future_deleter() );

    throw_if_error( f_connection, "Cassandra connection error" );
}


void sqlBackupRestore::storeContext( const int count )
{
    QSqlDatabase db( QSqlDatabase::database() );
    db.transaction();
    storeTables( count );
    db.commit();
}


void sqlBackupRestore::restoreContext()
{
    restoreTables();
}


#if 0
void sqlBackupRestore::outputSchema()
{
    const char* query( "DESCRIBE KEYSPACE snap_websites;" );
    std::shared_ptr<CassStatement> query_stmt    ( cass_statement_new( query, 0 )               , statement_deleter() );
    std::shared_ptr<CassFuture>    result_future ( cass_session_execute( f_session.get(), query_stmt.get() ), future_deleter()    );

    throw_if_error( result_future, "Cannot execute query for KEYSPACE" );

    std::shared_ptr<const CassResult> result ( cass_future_get_result(result_future.get()), result_deleter() );
    std::shared_ptr<CassIterator>     rows   ( cass_iterator_from_result(result.get())    , iterator_deleter() );

    while( cass_iterator_next(rows.get()) )
    {
        const CassRow*   row   = cass_iterator_get_row( rows.get() );
        const CassValue* value = cass_row_get_column( row, 0 );

        const char* byte_value;
        size_t      value_len;
        cass_value_get_string( value, &byte_value, &value_len );
        std::cout << byte_value << std::endl;
    }
}
#endif


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
void sqlBackupRestore::storeTables( const int count )
{
    snapTableList   dump_list;

    for( auto table_name : dump_list.tablesToDump() ) // f_context->tables() )
    {
        QString q_str = QString( "CREATE TABLE IF NOT EXISTS %1 "
                "( id INTEGER PRIMARY KEY"
                ", key LONGBLOB"
                ", column1 LONGBLOB"
                ", value LONGBLOB"
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

        const QString cql_select_string("SELECT key,column1,value FROM snap_websites.%1 LIMIT %2");
        if( table_name == "libQtCassandraLockTable" )
        {
            // TODO: ugly hack! We need to correct this in the cassandra table itself.
            q_str = cql_select_string.arg("\""+table_name+"\"").arg(count);
        }
        else
        {
            q_str = cql_select_string.arg(table_name).arg(count);
        }
        std::shared_ptr<CassStatement> query_stmt    ( cass_statement_new( q_str.toUtf8().data(), 0 ), statement_deleter() );
        std::shared_ptr<CassFuture>    result_future ( cass_session_execute( f_session.get(), query_stmt.get() ) , future_deleter()    );

        throw_if_error( result_future, QString("Cannot select from table '%1'!").arg(table_name) );

        std::shared_ptr<const CassResult> result ( cass_future_get_result(result_future.get()), result_deleter() );
        std::shared_ptr<CassIterator>     rows   ( cass_iterator_from_result(result.get())    , iterator_deleter() );

        while( cass_iterator_next(rows.get()) )
        {
            const CassRow*   row             = cass_iterator_get_row( rows.get() );
            const CassValue* key_value       = cass_row_get_column_by_name( row, "key"       );
            const CassValue* column1_value   = cass_row_get_column_by_name( row, "column1"   );
            const CassValue* value_value     = cass_row_get_column_by_name( row, "value"     );

            const char *    byte_value;
            size_t          value_len;

            q_str = QString( "INSERT OR REPLACE INTO %1 "
                    "(key, column1, value ) "
                    "VALUES "
                    "(:key, :column1, :value );"
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
            if( !q.exec() )
            {
                std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
                throw std::runtime_error( q.lastError().text().toUtf8().data() );
            }
        }
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

        const QString sql_select_string ("SELECT key,column1,value FROM %1");
        QString q_str                   ( sql_select_string.arg(table_name) );
        QSqlQuery q;
        q.prepare( q_str );
        if( !q.exec() )
        {
            std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
            std::cerr << "query error=[" << q.lastError().text() << "]" << std::endl;
            throw std::runtime_error( q.lastError().text().toUtf8().data() );
        }

        const int key_idx       = q.record().indexOf("key");
        const int column1_idx   = q.record().indexOf("column1");
        const int value_idx     = q.record().indexOf("value");
        for( q.first(); q.isValid(); q.next() )
        {
            const QByteArray key       ( q.value( key_idx       ).toByteArray() );
            const QByteArray column1   ( q.value( column1_idx   ).toByteArray() );
            const QByteArray value     ( q.value( value_idx     ).toByteArray() );

            const QString cql_insert_string("INSERT INTO snap_websites.%1 (key,column1,value) VALUES (?,?,?);");
            const QString qstr( cql_insert_string.arg(target_table_name) );

            std::shared_ptr<CassStatement> query_stmt( cass_statement_new( qstr.toUtf8().data(), 3 ), statement_deleter() );

            cass_statement_bind_string_n( query_stmt.get(), 0, key.constData(),     key.size()     );
            cass_statement_bind_string_n( query_stmt.get(), 1, column1.constData(), column1.size() );
            cass_statement_bind_string_n( query_stmt.get(), 2, value.constData(),   value.size()   );

            std::shared_ptr<CassFuture> result_future( cass_session_execute( f_session.get(), query_stmt.get() ), future_deleter() );
            cass_future_wait( result_future.get() );
            //
            throw_if_error( result_future, QString("Cannot insert into table 'snap_websites.%1'").arg(table_name) );
        }
    }
}


// vim: ts=4 sw=4 et
