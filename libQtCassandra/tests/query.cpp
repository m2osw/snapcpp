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

#include <iostream>

using namespace QtCassandra;

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

    QCassandraSession::pointer_t session( QCassandraSession::create() );
    session->connect( host );
    //
    if( !session->isConnected() )
    {
        throw std::runtime_error( "Not connected!" );
    }

    QCassandraQuery q( session );
    q.query( "SELECT * FROM system.schema_keyspaces" );
    q.start();
    while( q.nextRow() )
    {
        const std::string  keyspace_name    = q.getStringColumn  ( "keyspace_name"    ).toStdString();
        const bool         durable_writes   = q.getBoolColumn    ( "durable_writes"   );
        const std::string  strategy_class   = q.getStringColumn  ( "strategy_class"   ).toStdString();
        const QCassandraQuery::string_map_t strategy_options = q.getJsonMapColumn ( "strategy_options" );

        std::cout << "keyspace_name=" << keyspace_name
            << ", durable_writes=" << durable_writes
            << ", strategy_class=" << strategy_class
            << ", strategy_options:"
            << std::endl;

        for( auto pair : strategy_options )
        {
            std::cout << "\tkey=" << pair.first << ", value=" << pair.second << std::endl;
        }
    }

    session->disconnect();
}

// vim: ts=4 sw=4 et
