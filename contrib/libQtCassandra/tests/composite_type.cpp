/*
 * Text:
 *      tests/composite_type.cpp
 *
 * Description:
 *      Create a context with a table having columns with a composite type,
 *      then try to read and write data to the Cassandra cluster.
 *
 * Documentation:
 *      Run with no options, although supports the -h to define
 *      Cassandra's host.
 *      Fails if the test cannot create the context, create the table,
 *      read or write the data in the composite columns.
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

#include <QtCassandra/QCassandra.h>
#include <QtCore/QDebug>
#include <thrift-gencpp-cassandra/cassandra_types.h>

int main(int argc, char *argv[])
{
    QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );

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

    cassandra->connect(host);
    qDebug() << "Working on Cassandra Cluster Named" << cassandra->clusterName();
    qDebug() << "Working on Cassandra Protocol Version" << cassandra->protocolVersion();

    QtCassandra::QCassandraContext::pointer_t context(cassandra->context("qt_cassandra_test_ct"));
    try {
        context->drop();
        cassandra->synchronizeSchemaVersions();
    }
    catch(...) {
        // ignore errors, this happens when the context doesn't exist yet
    }

    QtCassandra::QCassandraSchema::Value replication;
    auto& replication_map(replication.map());
    replication_map["class"]              = QVariant("SimpleStrategy");
    replication_map["replication_factor"] = QVariant(1);

    auto& fields(context->fields());
    fields["replication"]    = replication;
    fields["durable_writes"] = QVariant(true);

    QtCassandra::QCassandraTable::pointer_t table(context->table("qt_cassandra_test_table"));

    QtCassandra::QCassandraSchema::Value compaction;
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
        cassandra->synchronizeSchemaVersions();
        qDebug() << "Context and its table were created!";
    }
    catch(org::apache::cassandra::InvalidRequestException& e) {
        qDebug() << "Exception is [" << e.why.c_str() << "]";
        exit(1);
    }

    //try {  // by default the rest should not generate an exception
    // now that it's created, we can access it with the [] operator
    QtCassandra::QCassandraValue value1(-1005);
    QtCassandra::QCassandraValue a1(QString("size"));
    QtCassandra::QCassandraValue a2(static_cast<int32_t>(123));
    QtCassandra::QCassandraRow::composite_column_names_t names1;
    names1.push_back(a1);
    names1.push_back(a2);
    QtCassandra::QCassandra& cass( *cassandra );
    cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].compositeCell(names1) = value1;
    cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"].clearCache();
    QtCassandra::QCassandraValue v1 = cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].compositeCell(names1);
    qDebug() << "Read -1005 value back as:" << v1.int32Value();

    QtCassandra::QCassandraValue value2(5678);
    QtCassandra::QCassandraValue a3(QString("foot"));
    QtCassandra::QCassandraValue a4(static_cast<int32_t>(123));
    QtCassandra::QCassandraRow::composite_column_names_t names2;
    names2.push_back(a3);
    names2.push_back(a4);
    cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].compositeCell(names2) = value2;
    cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"].clearCache();
    QtCassandra::QCassandraValue v2 = cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].compositeCell(names2);
    qDebug() << "Read 5678 value back as:" << v2.int32Value();

    QtCassandra::QCassandraValue value3(8080);
    QtCassandra::QCassandraValue a5(QString("size"));
    QtCassandra::QCassandraValue a6(static_cast<int32_t>(555));
    QtCassandra::QCassandraRow::composite_column_names_t names3;
    names3.push_back(a5);
    names3.push_back(a6);
    cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].compositeCell(names3) = value3;
    cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"].clearCache();
    QtCassandra::QCassandraValue v3 = cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].compositeCell(names3);
    qDebug() << "Read 8080 value back as:" << v3.int32Value();

    cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"].clearCache();
    QtCassandra::QCassandraValue v1_2 = cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].compositeCell(names1);
    qDebug() << "Read -1005 value again as:" << v1_2.int32Value();

    cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"].clearCache();
    QtCassandra::QCassandraValue v2_2 = cass["qt_cassandra_test_ct"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].compositeCell(names2);
    qDebug() << "Read 5678 value again as:" << v2_2.int32Value();

    context->drop();
    cassandra->synchronizeSchemaVersions();

    exit(0);
}

// vim: ts=4 sw=4 et
