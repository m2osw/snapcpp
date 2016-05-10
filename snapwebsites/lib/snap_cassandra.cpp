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
    // We now connect to our proxy instead. This allows us to have many
    // permanent connections to Cassandra (or some other data store) and
    // not have to have threads (at least the C/C++ driver forces us to
    // have threads for asynchronous and timeout handling...)
    //
    tcp_client_server::get_addr_port(f_parameters["snapdbproxy_listen"], f_snapdbproxy_addr, f_snapdbproxy_port, "tcp");

//std::cerr << "snap proxy info: " << f_snapdbproxy_addr << " and " << f_snapdbproxy_port << "\n";
    f_cassandra = QtCassandra::QCassandra::create();
    if(!f_cassandra)
    {
        QString const msg("could not create the QCassandra instance.");
        SNAP_LOG_FATAL(msg);
        throw snap_cassandra_not_available_exception(msg);
    }

    if( !f_cassandra->connect(f_snapdbproxy_addr, f_snapdbproxy_port) )
    {
        QString const msg("could not connect QCassandra to snapdbproxy.");
        SNAP_LOG_FATAL(msg);
        throw snap_cassandra_not_available_exception(msg);
    }
}


void snap_cassandra::init_context()
{
// WARNING: This function should not be used anymore (only to check whether
//          the context exists,) because the context is normally created
//          by snapmanager now.
SNAP_LOG_WARNING("snap_cassandra::init_context() should not be used anymore...");

    // create the context if it does not exist yet
    QtCassandra::QCassandraContext::pointer_t context( get_snap_context() );
    if( !context )
    {
        // create a new context
        QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
        SNAP_LOG_INFO("Creating \"")(context_name)("\"...");
        context = f_cassandra->context(context_name);

        // this is the default for contexts, but just in case we were
        // to change that default at a later time...
        auto & fields(context->fields());
        fields["durable_writes"] = QVariant(true);
        QtCassandra::QCassandraSchema::Value & replication_val = fields["replication"];

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
        QString msg("You must connect to cassandra first!");
        SNAP_LOG_FATAL(msg);
        throw snap_cassandra_not_available_exception(msg);
    }

    // we need to read all the contexts in order to make sure the
    // findContext() works
    //
    f_cassandra->contexts();
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    return f_cassandra->findContext(context_name);

    //QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    //return f_cassandra->context(context_name);
}


QString snap_cassandra::get_snapdbproxy_addr() const
{
    return f_snapdbproxy_addr;
}


int32_t snap_cassandra::get_snapdbproxy_port() const
{
    return f_snapdbproxy_port;
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
        auto & compaction_map(compaction.map());
        compaction_map["class"]         = QVariant("SizeTieredCompactionStrategy");
        compaction_map["min_threshold"] = QVariant(4);
        compaction_map["max_threshold"] = QVariant(22);

        auto & table_fields(table->fields());
        table_fields["comment"]                     = QVariant(comment);
        table_fields["memtable_flush_period_in_ms"] = QVariant(3600000); // Once per hour
        table_fields["gc_grace_seconds"]            = QVariant(86400);
        table_fields["compaction"]                  = compaction;

        table->create();

        f_created_table[table_name] = true;
    }
    else if(f_created_table.contains(table_name))
    {
        // TODO: add support for Future in case we create tables
        //       so we can properly synchronize with the tables
        //       here (although that requires a thread or something
        //       like that... so we'll have to be careful!)
        //for(auto tbl : created_table)
        //{
        //    tbl->wait_until_done();
        //}

        // a single synchronization is enough for all created tables
        //
        f_created_table.clear();
    }
    return table;
}


}
// namespace snap
// vim: ts=4 sw=4 et
