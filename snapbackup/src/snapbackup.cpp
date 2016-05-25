/*
 * Text:
 *      snapbackup.cpp
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
#include "snapbackup.h"
#include "snap_table_list.h"
#include "qstring_stream.h"
#include "dbutils.h"

#include <QtCassandra/QCassandraSchema.h>

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

snapbackup::snapbackup( getopt_ptr_t opt )
    : f_session( QCassandraSession::create() )
    , f_opt(opt)
{
}


void snapbackup::setSqliteDbFile( const QString& sqlDbFile )
{
	QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );
	db.setDatabaseName( sqlDbFile );
	if( !db.open() )
    {
        const QString error( QString("Cannot open SQLite database [%1]!").arg(sqlDbFile) );
        throw std::runtime_error( error.toUtf8().data() );
    }
}


void snapbackup::connectToCassandra()
{
    f_session->connect( f_opt->get_string("host"), f_opt->get_long("port") );
}


void snapbackup::storeSchema( const QString& context_name )
{
}


void snapbackup::dropContext( const QString& context_name )
{
    std::cout << QString("Dropping context [%1]...").arg(context_name);
    QCassandraQuery q;
    q.query( QString("DROP KEYSPACE IF EXISTS %1").arg(context_name) );
    q.start( false );
    while( !q.isReady() )
    {
        sleep(1);
    }
    q.getQueryResult();
    q.end();
}


void snapbackup::dumpContext()
{
    QSqlDatabase db( QSqlDatabase::database() );
    if( !db.isOpen() )
    {
        throw std::runtime_error( "SQLite database not opened!" );
    }

    db.transaction();
    storeTables( f_opt->get_long("count"), f_opt->get_string("context_name") );
    db.commit();
}


void snapbackup::restoreContext()
{
    QSqlDatabase db( QSqlDatabase::database() );
    if( !db.isOpen() )
    {
        throw std::runtime_error( "SQLite database not opened!" );
    }

    if( f_opt->is_defined("drop-context-first") )
    {
        dropContext( f_opt->get_string("context_name").c_str() );
    }

    restoreTables( f_opt->get_string("context_name") );
}


void snapbackup::appendRowsToSqliteDb( QCassandraQuery& cass_query, const QString& table_name )
{
    const QString q_str = QString( "INSERT OR REPLACE INTO %1 "
            "(key, column1, value ) "
            "VALUES "
            "(:key, :column1, :value );"
            ).arg(table_name);
    QSqlQuery q;
    q.prepare( q_str );
    //
    q.bindValue( ":key",     cass_query.getByteArrayColumn("key")     );
    q.bindValue( ":column1", cass_query.getByteArrayColumn("column1") );
    q.bindValue( ":value",   cass_query.getByteArrayColumn("value")   );
    //
    if( !q.exec() )
    {
        std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }
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
void snapbackup::storeTables( const int count, const QString& context_name )
{
    snapTableList   dump_list;

    for( auto const & table_name : dump_list.tablesToDump() )
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

        const QString cql_select_string("SELECT key,column1,value FROM snap_websites.%1");
        q_str = cql_select_string.arg(table_name);

        QCassandraQuery cass_query( f_session );
        cass_query.query( q_str );
        cass_query.setPagingSize( count );
        cass_query.start();

        do
        {
            while( cass_query.nextRow() )
            {
                appendRowsToSqliteDb( cass_query, table_name );
            }
        }
        while( cass_query.nextPage() );

        cass_query.end();
    }
}


/// \brief Restore snap_websites tables.
//
// This assumes that the Cassandra schema has been created already. On backup, follow the instructions
// above snapbackup::storeTable() to create your schema.sql file. Then dump the database.
//
// In order to restore, drop the "snap_websites" context on the Cassandra node you wish to restore.
// Then run the following commands:
//
//      snapdb --drop-context
//      cqlsh -f schema.sql
//
// Then call this method.
//
void snapbackup::restoreTables( const QString& context_name )
{
    snapTableList   dump_list;

    for( auto const & table_name : dump_list.tablesToDump() )
    {
        std::cout << "Restoring table [" << table_name << "]" << std::endl;

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
            const QString qstr( cql_insert_string.arg(table_name) );

            QCassandraQuery cass_query( f_session );
            cass_query.query( qstr, 3 );
            int bind_num = 0;
            cass_query.bindByteArray( bind_num++, key     );
            cass_query.bindByteArray( bind_num++, column1 );
            cass_query.bindByteArray( bind_num++, value   );

            cass_query.start();
            cass_query.end();
        }
    }
}


// vim: ts=4 sw=4 et
