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
 *      Copyright (c) 2012-2013 Made to Order Software Corp.
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

#include <QtCassandra/QCassandra.h>
#include <QtCore/QDebug>
#include <thrift-gencpp-cassandra/cassandra_types.h>

int main(int argc, char *argv[])
{
    QtCassandra::QCassandra     cassandra;
    int                         err(0);

    const char *host("localhost");
    for(int i(1); i < argc; ++i) {
        if(strcmp(argv[i], "--help") == 0) {
            qDebug() << "Usage:" << argv[0] << "[-h <hostname>]";
            exit(1);
        }
        if(strcmp(argv[i], "-h") == 0) {
            ++i;
            if(i >= argc) {
                qDebug() << "error: -h must be followed by a hostname.";
                exit(1);
            }
            host = argv[i];
        }
    }

    cassandra.connect(host);
    qDebug() << "Working on Cassandra Cluster Named" << cassandra.clusterName();
    qDebug() << "Working on Cassandra Protocol Version" << cassandra.protocolVersion();

    QtCassandra::QCassandraContext::pointer_t context(cassandra.context("qt_cassandra_test_sc"));
    try {
        context->drop();
        cassandra.synchronizeSchemaVersions();
    }
    catch(...) {
        // ignore errors, this happens when the context doesn't exist yet
    }

    context->setStrategyClass("SimpleStrategy"); // default is LocalStrategy
    //context->setDurableWrites(false); // by default this is 'true'
    context->setReplicationFactor(1); // by default this is undefined

    QtCassandra::QCassandraTable::pointer_t table(context->table("qt_cassandra_test_table"));
    //table->setComment("Our test table.");
    table->setColumnType("Standard"); // Standard or Super
    table->setKeyValidationClass("BytesType");
    table->setDefaultValidationClassForCounters();  // this is counter table
    table->setComparatorType("BytesType");
    table->setKeyCacheSavePeriodInSeconds(14400);
    table->setMemtableFlushAfterMins(60);
    //table->setMemtableThroughputInMb(247);
    //table->setMemtableOperationsInMillions(1.1578125);
    //table->setGcGraceSeconds(864000); // 10 days (default)
    table->setGcGraceSeconds(3600); // 1h.
    table->setMinCompactionThreshold(4);
    table->setMaxCompactionThreshold(22);
    table->setReplicateOnWrite(1);

    try {
        context->create();
        cassandra.synchronizeSchemaVersions();
        qDebug() << "Context and its table were created!";
    }
    catch(org::apache::cassandra::InvalidRequestException& e) {
        qDebug() << "Exception is [" << e.why.c_str() << "]";
        exit(1);
    }

    //try ...  // by default the rest should not generate an exception
    // now that it's created, we can access it with the [] operator
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] = 8;
    // In order to be able to read the value as a 64 bit value we clear the
    // cache that just saved the number 8 in an 'int' which is likely 32 bits
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
    qDebug() << "Size of counter should be 8, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
    qDebug() << "Read value should be 8, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
    if(cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != 8) {
        ++err;
    }

    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")]++;
    qDebug() << "Size of counter should be 8, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
    qDebug() << "Read value should be 9, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
    if(cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != 9) {
        ++err;
    }

    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] -= 10;
    qDebug() << "Size of counter should be 8, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
    qDebug() << "Read value should be -1, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
    if(cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != -1) {
        ++err;
    }

    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")]++;
    qDebug() << "Size of counter should be 8, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
    qDebug() << "Read value should be 0, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
    if(cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != 0) {
        ++err;
    }

    // test for overflow, Java like C/C++ does not generate errors on overflows
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] += 0x3FFFFFFFFFFFFFFF;
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] += 0x3FFFFFFFFFFFFFFF;
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] += 0x3FFFFFFFFFFFFFFF;
    qDebug() << "Size of counter should be 8, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().size();
    qDebug() << "Read value should be -4611686018427387907, it is" << cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value();
    cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].clearCache();
#pragma GCC push
#pragma GCC diagnostic ignored "-Wsign-compare"
    if(cassandra["qt_cassandra_test_sc"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")].value().int64Value() != 0xBFFFFFFFFFFFFFFD) {
        ++err;
    }
#pragma GCC pop

    context->drop();
    cassandra.synchronizeSchemaVersions();

    exit(err == 0 ? 0 : 1);
}

// vim: ts=4 sw=4 et
