// Snap Websites Server -- snap watchdog daemon
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

#include "snapwatchdog.h"

#include <snapwebsites/log.h>
#include <snapwebsites/snap_cassandra.h>
#include <snapwebsites/qdomhelpers.h>

#include <fstream>

#include <sys/wait.h>

#include "poison.h"


/** \file
 * \brief This file represents the Snap! Watchdog daemon.
 *
 * The snapwatchdog.cpp and corresponding header file represents the Snap!
 * Watchdog daemon. This is not exactly a server, although it somewhat
 * (mostly) behaves like one. This tool is used as a daemon to make
 * sure that various resources on a server remain available as expected.
 */


/** \mainpage
 * \brief Snap! Watchdog Documentation
 *
 * \section introduction Introduction
 *
 * The Snap! Watchdog is a tool that works in unisson with Snap! C++.
 * It is used to monitor all the servers used with Snap! in order to
 * ensure that they all continuously work as expected.
 */


namespace snap
{

// definitions from the plugins so we can define the name and filename of
// the server plugin
namespace plugins
{
extern QString g_next_register_name;
extern QString g_next_register_filename;
}


namespace watchdog
{



/** \brief Get a fixed watchdog plugin name.
 *
 * The watchdog plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_WATCHDOG_DATA_PATH:
        return "data_path";

    case SNAP_NAME_WATCHDOG_SERVERSTATS:
        return "serverstats";

    case SNAP_NAME_WATCHDOG_SIGNAL_NAME:
        return "snapwatchdog_udp_signal";

    case SNAP_NAME_WATCHDOG_STATISTICS_FREQUENCY:
        return "statistics_frequency";

    case SNAP_NAME_WATCHDOG_STATISTICS_PERIOD:
        return "statistics_period";

    case SNAP_NAME_WATCHDOG_STATISTICS_TTL:
        return "statistics_ttl";

    case SNAP_NAME_WATCHDOG_STOP:
        return "STOP";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_CPU_...");

    }
    NOTREACHED();
}

}


watchdog_server::watchdog_server()
{
    set_default_config_filename( "/etc/snapwebsites/snapwatchdog.conf" );
}


watchdog_server::pointer_t watchdog_server::instance()
{
    server::pointer_t s(get_instance());
    if(!s)
    {
        plugins::g_next_register_name = "server";
        plugins::g_next_register_filename = __FILE__;

        s = set_instance(server::pointer_t(new watchdog_server));

        plugins::g_next_register_name.clear();
        plugins::g_next_register_filename.clear();
    }
    return std::dynamic_pointer_cast<watchdog_server>(s);
}


/** \brief Print the version string to stderr.
 *
 * This function prints out the version string of this server to the standard
 * error stream.
 *
 * This is a virtual function so that way servers and daemons that derive
 * from snap::server have a chance to show their own version.
 */
void watchdog_server::show_version()
{
    std::cerr << SNAPWATCHDOG_VERSION_STRING << std::endl;
}


void watchdog_server::watchdog()
{
    SNAP_LOG_INFO("watchdog_server::watchdog(): starting watchdog daemon.");

    define_server_name();
    check_cassandra();
    init_parameters();

    // TODO: test that the "sites" table is available?
    //       (we will not need any such table here)

    char const *stop_message(get_name(watchdog::SNAP_NAME_WATCHDOG_STOP));

    snap_child::udp_server_t udp_signal(udp_get_server(get_parameter(get_name(watchdog::SNAP_NAME_WATCHDOG_SIGNAL_NAME))));
    for(;;)
    {
        // run the watchdog plugins once immediately on startup
        {
            watchdog_child processes(instance());
            processes.run_watchdog_plugins();
        }

        // TODO: we may want to synchronize the wait to the top of the minute
        //       and not a random shifting position...
        char buf[256];
        int const r(udp_signal->timed_recv(buf, sizeof(buf), f_statistics_frequency)); // wait up to 1 minute
        if(r != -1 || errno != EAGAIN)
        {
            if(r < 1 || r >= static_cast<int>(sizeof(buf) - 1))
            {
                perror("watchdog_server::watchdog(): f_udp_signal->timed_recv():");
                SNAP_LOG_FATAL("snap_backend::udp_monitor::run(): an error occurred in the UDP recv() call, returned size: ")(r);
                break;
            }
            buf[r] = '\0';

            if(strcmp(buf, stop_message) == 0)
            {
                SNAP_LOG_INFO("watchdog_server::watchdog(): STOP requested.");
                break;
            }

            // assuming we received PING...
        }
    }
}


void watchdog_server::define_server_name()
{
    // TODO: Should we first check and if a servername is defined in the
    //       snapwatchdog.conf file use that one? (it is a huge problem
    //       of bad data duplication!)
    //
    snap::snap_config wc;
    // TODO: hard coded path is totally WRONG!
    wc.read_config_file("/etc/snapwebsites/snapserver.conf");
    if(!wc.contains("server_name"))
    {
        SNAP_LOG_FATAL("watchdog_server::define_server_name(): snapwatchdog was not able to determine the name of this server.");
        exit(1);
    }

    // save it in our list of parameters
    // (we could directly access f_parameters[] but that way is cleaner)
    set_parameter("server_name", wc["server_name"]);
}


void watchdog_server::check_cassandra()
{
    snap_cassandra cassandra( f_parameters );
    cassandra.connect();
    cassandra.init_context();

    QtCassandra::QCassandraContext::pointer_t context( cassandra.get_snap_context() );
    if( !context )
    {
        SNAP_LOG_FATAL() << "snap_websites context does not exist! Exiting.";
        exit(1);
    }

    // this is sucky, the host/port info should not be taken that way!
    // also we should allow servers without access to cassandra...
    f_cassandra_host = cassandra.get_cassandra_host();
    f_cassandra_port = cassandra.get_cassandra_port();

    // create possibly missing tables
    create_table(context, get_name(watchdog::SNAP_NAME_WATCHDOG_SERVERSTATS),  "Statistics of all our servers.");
}


void watchdog_server::init_parameters()
{
    // Time Frequency (how often we gather the stats)
    {
        QString const statistics_frequency(get_parameter(get_name(watchdog::SNAP_NAME_WATCHDOG_STATISTICS_FREQUENCY)));
        f_statistics_frequency = static_cast<int64_t>(statistics_frequency.toLongLong());
        if(f_statistics_frequency < 60)
        {
            // minimum is 1 minute
            f_statistics_frequency = 60;
        }
        f_statistics_frequency *= 1000; // timed_recv() wants the value in ms
    }

    // Time Period (how many stats we keep in the db)
    {
        QString const statistics_period(get_parameter(get_name(watchdog::SNAP_NAME_WATCHDOG_STATISTICS_PERIOD)));
        f_statistics_period = static_cast<int64_t>(statistics_period.toLongLong());
        if(f_statistics_period < 3600)
        {
            // minimum is 1 hour
            f_statistics_period = 3600;
        }
        // round up to the hour, but keep it in seconds
        f_statistics_period = (f_statistics_period + 3599) / 3600 * 3600;
    }

    // Time To Live (TTL, used to make sure we do not over crowd the database)
    {
        QString const statistics_ttl(get_parameter(get_name(watchdog::SNAP_NAME_WATCHDOG_STATISTICS_TTL)));
        f_statistics_ttl = static_cast<int64_t>(statistics_ttl.toLongLong());
        if(f_statistics_ttl < 3600)
        {
            // minimum is 1 hour
            f_statistics_ttl = 3600;
        }
    }
}


watchdog_child::watchdog_child(server_pointer_t s)
    : snap_child(s)
{
}


watchdog_child::~watchdog_child()
{
}


void watchdog_child::run_watchdog_plugins()
{
    // create a child process so the data between sites does not get
    // shared (also the Cassandra data would remain in memory increasing
    // the foot print each time we run a new website,) but the worst
    // are the plugins; we can request a plugin to be unloaded but
    // frankly the system is not very well written to handle that case.
    pid_t const p(fork_child());
    if(p != 0)
    {
        // parent process
        if(p == -1)
        {
            SNAP_LOG_FATAL("watchdog_server::run_watchdog_process() could not create a child process.");
            // we do not try again, we just abandon the whole process
            exit(1);
            NOTREACHED();
        }
        // block until child is done
        //
        // XXX should we have a way to break the wait after a "long"
        //     while in the event the child locks up?
        int status(0);
        wait(&status);
        // TODO: check status?
        return;
    }

    // we are the child, run the watchdog_process() signal
    try
    {
        f_ready = false;

        // on fork() we lose the configuration so we have to reload it
        logging::reconfigure();

        init_start_date();

        connect_cassandra();

        auto server = std::dynamic_pointer_cast<watchdog_server>(f_server.lock());
        if(!server)
        {
            throw snap_child_exception_no_server("watchdog_child::run_watchdog_plugins(): The p_server weak pointer could not be locked");
        }

        // initialize the plugins
        init_plugins(false);

        f_ready = true;

        // create the watchdog document
        QDomDocument doc("watchdog");

        // run each plugin watchdog function
        server->process_watch(doc);
        QString result(doc.toString());
        if(result.isEmpty())
        {
            static bool err_once(true);
            if(err_once)
            {
                err_once = false;
                SNAP_LOG_ERROR("watchdog_child::run_watchdog_plugins() generated a completely empty result. This can happen if you do not define any watchdog plugins.");
            }
        }
        else
        {
            int64_t start_date(get_start_date());
            // round to the hour first, then apply period
            int64_t date((start_date / (1000000LL * 60LL) * 60LL) % server->get_statistics_period());

            // add the date in ns to this result
            QDomElement watchdog_tag(snap_dom::create_element(doc, "watchdog"));
            watchdog_tag.setAttribute("date", static_cast<qlonglong>(start_date));
            result = doc.toString();

            // save the result in a file first
            QString data_path(server->get_parameter(watchdog::get_name(watchdog::SNAP_NAME_WATCHDOG_DATA_PATH)));
            data_path += QString("/%1.xml").arg(date);
            {
                std::ofstream out;
                out.open(data_path.toUtf8().data(), std::ios_base::binary);
                if(out.is_open())
                {
                    // result already ends with a "\n"
                    out << result;
                }
            }

            // then try to save it in the Cassandra database
            // (if the cluster is not available, we still have the files!)
            //
            // retrieve server statistics table
            QString const table_name(get_name(watchdog::SNAP_NAME_WATCHDOG_SERVERSTATS));
            QtCassandra::QCassandraTable::pointer_t table(f_context->table(table_name));

            QtCassandra::QCassandraValue value;
            value.setStringValue(doc.toString());
            value.setTtl(server->get_statistics_ttl());
            QByteArray cell_key;
            QtCassandra::setInt64Value(cell_key, date);
            table->row(server->get_server_name())->cell(cell_key)->setValue(result);
        }

        // the child has to exit()
        exit(0);
        NOTREACHED();
    }
    catch(snap_exception const& except)
    {
        SNAP_LOG_FATAL("watchdog_server::run_watchdog_plugins(): exception caught ")(except.what());
    }
    catch(std::exception const& std_except)
    {
        SNAP_LOG_FATAL("watchdog_server::run_watchdog_plugins(): exception caught ")(std_except.what())(" (there are mainly two kinds of exceptions happening here: Snap logic errors and Cassandra exceptions that are thrown by thrift)");
    }
    catch(...)
    {
        SNAP_LOG_FATAL("watchdog_server::run_watchdog_plugins(): unknown exception caught!");
    }
    exit(1);
    NOTREACHED();
}


} // namespace snap

// vim: ts=4 sw=4 et
