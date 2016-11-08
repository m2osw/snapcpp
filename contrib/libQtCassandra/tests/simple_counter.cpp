/*
 * Text:
 *      simple_counter.cpp
 *
 * Description:
 *      Create a context with a counter table, then try to count using the
 *      the Cassandra cluster.
 *
 * Documentation:
 *      Run with no options, although supports the -h to define
 *      Cassandra's host.
 *      Fails if the test cannot create the context, create the table,
 *      or count as expected.
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

#include <iostream>

#include <QtCassandra/QCassandra.h>
#include <QtCore/QDebug>

int main(int argc, char *argv[])
{
    int err(0);

    try
    {
        QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );

        char const * host("localhost");
        for(int i(1); i < argc; ++i)
        {
            if(strcmp(argv[i], "--help") == 0)
            {
                qDebug() << "Usage:" << argv[0] << "[-h <hostname>]";
                exit(1);
            }
            if(strcmp(argv[i], "-h") == 0)
            {
                ++i;
                if(i >= argc)
                {
                    qDebug() << "error: -h must be followed by a hostname.";
                    exit(1);
                }
                host = argv[i];
            }
        }

        cassandra->connect(host);
        qDebug() << "Working on Cassandra Cluster Named" << cassandra->clusterName();
        qDebug() << "Working on Cassandra Protocol Version" << cassandra->protocolVersion();

        QtCassandra::QCassandraContext::pointer_t context(cassandra->context("qt_cassandra_test_sc"));
        try
        {
            context->drop();
        }
        catch(...)
        {
            // ignore errors, this happens when the context doesn't exist yet
        }

        casswrapper::schema::Value replication;
        auto& replication_map(replication.map());
        replication_map["class"]              = QVariant("SimpleStrategy");
        replication_map["replication_factor"] = QVariant(1);

        auto& fields(context->fields());
        fields["replication"]    = replication;
        fields["durable_writes"] = QVariant(true);

        QtCassandra::QCassandraTable::pointer_t table(context->table("qt_cassandra_test_table"));

        casswrapper::schema::Value compaction;
        auto& compaction_map(compaction.map());
        compaction_map["class"]         = QVariant("SizeTieredCompactionStrategy");
        compaction_map["min_threshold"] = QVariant(4);
        compaction_map["max_threshold"] = QVariant(22);

        auto& table_fields(table->fields());
        table_fields["comment"]                     = QVariant("Our test table.");
        table_fields["memtable_flush_period_in_ms"] = QVariant(60);
        table_fields["gc_grace_seconds"]            = QVariant(3600);
        table_fields["compaction"]                  = compaction;

        try {
            context->create();
            //table->create();
            qDebug() << "Context and its table were created!";
        }
        catch( const std::exception& e ) {
            qDebug() << "Exception is [" << e.what() << "]";
            exit(1);
        }

        //try ...  // by default the rest should not generate an exception
        // now that it's created, we can access it with the [] operator
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] = 8;
        // In order to be able to read the value as a 64 bit value we clear the
        // cache that just saved the number 8 in an 'int' which is likely 32 bits
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
        qDebug() << "Size of counter should be 8, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
        qDebug() << "Read value should be 8, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
        if((*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != 8) {
            ++err;
        }

        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")]++;
        qDebug() << "Size of counter should be 8, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
        qDebug() << "Read value should be 9, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
        if((*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != 9) {
            ++err;
        }

        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] -= 10;
        qDebug() << "Size of counter should be 8, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
        qDebug() << "Read value should be -1, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
        if((*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != -1) {
            ++err;
        }

        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")]++;
        qDebug() << "Size of counter should be 8, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
        qDebug() << "Read value should be 0, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
        if((*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != 0) {
            ++err;
        }

        // test for overflow, Java like C/C++ does not generate errors on overflows
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] += 0x3FFFFFFFFFFFFFFF;
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] += 0x3FFFFFFFFFFFFFFF;
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] += 0x3FFFFFFFFFFFFFFF;
        qDebug() << "Size of counter should be 8, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
        qDebug() << "Read value should be -4611686018427387907, it is" << (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
        (*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
#pragma GCC push
#pragma GCC diagnostic ignored "-Wsign-compare"
        if((*cassandra)["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != 0xBFFFFFFFFFFFFFFD) {
            ++err;
        }
#pragma GCC pop

        context->drop();
    }
    catch(std::overflow_error const & e)
    {
        std::cerr << "std::overflow_error caught -- " << e.what() << std::endl;
        ++err;
    }

    if( err )
    {
        std::cerr << err << " tests failed!" << std::endl;
    }

    exit(err == 0 ? 0 : 1);
}

// vim: ts=4 sw=4 et
