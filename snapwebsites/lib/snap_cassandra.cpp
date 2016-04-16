// Snap Websites Server -- snap websites server
// Copyright (C) 2011-2016  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "snap_cassandra.h"
#include "snapwebsites.h"
#include "log.h"

#include <unistd.h>

namespace snap
{


snap_cassandra::snap_cassandra( snap_config const & parameters )
    : f_parameters(parameters)
{
    // empty
}


void snap_cassandra::connect()
{
    // This function connects to the Cassandra database, but it doesn't
    // keep the connection. We are the server and the connection would
    // not be shared properly between all the children.
    f_cassandra_host = f_parameters["cassandra_host"];
    if(f_cassandra_host.isEmpty())
    {
        f_cassandra_host = "localhost";
    }
    //
    QString port_str( f_parameters["cassandra_port"] );
    if(port_str.isEmpty())
    {
        port_str = "9160";
    }
    bool ok;
    f_cassandra_port = port_str.toLong(&ok);
    if(!ok)
    {
        SNAP_LOG_FATAL("invalid cassandra_port, a valid number was expected instead of \"")(port_str)("\".");
        exit(1);
    }
    if(f_cassandra_port < 1 || f_cassandra_port > 65535)
    {
        SNAP_LOG_FATAL("invalid cassandra_port, a port must be between 1 and 65535, ")(f_cassandra_port)(" is not.");
        exit(1);
    }

    // TODO:
    // We must stay "alive" waiting for the cassandra server to come up.
    // This takes entries into the f_parameters file:
    // wait_interval and wait_max_tries.
    //
    int wait_interval(f_parameters["wait_interval"].toInt());
    if(wait_interval < 5)
    {
        wait_interval = 5;
    }
    int wait_max_tries(f_parameters["wait_max_tries"].toInt());
    f_cassandra = QtCassandra::QCassandra::create();
    Q_ASSERT(f_cassandra);
    while( !f_cassandra->connect(f_cassandra_host, f_cassandra_port) )
    {
        // if wait_max_tries is 1 we're about to call exit(1) so we are not going
        // to retry once more
        if(wait_max_tries != 1)
        {
            SNAP_LOG_WARNING()
                << "The connection to the Cassandra server failed ("
                << f_cassandra_host << ":" << f_cassandra_port << "). "
                << "Try again in " << wait_interval << " secs.";
            sleep( wait_interval );
        }
        //
        if( wait_max_tries > 0 )
        {
            if( --wait_max_tries <= 0 )
            {
                SNAP_LOG_FATAL("TIMEOUT: Could not connect to remote Cassandra server at ")
                    (f_cassandra_host)(":")(f_cassandra_port)(".");
                exit(1);
            }
        }
    }
}


void snap_cassandra::init_context()
{
    // create the context if it doesn't exist yet
    QtCassandra::QCassandraContext::pointer_t context( get_snap_context() );
    if( !context )
    {
        // create a new context
        QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
        SNAP_LOG_INFO("Creating \"")(context_name)("\"...");
        context = f_cassandra->context(context_name);

        // this is the default for contexts, but just in case we were
        // to change that default at a later time...
        auto& fields(context->fields());
        fields["durable_writes"] = QVariant(true);
        QtCassandra::QCassandraSchema::Value& replication_val = fields["replication"];

        // TODO: add support for replications defined as a % so if we
        //       discover 10 nodes, we user 5 when replication is 50%
        //       (however, once set, we do not change this number...)
        //
        // TODO: if the number of nodes is smaller than the number we
        //       get here, make sure to reduce that number!
        //
        int rep(3);
        QString replication(f_parameters["cassandra_replication"]);
        if(!replication.isEmpty())
        {
            bool ok(false);
            rep = replication.toInt(&ok);
            if(!ok)
            {
                SNAP_LOG_ERROR("unknown replication \"")(replication)("\", falling back to \"3\"");
                rep = 3;
                replication = "3";
            }
        }

        // for developers testing with a few nodes in a single data center,
        // SimpleStrategy is good enough; for anything larger ("a real
        // cluster",) it won't work right
        auto& replication_map(replication_val.map());
        QString const strategy(f_parameters["cassandra_strategy"]);
        if(strategy == "simple")
        {
            replication_map["class"]              = QVariant("SimpleStrategy");
            replication_map["replication_factor"] = QVariant(rep);

            // for simple strategy, use the replication_factor parameter
            // (see http://www.datastax.com/documentation/cql/3.0/cql/cql_reference/create_keyspace_r.html)
        }
        else
        {
            if(strategy == "local")
            {
                SNAP_LOG_FATAL("strategy \"local\" is no longer supported!");
            }
            else
            {
                if(!strategy.isEmpty() && strategy != "network")
                {
                    SNAP_LOG_ERROR("unknown strategy \"")(strategy)("\", falling back to \"network\"");
                }
                replication_map["class"] = QVariant("NetworkTopologyStrategy");
            }

            // here each data center gets a replication factor
            bool found(false);
            QString const data_centers(f_parameters["cassandra_data_centers"]);
            snap_string_list const names(data_centers.split(','));
            for( const auto& dc : names )
            {
                if( !dc.isEmpty() )
                {
                    replication_map[dc] = QVariant(replication);
                    found = true;
                }
            }
            //
            if(!found)
            {
                SNAP_LOG_FATAL("the list of data centers is required when creating a context in a cluster which is not using \"simple\" as its strategy");
            }
        }
        context->create();
        // we do not put the tables in here so we can call the create_table()
        // and have the tables created as required (i.e. as we add new ones
        // they get added as expected, no need for special handling.)
    }
}


QtCassandra::QCassandraContext::pointer_t snap_cassandra::get_snap_context()
{
    if( !f_cassandra )
    {
        SNAP_LOG_FATAL("You must connect to cassandra first!");
        exit(1);
    }

    // we need to read all the contexts in order to make sure the
    // findContext() works
    //
    f_cassandra->contexts();
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    return f_cassandra->findContext(context_name);
}


QString snap_cassandra::get_cassandra_host() const
{
    return f_cassandra_host;
}


int32_t snap_cassandra::get_cassandra_port() const
{
    return f_cassandra_port;
}


bool snap_cassandra::is_connected() const
{
    return f_cassandra->isConnected();
}


/** \brief Create a table in the snap context.
 *
 * The function checks whether the named table exists, if not it
 * creates it with default parameters. The result is a shared pointer
 * to the table in question.
 *
 * By default tables are just created in the Cassandra node you are
 * connected with. In order to use the table, it has to have been
 * propagated. This is done with a synchronization call. That call
 * is performed by this very function the first time a table is
 * queried if that table was created in an earlier call to this
 * function, then the synchronization function gets called and blocks
 * the process until the table was propagated. The current initialization
 * process expects the create_table() to be called a first time when
 * your plugin initial_update() is called, then called again once the
 * table is necessary. Therefore, this create_table() uses a 'call me
 * twice' scheme where the second call ensures the synchrony.
 *
 * \code
 *      // first call creates the table
 *      //
 *      create_table("my_table", "This is my table");
 *
 *      // second call get the table pointer, if necessary, it synchronizes
 *      //
 *      QtCassandra::QCassandraTable::pointer_t tbl(create_table("my_table", "This is my table"));
 * \endcode
 *
 * \todo
 * Provide a structure that includes the different table parameters
 * instead of using hard coded defaults.
 *
 * \param[in] table_name  The name of the new table, if it exists, nothing happens.
 * \param[in] comment  A comment about the new table.
 */
QtCassandra::QCassandraTable::pointer_t snap_cassandra::create_table(QString const & table_name, QString const & comment)
{
    QtCassandra::QCassandraContext::pointer_t context(get_snap_context());

    // does table exist?
    QtCassandra::QCassandraTable::pointer_t table(context->findTable(table_name));
    if(!table)
    {
        // table is not there yet, create it
        table = context->table(table_name);

        QtCassandra::QCassandraSchema::Value compaction;
        auto& compaction_map(compaction.map());
        compaction_map["class"]         = QVariant("SizeTieredCompactionStrategy");
        compaction_map["min_threshold"] = QVariant(4);
        compaction_map["max_threshold"] = QVariant(22);

        auto& table_fields(table->fields());
        table_fields["comment"]                     = QVariant(comment);
        table_fields["memtable_flush_period_in_ms"] = QVariant(60);
        table_fields["gc_grace_seconds"]            = QVariant(86400);
        table_fields["compaction"]                  = compaction;

        table->create();

        f_created_table[table_name] = true;
    }
    else if(f_created_table.contains(table_name))
    {
        // a single synchronization is enough for all created tables
        //
        f_created_table.clear();
    }
    return table;
}


}
// namespace snap
// vim: ts=4 sw=4 et
