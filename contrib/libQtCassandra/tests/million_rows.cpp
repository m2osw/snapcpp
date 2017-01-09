/*
 * Text:
 *      tests/million_rows.cpp
 *
 * Description:
 *      Create a context with a table, then create over one million
 *      rows to test that we can re-read them back.
 *
 *      WARNING: This test will actually overload your Cassandra
 *               cluster. Each time I try it fails after a little
 *               while (generally some 70,000 cells created or if
 *               the write succeeds, some 30% of the reads before
 *               it fails.)
 *
 * Documentation:
 *      Run with no options, although supports the -h to define
 *      Cassandra's host.
 *      Fails if the test cannot create the context, create the table,
 *      read or write the data.
 *
 * License:
 *      Copyright (c) 2012-2017 Made to Order Software Corp.
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

#include <unistd.h>

int main(int argc, char *argv[])
{
    int err(0);

    try
    {
        QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );

        bool drop(false);
        int replication_factor(1);
        const char *host("localhost");
        for(int i(1); i < argc; ++i)
        {
            if(strcmp(argv[i], "--help") == 0)
            {
                qDebug() << "Usage:" << argv[0] << "[-h <hostname>] [-r <replication-factor>]";
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
            else if(strcmp(argv[i], "-d") == 0)
            {
                drop = 1;
            }
            else if(strcmp(argv[i], "-r") == 0)
            {
                ++i;
                if(i >= argc) {
                    qDebug() << "error: -r must be followed by the number of replication to create in your context.";
                    exit(1);
                }
                replication_factor = atol(argv[i]);
            }
        }

        cassandra->connect(host);
        qDebug() << "Working on Cassandra Cluster Named" << cassandra->clusterName();
        qDebug() << "Working on Cassandra Protocol Version" << cassandra->protocolVersion();

        qDebug() << "+ Initialization";
        qDebug() << "++ Got an old context?";
        QtCassandra::QCassandraContext::pointer_t oldctxt(cassandra->findContext("qt_cassandra_test_large_rw"));
        if(oldctxt)
        {
            qDebug() << "++ Drop the old context";
            cassandra->dropContext("qt_cassandra_test_large_rw");
            qDebug() << "++ Synchronize after the drop";
            if(drop)
            {
                // just do the drop and it succeeded
                exit(0);
            }
        }
        else if(drop)
        {
            qDebug() << "warning: no old table to drop";
            exit(0);
        }
        qDebug() << "++ Setup new context...";
        QtCassandra::QCassandraContext::pointer_t context(cassandra->context("qt_cassandra_test_large_rw"));
        //
        casswrapper::schema::Value replication;
        auto& replication_map(replication.map());
        replication_map["class"]              = QVariant("SimpleStrategy");
        replication_map["replication_factor"] = QVariant(replication_factor);

        auto& fields(context->fields());
        fields["replication"]    = replication;
        fields["durable_writes"] = QVariant(true);

        QtCassandra::QCassandraTable::pointer_t table(context->table("qt_cassandra_test_table"));
        //
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

        try
        {
            context->create();
            qDebug() << "++ Context and its table were created!";
        }
        catch(const std::exception& e)
        {
            qDebug() << "Exception is [" << e.what() << "]";
            exit(1);
        }

        qDebug() << "Now we want to test a large number of rows. This test is slow.";

        //try/catch -- by default the rest should not generate an exception

        // create 'count' rows in the database
        static const int count(1200000);
        std::vector<int32_t> data;
        data.reserve(count);
        for(int i(0); i < count; ++i)
        {
            int32_t r(rand());
            data.push_back(r);
            QtCassandra::QCassandraValue value(r);
            value.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
            QString row(QString("row%1").arg(i));
    //qDebug() << "Save row" << row << "with" << r;
            for(int retry(5); retry > 0; --retry)
            {
                try
                {
                    (*cassandra)["qt_cassandra_test_large_rw"]["qt_cassandra_test_table"][row]["value"] = value;
                    retry = 0;
                }
                catch(const std::exception& e)
                {
                    printf(" [pause because we got exception: %s]", e.what());
                    fflush(stdout);
                    if(retry == 1)
                    {
                        // well... after 5 sec. still timing out, maybe the
                        // server is under super heavy load or completely
                        // disconnected from other nodes
                        printf(" timed out after %d rows inserted\n", i);
                        throw;
                    }
                    // if you do not have enough nodes or have a slow network
                    // (i.e. 100Mbit/sec.) then you are likely to get timed out
                    // exceptions; we need to let Cassandra do some work and
                    // try again; we do so here
                    sleep(1);
                }
            }

            // clear the cache once in a while so the 'count' rows don't stay in memory
            if(i % 100 == 0)
            {
                (*cassandra)["qt_cassandra_test_large_rw"]["qt_cassandra_test_table"].clearCache();
            }
            if((i % 5000) == 0)
            {
                printf(".");
                fflush(stdout);
                // some faster computers will really flood Cassandra which will then
                // throw a Timeout exception (because it does not have the time to
                // process all the data fast enough.)
                //struct timespec pause;
                //pause.tv_sec = 0;
                //pause.tv_nsec = 250000000; // 250ms
                //nanosleep(&pause, NULL);
                //sleep(1);
            }
        }
        printf(" done!\n");
        fflush(stdout);

        // now read the data
        auto column_predicate( std::make_shared<QtCassandra::QCassandraCellKeyPredicate>() );
        column_predicate->setCellKey("value");
        auto row_predicate( std::make_shared<QtCassandra::QCassandraRowPredicate>() );
        row_predicate->setCellPredicate(column_predicate);
        //row_predicate.setWrap();
        //row_predicate.setStartRowName("");
        //row_predicate.setEndRowName("");
        std::map<int32_t, bool> unique;
        //unique.reserve(count);
        for(int i(0); i < count * 2;)
        {
            table->clearCache();
            uint32_t max(table->readRows(row_predicate));
            if(max == 0)
            {
                // we expect to exit here on success
                break;
            }
            QString row_name;
            const QtCassandra::QCassandraRows& r(table->rows());
            for(QtCassandra::QCassandraRows::const_iterator o(r.begin()); o != r.end(); ++o, ++i)
            {
                const QtCassandra::QCassandraCells& c(o.value()->cells());
                if(c.size() != 1)
                {
                    fprintf(stderr, "error: invalid number of cells, excepted exactly 1.\n");
                    ++err;
                }
                QtCassandra::QCassandraCells::const_iterator v(c.begin());
                const QtCassandra::QCassandraValue& n(v.value()->value());
                int32_t l(n.int32Value());
                row_name = o.value()->rowName();
                int32_t rn(row_name.mid(3).toInt());
                if(data[rn] != l)
                {
                    fprintf(stderr, "error: expected value %d, got %d instead\n", data[rn], l);
                    ++err;
                }
                if(unique.find(rn) != unique.end())
                {
                    fprintf(stderr, "error: row \"%s\" found twice.\n", row_name.toUtf8().data());
                    ++err;
                }
                else
                {
                    unique[rn] = true;
                }
                if((i % 5000) == 0)
                {
                    printf(".");
                    fflush(stdout);
                }
            }
            //row_predicate.setStartRowName(row_name);
        }
        printf(" finished\n");
        fflush(stdout);

        // verify that we got it all by checking out the map
        for(int i(0); i < count; ++i)
        {
            if(unique.find(i) == unique.end())
            {
                std::cerr << "error: row \"" << i << "\" never found." << std::endl;
                ++err;
            }
        }

        // we're done with this test, drop the context
        //context->drop();
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
    catch( QtCassandra::QCassandraLogicException const & e )
    {
        qDebug() << "QtCassandra::QCassandraLogicException caught -- " << e.what();
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
        ++err;
    }

    exit(err == 0 ? 0 : 1);
}

// vim: ts=4 sw=4 et
