/*
 * Text:
 *      tests/query.cpp
 *
 * Description:
 *      Test the query class
 *
 * Documentation:
 *      Run with no options.
 *      Fails if the test cannot connect to the default Cassandra cluster.
 *
 * License:
 *      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 *
 *      https://snapwebsites.org/
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

#include "query_test.h"

#include "casswrapper/query.h"
#include "casswrapper/batch.h"
#include "casswrapper/schema.h"
#include "casswrapper/qstring_stream.h"

#include <QtSql/QtSql>

#include <exception>
#include <iostream>

using namespace casswrapper;
using namespace schema;


namespace
{

QString g_host = QString("127.0.0.1");

}
// no name namespace



query_test::query_test()
{
    f_session = Session::create();
    f_session->connect( g_host );
    //
    if( !f_session->isConnected() )
    {
        throw std::runtime_error( "Not connected!" );
    }
}


query_test::~query_test()
{
    f_session->disconnect();
}


void query_test::describeSchema()
{
    SessionMeta::pointer_t sm( SessionMeta::create(f_session) );
    sm->loadSchema();

    std::cout << "Keyspace fields:" << std::endl;
    for( auto kys : sm->getKeyspaces() )
    {
        std::cout << "Keyspace " << kys.first << std::endl;

        for( auto field : kys.second->getFields() )
        {
            std::cout << field.first << ": "
                      << field.second.output()
                      << std::endl;
        }

        std::cout << std::endl << "Tables: " << std::endl;
        for( auto table : kys.second->getTables() )
        {
            std::cout << table.first << ": "
                      << std::endl;

            std::cout << "\tFields:" << std::endl;
            for( auto field : table.second->getFields() )
            {
                std::cout << "\t\t" << field.first << ": " << field.second.output() << std::endl;
            }

            std::cout << std::endl;
            std::cout << "\tColumns:" << std::endl;
            for( auto column : table.second->getColumns() )
            {
                std::cout << "\t\t" << column.first << ": " << std::endl;
                for( auto field : column.second->getFields() )
                {
                    std::cout << "\t\t\t" << field.first << ": " << std::endl;
                    std::cout << "\t\t\t\t" << field.second.output() << std::endl;
                }
            }
        }

        std::cout << "CQL Keyspace schema output:" << std::endl;
        std::cout << kys.second->getKeyspaceCql();
        std::cout << std::endl;
        std::cout << "CQL Tables schema output:" << std::endl;
        for( auto entry : kys.second->getTablesCql() )
        {
            std::cout << "Table [" << entry.first << "]:" << std::endl;
            std::cout << entry.second << std::endl;
        }
    }
}


void query_test::createSchema()
{
    std::cout << "Creating keyspace and tables..." << std::endl;
    auto q = Query::create( f_session );
    q->query( "CREATE KEYSPACE IF NOT EXISTS qtcassandra_query_test "
        "WITH replication = {'class': 'SimpleStrategy', 'replication_factor': '1'} "
        "AND durable_writes = true"
        );
    q->start();
    q->end();

    std::cout << "Creating table 'data'..." << std::endl;
    q->query( "CREATE TABLE IF NOT EXISTS qtcassandra_query_test.data \n"
                "( id INT\n"                      //  0
                ", name TEXT\n"                   //  1
                ", test BOOLEAN\n"                //  2
                //", float_value FLOAT\n"           //  3
                ", double_value DOUBLE\n"         //  4
                ", blob_value BLOB\n"             //  5
                ", json_value TEXT\n"             //  6
                ", map_value map<TEXT, TEXT>\n"   //  7
                ", PRIMARY KEY (id, name)\n"
                ");"
        );
    q->start();
    q->end();

    std::cout << "Creating table 'large_table'..." << std::endl;
    q->query( "CREATE TABLE IF NOT EXISTS qtcassandra_query_test.large_table \n"
                "( id INT\n"                      //  0
                ", name TEXT\n"                   //  2
                ", blob_value BLOB\n"             //  3
                ", PRIMARY KEY (id, name)\n"
                ") WITH CLUSTERING ORDER BY (name ASC);"
        );
    q->start();
    q->end();
    std::cout << "Keyspace and tables created..." << std::endl;
}


void query_test::dropSchema()
{
    std::cout << "Dropping keyspace... (this may timeout if auto_snapshot is true in conf/cassandra.yaml)" << std::endl;

    auto q = Query::create( f_session );
    q->query( "DROP KEYSPACE IF EXISTS qtcassandra_query_test" );
    q->start();
}


void query_test::simpleInsert()
{
    std::cout << "Insert into table 'data'..." << std::endl;
    auto q = Query::create( f_session );
    q->query( "INSERT INTO qtcassandra_query_test.data "
                "(id, name, test, double_value, blob_value, json_value, map_value) "
                "VALUES "
                "(?,?,?,?,?,?,?)"
        );
    int bind_num = 0;
    q->bindVariant ( bind_num++, 5 );
    q->bindVariant ( bind_num++, "This is a test" );
    q->bindVariant ( bind_num++, true );
    //q->bindVariant ( bind_num++, static_cast<float>(4.5) );
    q->bindVariant ( bind_num++, static_cast<double>(45234.5) );

    QByteArray arr;
    arr += "This is a test";
    arr += " and yet more chars...";
    q->bindByteArray( bind_num++, arr );

    Query::string_map_t json_map;
    json_map["foo"]   = "bar";
    json_map["meyer"] = "bidge";
    json_map["silly"] = "walks";
    q->bindJsonMap( bind_num++, json_map );

    Query::string_map_t cass_map;
    cass_map["test"] = "more tests";
    cass_map["map"]  = "this";
    cass_map["fun"]  = "work";
    q->bindMap( bind_num++, json_map );
    q->start();
}


void query_test::simpleSelect()
{
    std::cout << "Select from table 'data'..." << std::endl;
    auto q = Query::create( f_session );
    q->query( "SELECT id,name,test,double_value,blob_value,json_value,map_value\n"
             ",COUNT(*) AS count\n"
             ",WRITETIME(blob_value) AS timestamp\n"
             "FROM qtcassandra_query_test.data" );
    q->start();
    while( q->nextRow() )
    {
        const int32_t                       id           = q->getVariantColumn   ( "id"           ).toInt();
        const std::string                   name         = q->getVariantColumn   ( "name"         ).toString().toStdString();
        const bool                          test         = q->getVariantColumn   ( "test"         ).toBool();
        const int64_t                       count        = q->getVariantColumn   ( "count"        ).toLongLong();
        //const float                         float_value  = q->getVariantColumn   ( "float_value"  ).toDouble();
        const double                        double_value = q->getVariantColumn   ( "double_value" ).toDouble();
        const QByteArray                    blob_value   = q->getByteArrayColumn ( "blob_value"   );
        const Query::string_map_t           json_value   = q->getJsonMapColumn   ( "json_value"   );
        const Query::string_map_t           map_value    = q->getMapColumn       ( "map_value"    );
        const int64_t	                    timestamp    = q->getVariantColumn   ( "timestamp"    ).toLongLong() ;

        std::cout   << "id ="          << id                << std::endl
                    << "name="         << name              << std::endl
                    << "test="         << test              << std::endl
                    << "count="        << count             << std::endl
                    //<< "float_value="  << float_value       << std::endl
                    << "double_value=" << double_value      << std::endl
                    << "blob_value="   << blob_value.data() << std::endl
                    << "timestamp="    << timestamp         << std::endl
                    ;

        std::cout << "json_value:" << std::endl;
        for( auto pair : json_value )
        {
            std::cout << "\tkey=" << pair.first << ", value=" << pair.second << std::endl;
        }

        std::cout << std::endl << "map_value:" << std::endl;
        for( auto pair : map_value )
        {
            std::cout << "\tkey=" << pair.first << ", value=" << pair.second << std::endl;
        }
    }
}


void query_test::batchTest()
{
    const int32_t row_count = 1000;

    std::cout << "Batch insert into table 'large_table'..." << std::endl;

    auto batch( LoggedBatch::create() );

    for( int32_t i = 0; i < row_count; ++i )
    {
        auto q = Query::create( f_session );
        batch->addQuery( q );
        q->query( "INSERT INTO qtcassandra_query_test.large_table "
                "(id, name, blob_value) "
                "VALUES "
                "(?,?,?)"
               );
        int bind_num = 0;
        q->bindVariant ( bind_num++, i );
        q->bindVariant ( bind_num++, QString("This is test %1.").arg(i) );

        QString blob;
        blob.fill( QChar('b'), 10 );
        q->bindByteArray( bind_num++, blob.toUtf8() );
    }

    batch->run();

    std::map<int32_t,QString> string_map;

    {
        std::cout << "POST BATCH: Select from 'large_table' and test paging functionality..." << std::endl;
        auto q = Query::create( f_session );
        q->query( "SELECT id, name, WRITETIME(blob_value) AS timestamp FROM qtcassandra_query_test.large_table" );
        q->setPagingSize( 10 );
        q->start();
        do
        {
            //        std::cout << "Iterate through batch page..." << std::endl;
            while( q->nextRow() )
            {
                const int32_t id(q->getVariantColumn("id").toInt());
                const QString name(q->getVariantColumn("name").toString());
                string_map[id] = name;
#if 0
                std::cout
                        << "id=" << id
                        << ", name='" << name.toStdString() << "'"
                        << ", timestamp=" << q->getInt64Column("timestamp")
                        << std::endl;
#endif
            }
        }
        while( q->nextPage() );
    }

    std::cout << "Check order of recovered records:" << std::endl;
    if( string_map.size() != static_cast<size_t>(row_count) )
    {
        throw std::runtime_error( "Row count is not correct!" );
    }

    for( int32_t idx = 0; idx < row_count; ++idx )
    {
        if( string_map.find(idx) == string_map.end() )
        {
            throw std::runtime_error( QString("Index %1 not found in map!").arg(idx).toStdString().c_str() );
        }
    }

    std::cout << "Batch process done!" << std::endl;
}


void query_test::largeTableTest()
{
    const int32_t row_count = 10000;

    std::cout << "Insert into table 'large_table' [NO BATCH]..." << std::endl;
    auto q = Query::create( f_session );

    // Empty the table out first
    //
    q->query( "TRUNCATE qtcassandra_query_test.large_table" );
    q->start();
    q->end();

    for( int32_t i = 0; i < row_count; ++i )
    {
        q->query( "INSERT INTO qtcassandra_query_test.large_table "
                "(id, name, blob_value) "
                "VALUES "
                "(?,?,?)"
               );
        int bind_num = 0;
        q->bindVariant ( bind_num++, i );
        q->bindVariant ( bind_num++, QString("This is test %1.").arg(i) );

        QString blob;
        blob.fill( QChar('b'), 10000 );
        q->bindByteArray( bind_num++, blob.toUtf8() );

        q->start();
        q->end();
    }

    std::map<int32_t,QString> string_map;

    std::cout << "Select from 'large_table' and test paging functionality..." << std::endl;
    q->query( "SELECT id, name, WRITETIME(blob_value) AS timestamp FROM qtcassandra_query_test.large_table" );
    q->setPagingSize( 10 );
    q->start();
    do
    {
//        std::cout << "Iterate through page..." << std::endl;
        while( q->nextRow() )
        {
            const int32_t id(q->getVariantColumn("id").toInt());
            const QString name(q->getVariantColumn("name").toString());
//            std::cout
//                    << "id=" << id
//                    << ", name='" << name.toStdString() << "'"
//                    << ", timestamp=" << q->getInt64Column("timestamp")
//                    << std::endl;
            string_map[id] = name;
        }
    }
    while( q->nextPage() );

    std::cout << "Check order of recovered records:" << std::endl;
    if( string_map.size() != static_cast<size_t>(row_count) )
    {
        throw std::runtime_error( "Row count is not correct!" );
    }

    for( int32_t idx = 0; idx < row_count; ++idx )
    {
        if( string_map.find(idx) == string_map.end() )
        {
            throw std::runtime_error( QString("Index %1 not found in map!").arg(idx).toStdString().c_str() );
        }
    }

    std::cout << "Non-batch process done!" << std::endl;
}


void query_test::qtSqlDriverTest()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QCassandra");
    if( !db.isValid() )
    {
        const QString error( "QCASSANDRA database is not valid for some reason!" );
        std::cerr << "QCASSANDRA not valid!!!" << std::endl;
        throw std::runtime_error( error.toUtf8().data() );
    }

    QString const database_name("qtcassandra_query_test");
    db.setHostName( g_host );
    db.setDatabaseName(database_name);
    if( !db.open() )
    {
        const QString error( QString("Cannot open QCASSANDRA database [%1]!").arg(database_name) );
        std::cerr << "QCASSANDRA not open!!!" << std::endl;
        throw std::runtime_error( error.toUtf8().data() );
    }

    std::cout << "QCassandra: Insert into table 'data'..." << std::endl;
    for( int idx = 0; idx < 10000; ++idx )
    {
        QSqlQuery q;
        q.prepare( "INSERT INTO data "
                       "(id, name, test, double_value, blob_value) "
                       "VALUES "
                       "(?,?,?,?,?)"
                   );
        int bind_num = 0;
        q.bindValue ( bind_num++, 5+idx );
        q.bindValue ( bind_num++, QString("This is test %1").arg(idx) );
        q.bindValue ( bind_num++, true );
        q.bindValue ( bind_num++, static_cast<double>(45234.5) * idx );

        QByteArray arr;
        arr += "This is a test";
        arr += " and yet more chars...";
        arr += QString("And a number=%1").arg(idx);
        q.bindValue( bind_num++, arr );

        if( !q.exec() )
        {
            std::cerr << "lastQuery=[" << q.lastQuery() << "]" << std::endl;
            std::cerr << "query error=[" << q.lastError().text() << "]" << std::endl;
            throw std::runtime_error( q.lastError().text().toUtf8().data() );
        }
    }

#if 0
    // TODO: Develop up a test which handles async database responses.
    {
        std::cout << "QCassandra: Select from table 'data'..." << std::endl;
        QSqlQuery q
            ( QString("SELECT id,name,test,double_value,blob_value\n"
                  ",COUNT(*) AS count\n"
                  "FROM %1.data").arg(database_name)
              );
        auto result_func = [&](const QString &notification_name)
            {
                if( notification_name != "QCassandraDriver::queryFinished()" )
                {
                    return;
                }

                bool result = q.first();
                std::cout << "result=" << result << std::endl;
                do
                {
                    const int32_t     id           = q.value( "id"           ).toInt();
                    const std::string name         = q.value( "name"         ).toString().toStdString();
                    const bool        test         = q.value( "test"         ).toBool();
                    const int64_t     count        = q.value( "count"        ).toLongLong();
                    const double      double_value = q.value( "double_value" ).toDouble();
                    const QByteArray  blob_value   = q.value( "blob_value"   ).toByteArray();

                    std::cout   << "id ="          << id                << std::endl
                                << "name="         << name              << std::endl
                                << "test="         << test              << std::endl
                                << "count="        << count             << std::endl
                                << "double_value=" << double_value      << std::endl
                                << "blob_value="   << blob_value.data() << std::endl
                                   ;
                }
                while( q.next() );
            };
        QObject::connect( q.driver()
                        , static_cast<void(QSqlDriver::*)(const QString&)>(&QSqlDriver::notification)
                        , result_func
                        );
        q.driver()->subscribeToNotification( "QCassandraDriver::queryFinished()" );
        exec(q);

        // This doesn't work because the application ends before we receive any hits
        // since it doesn't block. Some kind of GUI test might be appropriate here.
    }
#endif

    {
        std::cout << "QCassandra: Count rows in table 'data'..." << std::endl;
        QSqlQuery q( "SELECT COUNT(*) AS count FROM data" );

        if( !q.first() )
        {
            throw std::runtime_error( "should be one row!" );
        }

        const int64_t  count = q.value( "count" ).toLongLong();
        std::cout << "count=" << count << std::endl;

        if( q.next() )
        {
            throw std::runtime_error( "should be at only one row!" );
        }
    }

    {
        std::cout << "QCassandra: Select from table 'data'..." << std::endl;
        QSqlQuery q( "SELECT id,name,test,double_value,blob_value\n"
                          "FROM data");
        if( q.size() <= 0 )
        {
            throw std::runtime_error( "There is a problem with the query!" );
        }

        do
        {
            for( q.first(); q.next(); )
            {
                const int32_t     id           = q.value( "id"           ).toInt();
                const std::string name         = q.value( "name"         ).toString().toStdString();
                const bool        test         = q.value( "test"         ).toBool();
                const double      double_value = q.value( "double_value" ).toDouble();
                const QByteArray  blob_value   = q.value( "blob_value"   ).toByteArray();

                std::cout   << "id ="          << id                << std::endl
                            << "name="         << name              << std::endl
                            << "test="         << test              << std::endl
                            << "double_value=" << double_value      << std::endl
                            << "blob_value="   << blob_value.data() << std::endl
                               ;
            }
        }
        while( q.exec() );
    }

    {
        std::cout << "QCassandra: Select from table 'data' with '*'..." << std::endl;
        QSqlQuery q( "SELECT * FROM data" );
        if( q.size() <= 0 )
        {
            throw std::runtime_error( "There is a problem with the query!" );
        }

        do
        {
            for( q.first(); q.next(); )
            {
                const int32_t     id           = q.value( "id"           ).toInt();
                const std::string name         = q.value( "name"         ).toString().toStdString();
                const bool        test         = q.value( "test"         ).toBool();
                const int64_t     count        = q.value( "count"        ).toLongLong();
                const double      double_value = q.value( "double_value" ).toDouble();
                const QByteArray  blob_value   = q.value( "blob_value"   ).toByteArray();

                std::cout   << "id ="          << id                << std::endl
                            << "name="         << name              << std::endl
                            << "test="         << test              << std::endl
                            << "count="        << count             << std::endl
                            << "double_value=" << double_value      << std::endl
                            << "blob_value="   << blob_value.data() << std::endl
                               ;
            }
        }
        while( q.exec() );
    }
}


void query_test::set_host( QString const& host )
{
    g_host = host;
}

QString query_test::get_host()
{
    return g_host;
}

// vim: ts=4 sw=4 et
