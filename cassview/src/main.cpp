/*
 * Text:
 *      main.cpp
 *
 * Description:
 *
 * Documentation:
 *
 * License:
 *      Copyright (c) 2011-2012 Made to Order Software Corp.
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
#include <algorithm>

#include "qt4.h"

using namespace QtCassandra;

int main(int /*argc*/, char * /*argv*/[])
{
    QCassandra     cassandra;

    //cassandra.connect( "localhost", 9160, true );
    cassandra.connect();
    qDebug() << "Working on Cassandra Cluster Named" << cassandra.clusterName();
    qDebug() << "Working on Cassandra Protocol Version" << cassandra.protocolVersion();

    const QCassandraContexts& context_list = cassandra.contexts();
    QList<QString> keys = context_list.keys();

    std::for_each( keys.begin(), keys.end(),
            [&context_list]( const QString& key )
            {
                qDebug() << "key = " << key;
                //
                const QCassandraTables& tables = context_list[key]->tables();
                //QList<QString> table_names = tables.keys();
                std::for_each( tables.begin(), tables.end(),
                    [&tables]( const QSharedPointer<QCassandraTable>& table )
                    {
                        qDebug() << "\ttable_name = " << table->tableName();

                        QCassandraRowPredicate rowp;
                        rowp.setStartRowName("");
                        rowp.setEndRowName("");
                        rowp.setCount(10); // 100 is the default
                        table->readRows( rowp );

                        const QCassandraRows& rows = table->rows();
                        std::for_each( rows.begin(), rows.end(),
                            [&rows]( const QSharedPointer<QCassandraRow>& row )
                            {
                                qDebug() << "\t\trow_name = " << row->rowName();
                                /*const QCassandraRows& rows = rows[row_name]->rows();
                                QList<QString> row_names = rows.keys();*/
                            });
                    });
            });

#if 0
    QSharedPointer<QtCassandra::QCassandraContext> context = cassandra.context("browse_test");
	//cassandra["browse_test"]["my_table"]["1"]["name"] = "This is my name";
    QSharedPointer<QtCassandra::QCassandraTable> table = context->table("my_table");
    QSharedPointer<QtCassandra::QCassandraRow> row = table->row(QString("1"));
    QSharedPointer<QtCassandra::QCassandraCell> cell = row->cell(QString("name"));
    cell->setValue("This is my name");

    //context->create();
    context->drop();

    const QString value = cassandra["browse_test"]["my_table"][QString("1")][QString("name")].value().stringValue();
    qDebug() << "value=" << value;

    //cassandra.dropContext( "browse_test" );
    QSharedPointer<QtCassandra::QCassandraContext> context(cassandra.context("qt_cassandra_test_rw"));
    context->setStrategyClass("SimpleStrategy"); // default is LocalStrategy
    //context->setDurableWrites(false); // by default this is 'true'
    context->setReplicationFactor(1); // by default this is undefined

    QSharedPointer<QtCassandra::QCassandraTable> table(context->table("qt_cassandra_test_table"));
    //table->setComment("Our test table.");
    table->setColumnType("Standard"); // Standard or Super
    table->setKeyValidationClass("BytesType");
    table->setDefaultValidationClass("BytesType");
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
        qDebug() << "Context and its table were created!";
    }
    catch(org::apache::cassandra::InvalidRequestException& e) {
        qDebug() << "Exception is [" << e.why.c_str() << "]";
    }

    //try {  // by default the rest should not generate an exception
    // now that it's created, we can access it with the [] operator
    QtCassandra::QCassandraValue value1(-55);
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")] = value1;
    QtCassandra::QCassandraValue value2(1000000);
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("million")] = value2;

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
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][row_key][column_key] = value3;

    // read this one from the memory cache
    QtCassandra::QCassandraValue v1 = cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")];
    if(v1.int32Value() != -55) {
        qDebug() << "Reading the size value failed. Got" << v1.int32Value() << "instead of -55";
    }

    // clear the cache
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();
    if(cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].exists(QString("size"))) {
        qDebug() << "Yeah! exists(\"size\") worked! (from Cassandra)";
    }
    else {
        qDebug() << "Could not find \"size\" which should be defined";
    }

    // clear the cache
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();
    if(cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"].exists(QString("http://www.snapwebsites.org/page/3"))) {
        qDebug() << "Yeah! exists(\"http://www.snapwebsites.org/page/3\") worked! (from Cassandra)";
    }
    else {
        qDebug() << "Could not find \"http://www.snapwebsites.org/page/3\" which should be defined";
    }

    if(cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"].exists(QString("unknown row"))) {
        qDebug() << "Hmmm... exists(\"unknown row\") worked... (from Cassandra)";
    }
    else {
        qDebug() << "Could not find \"unknown row\" which was expected!";
    }

    // clear the cache
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();

    // re-read this one from Cassandra
    QtCassandra::QCassandraValue v1b = cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("size")];
    if(v1b.int32Value() != -55) {
        qDebug() << "Reading the size value failed. Got" << v1b.int32Value() << "instead of -55";
    }

    QtCassandra::QCassandraValue v2 = cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")][QString("million")];
    if(v2.int32Value() != 1000000) {
        qDebug() << "Reading the size value failed. Got" << v2.int32Value() << "instead of -55";
    }

    if(cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].exists(QString("million"))) {
        qDebug() << "Yeah! exists(\"million\") worked! (from memory)";
    }
    else {
        qDebug() << "Could not find \"million\" which should be defined";
    }

    // undefined cell...
    if(cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].exists(QString("this one"))) {
        qDebug() << "Somehow \"this one\" exists!";
    }
    else {
        qDebug() << "Could not find \"this one\" as expected";
    }

    // clear the cache, test that we can find all the cells
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();
    QtCassandra::QCassandraColumnPredicate column_predicate;
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].readCells(column_predicate);
    const QtCassandra::QCassandraCells& cells(cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].cells());
    qDebug() << "cells in 1st row" << cells.size();
    for(QtCassandra::QCassandraCells::const_iterator it = cells.begin(); it != cells.end(); ++it) {
        qDebug() << "  name" << (*it)->columnName();
    }

    qDebug() << "cellCount()" << cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].cellCount();

    // remove one of the cells
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].dropCell(QString("million"));

    // clear the cache, test that we can find all the cells
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"].clearCache();
    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].readCells(column_predicate);
    const QtCassandra::QCassandraCells& cells2(cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].cells());
    qDebug() << "AFTER REMOVE: cells in 1st row" << cells.size();
    for(QtCassandra::QCassandraCells::const_iterator it = cells2.begin(); it != cells2.end(); ++it) {
        qDebug() << "  name" << (*it)->columnName();
    }

    qDebug() << "cellCount()" << cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"][QString("http://www.snapwebsites.org/page/3")].cellCount();

    cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"].dropRow(QString("http://www.snapwebsites.org/page/3"), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday() + 10000000, QtCassandra::CONSISTENCY_LEVEL_ONE);
    //if(cassandra["qt_cassandra_test_rw"]["qt_cassandra_test_table"].exists(QString("http://www.snapwebsites.org/page/3"))) {
    //    qDebug() << "error: dropped row still exists...";
    //}
    //else {
    //    qDebug() << "dropped row does not exist anymore";
    //}

    //}
    //catch(org::apache::cassandra::InvalidRequestException& e) {
    //    qDebug() << "While Working: exception is [" << e.why.c_str() << "]";
    //}


    //context->drop();
#endif
}

// vim: ts=4 sw=4 et
