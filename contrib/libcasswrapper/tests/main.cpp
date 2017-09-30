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
 *      Copyright (c) 2011-2017 Made to Order Software Corp.
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

#include "query_test.h"

#include <QtCore>

#include <iostream>

int main( int argc, char *argv[] )
{
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
            query_test::set_host( argv[i] );
        }
    }

    //try
    {
        QCoreApplication    app( argc, argv );
        app.setApplicationName( "query_tests" );

        query_test test;
        test.describeSchema();
        test.dropSchema();
        test.createSchema();

        test.qtSqlDriverTest();
#if 1
        test.simpleInsert();
        test.simpleSelect();
        test.batchTest();
        test.largeTableTest();
#endif
    }
#if 0
    catch( const std::exception& ex )
    {
        std::cerr << "Exception caught! what=[" << ex.what() << "]" << std::endl;
        return 1;
    }
#endif

    return 0;
}

// vim: ts=4 sw=4 et
