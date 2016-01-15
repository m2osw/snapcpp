/*
 * Text:
 *      read_uuid.cpp
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
#include <QUuid>
#include <thrift-gencpp-cassandra/cassandra_types.h>

//using namespace QtCassandra;

int main(int argc, char *argv[])
{
    QtCassandra::QCassandra     cassandra;

    QUuid uuid("{13818e20-1dd2-11b2-0000-0049660bcef5}");

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

qDebug() << "Get context";
    QSharedPointer<QtCassandra::QCassandraContext> context(cassandra.context("snap_websites"));
qDebug() << "Get table";
    QSharedPointer<QtCassandra::QCassandraTable> table(context->table("uuid_test"));
qDebug() << "Get row" << uuid;

//QByteArray a(uuid.toRfc4122());
//for(int i=0;i<a.size();++i){
//QString s(QString("%1").arg((int)a.at(i)));
//qDebug()<<i<<s;
//}

    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(uuid));
qDebug() << "Row is" << row.data();
    QSharedPointer<QtCassandra::QCassandraCell> cell(row->cell("abc"));
//    QCassandraValue p;
//    p.setInt32Value(1234);
//    cell->setValue(p);
//exit(0);
qDebug() << "Cell is" << cell.data();
    QtCassandra::QCassandraValue v(cell->value());
qDebug() << "Value is" << v.size() << "bytes" << v.int32Value();
QByteArray ll;
ll = QByteArray::number(0xc);//0x0102030405060708LL);
qDebug() << "QByteArray long long" << ll.size() << "bytes";
for(int i=0;i<ll.size();++i){
QString s(QString("0x%1").arg((int)ll.at(i), 2, 16, QChar('0')));
qDebug()<<i<<s;
}

//qDebug() << "Get value";
    QtCassandra::QCassandraValue r((*context)["uuid_test"][uuid][":"]);
qDebug() << "Value is" << r.size() << "bytes";

    qDebug() << "Value is" << r.int16Value();

    exit(0);
}

// vim: ts=4 sw=4 et
