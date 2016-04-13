/*
 * Text:
 *      query.cpp
 *
 * Description:
 *      Test the QCassandraQuery class
 *
 * Documentation:
 *      Run with no options.
 *      Fails if the test cannot connect to the default Cassandra cluster.
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

#include "QtCassandra/QCassandraQuery.h"
#include <QtCore>

#include <exception>
#include <iostream>

using namespace QtCassandra;

class QueryTest
{
public:
    QueryTest( const QString& host );
    ~QueryTest();

    void describeTest();

    void createSchema();
    void dropSchema();

    void simpleInsert();
    void simpleSelect();

    void largeTableTest();

private:
    QCassandraSession::pointer_t f_session;
};


QueryTest::QueryTest( const QString& host )
{
    f_session = QCassandraSession::create();
    f_session->connect( host );
    //
    if( !f_session->isConnected() )
    {
        throw std::runtime_error( "Not connected!" );
    }
}


QueryTest::~QueryTest()
{
    f_session->disconnect();
}


void QueryTest::describeTest()
{
    // ONLY for debugging purposes--not called normall as this causes a Syntax Error.
    QCassandraQuery q( f_session );
    q.query( "DESCRIBE KEYSPACES" );
    q.start();
    while( q.nextRow() )
    {
        const std::string text = q.getStringColumn( 0 ).toStdString();
        std::cout << text << std::endl;
    }
}


void QueryTest::createSchema()
{
    std::cout << "Creating keyspace..." << std::endl;
    QCassandraQuery q( f_session );
    q.query( "CREATE KEYSPACE IF NOT EXISTS qtcassandra_query_test "
        "WITH replication = {'class': 'SimpleStrategy', 'replication_factor': '1'} "
        "AND durable_writes = true"
        );
    q.start();
    q.end();

    std::cout << "Creating table 'data'..." << std::endl;
    q.query( "CREATE TABLE IF NOT EXISTS qtcassandra_query_test.data \n"
                "( id INT\n"                      //  0
                ", name TEXT\n"                   //  1
                ", test BOOLEAN\n"                //  2
                ", float_value FLOAT\n"           //  3
                ", double_value DOUBLE\n"         //  4
                ", blob_value BLOB\n"             //  5
                ", json_value TEXT\n"             //  6
                ", map_value map<TEXT, TEXT>\n"   //  7
                ", PRIMARY KEY (id, name)\n"
                ");"
        );
    q.start();
    q.end();

    std::cout << "Creating table 'large_table'..." << std::endl;
    q.query( "CREATE TABLE IF NOT EXISTS qtcassandra_query_test.large_table \n"
                "( id INT\n"                      //  0
                ", name TEXT\n"                   //  2
                ", blob_value BLOB\n"             //  3
                ", PRIMARY KEY (id, name)\n"
                ") WITH CLUSTERING ORDER BY (name ASC);"
        );
    q.start();
    q.end();
    std::cout << "Dropping keyspace..." << std::endl;
}


void QueryTest::dropSchema()
{
    std::cout << "Dropping keyspace..." << std::endl;

    QCassandraQuery q( f_session );
    q.query( "DROP KEYSPACE IF EXISTS qtcassandra_query_test" );
    q.start();
}


void QueryTest::simpleInsert()
{
    std::cout << "Insert into table 'data'..." << std::endl;
    QCassandraQuery q( f_session );
    q.query( "INSERT INTO qtcassandra_query_test.data "
                "(id, name, test, float_value, double_value, blob_value, json_value, map_value) "
                "VALUES "
                "(?,?,?,?,?,?,?,?)"
             , 8
        );
    int bind_num = 0;
    q.bindInt32  ( bind_num++, 5 );
    q.bindString ( bind_num++, "This is a test" );
    q.bindBool   ( bind_num++, true );
    q.bindFloat  ( bind_num++, 4.5 );
    q.bindDouble ( bind_num++, 45234.5L );

    QByteArray arr;
    arr += "This is a test";
    arr += " and yet more chars...";
    q.bindByteArray( bind_num++, arr );

    QCassandraQuery::string_map_t json_map;
    json_map["foo"]   = "bar";
    json_map["meyer"] = "bidge";
    json_map["silly"] = "walks";
    q.bindJsonMap( bind_num++, json_map );

    QCassandraQuery::string_map_t cass_map;
    cass_map["test"] = "more tests";
    cass_map["map"]  = "this";
    cass_map["fun"]  = "work";
    q.bindMap( bind_num++, json_map );
    q.start();
}


void QueryTest::simpleSelect()
{
    std::cout << "Select from table 'data'..." << std::endl;
    QCassandraQuery q( f_session );
    q.query( "SELECT * FROM qtcassandra_query_test.data" );
    q.start();
    while( q.nextRow() )
    {
        const int32_t                       id           = q.getInt32Column     ( "id"           );
        const std::string                   name         = q.getStringColumn    ( "name"         ).toStdString();
        const bool                          test         = q.getBoolColumn      ( "test"         );
        const int64_t                       count        = q.getInt64Column     ( "count"        );
        const float                         float_value  = q.getFloatColumn     ( "float_value"  );
        const double                        double_value = q.getDoubleColumn    ( "double_value" );
        const QByteArray                    blob_value   = q.getByteArrayColumn ( "blob_value"   );
        const QCassandraQuery::string_map_t json_value   = q.getJsonMapColumn   ( "json_value"   );
        const QCassandraQuery::string_map_t map_value    = q.getMapColumn       ( "map_value"    );

        std::cout   << "id ="          << id                << std::endl
                    << "name="         << name              << std::endl
                    << "test="         << test              << std::endl
                    << "count="        << count             << std::endl
                    << "float_value="  << float_value       << std::endl
                    << "double_value=" << double_value      << std::endl
                    << "blob_value="   << blob_value.data() << std::endl;

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


void QueryTest::largeTableTest()
{
    const int32_t row_count = 10000;

    std::cout << "Insert into table 'large_table'..." << std::endl;
    QCassandraQuery q( f_session );

    for( int32_t i = 0; i < row_count; ++i )
    {
        q.query( "INSERT INTO qtcassandra_query_test.large_table "
                "(id, name, blob_value) "
                "VALUES "
                "(?,?,?)"
                , 3
               );
        int bind_num = 0;
        q.bindInt32  ( bind_num++, i );
        q.bindString ( bind_num++, QString("This is test %1.").arg(i) );

        QString blob;
        blob.fill( 'b', 10000 );
        q.bindByteArray( bind_num++, blob.toUtf8() );

        q.start();
        q.end();
    }

    std::map<int32_t,QString> string_map;

    std::cout << "Select from 'large_table' and test paging functionality..." << std::endl;
    q.query( "SELECT id,name FROM qtcassandra_query_test.large_table" );
    q.setPagingSize( 10 );
    q.start();
    do
    {
        std::cout << "Iterate through page..." << std::endl;
        while( q.nextRow() )
        {
            const int32_t id(q.getInt32Column("id"));
            const QString name(q.getStringColumn("name"));
            std::cout << "id=" << id << ", name=" << name.toStdString() << std::endl;
            string_map[id] = name;
        }
    }
    while( q.nextPage() );

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

    std::cout << "Process done!" << std::endl;
}


int main( int argc, char *argv[] )
{
    const char *host( "localhost" );
    for ( int i( 1 ); i < argc; ++i )
    {
        if ( strcmp( argv[i], "--help" ) == 0 )
        {
            qDebug() << "Usage:" << argv[0] << "[-h <hostname>]";
            exit( 1 );
        }
        if ( strcmp( argv[i], "-h" ) == 0 )
        {
            ++i;
            if ( i >= argc )
            {
                qDebug() << "error: -h must be followed by a hostname.";
                exit( 1 );
            }
            host = argv[i];
        }
    }

    try
    {
        QueryTest test( host );
        //test.describeTest();
        test.dropSchema();
        test.createSchema();
        test.simpleInsert();
        test.simpleSelect();
        test.largeTableTest();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Exception caught! what=[" << ex.what() << "]" << std::endl;
        return 1;
    }

    return 0;
}

// vim: ts=4 sw=4 et
