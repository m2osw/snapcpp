// Snap Websites Server -- snap websites server
// Copyright (C) 2011-2014  Made to Order Software Corp.
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

#include <controlled_vars/controlled_vars.h>
#include <QtCassandra/QCassandra.h>

#include <QString>

namespace snap
{


snap_cassandra::snap_cassandra()
{
    // empty
}

void snap_cassandra::connect( snap_config const& config )
{
    // This function connects to the Cassandra database, but it doesn't
    // keep the connection. We are the server and the connection would
    // not be shared properly between all the children.
    f_cassandra_host = config["cassandra_host"];
    if(f_cassandra_host.isEmpty())
    {
        f_cassandra_host = "localhost";
    }
    //
    QString port_str( config["cassandra_port"] );
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
    // This will take entries into the configuration file:
    // wait_interval, and wait_max_tries.
    //
    int wait_interval  = config["wait_interval"].toInt();
    if(wait_interval < 5)
    {
        wait_interval = 5;
    }
    int wait_max_tries = config["wait_max_tries"].toInt();
    f_cassandra = QtCassandra::QCassandra::create();
    Q_ASSERT(f_cassandra);
    while( !f_cassandra->connect(f_cassandra_host, f_cassandra_port) )
    {
        // if wait_max_tries is 1 we're about to call exit(1) so we're not going
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
                SNAP_LOG_FATAL() << "TIMEOUT: Could not connect to remote Cassandra server at ("
                    << f_cassandra_host << ":" << f_cassandra_port << ")!";
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
        const QString context_name(snap::get_name(snap::SNAP_NAME_CONTEXT));
        context = f_cassandra->context(context_name);
        context->setStrategyClass("org.apache.cassandra.locator.SimpleStrategy");
        context->setReplicationFactor(1);
        context->create();
        // we don't put the tables in here so we can call the create_table()
        // and have the tables created as required (i.e. as we add new ones
        // they get added as expected, no need for special handling.)
    }
}


QtCassandra::QCassandraContext::pointer_t snap_cassandra::get_snap_context()
{
    if( !f_cassandra )
    {
        SNAP_LOG_FATAL() << "Must connect to cassandra first!";
        exit(1);
    }

    // we need to read all the contexts in order to make sure the
    // findContext() works
    f_cassandra->contexts();
    const QString context_name(snap::get_name(snap::SNAP_NAME_CONTEXT));
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


}
// namespace snap

// vim: ts=4 sw=4 et
