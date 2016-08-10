/*
 * Text:
 *      read_write_data.cpp
 *
 * Description:
 *      Create a context with a table, then try to read and write data to
 *      the Cassandra cluster.
 *
 * Documentation:
 *      Run with no options, although supports the -h to define
 *      Cassandra's host.
 *      Fails if the test cannot create the context, create the table,
 *      read or write the data.
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

#include <QtCassandra/QCassandra.h>
#include <QtCore/QDebug>

int main(int argc, char *argv[])
{
    int exit_code = 0;
    try
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

        QtCassandra::QCassandraContext::pointer_t context(cassandra->context("qt_cassandra_test_rw"));
        try
        {
            context->drop();
        }
        catch(...)
        {
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

        try
        {
            context->create();
            qDebug() << "Context and its table were created!";
        }
        catch(const std::exception& e)
        {
            qDebug() << "Exception is [" << e.what() << "]";
            exit(1);
        }

        // in a normal situation, the rest should not generate exceptions

        // now that it's created, we can access it with the [] operator
        QtCassandra::QCassandraValue value1(-55);
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] = value1;
        QtCassandra::QCassandraValue value2(1000000);
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("million")] = value2;

        // binary
        QByteArray bin;
        bin.append((char)1);
        bin.append((char)255);
        bin.append('G');
        bin.append('I');
        bin.append('F');
        bin.append('.');
        bin.append((char)32);
        bin.append((char)7);
        bin.append((char)0xC0); // 192
        //bin.clear();  // here you can test that empty values are acceptable by Cassandra
        QtCassandra::QCassandraValue value3(bin);
        QByteArray row_key;
        row_key.append((char)0);
        row_key.append((char)255);
        row_key.append((char)9);
        row_key.append((char)25);
        row_key.append((char)0);
        QByteArray column_key;
        column_key.append((char)0);
        column_key.append((char)1);
        column_key.append((char)15);
        column_key.append((char)0);
        column_key.append((char)255);
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][row_key][column_key] = value3;

        // read this one from the memory cache
        QtCassandra::QCassandraValue v1 = (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")];
        if(v1.int32Value() != -55)
        {
            qDebug() << "Reading the size value failed. Got" << v1.int32Value() << "instead of -55";
            exit_code = 1;
        }

        // clear the cache
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();
        if((*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].exists(QString("size")))
        {
            qDebug() << "Yeah! exists(\"size\") worked! (from Cassandra)";
        }
        else
        {
            qDebug() << "Could not find \"size\" which should be defined";
            exit_code = 1;
        }

        // clear the cache
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();
        if((*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"].exists(QString("http://www.snapwebsites.org/page/3")))
        {
            qDebug() << "Yeah! exists(\"http://www.snapwebsites.org/page/3\") worked! (from Cassandra)";
        }
        else
        {
            qDebug() << "Could not find \"http://www.snapwebsites.org/page/3\" which should be defined";
            exit_code = 1;
        }

        if((*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"].exists(QString("unknown row")))
        {
            qDebug() << "Hmmm... exists(\"unknown row\") worked... (from Cassandra)";
            exit_code = 1;
        }
        else
        {
            qDebug() << "Could not find \"unknown row\" which was expected!";
        }

        // clear the cache
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();

        // re-read this one from Cassandra
        QtCassandra::QCassandraValue v1b = (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")];
        if(v1b.int32Value() != -55)
        {
            qDebug() << "Reading the size value failed. Got" << v1b.int32Value() << "instead of -55";
            exit_code = 1;
        }

        QtCassandra::QCassandraValue v2 = (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("million")];
        if(v2.int32Value() != 1000000)
        {
            qDebug() << "Reading the size value failed. Got" << v2.int32Value() << "instead of -55";
            exit_code = 1;
        }

        if((*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].exists(QString("million")))
        {
            qDebug() << "Yeah! exists(\"million\") worked! (from memory)";
        }
        else
        {
            qDebug() << "Could not find \"million\" which should be defined";
            exit_code = 1;
        }

        // undefined cell...
        if((*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].exists(QString("this one")))
        {
            qDebug() << "Somehow \"this one\" exists!";
        }
        else
        {
            qDebug() << "Could not find \"this one\" as expected";
            exit_code = 1;
        }

        // clear the cache, test that we can find all the cells
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();
        auto column_predicate( std::make_shared<QtCassandra::QCassandraCellPredicate>() );
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].readCells(column_predicate);
        const QtCassandra::QCassandraCells& cells((*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].cells());
        qDebug() << "cells in 1st row" << cells.size();
        for(QtCassandra::QCassandraCells::const_iterator it = cells.begin(); it != cells.end(); ++it)
        {
            qDebug() << "  name" << (*it)->columnName();
        }

        qDebug() << "cellCount()" << (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].cellCount();

        // remove one of the cells
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].dropCell(QString("million"));

        // clear the cache, test that we can find all the cells
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();
        (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].readCells(column_predicate);
        const QtCassandra::QCassandraCells& cells2((*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].cells());
        qDebug() << "AFTER REMOVE: cells in 1st row" << cells.size();
        for(QtCassandra::QCassandraCells::const_iterator it = cells2.begin(); it != cells2.end(); ++it)
        {
            qDebug() << "  name" << (*it)->columnName();
        }

        qDebug() << "cellCount()" << (*cassandra)["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].cellCount();

        (*cassandra)
                ["qt_cassandra_test_rw"]
                ["qt_cassandra_test_table"]
                .dropRow( QString("http://www.snapwebsites.org/page/3") );

        context->drop();
    }
    catch(std::overflow_error const & e)
    {
        qDebug() << "std::overflow_error caught -- " << e.what();
        exit_code = 1;
    }

    exit(exit_code);
}

// vim: ts=4 sw=4 et
