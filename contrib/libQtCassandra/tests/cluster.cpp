/*
 * Text:
 *      tests/cluster.cpp
 *
 * Description:
 *      Read the cluster information (name, version, schema).
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

#include <QtCassandra/QCassandra.h>
#include <QtCore/QDebug>

int main(int argc, char *argv[])
{
    try
    {
        QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );

        qDebug() << "+ libQtCassandra version" << cassandra->version();

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
        QString name = cassandra->clusterName();
        qDebug() << "+ Cassandra Cluster Name is" << name;

        for( auto context : cassandra->contexts() )
        {
            QString context_name = context->contextName();
            qDebug() << "  + Context Name" << context_name;
            for( const auto& pair : context->fields() )
            {
                qDebug() << "    + " << pair.first << " = " << pair.second.output();
            }

            // Test to make sure we get a NULL pointer when we try to retrieve an undefined table
            //QSharedPointer<QtCassandra::QCassandraTable> tbl = context->table("random");
            //qDebug() << " --- tbl" << tbl.isNull();
            for( auto table : context->tables() )
            {
                qDebug() << "      + Table" << table->tableName()
                         //<< "/" << table->identifier()
                         << " (From Context" << table->contextName() << ")";

                for( const auto& pair : table->fields() )
                {
                    qDebug() << "        + " << pair.first << " = " << pair.second.output();
                }
            }
        }
    }
    catch( QtCassandra::QCassandraOverflowException const & e )
    {
        qDebug() << "QtCassandra::QCassandraOverflowException caught -- " << e.what();
        qDebug() << "Stack trace: ";
        for( auto const & stack_line : e.get_stack_trace() )
        {
            qDebug() << stack_line;
        }
        qDebug() << "End stack trace!";
        exit(1);
    }
    catch( QtCassandra::QCassandraException const & e )
    {
        qDebug() << "QtCassandra::QCassandraException caught -- " << e.what();
        qDebug() << "Stack trace: ";
        for( auto const & stack_line : e.get_stack_trace() )
        {
            qDebug() << stack_line;
        }
        qDebug() << "End stack trace!";
        exit(1);
    }
    catch(std::overflow_error const & e)
    {
        qDebug() << "std::overflow_error caught -- " << e.what();
        exit(1);
    }

    return 0;
}

// vim: ts=4 sw=4 et
