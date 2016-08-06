/////////////////////////////////////////////////////////////////////////////////
// Snap Init Server -- snap initialization server

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
//
// This server reads in a configuration file and keeps specified services running.
// When signaled, it will terminate those services cleanly.
/////////////////////////////////////////////////////////////////////////////////

// snapinit
//
#include "snapinit.h"
#include "common.h"

// snapwebsites library
//
#include "chownnm.h"
#include "log.h"
#include "mkdir_p.h"
#include "not_used.h"

// C++ library
//
#include <sstream>
#include <fstream>

// C library
//
#include <fcntl.h>
#include <glob.h>
#include <syslog.h>
#include <sys/wait.h>


/** \file
 * \brief Initialize a Snap! server on your server.
 *
 * This tool is the snapserver controller, used to start and stop the
 * server and backend processes.
 *
 * The tool is actually in charge of starting all the elements that can
 * be started on a Snap! server:
 *
 * \li snapinit -- snapinit gets started by script /etc/init.d/snapserver
 *     (we will later make it compatible with the new boot system, though)
 * \li snapcommunicator -- the RPC system used by snap to communicate
 *     between all the servers used by snap.
 * \li snaplock -- a fail safe multi-computer locking mechanism
 * \li snapdbproxy -- a service that connects to Cassandra nodes and sits
 *     around making it a lot faster to access the database and also make
 *     sure that if any one node goes down, it continues to work smoothly
 *     (and since the Cassandra C++ driver makes use of threads, it saves
 *     us from having such in our main application!)
 * \li snapserver -- the actual snap server listening for incoming client
 *     connections (through Apache2 and snap.cgi for now)
 * \li snapbackend -- various backends to support working on slow tasks
 *     so front ends do not have to do those slow task and have the client
 *     wait for too long... (i.e. images, pagelist, sendmail, ...)
 * \li snapmanagerdaemon -- a daemon used to run managerial commands on
 *     any computer in a snap cluster
 * \li snapwatchdogserver -- a server which checks various things to
 *     determine the health of the server it is running on
 * \li snapfirewall -- a service allowing any other process to block an IP
 *     address with the iptables filtering system
 * \li "snapcron" -- this task actually makes use of snapbackend without
 *     the --action command line option; it runs tasks that are to be
 *     run once in a while (by default every 5 minutes) such as clean ups,
 *     aggregation, etc.
 *
 * The snapinit tool reads a snapinit.xml file, by default it is expected
 * to be found under /etc/snapwebsites. That file declares any number of
 * parameters as required by the snapinit tool to start the service.
 *
 * A sample XML is briefly shown here:
 *
 * \code{.xml}
 *    <?xml version="1.0"?>
 *    <!-- Snap Communicator is started as a service -->
 *    <service name="snapcommunicator" required="required">
 *      <!-- we give this one a very low priority as it has to be started
 *           before anything else -->
 *      <priority>-10</priority>
 *      <config>/etc/snapwebsites/snapcommunicator.conf</config>
 *      <connect>127.0.0.1:4040</connect>
 *      <wait>10</wait>
 *    </service>
 * \endcode
 *
 * TBD: since each backend service can be run only once, we may want to
 *      look in having this XML file as a common file and the definitions
 *      would include server names where the services are expected to run
 *      when things are normal and which server to use as fallbacks when
 *      something goes wrong. Right now, I think that I will keep it simpler.
 *      The sharing of the XML could be done via snapcommunitor or Cassandra
 *      but then that would mean snapinit would have to know how to start
 *      snapcommunicator without the XML...
 *
 * The snapinit object is also a state machine, albeit very simple. It has
 * two states to speak of: Ready and Stopping. While Stopping we may not
 * do certain things such as attempting to restart a process. Yet the
 * services will know where they are at so it should not matter much at
 * this level.
 *
 * \code
 *                     O
 *                     |
 *                     | create snapinit
 *                     |
 *                     V
 *              +-----------------+
 *              |                 |
 *              | Ready           |
 *              |                 |
 *              +------+----------+
 *                     |
 *                     | terminate [if STOP, QUIT, ... or services cannot run]
 *                     |
 *                     V
 *              +-----------------+
 *              |                 |
 *              | Stopping        |
 *              |                 |
 *              +------+----------+
 *                     |
 *                     | exit snapinit
 *                     |
 *                     O
 * \endcode
 *
 * Note that we moved the connecting to the snapcommunicator part to
 * the permanent TCP connection type by deriving our class from such
 * instead of having to reimplement a similar algorithm in snapinit.
 * (see the snap_tcp_client_permanent_message_connection class)
 *
 * This means the following goes from the Ready state to the Connected
 * state on its own once snapcommunicator is running. Note that their
 * can be a delay of up to three seconds before the connection occurs.
 *
 * \code
 *                       O
 *                       |
 *                       | create listener
 *                       |
 *                       V
 *                +-----------------+
 *                |                 |  timer
 *  +------------>| Ready           |<----------[snap_communicator]
 *  |             |                 |
 *  |             |                 +----------+
 *  |             |                 |          |
 *  |             +------+----------+          |
 *  ^                    |                     |
 *  |                    | connected           |
 *  |                    |                     |
 *  |                    V                     |
 *  | lost        +-----------------+          V
 *  | connection  |                 |          |
 *  +-------------+ Connected       |          |
 *                |                 |          |
 *                +------+----------+          |
 *                       |                     |
 *                       +---------------------+
 *                       |
 *                       | destroy listener
 *                       |
 *                       O
 * \endcode
 *
 * Once connected we get the process_connected() callback and use it to
 * send the REGISTER message to snapcommunicator.
 *
 * The following is an attempt at describing the process messages used
 * to start everything and stop everything (that's an older version although
 * the concept remains quite similar):
 *
 * \msc
 * hscale = "2";
 * a [label="snapinit"],
 * b [label="snapcommunicator"],
 * c [label="snapserver"],
 * d [label="snapbackend (permanent)"],
 * e [label="snapbackend (cron)"],
 * f [label="neighbors"],
 * g [label="snapsignal"];
 *
 * d note d [label="images, page_list, sendmail,snapwatchdog"];
 *
 * #
 * # snapinit initialization
 * #
 * a=>a [label="init()"];
 * a=>a [label="--detach (optional)"];
 * |||;
 * ... [label="pause (0 seconds)"];
 * |||;
 * a=>>a [label="connection timeout"];
 * a=>b [label="start (fork+execv)"];
 * |||;
 * b>>a;
 *
 * #
 * # snapcommunicator initialization
 * #
 * b=>b [label="open socket to neighbor"];
 * b->f [label="CONNECT type=frontend ..."];
 * f->b [label="ACCEPT type=backend ..."];
 * ... [label="or"];
 * f->b [label="REFUSE type=backend"];
 * |||;
 * ... [label="neighbors may try to connect too"];
 * |||;
 * f=>f [label="open socket to neighbor"];
 * f->b [label="CONNECT type=backend ..."];
 * b->f [label="ACCEPT type=frontend ..."];
 * ... [label="or"];
 * b->f [label="REFUSE type=frontend"];
 *
 * #
 * # snapinit registers with snapcommunicator
 * #
 * |||;
 * ... [label="pause (10 seconds)"];
 * |||;
 * a=>a [label="open socket to snapcommunicator"];
 * a->b [label="REGISTER service=snapinit;version=<version>"];
 * b->a [label="READY"];
 * a->b [label="SERVICES list=...depends on snapinit.xml..."];
 * a=>a [label="wakeup services"];
 * |||;
 * b->a [label="HELP"];
 * a->b [label="COMMANDS list=HELP,QUITTING,READY,STOP"];
 *
 * #
 * # snapinit starts snapserver which registers with snapcommunicator
 * #
 * |||;
 * ... [label="pause (0 seconds)"];
 * |||;
 * --- [label="...start snapserver..."];
 * a=>>a [label="connection timeout"];
 * a=>c [label="start (fork+execv)"];
 * c>>a;
 * c=>c [label="open socket to snapcommunicator"];
 * c->b [label="REGISTER service=snapserver;version=<version>"];
 * b->c [label="READY"];
 *
 * #
 * # snapinit starts various backends (images, sendmail, ...)
 * #
 * |||;
 * ... [label="pause (<wait> seconds, at least 1 second)"];
 * |||;
 * --- [label="...(start repeat for each backend)..."];
 * a=>>a [label="connection timeout"];
 * a=>d [label="start (fork+execv)"];
 * d>>a;
 * d=>d [label="open socket to snapcommunicator"];
 * d->b [label="REGISTER service=<service name>;version=<version>"];
 * b->d [label="READY"];
 * b->d [label="STATUS service=snapwatchdog"];
 * |||;
 * ... [label="pause (<wait> seconds, at least 1 second)"];
 * |||;
 * --- [label="...(end repeat)..."];
 *
 * #
 * # snapinit starts snapback (CRON task)
 * #
 * |||;
 * ... [label="...cron task, run once per timer tick event..."];
 * |||;
 * a=>>a [label="CRON timer tick"];
 * a=>a [label="if CRON tasks still running, return immediately"];
 * a=>e [label="start (fork+execv)"];
 * e>>a;
 * e=>e [label="open socket to snapcommunicator"];
 * e->b [label="REGISTER service=snapbackend;version=<version>"];
 * b->e [label="READY"];
 * |||;
 * e=>>e [label="run CRON task 1"];
 * e=>>e [label="run CRON task 2"];
 * ...;
 * e=>>e [label="run CRON task n"];
 * |||;
 * e->b [label="UNREGISTER service=snapbackend"];
 * |||;
 * ... [label="...(end of cron task)..."];
 *
 * #
 * # STOP process
 * #
 * |||;
 * --- [label="snapinit STOP process with: 'snapinit stop' or 'snapsignal snapinit/STOP'"];
 *
 * |||;
 * g->b [label="'snapsignal snapinit/STOP' command sends STOP to snapcommunicator"];
 * b->a [label="STOP"];
 * ... [label="...or..."];
 * a->a [label="'snapinit stop' command sends STOP to snapinit"];
 * ...;
 * a->b [label="UNREGISTER service=snapinit"];
 * a->b [label="STOP"];
 * b->c [label="snapserver/STOP"];
 * b->d [label="<service name>/STOP"];
 * b->e [label="snapbackend/STOP"];
 * c->b [label="UNREGISTER service=snapserver"];
 * c->c [label="exit(0)"];
 * d->b [label="UNREGISTER service=<service name>"];
 * d->d [label="exit(0)"];
 * e->b [label="UNREGISTER service=snapbackend (if still running at the time)"];
 * e->e [label="exit(0)"];
 * ... [label="once all services are unregistered"];
 * b->f [label="DISCONNECT"];
 * \endmsc
 */

namespace snapinit
{
namespace
{


/** \brief Capture the glob pointer in a shared pointer, this deletes it.
 *
 * This function is used to RAII the pointer returned by glob.
 *
 * \param[in] g  The glob pointer.
 */
void glob_deleter(glob_t * g)
{
    globfree(g);
}


/** \brief Capture errors happening while glob() is running.
 *
 * This function gets called whenever glob() encounters an I/O error.
 *
 * \return 0 asking for glob() to stop ASAP.
 */
int glob_error_callback(const char * epath, int eerrno)
{
    SNAP_LOG_ERROR("an error occurred while reading directory under \"")
                  (epath)
                  ("\". Got error: ")
                  (eerrno)
                  (", ")
                  (strerror(eerrno))
                  (".");

    // do not abort on a directory read error...
    return 0;
}


/** \brief Define whether the logger was initialized.
 *
 * This variable defines whether the logger was already initialized.
 */
bool g_logger_ready = false;



/** \brief List of configuration files.
 *
 * This variable is used as a list of configuration files. It is
 * empty here because the configuration file may include parameters
 * that are not otherwise defined as command line options.
 */
std::vector<std::string> const g_configuration_files; // Empty


/** \brief Command line options.
 *
 * This table includes all the options supported by the server.
 */
advgetopt::getopt::option const g_snapinit_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "Usage: %p [-<opt>] <start|restart|stop>",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "where -<opt> is one or more of:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        'b',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "binary-path",
        "/usr/bin",
        "Path where snap! binaries can be found (e.g. snapserver and snapbackend).",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'c',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        "/etc/snapwebsites/snapinit.conf",
        "Configuration file to initialize snapinit.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "debug",
        nullptr,
        "Start the server and backend services in debug mode.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'd',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "detach",
        nullptr,
        "Background the snapinit server.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show usage and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "list",
        nullptr,
        "Display the list of services and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'k',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "lockdir",
        "/run/lock/snapwebsites",
        "Full path to the snapinit lockdir.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'l',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "logfile",
        nullptr,
        "Full path to the snapinit logfile.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'n',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "nolog",
        nullptr,
        "Only output to the console, not the log file.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "remove-lock",
        nullptr,
        "For the removal of an existing lock (useful if a spurious lock still exists).",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "running",
        nullptr,
        "test whether snapinit is running; exit with 0 if so, 1 otherwise.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "tree",
        nullptr,
        "Generate the tree of services in a dot file and then output an image in the snapinit data_path directory.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "show the version of the snapinit executable.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::default_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};


}
// no name namespace




/////////////////////////////////////////////////
// SNAP INIT (class implementation)            //
/////////////////////////////////////////////////


snap_init::pointer_t snap_init::f_instance;


snap_init::snap_init( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapinit_options, g_configuration_files, "SNAPINIT_OPTIONS")
    , f_lock_filename( QString("%1/snapinit-lock.pid")
                       .arg(QString::fromUtf8(f_opt.get_string("lockdir").c_str()))
                     )
    , f_lock_file( f_lock_filename )
    , f_communicator(snap::snap_communicator::instance())
{
    // commands that return immediately
    //
    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    if( f_opt.is_defined( "help" ) )
    {
        usage();
        snap::NOTREACHED();
    }
    if(f_opt.is_defined("running"))
    {
        // WARNING: shell true/false are inverted compared to C++
        exit(is_running() ? 0 : 1);
        snap::NOTREACHED();
    }
    if(f_opt.is_defined("remove-lock"))
    {
        // exit() does not force the lock removal so we have to call
        // it here...
        //
        remove_lock(true);
        exit(0);
        snap::NOTREACHED();
    }

    f_debug = f_opt.is_defined("debug");

    // read the configuration file
    //
    f_config.read_config_file( f_opt.get_string("config").c_str() );

    // get the server name
    // (we do it early so the logs can make use of it)
    //
    if(f_config.contains("server_name"))
    {
        f_server_name = f_config["server_name"];
    }
    if(f_server_name.isEmpty())
    {
        // use hostname by default if undefined in configuration file
        //
        char host[HOST_NAME_MAX + 1];
        host[HOST_NAME_MAX] = '\0';
        if(gethostname(host, sizeof(host)) != 0
        || strlen(host) == 0)
        {
            common::fatal_message("server_name is not defined in your configuration file and hostname is not available as the server name,"
                            " snapinit not started. (in snapinit.cpp/snap_init::snap_init())");

            // we MUST exit with zero or systemctl will restart us in an
            // infinite loop!
            //
            exit(0);
            snap::NOTREACHED();
        }
        // TODO: add code to verify that we like that name (i.e. if the
        //       name includes periods we will reject it when sending
        //       messages to/from snapcommunicator)
        //
        f_server_name = host;
    }
    {
        QString name;
        bool found_dot(false);
        for(QChar *s(f_server_name.data()); !s->isNull() && !found_dot; ++s)
        {
            switch(s->unicode())
            {
            case '-':
                // the dash is not acceptable in our server name
                // replace it with an underscore
                //
                SNAP_LOG_WARNING("Hostname \"")(f_server_name)("\" includes a dash character (-) which is not supported by snap. Replacing with an underscore (_). If that is not what you expect, edit snapinit.conf and set the name as you want it in server_name=...");
                name += QChar('_');
                break;

            case '.':
                // according to the hostname documentation, the FQDN is
                // the name before the first dot; this means if you have
                // more than two dots, the sub-sub-sub...sub-domain is
                // the FQDN
                //
                SNAP_LOG_WARNING("Hostname \"")(f_server_name)("\" includes a dot character (.) which is not supported by snap. We assume that indicates the end of the name. If that is not what you expect, edit snapinit.conf and set the name as you want it in server_name=...");
                found_dot = true;
                break;

            default:
                // force lowercase -- hostnames are expected to be in
                // lowercase although they are case insensitive so we
                // certainly want them to be in lowercase anyway
                //
                name += s->toLower();
                break;

            }
        }

        // TBD: We could further prevent the name from starting/ending with '_'?
        //
        if(name != f_server_name)
        {
            // warning about changing the name (not that in the above loop
            // we do not warn about changing the name to lowercase)
            //
            SNAP_LOG_WARNING("Your server_name parameter \"")(f_server_name)("\" was transformed to \"")(name)("\" to be compatible with Snap!");
            f_server_name = name;
        }

        // make sure the computer name is no more than 63 characters
        //
        if(f_server_name.isEmpty()
        || f_server_name.length() > 63)
        {
            QString const msg(QString("Server name \"%1\" is too long. The maximum length allowed is 63 characters.")
                                .arg(f_server_name));
            common::fatal_message(msg);

            // we MUST exit with zero or systemctl will restart us in an
            // infinite loop!
            //
            exit(0);
            snap::NOTREACHED();
        }

        // make sure we can use that name to send messages between computers
        //
        try
        {
            snap::snap_communicator_message::verify_name(f_server_name, false, true);
        }
        catch(snap::snap_communicator_invalid_message & e)
        {
            QString const msg(QString("even with possible corrections, snap does not like your server name \"%1\". Error: %2")
                                .arg(f_server_name)
                                .arg(e.what()));
            common::fatal_message(msg);

            // we MUST exit with zero or systemctl will restart us in an
            // infinite loop!
            //
            exit(0);
            snap::NOTREACHED();
        }
    }

    // setup the logger
    //
    if( f_opt.is_defined( "nolog" ) )
    {
        snap::logging::set_progname(argv[0]);
        snap::logging::configure_console();
    }
    else if( f_opt.is_defined("logfile") )
    {
        snap::logging::configure_logfile( QString::fromUtf8(f_opt.get_string( "logfile" ).c_str()) );
    }
    else
    {
        if( f_config.contains("log_config") )
        {
            // use .conf definition when available
            f_log_conf = f_config["log_config"];
        }
        snap::logging::configure_conffile( f_log_conf );
    }

    if( f_debug )
    {
        // Force the logger level to DEBUG
        // (unless already lower)
        //
        snap::logging::reduce_log_output_level( snap::logging::log_level_t::LOG_LEVEL_DEBUG );
    }

    g_logger_ready = true;

    // user can change the current directory to another directory
    //
    if(f_config.contains("data_path"))
    {
        f_data_path = f_config["data_path"];
    }

    // try to go to our home directory, warn if it fails, but go on
    //
    if(chdir(f_data_path.toUtf8().data()) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("could not change to the snapinit home directory \"")(f_data_path)("\" (errno: ")(e)(", ")(strerror(e))(")");
        // go on...
    }

    // do not do too much in the constructor or we may get in
    // trouble (i.e. calling shared_from_this() from the
    // constructor fails)

    init_message_functions();
}


/** \brief Clean up the snap_init object.
 *
 * The destructor makes sure that the snapinit lock file gets removed
 * before exiting the process.
 */
snap_init::~snap_init()
{

// WARNING: Do not expect the destructor to ever be called, instead
//          we call the snap_init::exit() which in most cases means
//          that the destructor does not get called because it directly
//          calls the C ::exit() function...

    remove_lock();
}


/** \brief Initialize the map of functions to handle message.
 *
 * This function creates a couple of function maps. One is for the
 * UDP message and the other is for the TCP messages.
 *
 * See the process_message() function for their usage.
 */
void snap_init::init_message_functions()
{
    // ******************* TCP and UDP messages

    auto stop_func =
            [&]( snap::snap_communicator_message const& )
            {
                // someone asking us to stop snap_init; this means we want to stop
                // all the services that snap_init started; if we have a
                // snapcommunicator, then we use that to send the STOP signal to
                // all services at once
                //
                terminate_services();
            };

    // someone sent "snapinit/STOP" to snapcommunicator
    // or "[whatever/]STOP" directly to snapinit (via UDP)
    //
    f_udp_message_map = {
        {
            "STOP",
            stop_func
        }
    };

    // ******************* TCP only messages
    f_tcp_message_map = {
        {
            // all have to implement the HELP command
            //
            "HELP",
            [&]( snap::snap_communicator_message const & )
            {
                snap::snap_communicator_message reply;
                reply.set_command("COMMANDS");

                // list of commands understood by snapinit
                //
                reply.add_parameter("list", "HELP,LOG,QUITTING,READY,RELOADCONFIG,SAFE,STATUS,STOP,UNKNOWN");

                f_listener_connection->send_message(reply);
            }
        },
        {
            "LOG",
            [&]( snap::snap_communicator_message const & )
            {
                SNAP_LOG_INFO("Logging reconfiguration.");
                snap::logging::reconfigure();
            }
        },
        {
            "QUITTING",
            stop_func
        },
        {
            "READY",
            [&]( snap::snap_communicator_message const & )
            {
                // mark the snapcommunicator and snapinit services
                // as registered
                //
                // we do not receive the STATUS event for the snapinit
                // service because it has to register itself before it
                // can send the COMMNANDS message and therefore
                // snapcommunicator does not yet know we are interested
                // by that message.
                //
                f_snapcommunicator_service->get_process().action_process_registered();
                f_snapinit_service->get_process().action_process_registered();

                // send the list of local services to the snapcommunicator
                //
                snap::snap_communicator_message reply;
                reply.set_command("SERVICES");

                // generate the list of services as a string of
                // comma separated names
                //
                snap::snap_string_list service_list_name;
                auto get_service_name = [&service_list_name]( auto const & svc )
                {
                    if(svc)
                    {
                        service_list_name << svc->get_service_name();
                    }
                };
                //
                std::for_each( std::begin(f_service_list), std::end(f_service_list), get_service_name );
                QString const services(service_list_name.join(","));
                //
                SNAP_LOG_TRACE("READY: list to send to server: [")(services)("].");
                reply.add_parameter("list", services);

                f_listener_connection->send_message(reply);
            }
        },
        {
            "RELOADCONFIG",
            [&]( snap::snap_communicator_message const& )
            {
                // we need a full restart in this case (because snapinit
                // cannot restart itself!)
                //
                // also if you are a programmer we cannot do a systemctl
                // restart so we just skip the feature...
                //
                if(getuid() == 0)
                {
                    snap::NOTUSED(system("systemctl restart snapinit"));
                }
                else
                {
                    SNAP_LOG_WARNING("You are not running snapinit as root (because you are running as a programmer?) so the RELOADCONFIG will be ignored.");
                }
            }
        },
        {
            "SAFE",
            [&]( snap::snap_communicator_message const & message )
            {
                // we received a "we are safe" message so we can move on and
                // start the next service(s)
                //
                bool ok(false);
                QString const & pid_string(message.get_parameter("pid"));
                pid_t const pid(pid_string.toInt(&ok, 10));
                if(!ok)
                {
                    // we need to terminate the existing services cleanly
                    // so we do not use common::fatal_error() here
                    //
                    common::fatal_message(QString("received SAFE message with an invalid \"pid\" parameter (\"%1\").")
                                        .arg(pid_string));

                    // Simulate a STOP, we cannot continue safely
                    //
                    terminate_services();
                    return;
                }

                // search for the process by pid
                //
                auto const s(std::find_if(
                        f_service_list.begin(),
                        f_service_list.end(),
                        [&pid](auto const & svc)
                        {
                            if(svc)
                            {
                                return svc->get_process().get_pid() == pid;
                            }
                            return false;
                        }));
                if(s == f_service_list.end()
                || !*s)
                {
                    // process not found
                    //
                    common::fatal_message(QString("received SAFE message with a \"pid\" parameter that does not match any of our services (\"%1\").")
                                        .arg(pid_string));

                    // Simulate a STOP, we cannot continue safely
                    //
                    terminate_services();
                    return;
                }

                // if the safe message is valid, the following call will
                // make things move forward as expected
                //
                (*s)->get_process().action_safe_message(message.get_parameter("name"));

                // // wakeup other services (i.e. when SAFE is required
                // // the system does not start all the processes timers
                // // at once--now that we have dependencies we could
                // // change that though)
                // //
                // wakeup_services();
            }
        },
        {
            "STATUS",
            [&]( snap::snap_communicator_message const & message )
            {
                auto const service_parm(message.get_parameter("service"));
                auto const status_parm(message.get_parameter("status"));
                //
                auto const & iter(std::find_if(
                        f_service_list.begin(),
                        f_service_list.end(), 
                        [service_parm](auto const & svc)
                        {
                            if(svc)
                            {
                                return svc->get_service_name() == service_parm;
                            }
                            return false;
                        })
                    );
                if( iter != std::end(f_service_list) )
                {
                    if(status_parm == "up")
                    {
                        (*iter)->get_process().action_process_registered();
                    }
                    else
                    {
                        (*iter)->get_process().action_process_unregistered();
                    }
                    SNAP_LOG_TRACE("received status from server: service=")(service_parm)(", status=")(status_parm);
                }
                //else -- many services get started and are not children
                //        of snapinit (i.e. locks, snap_child, ...)
            }
        },
        {
            "STOP",
            stop_func
        },
        {
            "UNKNOWN",
            [&]( snap::snap_communicator_message const & message )
            {
                SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            }
        },
    };
}


/** \brief Actually initialize this snap_init object.
 *
 * This function checks all the parameters and services and initializes
 * them all.
 */
void snap_init::init()
{
    if( f_opt.is_defined( "list" ) )
    {
        // list services
        //
        f_command = command_t::COMMAND_LIST;
    }
    else if( f_opt.is_defined( "tree" ) )
    {
        // create a dot file with the service tree
        //
        f_command = command_t::COMMAND_TREE;
    }
    else
    {
        SNAP_LOG_INFO("--------------------------------- snapinit v" SNAPINIT_VERSION_STRING " manager started on ")(f_server_name);

        if( f_opt.is_defined( "--" ) )
        {
            std::string const command( f_opt.get_string("--") );

            // make sure we accept this command
            //
            if(command == "start")
            {
                f_command = command_t::COMMAND_START;
            }
            else if(command == "stop")
            {
                f_command = command_t::COMMAND_STOP;

                // `snapinit --detach stop` is not supported, --detach is ignore then
                //
                if( f_opt.is_defined("detach") )
                {
                    SNAP_LOG_WARNING("The --detach option is ignored with the 'stop' command.");
                }
            }
            else if(command == "restart")
            {
                f_command = command_t::COMMAND_RESTART;
            }
            else
            {
                SNAP_LOG_FATAL("Unknown command \"")(command)("\".");
                usage();
                snap::NOTREACHED();
            }
        }
        else
        {
            SNAP_LOG_FATAL("A command is required!");
            usage();
            snap::NOTREACHED();
        }
    }

    // user can change were the "cron" data managed by snapinit gets saved
    //
    if(f_config.contains("spool_path"))
    {
        f_spool_path = f_config["spool_path"];
    }

    // make sure we can load the XML file with the various service
    // definitions
    //
    {
        QString const xml_services_path(f_config.contains("xml_services")
                                        ? f_config["xml_services"]
                                        : "/etc/snapwebsites/snapinit.d");
        if(xml_services_path.isEmpty())
        {
            // the XML services are mandatory (it cannot be set to an empty string)
            common::fatal_error("the xml_services parameter cannot be empty, it has to be a path to the services XML files.");
            snap::NOTREACHED();
        }

        QString const pattern(QString("%1/service-*.xml").arg(xml_services_path));
        glob_t dir = glob_t();
        int const r(glob(
                      pattern.toUtf8().data()
                    , GLOB_NOESCAPE
                    , glob_error_callback
                    , &dir));
        std::shared_ptr<glob_t> ai(&dir, glob_deleter);

        if(r != 0)
        {
            // do nothing when errors occur
            //
            switch(r)
            {
            case GLOB_NOSPACE:
                common::fatal_error("glob() did not have enough memory to alllocate its buffers.");
                break;

            case GLOB_ABORTED:
                common::fatal_error("glob() was aborted after a read error.");
                break;

            case GLOB_NOMATCH:
                common::fatal_error("glob() could not find any status information.");
                break;

            default:
                common::fatal_error(QString("unknown glob() error code: %1.").arg(r));
                break;

            }
            snap::NOTREACHED();
        }

        std::vector<QString> common_options;

        // create a service representing ourselves
        //
        f_snapinit_service = std::make_shared<service>(shared_from_this());
        f_snapinit_service->configure_as_snapinit();
        if(f_debug)
        {
            common_options.push_back("--debug");
        }
        common_options.push_back("--server-name");
        common_options.push_back(f_server_name);
        f_communicator->add_connection( f_snapinit_service );
        f_service_list.push_back( f_snapinit_service );

        // load each service file
        //
        for(size_t idx(0); idx < dir.gl_pathc; ++idx)
        {
            QString const xml_service_filename(QString::fromUtf8(dir.gl_pathv[idx]));

            QFile xml_service_file(xml_service_filename);
            if(!xml_service_file.open(QIODevice::ReadOnly))
            {
                // the XML services is a mandatory file we need to be able to read
                int const e(errno);
                common::fatal_error(QString("the XML file \"%1\" could not be opened (%2).")
                                .arg(xml_service_filename)
                                .arg(strerror(e)));
                snap::NOTREACHED();
            }

            {
                QString error_message;
                int error_line;
                int error_column;
                QDomDocument doc;
                if(!doc.setContent(&xml_service_file, false, &error_message, &error_line, &error_column))
                {
                    // the XML is probably not valid, setContent() returned false...
                    // (it could also be that the file could not be read and we
                    // got some I/O error.)
                    //
                    common::fatal_error(QString("the XML file \"%1\" could not be parse as valid XML (%2:%3: %4; on column: %5).")
                                .arg(xml_service_filename)
                                .arg(xml_service_filename)
                                .arg(error_line)
                                .arg(error_message)
                                .arg(error_column));
                    snap::NOTREACHED();
                }
                xml_to_service(doc, xml_service_filename, common_options);
            }
        }

        // In the end, we MUST have this service specified in the XML file,
        // otherwise fail!
        //
        if( !f_snapcommunicator_service )
        {
            common::fatal_error("You must have a snapcommunicator service specified in the XML file!");
            snap::NOTREACHED();
        }

        // finish the initialization of the services now that we loaded them
        // all (i.e. we cannot calculate the pre-requirements without having
        // the complete list of services.)
        //
        std::for_each(
                std::begin(f_service_list),
                std::end(f_service_list),
                [&common_options](service::pointer_t const svc)
                {
                    if(svc)
                    {
                        svc->finish_configuration(common_options);
                    }
                });

        // sort those services by priority
        //
        // unfortunately, the following would sort items by pointer were
        // we to not specifying our own sort function
        //
        std::sort(
                std::begin(f_service_list),
                std::end(f_service_list),
                [](service::pointer_t const a, service::pointer_t const b)
                {
                    if(!a || !b)
                    {
                        return false;
                    }
                    return *a < *b;
                });

        // sanity check, we MUST have snapcommunicator, snapinit, then other
        // services, if another order is used, it is likely to not work
        // quite right...
        //
        if(f_service_list.size() < 2
        || !f_service_list[0] || f_service_list[0]->get_service_name() != "snapcommunicator"
        || !f_service_list[1] || f_service_list[1]->get_service_name() != "snapinit")
        {
            common::fatal_error(QString("the system cannot run with at least snapcommunicator and snapinit, defined in that order."));
            snap::NOTREACHED();
        }
    }

    // retrieve the direct listen information for the UDP port
    // on which we listen as a fallback in case snapcommunicator
    // is not available
    //
    {
        QString direct_listen;
        if(f_config.contains("direct_listen"))
        {
            // use .conf definition when available
            direct_listen = f_config["direct_listen"];
        }
        f_udp_addr = "127.0.0.1";
        f_udp_port = 4039;
        tcp_client_server::get_addr_port(direct_listen, f_udp_addr, f_udp_port, "udp");
    }

    if(f_config.contains("stop_max_wait"))
    {
        bool ok(false);
        f_stop_max_wait = f_config["stop_max_wait"].toInt(&ok, 10);
        if(!ok)
        {
            common::fatal_error(QString("the stop_max_wait parameter must be a number of seconds, \"%1\" is not valid.")
                                .arg(f_config["stop_max_wait"]));
            snap::NOTREACHED();
        }
        if(f_stop_max_wait < 10)
        {
            common::fatal_error(QString("the stop_max_wait parameter must be at least 10 seconds, \"%1\" is too small. The default value is 60.")
                                .arg(f_config["stop_max_wait"]));
            snap::NOTREACHED();
        }
    }

    if(f_command == command_t::COMMAND_LIST)
    {
        // TODO: add support for --verbose and print much more than just
        //       the service name
        //
        std::cout << "List of services, sorted by priority, to start on this server:" << std::endl;
        auto output_service_name = []( auto const & svc )
        {
            if(svc)
            {
                std::cout << svc->get_service_name();
                if(svc->is_cron_task())
                {
                    std::cout << " [CRON]";
                }
                if(svc->is_disabled())
                {
                    std::cout << " (disabled)";
                }
                std::cout << std::endl;
            }
        };
        std::for_each( std::begin(f_service_list), std::end(f_service_list), output_service_name );
        // the --list command is over!
        exit(1);
        snap::NOTREACHED();
    }

    if(f_command == command_t::COMMAND_TREE)
    {
        // TODO: add support for --verbose and print much more than just
        //       the service name
        //
        create_service_tree();
        // the --tree command is over!
        exit(1);
        snap::NOTREACHED();
    }

    // if not --list we still write the list of services but in log file only
    log_selected_servers();

    QString const user ( f_config.contains("user")  ? f_config["user"]  : "snapwebsites" );
    QString const group( f_config.contains("group") ? f_config["group"] : "snapwebsites" );

    // make sure the path to the lock file exists
    //
    {
        QString const lock_path(QString::fromUtf8(f_opt.get_string("lockdir").c_str()));
        if(snap::mkdir_p(lock_path, false) != 0)
        {
            common::fatal_error(QString("the path to the lock filename could not be created (mkdir -p \"%1\").")
                                .arg(lock_path));
            snap::NOTREACHED();
        }

        // for sub-processes to be able to access that folder we need to
        // also setup the user and group as expected
        //
        snap::chownnm(lock_path, user, group);
    }

    // create the run-time directory because other processes may not
    // otherwise have enough permissions (i.e. not be root as possibly
    // required for this task)
    //
    // however, if we are not root ourselves, then we probably are
    // running as the developer and that means we cannot actually
    // do that (either the programmer does it manually on each reboot
    // or he changes the path to a different place...)
    //
    if(getuid() == 0)
    {
        // user can change the path in snapinit.conf (although it does not
        // get passed down at this point... so each tool has to be properly
        // adjusted if modified here.)
        //
        QString const runpath(f_config.contains("runpath") ? f_config["runpath"] : "/run/snapwebsites");
        if(snap::mkdir_p(runpath, false) != 0)
        {
            common::fatal_error(QString("the path to runtime data could not be created (mkdir -p \"%1\").")
                                .arg(runpath));
            snap::NOTREACHED();
        }

        // for sub-processes to be able to access that folder we need to
        // also setup the user and group as expected
        //
        snap::chownnm(runpath, user, group);
    }

    // Stop on these signals, log them, then terminate.
    //
    // Note: the handler may access the snap_init instance
    //
    signal( SIGSEGV, sighandler );
    signal( SIGBUS,  sighandler );
    signal( SIGFPE,  sighandler );
    signal( SIGILL,  sighandler );
    //signal( SIGTERM, sighandler ); -- we capture those with connections
    //signal( SIGINT,  sighandler );
    //signal( SIGQUIT, sighandler );
}


void snap_init::create_service_tree()
{
    // create the snapinit.dot file
    std::ofstream dot_file;
    dot_file.open("snapinit.dot");
    if(!dot_file.is_open())
    {
        int const e(errno);
        std::cerr << "error: could not create snapinit.dot file. (errno: " << e << ", " << strerror(e) << ")." << std::endl;
        return;
    }

    dot_file << "// auto-generated snapinit.dot file -- by `snapinit --tree`" << std::endl;
    dot_file << "strict digraph {" << std::endl
             << "rankdir=BT;" << std::endl
             << "label=\"snapinit service dependency graph\";" << std::endl;

    int node_count(0);
    std::for_each(
            std::begin(f_service_list),
            std::end(f_service_list),
            [&](auto const & svc)
            {
                if(svc)
                {
                    svc->set_service_index(node_count);
                    std::string color("#000000");
                    if(svc->is_disabled())
                    {
                        color = "#666666";
                    }
                    else if(svc->is_paused())
                    {
                        color = "#ff0000";
                    }
                    else if(svc->is_registered())
                    {
                        color = "#008800";
                    }
                    dot_file << "n" << node_count
                             << " [label=\"" << svc->get_service_name()
                             << "\",color=\"" << color
                             << "\",fontcolor=\"" << color
                             << "\",shape=box];" << std::endl;
                    ++node_count;
                }
            });

    // edges font size to small
    dot_file << "edge [fontsize=8,fontcolor=\"#990033\"];" << std::endl;

    std::for_each(
            std::begin(f_service_list),
            std::end(f_service_list),
            [&](auto const & svc)
            {
                if(svc)
                {
                    int const service_index(svc->get_service_index());
                    service::weak_vector_t const & depends(svc->get_depends_list());
                    for(auto d : depends)
                    {
                        auto const & dep(d.lock());
                        if(dep)
                        {
                            if(svc->is_weak_dependency(dep->get_service_name()))
                            {
                                dot_file << "edge [style=dashed,color=\"#888888\"];" << std::endl;
                            }
                            else
                            {
                                dot_file << "edge [style=solid,color=\"#000000\"];" << std::endl;
                            }
                            dot_file << "n" << service_index
                                     << " -> n" << dep->get_service_index()
                                     << ";" << std::endl;
                        }
                    }
                }
            });

    dot_file << "}" << std::endl;
    dot_file.close();

    // create the final output
    //
    snap::NOTUSED(system("dot -Tsvg snapinit.dot >snapinit-graph.svg"));
}


/** \brief Exiting requires the removal of the lock.
 *
 * This function stops snapinit with an exit() call. The problem with
 * a direct exit() we do not get the destructor called and thus that
 * means the lock file does not get deleted.
 *
 * We over load the exit() command so that way we can make sure that
 * at least the lock gets destroyed.
 *
 * \param[in] code  The exit code, to be returned to the parent process.
 */
void snap_init::exit(int code) const
{
    remove_lock();

    ::exit(code);
}


void snap_init::create_instance( int argc, char * argv[] )
{
    f_instance.reset( new snap_init( argc, argv ) );
    if(!f_instance)
    {
        // new should throw std::bad_alloc or some other error anyway
        throw std::runtime_error("snapinit failed to create an instance of a snap_init object");
    }
    f_instance->init();
}


snap_init::pointer_t snap_init::instance()
{
    if( !f_instance )
    {
        throw std::invalid_argument( "snapinit instance must be created with create_instance()!" );
    }
    return f_instance;
}


void snap_init::xml_to_service(QDomDocument doc, QString const & xml_services_filename, std::vector<QString> & common_options)
{
    // make sure the root element is valid and not disabled
    //
    QDomElement e(doc.documentElement());
    if(e.isNull())      // it should always be an element
    {
        return;
    }

    // if user wants to see a list of services, then we want to show them
    // all, whether they are disabled or not
    //
    // otherwise, just skip (TODO: although if we want to ever support a
    // runtime reload, this is not a good solution!)
    //
    if((f_command != command_t::COMMAND_LIST && f_command != command_t::COMMAND_TREE)
    && e.attributes().contains("disabled"))
    {
        return;
    }

    // create the service object and have it parse the XML data
    //
    // Note: not found processes generate a warning instead of an error
    //       when the command is not --list, --tree, or --stop
    //
    service::pointer_t s(std::make_shared<service>(shared_from_this()));
    QString const binary_path( QString::fromUtf8(f_opt.get_string("binary-path").c_str()) );
    s->configure(
            e,
            binary_path,
            common_options,
            f_command == command_t::COMMAND_LIST
                || f_command == command_t::COMMAND_TREE
                || f_command == command_t::COMMAND_STOP
        );

    // avoid two services with the exact same name, we do not support such
    //
    QString const new_service_name(s->get_service_name());
    if(f_service_list.end() != std::find_if(
            f_service_list.begin(),
            f_service_list.end(),
            [new_service_name](auto const & svc)
            {
                if(svc)
                {
                    return svc->get_service_name() == new_service_name;
                }
                return false;
            }))
    {
        common::fatal_error(QString("snapinit cannot start the same service more than once on \"%1\". It found \"%2\" twice in \"%3\".")
                      .arg(f_server_name)
                      .arg(s->get_service_name())
                      .arg(xml_services_filename));
        snap::NOTREACHED();
    }

    if(s->is_snapcommunicator())
    {
        // we currently only support one snapcommunicator connection
        // mechanism, snapinit does not know anything about connecting
        // with any other service; so if we find more than one connection
        // service, we fail early
        //
        if(f_snapcommunicator_service)
        {
            common::fatal_error(QString("snapinit only supports one connection service at this time on \"%1\". It found two: \"%2\" and \"%3\" in \"%4\".")
                          .arg(f_server_name)
                          .arg(s->get_service_name())
                          .arg(f_snapcommunicator_service->get_service_name())
                          .arg(xml_services_filename));
            snap::NOTREACHED();
        }
        //
        f_snapcommunicator_service = s;
    }

    // make sure to add all services as timer connections
    // to the communicator so we can wake a service on its
    // own (especially to support the <recovery> feature.)
    //
    f_communicator->add_connection( s );

    f_service_list.push_back( s );
}


/** \brief Start a process depending on the command line command.
 *
 * This function is called once the snap_init object was initialized.
 * The function calls the corresponding function.
 *
 * At this time only three commands are supported:
 *
 * \li start
 * \li stop
 * \li restart
 *
 * The restart first calls stop() if snapinit is still running.
 * Then it calls start().
 */
void snap_init::run_processes()
{
    if( f_command == command_t::COMMAND_START )
    {
        start();
    }
    else if( f_command == command_t::COMMAND_STOP )
    {
        stop();
    }
    else if( f_command == command_t::COMMAND_RESTART )
    {
        restart();
    }
    else
    {
        SNAP_LOG_ERROR("Command '")(f_opt.get_string("--"))("' not recognized!");
        usage();
        snap::NOTREACHED();
    }
}


/** \brief Process a message.
 *
 * Once started, snapinit accepts messages on a UDP port. This is offered so
 * one can avoid starting snapcommunicator. Only the STOP command should be
 * sent through the UDP port.
 *
 * When snapcommunicator is a service that snapinit is expected to start
 * (it should be in almost all cases), then this function is also called
 * as soon as the snapcommunicator system is in place.
 *
 * \param[in] message  The message to be handled.
 * \param[in] udp  Whether the message was received via the UDP listener.
 */
void snap_init::process_message(snap::snap_communicator_message const & message, bool udp)
{
    SNAP_LOG_TRACE("received message [")(message.to_message())("]");

    QString const command(message.get_command());

    // UDP messages that we accept are very limited...
    // (especially since we cannot send a reply)
    //
    if( udp )
    {
        auto const & udp_command( f_udp_message_map.find(command) );
        if( udp_command == f_udp_message_map.end() )
        {
            SNAP_LOG_ERROR("command \"")(command)("\" is not supported on the UDP connection.");
            return;
        }

        // Execute the command and exit
        //
        (udp_command->second)(message);
        return;
    }

    auto const & tcp_command( f_tcp_message_map.find(command) );
    if( tcp_command == f_tcp_message_map.end() )
    {
        // unknown command is reported and process goes on
        //
        SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received on the TCP connection.");
        snap::snap_communicator_message reply;
        reply.set_command("UNKNOWN");
        reply.add_parameter("command", command);
        f_listener_connection->send_message(reply);
        return;
    }

    // Execute the command
    //
    (tcp_command->second)(message);
}


/** \brief This callback gets called on a SIGCHLD signal.
 *
 * Whenever a child dies, we receive a SIGCHLD. The snapcommunicator
 * library knows how to handle those signals and ends up calling this
 * function when one happens. Only, at this point the snapcommunicator
 * does not tell us which child died. So we quickly look through our
 * list (in comparison to having a timer and poll the list once a
 * second, this is still way faster since 99.9% of the time our
 * processes do not just die!)
 *
 * In most cases, this process will restart the service. Only if the
 * service was restarted many times in a very short period of time
 * it may actually be removed from the list instead or put to sleep
 * for a while ("put to sleep" means not restarted at all...)
 *
 * \warning
 * This function will call itself if it detects that a process dies
 * and it has to terminate snapinit itself.
 */
void snap_init::service_died()
{
    SNAP_LOG_TRACE("snap_init::service_died()");

    // this loop takes care of all the children that just sent us a SIGCHLD
    //
    // IMPORTANT NOTE: although the pid is a process resource and we
    //                 could think that it would be better/cleaner
    //                 to call a 'did_process_died()' function, it
    //                 would then mean we have to check ALL processes;
    //                 so with 12 or so daemons, you'd call waitpid()
    //                 12 times; this current loop calls waitpid()
    //                 once per dead process + 1 only (so most often
    //                 2 times); it can also become difficult to
    //                 interpret the return type of a function such
    //                 as 'did_process_died()' as it is likely to
    //                 change over time to incorporate more things
    //                 that have nothing to do with SIGCHLD...
    //
    for(;;)
    {
        int status;
        pid_t const died_pid(waitpid(-1, &status, WNOHANG));
        if(died_pid == 0)
        {
            // all children that died were checked, we are done
            //
            break;
        }

        // waitpid() returned an error
        //
        if( died_pid == -1 )
        {
            // when using waitpid(-1, ...) we get here and not in the
            // case where waitpid() returns zero!
            //
            if(errno == ECHILD)
            {
                break;
            }

            // we probably do not want to continue on errors
            // we may even need to call fatal_error() instead
            //
            int const e(errno);
            SNAP_LOG_ERROR("waitpid() returned an error (")(strerror(e))(").");

            // should we continue to waitpid()? I'm not too sure what that
            // would give us outside of an infinite loop
            //
            break;
        }

        // we found a child, search for it
        //
        auto const dead_service_iter(std::find_if(
                f_service_list.begin(),
                f_service_list.end(),
                [died_pid](auto const svc)
                {
                    if(svc)
                    {
                        return svc->get_process().get_pid() == died_pid;
                    }
                    return false;
                }));
        bool const found(dead_service_iter != f_service_list.end()
                      && *dead_service_iter);

        QString service_name(found ? (*dead_service_iter)->get_service_name() : "unknown_service");
        if(!found)
        {
            SNAP_LOG_FATAL("waitpid() returned unknown PID ")(died_pid);
        }

        termination_t termination(termination_t::TERMINATION_ABORT);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if(WIFEXITED(status))
        {
            int const exit_code(WEXITSTATUS(status));

            if( exit_code == 0 )
            {
                // when this happens there is not really anything to tell about
                SNAP_LOG_DEBUG("Service \"")(service_name)("\" terminated normally.");
                termination = termination_t::TERMINATION_NORMAL;
            }
            else
            {
                SNAP_LOG_INFO("Service \"")(service_name)("\" terminated normally, but with exit code ")(exit_code);
                termination = termination_t::TERMINATION_ERROR;
            }
        }
        else if(WIFSIGNALED(status))
        {
            int const signal_code(WTERMSIG(status));
            bool const has_code_dump(!!WCOREDUMP(status));

            SNAP_LOG_ERROR("Service \"")
                          (service_name)
                          ("\" terminated because of OS signal \"")
                          (strsignal(signal_code))
                          ("\" (")
                          (signal_code)
                          (")")
                          (has_code_dump ? " and a core dump was generated" : "")
                          (".");
        }
        else
        {
            // I do not think we can reach here...
            //
            SNAP_LOG_ERROR("Service \"")(service_name)("\" terminated abnormally in an unknown way.");
        }
#pragma GCC diagnostic pop

        if(dead_service_iter != f_service_list.end()
        && *dead_service_iter)
        {
            // call this after we generated the error output so the logs
            // appear in a sensible order
            //
            (*dead_service_iter)->get_process().action_died(termination);
        }
        else
        {
            // making this a fatal issue, frankly there is no way we could
            // lose the child before we tell it to get lost!
            //
            common::fatal_error("snapinit received the PID from an unknown process.");
            snap::NOTREACHED();
        }
    }
}


/** \brief Remove a service from the list of services.
 *
 * This function searches for the specified service and removes it from
 * the list of services managed by snapinit.
 *
 * snapinit also has copies of a few other services that it keeps around
 * to help with calling various functions. These are resetted alongside.
 * (i.e. since a find_if() of that service would return std::end() and
 * we need to match that behavior either way.)
 *
 * A service can be removed once. After that it is gone for good.
 *
 * \param[in] service  The service being removed.
 */
void snap_init::remove_service(service::pointer_t service)
{
    SNAP_LOG_TRACE("request to remove service \"")
                  (service->get_service_name())
                  ("\".");

    // remove the service from our main list
    //
    snap::NOTUSED(std::find_if(
            f_service_list.begin(),
            f_service_list.end(),
            [&service](auto & svc)
            {
                if(svc == service)
                {
                    svc.reset();
                    return true;
                }
                return false;
            }));

    // the service is also a timer that we need to remove from
    // the snapcommunicator list
    //
    f_communicator->remove_connection(service);

    // connection service gone?
    //
    if(service == f_snapcommunicator_service)
    {
        f_snapcommunicator_service.reset();

        // if the snapcommunicator is no more so is its connection
        //
        if(f_listener_connection)
        {
            f_communicator->remove_connection(f_listener_connection);
            f_listener_connection.reset();
        }
    }
    else if(service == f_snapinit_service)
    {
        f_snapinit_service.reset();
    }

    // the list does not get empty because we cannot remove pointers
    // (we have recursive loops and that would crash with SEGV or such)
    //
    if( f_service_list.end() == std::find_if(
                                    f_service_list.begin(),
                                    f_service_list.end(),
                                    [](auto const & svc)
                                    {
                                        return !!svc;
                                    }))
    {
        SNAP_LOG_TRACE("snap_init::remove_service(): service list empty!");

        // no more services, also remove our other connections so
        // we exit the snapcommunicator loop
        //
        f_communicator->remove_connection(f_ping_server);
        f_communicator->remove_connection(f_child_signal);
        f_communicator->remove_connection(f_term_signal);
        f_communicator->remove_connection(f_quit_signal);
        f_communicator->remove_connection(f_int_signal);

        if(f_listener_connection)
        {
            f_communicator->remove_connection(f_listener_connection);
            f_listener_connection.reset();

            SNAP_LOG_FATAL("f_listener_connection was not properly removed when the f_connection_service was removed!");
        }
    }
#if 0
    else
    {
        SNAP_LOG_TRACE("**** snap_init::remove_service(): service list NOT empty:");
        for( auto const & svc : f_service_list )
        {
            if(svc)
            {
                SNAP_LOG_TRACE("******* service '")(svc->get_service_name())("' is still in the list!");
            }
        }
    }
#endif
}




/** \brief Process a user termination signal.
 *
 * This funtion is called whenever the user presses Ctlr-C, Ctrl-?, or Ctrl-\
 * on their keyboard (SIGINT, SIGTERM, or SIGQUIT). This function makes sure
 * to stop the process cleanly in this case by calling the
 * terminate_services() function.
 *
 * \param[in] sig_name  The name of the Unix signal that generated this call.
 */
void snap_init::user_signal_caught(char const * sig_name)
{
    std::stringstream ss;
    ss << "User signal caught: " << sig_name;
    SNAP_LOG_INFO(ss.str());
    if(common::is_a_tty())
    {
        std::cerr << "snapinit: " << ss.str() << std::endl;
    }

    // by calling this function, snapinit will quit once all the
    // services stopped
    //
    terminate_services();
}


/** \brief Check whether snapinit is running (has a lock file in place.)
 *
 * The snapinit process creates a lock file on the 'start' command.
 * If that lock file exists, then it is viewed as locked and that
 * snapinit is already running. This prevents you from starting
 * multiple instances of the snapinit server. It is still possible
 * to start snapinit with other commands, especially the 'stop'
 * and 'restart' commands, but also the --version and --list
 * command line options work just fine even when the lock is in
 * place.
 *
 * \return true if the snapinit process lock file exists.
 */
bool snap_init::is_running() const
{
    return f_lock_file.exists();
}


/** \brief Retrieve the path to the spool directory.
 *
 * The spool directory is used by the anacron tool and we do the
 * same thing. We save the time in seconds when we last ran a
 * CRON process in a file under that directory.
 *
 * This function makes sure that the spool directory exists
 * the first time it is called. After that, it is assumed
 * that the path never changes so it does not try to recreate
 * the path.
 *
 * \return The path to the spool file.
 */
QString const & snap_init::get_spool_path() const
{
    if(!f_spool_directory_created)
    {
        f_spool_directory_created = true;

        // make sure that the directory exists
        //
        if(snap::mkdir_p(f_spool_path, false) != 0)
        {
            common::fatal_error(QString("snapinit could not create directory \"%1\" to save spool data.")
                                .arg(f_spool_path));
            snap::NOTREACHED();
        }
    }

    return f_spool_path;
}


/** \brief Retrieve the name of the server.
 *
 * This parameter returns the value of the server_name=... parameter
 * defined in the snapinit configuration file or the hostname if
 * the server_name=... parameter was not defined.
 *
 * \return The name of the server this snapinit instance is running on.
 */
QString const & snap_init::get_server_name() const
{
    return f_server_name;
}


/** \brief Check whether we were started in debug mode.
 *
 * \return true if the --debug command line optionw as used.
 */
bool snap_init::get_debug() const
{
    return f_debug;
}


/** \brief Retrieve a copy of the data path.
 *
 * This function returns the path to the snapinit home directory.
 *
 * \return The data_path parameter as defined in the snapinit.conf or the
 *         default which is "/var/lib/snapwebsites".
 */
QString const & snap_init::get_data_path() const
{
    return f_data_path;
}


/** \brief Retrieve the service used to inter-connect services.
 *
 * This function returns the information about the server that is
 * used to inter-connect services together. This should be the
 * snapcommunicator service.
 *
 * \exception std::logic_error
 * The function raises that exception if it gets called too soon
 * (i.e. before a connection service is found in the XML file.)
 *
 * \return A smart pointer to the snapcommunicator service.
 */
service::pointer_t snap_init::get_snapcommunicator_service() const
{
    if(!f_snapcommunicator_service)
    {
        throw std::logic_error("snapcommunicator service requested before it was defined or after it was dropped.");
    }

    return f_snapcommunicator_service;
}


/** \brief Send a message to snapcommunicator
 *
 * This function sends a message to the snapcommunicator service.
 *
 * \note
 * If snapcommunicator is not yet connected or the connection was
 * lost, the message will be stacked and sent as soon as the
 * snapcommunicator comes back.
 *
 * \param[in] message  The message to send.
 */
void snap_init::send_message(snap::snap_communicator_message const & message)
{
    if(f_listener_connection)
    {
        f_listener_connection->send_message(message);
    }
}


/** \brief List the servers we are starting to the log.
 *
 * This function prints out the list of services that this instance
 * of snapinit is managing.
 *
 * The list may be shorten as time goes if some services die too
 * many times. This gives you an exact list on startup.
 *
 * Note that services marked as disabled in the snapinit.xml file
 * are not loaded at all so they will not make it to the log from
 * this function.
 */
void snap_init::log_selected_servers() const
{
    std::stringstream ss;
    ss << "Enabled servers:";

    auto log_service_name = [&ss](auto const & svc)
    {
        if(svc)
        {
            ss << " [" << svc->get_service_name() << "]";
        }
    };

    std::for_each( std::begin(f_service_list), std::end(f_service_list), log_service_name );

    SNAP_LOG_INFO(ss.str());
}



/** \brief Find who depends on the named service.
 *
 * \note
 * This function is not recursive. It only returns the immediate
 * pre-requirements and not the whole tree. This is generally enough
 * since the other functions using the pre-requirements are recursive
 * anyway.
 *
 * \param[in] service_name  The service for which pre-requirements are to be searched.
 * \param[out] ret_list  The list of pre-requirements for this service.
 *
 * \return list of services who depend on the named service
 */
void snap_init::get_prereqs_list( QString const & service_name, service::weak_vector_t & ret_list ) const
{
    // clear the output by default
    //
    ret_list.clear();

    // make sure the service exists
    //
    auto const the_service( get_service(service_name) );
    if( !the_service )
    {
        return;
    }

    // check whether each 'service' is a dependency of 'service_name'
    //
    std::for_each(
            f_service_list.begin(),
            f_service_list.end(),
            [&service_name, &ret_list](auto const & svc)
            {
                if(svc)
                {
                    //SNAP_LOG_TRACE( "snap_init::get_prereqs_list(): the_service='")(service_name)("', service='")(service->get_service_name());
                    if( svc->is_dependency_of( service_name ) )
                    {
                        //SNAP_LOG_TRACE("   snap_init::get_prereqs_list(): adding service '")(service->get_service_name());
                        ret_list.push_back(svc);
                    }
                }
            });
}


/** \brief Query a service by name.
 *
 * \param[in] service_name  The name of the service to be searched.
 *
 * \return The pointer to the service, may be a nullptr.
 */
service::pointer_t snap_init::get_service( QString const & service_name ) const
{
    auto get_service_name = [service_name]( auto const & svc )
    {
        if(svc)
        {
            return svc->get_service_name() == service_name;
        }
        return false;
    };
    //
    auto iter = std::find_if( f_service_list.begin(), f_service_list.end(), get_service_name );
    //
    if( iter == f_service_list.end() )
    {
        return service::pointer_t();
    }

    return (*iter);
}


/** \brief Ask all services to go down so snapinit can quit.
 *
 * In most cases, this function is called when the snapinit tool
 * receives the STOP signal. It simple requests all services
 * to quit as soon as possible by calling they action_stop()
 * function.
 *
 * The STOP process is described in the service.cpp file (at
 * the top, see the \@file definition). It involves sending
 * a STOP message (if possible) or a SIGTERM/SIGKILL.
 *
 * This is done by marking all the services as stopping and then
 * sending the STOP signal to the snapcommunicator.
 *
 * If all the services were already stopped, then the function
 * does not send a STOP (since snapcommunicator would not even
 * be running.)
 *
 * \warning
 * This function does NOT block. Instead it sends messages and
 * then returns.
 *
 * \bug
 * At this time we have no clue whether the service is already
 * connected to the snapcommunicator or not. Although we have
 * a SIGTERM + SIGKILL fallback anyway, in reality we end up
 * having an ugly termination if the service was not yet
 * connected at the top we send the STOP signal. That being
 * said, if that happens, it is not unlikely that the process
 * was not doing much yet. On the other hand, I prefer correctness
 * and I think that accepting the snapcommunicator STATUS signal
 * would give us a way to know where we are and send the SIGTERM
 * immediately preventing the child process from starting a real
 * task (because until connected to the snapcommunicator it
 * should not be any important work.) Also all children could
 * have the SIGTERM properly handle a quit.
 */
void snap_init::terminate_services()
{
    if(f_snapinit_state != snapinit_state_t::SNAPINIT_STATE_STOPPING)
    {
        // change status to STOPPING
        //
        f_snapinit_state = snapinit_state_t::SNAPINIT_STATE_STOPPING;

        // call action_stop() on each service in reverse order
        //
        // We have to do it in reverse order in case some processes
        // are still or are already dead because they should be
        // removed immediately
        //
        std::for_each(
                f_service_list.rbegin(),
                f_service_list.rend(),
                [](auto const & svc)
                {
                    if(svc)
                    {
                        svc->action_stop();
                    }
                });
    }
}


/** \brief Start the snapinit services.
 *
 * This function starts the Snap! Websites services.
 *
 * If the --detach command line option was used, then the function
 * calls fork() to detach the process from the calling shell.
 */
void snap_init::start()
{
    // The following open() prevents race conditions
    //
    int const fd(::open( f_lock_file.fileName().toUtf8().data(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR ));
    if( fd == -1 )
    {
        int const e(errno);
        if(e == EEXIST)
        {
            int lock_file_pid(-1);
            {
                if(f_lock_file.open(QFile::ReadOnly))
                {
                    QByteArray const data(f_lock_file.readAll());
                    f_lock_file.close();
                    QString const pid_string(QString::fromUtf8(data).trimmed());
                    bool ok(false);
                    lock_file_pid = pid_string.toInt(&ok, 10);
                    if(!ok)
                    {
                        // just in case, make 100% sure that we have -1 as
                        // the PID when invalid
                        lock_file_pid = -1;
                    }
                }
            }

            if(lock_file_pid != -1)
            {
                if(getpgid(lock_file_pid) < 0)
                {
                    // although the lock file is in place, the PID defined in
                    // it does not exist, change the error message accordingly
                    //
                    // TODO: look into implementing a delete, but for that we
                    //       need to open the file locked, otherwise we may
                    //       have a race condition!
                    //       (see SNAP-133 which is closed)
                    //
                    common::fatal_error(QString("Lock file \"%1\" exists! However, process with PID %2 is not running. To delete the lock, use `snapinit --remove-lock`.")
                                  .arg(f_lock_filename)
                                  .arg(lock_file_pid));
                }
                else
                {
                    // snapinit is running
                    //
                    common::fatal_error(QString("Lock file \"%1\" exists! snapinit is already running as PID %2.")
                                  .arg(f_lock_filename)
                                  .arg(lock_file_pid));
                }
            }
            else
            {
                // snapinit is running
                //
                common::fatal_error(QString("Lock file \"%1\" exists! Is this a race condition? (errno: %2 -- %3)")
                              .arg(f_lock_filename)
                              .arg(e)
                              .arg(strerror(e)));
            }
        }
        else
        {
            common::fatal_error(QString("Lock file \"%1\" could not be created. (errno: %2 -- %3)")
                            .arg(f_lock_filename)
                            .arg(e)
                            .arg(strerror(e)));
        }
        snap::NOTREACHED();
    }

    // save fd in the QFile object
    //
    // WARNING: this call removes the filename from the QFile
    //          hence, we generally use the f_lock_filename instead of
    //          the f_lock_file.fileName() function
    //
    if(!f_lock_file.open( fd, QFile::ReadWrite ))
    {
        common::fatal_error(QString("Lock file \"%1\" could not be registered with Qt.")
                            .arg(f_lock_filename));
        snap::NOTREACHED();
    }

    if( f_opt.is_defined("detach") )
    {
        // fork(), then stay resident
        // Listen for STOP command on UDP port.
        //
        int const pid(fork());
        if( pid != 0 )
        {
            // the parent
            //
            if( pid < 0 )
            {
                // the child did not actually start
                //
                int const e(errno);
                common::fatal_error(QString("fork() failed, snapinit could not detach itself. (errno: %1).")
                                .arg(strerror(e)));
                snap::NOTREACHED();
            }

            // save our (child) PID in the lock file (useful for the stop() processus)
            // the correct Debian format is the PID followed by '\n'
            //
            // WARNING: This is done by the parent because at the time the parent
            //          returns the systemctl environment expects the PID to be
            //          valid (otherwise we get a "Failed to read PID from file ...".
            //
            // FHS Version 2.1+:
            //   > The file should consist of the process identifier in ASCII-encoded
            //   > decimal, followed by a newline character. For example, if crond was
            //   > process number 25, /var/run/crond.pid would contain three characters:
            //   > two, five, and newline.
            //
            f_lock_file.write(QString("%1\n").arg(pid).toUtf8());
            f_lock_file.flush();

            // in this case we MUST keep the lock in place,
            // which is done by closing that file; if the file
            // is closed whenever we hit the remove_lock()
            // function, then the file does not get deleted
            //
            f_lock_file.close();
            return;
        }

        // the child goes on
    }
    else
    {
        // if not detaching, we have to save the PID ourselves
        // (for more see details, see the previous write() comment)
        //
        f_lock_file.write(QString("%1\n").arg(getpid()).toUtf8());
        f_lock_file.flush();
    }

    // now we are ready to mark all the services as ready so they get
    // started (by default they are in the DISABLED state)
    //
    std::for_each(
            std::begin(f_service_list),
            std::end(f_service_list),
            [](auto const & svc)
            {
                if(svc)
                {
                    svc->action_ready();
                }
            });

    // this is to connect to the snapcommunicator
    //
    // here we make use of a permanent TCP connection so that way
    // we auto-reconnect whenever necessary without having to have
    // yet another state machine in the snapinit realm
    //
    {
        QString const host(f_snapcommunicator_service->get_snapcommunicator_addr());
        int const port(f_snapcommunicator_service->get_snapcommunicator_port());
        f_listener_connection = std::make_shared<listener_impl>(shared_from_this(), host.toUtf8().data(), port);
        f_listener_connection->set_name("snapinit listener");
        f_listener_connection->set_priority(0);
        f_communicator->add_connection(f_listener_connection);
    }

    // initialize a UDP server as a fallback in case you want to use
    // snapinit without a snapcommunicator server
    //
    {
        // just in case snapcommunicator does not get started, we still can
        // receive messages over a UDP port (mainly the STOP message)
        //
        f_ping_server = std::make_shared<ping_impl>(shared_from_this(), f_udp_addr.toUtf8().data(), f_udp_port);
        f_ping_server->set_name("snapinit UDP backup server");
        f_ping_server->set_priority(30);
        f_communicator->add_connection(f_ping_server);
    }

    // initialize the SIGCHLD signal
    //
    {
        f_child_signal = std::make_shared<sigchld_impl>(shared_from_this());
        f_child_signal->set_name("snapinit SIGCHLD signal");
        f_child_signal->set_priority(55);
        f_communicator->add_connection(f_child_signal);
    }

    // initialize the SIGTERM signal
    //
    {
        f_term_signal = std::make_shared<sigterm_impl>(shared_from_this());
        f_term_signal->set_name("snapinit SIGTERM signal");
        f_term_signal->set_priority(65);
        f_communicator->add_connection(f_term_signal);
    }

    // initialize the SIGQUIT signal
    //
    {
        f_quit_signal.reset(new sigquit_impl(shared_from_this()));
        f_quit_signal->set_name("snapinit SIGQUIT signal");
        f_quit_signal->set_priority(65);
        f_communicator->add_connection(f_quit_signal);
    }

    // initialize the SIGINT signal
    //
    {
        f_int_signal.reset(new sigint_impl(shared_from_this()));
        f_int_signal->set_name("snapinit SIGINT signal");
        f_int_signal->set_priority(65);
        f_communicator->add_connection(f_int_signal);
    }

    // run the event loop until we receive a STOP message
    //
    f_communicator->run();

    remove_lock();

    SNAP_LOG_INFO("Normal shutdown.");
}


/** \brief Attempts to restart Snap! Websites services.
 *
 * This function stops the existing snapinit instance and waits for it
 * to be done. If that succeeds, then it attempts to restart the
 * services immediately after that. The restart does not return
 * until itself stopped unless the detach option is used.
 */
void snap_init::restart()
{
    SNAP_LOG_INFO("Restart Snap! Websites services.");

    // call stop only if the server is running
    //
    if( is_running() )
    {
        stop();
    }

    // start and block unless "detach" is true
    //
    start();
}


/** \brief Run the 'stop' command of snapinit.
 *
 * This function runs the stop command, which attempts to stop the
 * existing / running snapinit process.
 *
 * If snapinit is not currently running, the function returns immediately
 * after logging and informational message about the feat.
 *
 * \return true if snapinit successfully stopped.
 */
void snap_init::stop()
{
    if( !is_running() )
    {
        // if not running, is this an error?
        //
        SNAP_LOG_INFO("'snapinit stop' called while snapinit is not running.");
        if( common::is_a_tty() )
        {
            std::cerr << "snapinit: info: 'snapinit stop' called while snapinit is not running."
                      << std::endl;
        }
        return;
    }

    // read the PID of the locking process so we can wait on its PID
    // and not just the lock (because in case it is restarted immediately
    // we would not see the lock file disappear...)
    //
    int lock_file_pid(-1);
    {
        if(f_lock_file.open(QFile::ReadOnly))
        {
            QByteArray const data(f_lock_file.readAll());
            f_lock_file.close();
            QString const pid_string(QString::fromUtf8(data).trimmed());
            bool ok(false);
            lock_file_pid = pid_string.toInt(&ok, 10);
            if(ok)
            {
                if(getpgid(lock_file_pid) < 0)
                {
                    common::fatal_error("'snapinit stop' called while snapinit is not running, although a lock file exists. Try snapinit --remove-lock.");
                    snap::NOTREACHED();
                }
            }
            else
            {
                // just in case, make 100% sure that we have -1 as the PID
                lock_file_pid = -1;
            }
        }
    }

    // if lock_file_pid is -1 then we consider that the snapinit instance
    // may have already removed that file (before we had the chance to
    // open it), so this is a valid case here.

    SNAP_LOG_INFO("Stop Snap! Websites services (pid = ")(lock_file_pid)(").");

    // TODO: check whether the snapcommunicator is running or not
    //       if not, we should look into sending the STOP message
    //       directly to snapinit instead of through the
    //       snapcommunicator

    QString udp_addr;
    int udp_port;
    get_addr_port_for_snap_communicator( udp_addr, udp_port );

    // send the UDP message now
    //
    snap::snap_communicator_message stop_message;
    stop_message.set_service("snapinit");
    stop_message.set_command("STOP");
    if(!snap::snap_communicator::snap_udp_server_message_connection::send_message(udp_addr.toUtf8().data(), udp_port, stop_message))
    {
        common::fatal_error("'snapinit stop' failed to send the STOP message to the running instance.");
        snap::NOTREACHED();
    }

    // wait for the processes to end and snapinit to delete the lock file
    //
    // if it takes too long, we will exit the loop and things will
    // eventually still be running...
    //
    for(int idx(0); idx < f_stop_max_wait; ++idx)
    {
        sleep(1);

        // the lock_file_pid should always be >= 0
        //
        if(lock_file_pid >= 0)
        {
            if(getpgid(lock_file_pid) < 0)
            {
                // errno == ESRCH -- the process does not exist anymore
                return;
            }
        }
        else
        {
            if( !f_lock_file.exists() )
            {
                // it worked!
                return;
            }
        }
    }

    // it failed...
    common::fatal_error(QString("snapinit waited for %1 seconds and the running version did not return.")
                    .arg(f_stop_max_wait));
    snap::NOTREACHED();
}


void snap_init::get_addr_port_for_snap_communicator( QString & udp_addr, int & udp_port )
{
    if( !f_snapcommunicator_service )
    {
        common::fatal_error("somehow the snapcommunicator service has not yet been initialized!");
        snap::NOTREACHED();
    }

    // we can send a UDP message to snapcommunicator, only we need
    // the address and port and those are defined in the
    // snapcommunicator settings
    //
    QString snapcommunicator_config_filename(f_snapcommunicator_service->get_process().get_config_filename());
    if(snapcommunicator_config_filename.isEmpty())
    {
        // in case it was not defined, use the default
        snapcommunicator_config_filename = "/etc/snapwebsites/snapcommunicator.conf";
    }
    snap::snap_config snapcommunicator_config;
    snapcommunicator_config.read_config_file( snapcommunicator_config_filename.toUtf8().data() );
    tcp_client_server::get_addr_port(snapcommunicator_config["signal"], udp_addr, udp_port, "udp");
}


/** \brief Print out the usage information for snapinit.
 *
 * This function returns the snapinit usage information to the user whenever
 * an invalid command line option is used or --help is used explicitly.
 *
 * The function does not return.
 */
void snap_init::usage()
{
    f_opt.usage( advgetopt::getopt::status_t::no_error, "snapinit" );
    snap::NOTREACHED();
}


/** \brief Remove the lock file.
 *
 * This function is called to remove the lock file so that way
 * a server can restart the snapinit tool on the next run.
 *
 * \todo
 * At this time this is not 100% RAII because we have many
 * fatal errors that call exit(1) directly.
 *
 * \param[in] force  Force the removal, whether the file was opened or not.
 */
void snap_init::remove_lock(bool force) const
{
    if( f_lock_file.isOpen() || force )
    {
        // We first have to close the handle, otherwise the remove does not work.
        //
        if( f_lock_file.isOpen() )
        {
            ::close( f_lock_file.handle() );

            // the Qt close() by itself does not work right, but
            // we want the QFile to be marked as closed
            //
            const_cast<QFile &>(f_lock_file).close();
        }

        QFile lock_file( f_lock_filename );
        lock_file.remove();
    }
}


/** \brief A static function to capture various signals.
 *
 * This function captures unwanted signals like SIGSEGV and SIGILL.
 *
 * The handler logs the information and then the service exists.
 * This is done mainly so we have a chance to debug problems even
 * when it crashes on a server.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void snap_init::sighandler( int sig )
{
    QString signame;
    switch( sig )
    {
    case SIGSEGV:
        signame = "SIGSEGV";
        break;

    case SIGBUS:
        signame = "SIGBUS";
        break;

    case SIGFPE:
        signame = "SIGFPE";
        break;

    case SIGILL:
        signame = "SIGILL";
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    snap::snap_exception_base::output_stack_trace();
    common::fatal_message(QString("Fatal signal caught: %1").arg(signame));

    // Make sure the lock file gets removed
    //
    snap_init::pointer_t si( snap_init::instance() );
    if(si)
    {
        si->remove_lock();
    }

    // Exit with error status
    //
    ::exit( 1 );
    snap::NOTREACHED();
}


} // snapinit namespace
// vim: ts=4 sw=4 et
