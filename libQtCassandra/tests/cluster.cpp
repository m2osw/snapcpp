/*
 * Text:
 *      cluster.cpp
 *
 * Description:
 *      Read the cluster information (name, version, schema).
 *
 * Documentation:
 *      Run with no options.
 *      Fails if the test cannot connect to the default Cassandra cluster.
 *
 * License:
 *      Copyright (c) 2011-2013 Made to Order Software Corp.
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

    const QtCassandra::QCassandraContexts& contexts = cassandra->contexts();
    for(QtCassandra::QCassandraContexts::const_iterator
            c = contexts.begin(); c != contexts.end(); ++c)
    {
        QString context_name = (*c)->contextName();
        qDebug() << "  + Context Name" << context_name;
        qDebug() << "    Strategy class" << (*c)->strategyClass();
        const QtCassandra::QCassandraContext::QCassandraContextOptions options = (*c)->descriptionOptions();
        for(QtCassandra::QCassandraContext::QCassandraContextOptions::const_iterator
                o = options.begin(); o != options.end(); ++o)
        {
            qDebug() << "    + Option" << o.key() << "=" << o.value();
        }
        qDebug() << "    Replication Factor:" << (*c)->replicationFactor();
        qDebug() << "    Durable Writes:" << (*c)->durableWrites();
        // Test to make sure we get a NULL pointer when we try to retrieve an undefined table
        //QSharedPointer<QtCassandra::QCassandraTable> tbl = (*c)->table("random");
        //qDebug() << " --- tbl" << tbl.isNull();
        const QtCassandra::QCassandraTables& tables = (*c)->tables();
        for(QtCassandra::QCassandraTables::const_iterator
                t = tables.begin(); t != tables.end(); ++t)
        {
            qDebug() << "    + Table" << (*t)->tableName() << "/" << (*t)->identifier() << " (From Context" << (*t)->contextName() << ")";
            QString comment = (*t)->comment();
            if(!comment.isEmpty()) {
                qDebug() << "      Comment:" << comment;
            }
            qDebug() << "      Column Type" << (*t)->columnType();
            qDebug() << "      Default Validation Class" << (*t)->defaultValidationClass();
            QString key_alias = (*t)->keyAlias();
            if(key_alias.isEmpty()) {
                qDebug() << "      Key Validation Class" << (*t)->keyValidationClass();
            }
            else {
                qDebug() << "      Key Validation Class" << (*t)->keyValidationClass() << "and alias" << key_alias;
            }
            QString subcomparator = (*t)->subcomparatorType();
            if(subcomparator.isEmpty()) {
                qDebug() << "      Comparator Type" << (*t)->comparatorType();
            }
            else {
                qDebug() << "      Comparator Type" << (*t)->comparatorType() << "and subtype" << subcomparator;
            }
            qDebug() << "      Row Cache Provider" << (*t)->rowCacheProvider();
            qDebug() << "      Row Cache Size" << (*t)->rowCacheSize() << "for" << (*t)->rowCacheSavePeriodInSeconds() << "seconds";
            qDebug() << "      Key Cache Size" << (*t)->keyCacheSize() << "for" << (*t)->keyCacheSavePeriodInSeconds() << "seconds";
            qDebug() << "      Read repair chance" << (*t)->readRepairChance();
            qDebug() << "      Compaction Threshold: minimum" << (*t)->minCompactionThreshold() << "maximum" << (*t)->maxCompactionThreshold();
            qDebug() << "      Replicate on Write" << (*t)->replicateOnWrite();
            qDebug() << "      Merge Shards Chance" << (*t)->mergeShardsChance();
            qDebug() << "      Garbage Collection Grace Period" << (*t)->gcGraceSeconds() << "seconds";
            qDebug() << "      Memory Tables Size (Mb)" << (*t)->memtableThroughputInMb()
                               << "Flush After (min.)" << (*t)->memtableFlushAfterMins()
                               << "Operations in Millions" << (*t)->memtableOperationsInMillions();

            const QtCassandra::QCassandraColumnDefinitions& columns = (*t)->columnDefinitions();
            if(columns.begin() == columns.end()) {
                qDebug() << "      No column defintions";
            }
            for(QtCassandra::QCassandraColumnDefinitions::const_iterator
                    col = columns.begin(); col != columns.end(); ++col)
            {
                qDebug() << "      + Column" << (*col)->columnName();
                qDebug() << "        Validation Class" << (*col)->validationClass();
                QString type;
                switch((*col)->indexType()) {
                case QtCassandra::QCassandraColumnDefinition::INDEX_TYPE_KEYS:
                    type = "KEYS";
                    break;

                default: // unknown
                    type = "Unknown";
                    break;

                }
#pragma GCC push
#pragma GCC diagnostic ignored "-Wsign-promo"
                qDebug() << "        Index Type" << (*col)->indexType()
                                                 << ("(" + type + ")");
                qDebug() << "        Index Name" << (*col)->indexName();
#pragma GCC pop
            }
        }
    }
}

// vim: ts=4 sw=4 et
