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
#include "service.h"

// our library
//
#include "chownnm.h"
#include "log.h"
#include "mkdir_p.h"
#include "not_used.h"

// c++ library
//
#include <algorithm>
#include <exception>
#include <sstream>

// c library
//
#include <fcntl.h>
#include <proc/sysinfo.h>
#include <syslog.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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
 * \li snapserver -- the actual snap server listening for incoming client
 *     connections (through Apache2 and snap.cgi for now)
 * \li snapbackend -- various backends to support working on slow tasks
 *     so front ends do not have to do those slow task and have the client
 *     wait for too long... (i.e. images, pagelist, sendmail, ...)
 * \li snapwatchdogserver -- a server which checks various things to
 *     determine the health of the server it is running on
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
 *    <snapservices>
 *      <!-- Snap Communicator is started as a service -->
 *      <service name="snapcommunicator" required="required">
 *        <!-- we give this one a very low priority as it has to be started
 *             before anything else -->
 *        <priority>-10</priority>
 *        <config>/etc/snapwebsites/snapcommunicator.conf</config>
 *        <connect>127.0.0.1:4040</connect>
 *        <wait>10</wait>
 *      </service>
 *      <service name="snapserver" required="required">
 *        <!-- the server should be started before the other backend
 *             services because the first time it initializes things
 *             that other backends need to run as expected; although
 *             some of that will change -->
 *        <priority>0</priority>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *      </service>
 *      <!-- various other services; these get a priority of 50 (default)
 *           and may use the same configuration file as the snapserver
 *           service -->
 *      <service name="sendmail">
 *        <command>/usr/bin/snapbackend</command>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *      </service>
 *      <!-- you may use the disabled flag to prevent starting this
 *           service. -->
 *      <service name="pagelist" disabled="disabled">
 *        <!-- commands are generally defined using a full path, while
 *             doing development, though, you may use just the name of
 *             the binary and use the --binary-path <path> on the
 *             command line -->
 *        <command>/usr/bin/snapbackend</command>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *      </service>
 *      <service name="images">
 *        <command>/usr/bin/snapbackend</command>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *      </service>
 *      <service name="snapwatchdog">
 *        <command>/usr/bin/snapwatchdogserver</command>
 *        <priority>90</priority>
 *        <config>/etc/snapwebsites/snapwatchdog.conf</config>
 *      </service>
 *      <service name="backend">
 *        <priority>75</priority>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *        <cron>300</cron>
 *      </service>
 *    </snapservices>
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
 * The following is an attempt at describing the process messages used
 * to start everything and stop everything:
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

namespace
{


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
        advgetopt::getopt::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "where -<opt> is one or more of:",
        advgetopt::getopt::help_argument
    },
    {
        'b',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "binary-path",
        "/usr/bin",
        "Path where snap! binaries can be found (e.g. snapserver and snapbackend).",
        advgetopt::getopt::optional_argument
    },
    {
        'c',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        "/etc/snapwebsites/snapinit.conf",
        "Configuration file to initialize snapinit.",
        advgetopt::getopt::optional_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "debug",
        nullptr,
        "Start the server and backend services in debug mode.",
        advgetopt::getopt::no_argument
    },
    {
        'd',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "detach",
        nullptr,
        "Background the snapinit server.",
        advgetopt::getopt::no_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show usage and exit.",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "list",
        nullptr,
        "Display the list of services and exit.",
        advgetopt::getopt::no_argument
    },
    {
        'k',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "lockdir",
        "/run/lock/snapwebsites",
        "Full path to the snapinit lockdir.",
        advgetopt::getopt::optional_argument
    },
    {
        'l',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "logfile",
        nullptr,
        "Full path to the snapinit logfile.",
        advgetopt::getopt::optional_argument
    },
    {
        'n',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "nolog",
        nullptr,
        "Only output to the console, not the log file.",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "remove-lock",
        nullptr,
        "For the removal of an existing lock (useful if a spurious lock still exists).",
        advgetopt::getopt::optional_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "running",
        nullptr,
        "test whether snapinit is running; exit with 0 if so, 1 otherwise.",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "show the version of the snapinit executable.",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::default_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::end_of_options
    }
};


}
//namespace




/////////////////////////////////////////////////
// SNAP INIT (class implementation)            //
/////////////////////////////////////////////////


snap_init::pointer_t snap_init::f_instance;


snap_init::snap_init( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapinit_options, g_configuration_files, "SNAPINIT_OPTIONS")
    , f_log_conf( "/etc/snapwebsites/snapinit.properties" )
    , f_lock_filename( QString("%1/snapinit-lock.pid")
                       .arg(f_opt.get_string("lockdir").c_str())
                     )
    , f_lock_file( f_lock_filename )
    , f_spool_path( "/var/spool/snap/snapinit" )
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
            common::fatal_error("server_name is not defined in your configuration file and hostname is not available as the server name,"
                            " snapinit not started. (in snapinit.cpp/snap_init::snap_init())");
            snap::NOTREACHED();
        }
        // TODO: add code to verify that we like that name (i.e. if the
        //       name includes periods we will reject it when sending
        //       messages to/from snapcommunicator)
        //
        f_server_name = host;
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

    // do not do too much in the constructor or we may get in
    // trouble (i.e. calling shared_from_this() from the
    // constructor fails)

    init_message_functions();
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
        // use a default command name
        //
        f_command = command_t::COMMAND_LIST;
    }
    else
    {
        SNAP_LOG_INFO("--------------------------------- snapinit manager started on ")(f_server_name);

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
    if(f_config.contains("spool_path"))
    {
        f_spool_path = f_config["spool_path"];
    }

    // make sure we can load the XML file with the various service
    // definitions
    //
    {
        QString const xml_services_filename(f_config.contains("xml_services")
                                        ? f_config["xml_services"]
                                        : "/etc/snapwebsites/snapinit.xml");
        if(xml_services_filename.isEmpty())
        {
            // the XML services is mandatory (it cannot be set to an empty string)
            common::fatal_error("the xml_services parameter cannot be empty, it has to be a path to the snapinit.xml file.");
            snap::NOTREACHED();
        }
        QFile xml_services_file(xml_services_filename);
        if(!xml_services_file.open(QIODevice::ReadOnly))
        {
            // the XML services is a mandatory file we need to be able to read
            int const e(errno);
            common::fatal_error(QString("the XML file \"%1\" could not be opened (%2).")
                            .arg(xml_services_filename)
                            .arg(strerror(e)));
            snap::NOTREACHED();
        }
        {
            QString error_message;
            int error_line;
            int error_column;
            QDomDocument doc;
            if(!doc.setContent(&xml_services_file, false, &error_message, &error_line, &error_column))
            {
                // the XML is probably not valid, setContent() returned false...
                // (it could also be that the file could not be read and we
                // got some I/O error.)
                //
                common::fatal_error(QString("the XML file \"%1\" could not be parse as valid XML (%2:%3: %4; on column: %5).")
                            .arg(xml_services_filename)
                            .arg(xml_services_filename)
                            .arg(error_line)
                            .arg(error_message)
                            .arg(error_column));
                snap::NOTREACHED();
            }
            xml_to_services(doc, xml_services_filename);
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
        std::cout << "List of services to start on this server:" << std::endl;
        for(auto const & s : f_service_list)
        {
            std::cout << s->get_service_name() << std::endl;
        }
        // the --list command is over!
        exit(1);
        snap::NOTREACHED();
    }

    // if not --list we still write the list of services but in log file only
    log_selected_servers();

    // make sure the path to the lock file exists
    //
    if(snap::mkdir_p(f_lock_filename, true) != 0)
    {
        common::fatal_error(QString("the path to the lock filename could not be created (mkdir -p \"%1\"; without the filename).")
                            .arg(f_lock_filename));
        snap::NOTREACHED();
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
        QString const runpath(f_config.contains("runpath") ? f_config["runpath"] : "/var/run/snapwebsites");
        if(snap::mkdir_p(runpath, false) != 0)
        {
            common::fatal_error(QString("the path to runtime data could not be created (mkdir -p \"%1\").")
                                .arg(runpath));
            snap::NOTREACHED();
        }

        QString const user ( f_config.contains("user")  ? f_config["user"]  : "snapwebsites" );
        QString const group( f_config.contains("group") ? f_config["group"] : "snapwebsites" );

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
}


/** \brief Clean up the snap_init object.
 *
 * The destructor makes sure that the snapinit lock file gets removed
 * before exiting the process.
 */
snap_init::~snap_init()
{
    remove_lock();
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


void snap_init::xml_to_services(QDomDocument doc, QString const & xml_services_filename)
{
    QDomNodeList services(doc.elementsByTagName("service"));

    QString const binary_path( QString::fromUtf8(f_opt.get_string("binary-path").c_str()) );

    // use a map to make sure that each service has a distinct name
    //
    service::map_t service_list_by_name;

    int const max_services(services.size());
    for(int idx(0); idx < max_services; ++idx)
    {
        QDomElement e(services.at( idx ).toElement());
        if(!e.isNull()      // it should always be an element
        && !e.attributes().contains("disabled"))
        {
            service::pointer_t s(std::make_shared<service>(shared_from_this()));
            s->configure( e, binary_path, f_debug, f_command == command_t::COMMAND_LIST );

            // avoid two services with the same name
            //
            if( service_list_by_name.find( s->get_service_name() ) != service_list_by_name.end() )
            {
                common::fatal_error(QString("snapinit cannot start the same service more than once on \"%1\". It found \"%2\" twice in \"%3\".")
                              .arg(f_server_name)
                              .arg(s->get_service_name())
                              .arg(xml_services_filename));
                snap::NOTREACHED();
            }
            service_list_by_name[s->get_service_name()] = s;

            // we currently only support one snapcommunicator connection
            // mechanism, snapinit does not know anything about connecting
            // with any other service; so if we find more than one connection
            // service, we fail early
            //
            if(s->is_connection_required())
            {
                if(f_connection_service)
                {
                    common::fatal_error(QString("snapinit only supports one connection service at this time on \"%1\". It found two: \"%2\" and \"%3\" in \"%4\".")
                                  .arg(f_server_name)
                                  .arg(s->get_service_name())
                                  .arg(f_connection_service->get_service_name())
                                  .arg(xml_services_filename));
                    snap::NOTREACHED();
                }
                f_connection_service = s;
            }

            // we are starting the snapdbproxy system which offers an
            // address and port to connect to (itself, it listens to
            // those) and we have to send that information to all the
            // children we start so we need to save that pointer
            //
            if(s->is_snapdbproxy())
            {
                if(f_snapdbproxy_service)
                {
                    common::fatal_error(QString("snapinit only supports one snapdbproxy service at this time on \"%1\". It found two: \"%2\" and \"%3\" in \"%4\".")
                                  .arg(f_server_name)
                                  .arg(s->get_service_name())
                                  .arg(f_snapdbproxy_service->get_service_name())
                                  .arg(xml_services_filename));
                    snap::NOTREACHED();
                }
                f_snapdbproxy_service = s;
            }

            // make sure to add all services as a timer connection
            // to the communicator so we can wake a service on its
            // own (especially to support the <recovery> feature.)
            //
            f_communicator->add_connection( s );

            f_service_list.push_back( s );
        }
    }

    // make sure we have at least one service;
    //
    // TODO: we may want to require certain services such as:
    //       snapcommunicator and snapwatchdog?
    //
    if(f_service_list.empty())
    {
        common::fatal_error(QString("no services were specified in \"%1\" for snapinit to manage.")
                      .arg(xml_services_filename));
        snap::NOTREACHED();
    }

    // sort those services by priority
    //
    // unfortunately, the following will sort items by pointer if
    // we were not specifying our own sort function
    //
    std::sort(f_service_list.begin(), f_service_list.end(), [](service::pointer_t const a, service::pointer_t const b){ return *a < *b; });
}


/** \brief Nuge services so they wake up.
 *
 * This function enables the timer of all the services that are
 * not requiring a connection (i.e. snapcommunicator.)
 *
 * The function also defines a timeout delay if some service
 * wants a bit of time to themselves to get started before
 * others (following) services get kicked in.
 *
 * Note that cron tasks do not get their tick date and time modified
 * here since it has to start exactly on their specific tick date
 * and time.
 *
 * \todo
 * Redesign the waking up to have a current state instead of having
 * special, rather complicated rules as we have now.
 */
void snap_init::wakeup_services()
{
    SNAP_LOG_TRACE("Wake Up Services called. (Total number of services: ")(f_service_list.size())(")");

    int64_t timeout_date(snap::snap_child::get_current_date());
    for(auto const & s : f_service_list)
    {
        // ignore the connection service, it already got started when
        // this function is called
        //
        // TODO: as noted in the documentation above, we need to redisign
        //       this "wake up services" for several reasons, but here
        //       the "is_running()" call is actually absolutely incorrect
        //       since the process could have died in between and thus
        //       we would get false when we would otherwise expect true.
        //
        if(s->is_connection_required()
        || s->is_running())
        {
            continue;
        }

        // cron tasks handle their own timeout as a date to have ticks
        // at a very specific date and time; avoid changing that timer!
        //
        if(!s->cron_task())
        {
            s->set_timeout_date(timeout_date);
        }

        // now this task timer is enabled; when we receive that callback
        // we can check whether the process is running and if not start
        // it as required by the current status
        //
        s->set_enable(true);

        // if we just started a service that has to send us a SAFE message
        // then we cannot start anything more at this point
        //
        if(s->is_safe_required())
        {
            f_expected_safe_message = s->get_safe_message();
            break;
        }

        // this service may want the next service to start later
        // (notice how that will not affect a cron task...)
        //
        // we put a minimum of 1 second so that way we do not start
        // many tasks all at once which the OS does not particularly
        // like and it makes nearly no difference on our services
        //
        timeout_date += std::max(1, s->get_wait_interval()) * 1000000LL;
    }
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


/** \brief Connect the listener to snapcommunicator.
 *
 * This function starts a connection with the snapcommunicator and
 * sends a CONNECT message.
 *
 * \note
 * The listener is created in the main thread, meaning that the thread
 * dies out until the connection either succeeds or fails. This is done
 * by design since at this point the only service running is expected
 * to be snapcommunicator and there is no other event we can receive
 * unless the connection fails (i.e. snapcommunicator can crash and
 * we want to know about that, but the connection will fail if the
 * snapcommunicator crashed.) Since this is local connection, it should
 * be really fast anyway.
 *
 * \param[in] service_name  Name of the service requesting a listener connection.
 * \param[in] host  The host to connect to.
 * \param[in] port  The port to use to connect to.
 *
 * \return true if the connection succeeded.
 */
bool snap_init::connect_listener(QString const & service_name, QString const & host, int port)
{
    // TODO: count attempts and after X attempts, fail completely
    try
    {
        // this is snapcommunicator, connect to it
        //
        f_listener_connection.reset(new listener_impl(shared_from_this(), host.toUtf8().data(), port));
        f_listener_connection->set_name("snapinit listener");
        f_listener_connection->set_priority(0);
        f_communicator->add_connection(f_listener_connection);

        // and now connect to it
        //
        snap::snap_communicator_message register_snapinit;
        register_snapinit.set_command("REGISTER");
        register_snapinit.add_parameter("service", "snapinit");
        register_snapinit.add_parameter("version", snap::snap_communicator::VERSION);
        f_listener_connection->send_message(register_snapinit);

        return true;
    }
    catch(tcp_client_server::tcp_client_server_runtime_error const & e)
    {
        // this can happen if we try too soon and the snapconnection
        // listening socket is not quite ready yet
        //
        SNAP_LOG_WARNING("connection to service \"")(service_name)("\" failed.");

        // clean up the listener connection
        //
        if(f_listener_connection)
        {
            f_communicator->remove_connection(f_listener_connection);
            f_listener_connection.reset();
        }
    }

    return false;
}


/** \brief Initialize the lamba functions for each message we can recieve
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
            [&]( snap::snap_communicator_message const& )
            {
                snap::snap_communicator_message reply;
                reply.set_command("COMMANDS");

                // list of commands understood by snapinit
                //
                reply.add_parameter("list", "HELP,LOG,QUITTING,READY,SAFE,STATUS,STOP,UNKNOWN");

                f_listener_connection->send_message(reply);
            }
        },
        {
            "LOG",
            [&]( snap::snap_communicator_message const& )
            {
                SNAP_LOG_INFO("Logging reconfiguration.");
                snap::logging::reconfigure();
            }
        },
        {
            "QUITTING",
            [&]( snap::snap_communicator_message const& )
            {
                // it looks like we sent a message after a STOP was received
                // by snapcommunicator; this means we should receive a STOP
                // shortly too, but we just react the same way to QUITTING
                // than to STOP.
                //
                terminate_services();
            }
        },
        {
            "READY",
            [&]( snap::snap_communicator_message const & )
            {
                // now we can start all the other services (except CRON tasks)
                //
                wakeup_services();

                // send the list of local services to the snapcommunicator
                //
                snap::snap_communicator_message reply;
                reply.set_command("SERVICES");

                // generate the list of services as a string of comma names
                //
                snap::snap_string_list services;
                services << "snapinit";
                for(auto const & s : f_service_list)
                {
                    services << s->get_service_name();
                }
                reply.add_parameter("list", services.join(","));

                f_listener_connection->send_message(reply);
            }
        },
        {
            "SAFE",
            [&]( snap::snap_communicator_message const& message )
            {
                // we received a "we are safe" message so we can move on and
                // start the next service
                //
                if(f_expected_safe_message != message.get_parameter("name"))
                {
                    // we need to terminate the existing services cleanly
                    // so we do not use common::fatal_error() here
                    //
                    QString msg(QString("received wrong SAFE message. We expected \"%1\" but we received \"%2\".")
                        .arg(f_expected_safe_message)
                        .arg(message.get_parameter("name")));
                    SNAP_LOG_FATAL(msg);
                    syslog( LOG_CRIT, "%s", msg.toUtf8().data() );

                    // Simulate a STOP, we cannot continue safely
                    //
                    terminate_services();
                    return;
                }

                // wakeup other services (i.e. when SAFE is required
                // the system does not start all the processes timers
                // at once--now that we have dependencies we could
                // change that though)
                //
                wakeup_services();
            }
        },
        {
            "STATUS",
            [&]( snap::snap_communicator_message const& message )
            {
                auto const service_parm(message.get_parameter("service"));
                auto const status_parm(message.get_parameter("status"));
                //
                auto const & iter = std::find_if( f_service_list.begin() , f_service_list.end(),
                    [&](service::pointer_t const & s)
                    {
                        return s->get_service_name() == service_parm;
                    });
                if( iter != f_service_list.end() )
                {
                   (*iter)->set_registered( status_parm == "up" );
                }
                SNAP_LOG_TRACE("received status from server: service=")(service_parm)(", status=")(status_parm);
            }
        },
        {
            "STOP",
            stop_func
        },
        {
            "UNKNOWN",
            [&]( snap::snap_communicator_message const& message )
            {
                SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            }
        },
    };
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
        const auto& udp_command( f_udp_message_map.find(command) );
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

    const auto& tcp_command( f_tcp_message_map.find(command) );
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
    // first go through the list and allow any service which is
    // not died and should not have to be restarted (i.e. all
    // services except CRON services for now)
    //
    auto if_may_have_died = [&]( const auto& svc )
    {
        return svc->service_may_have_died();
    };
    service::vector_t dead_services;
    std::copy_if( f_service_list.begin(), f_service_list.end(), std::back_inserter(dead_services), if_may_have_died );

    for( const auto& svc : dead_services )
    {
        // if snapcommunicator already died, we cannot forward
        // the DIED or any other message
        //
        if(f_listener_connection)
        {
            snap::snap_communicator_message register_snapinit;
            register_snapinit.set_command("DIED");
            register_snapinit.set_service(".");
            register_snapinit.add_parameter("service", svc->get_service_name());
            register_snapinit.add_parameter("pid", svc->get_old_pid());
            f_listener_connection->send_message(register_snapinit);
        }

        // This has a functional side effect of (possibly) removing the service from
        // the f_service_list vector.
        //
        svc->mark_service_as_dead();
    }

    // check whether a service failed and is marked as required
    // although if recovery is not zero we ignore the situation...
    //
    {
        auto required_and_failed = [](service::pointer_t s)
        {
            // no need to test whether recovery == 0 since it would
            // not be in the failed state if recovery != 0
            //
            return s->failed() && s->is_service_required();
        };
        service::vector_t::const_iterator required_failed(std::find_if(f_service_list.begin(), f_service_list.end(), required_and_failed));
        if(required_failed != f_service_list.end())
        {
            // we need to terminate the existing services cleanly
            // so we do not use common::fatal_error() here
            //
            QString msg(QString("service \"%1\" failed and since it is required, we are stopping snapinit now.")
                        .arg((*required_failed)->get_service_name()));
            SNAP_LOG_FATAL(msg);
            syslog( LOG_CRIT, "%s", msg.toUtf8().data() );

            // terminate snapinit
            //
            terminate_services();
            return;
        }
    }

    // completely forget about failed services with
    // no possible recovery fallback
    //
    {
        auto if_failed = [](service::pointer_t s)
        {
            return s->failed() && s->get_recovery() == 0;
        };
        f_service_list.erase(std::remove_if(f_service_list.begin(), f_service_list.end(), if_failed), f_service_list.end());
    }

    remove_terminated_services();
}


/** \brief Detected that a connection service dropped.
 *
 * This function is called whenever the listener connection service
 * is down. It is not unlikely that we already received a hang up
 * callback on that connection though.
 *
 * \param[in] s  The the service that just died.
 */
void snap_init::service_down(service::pointer_t s)
{
    NOTUSED(s);

    if(f_listener_connection)
    {
        f_communicator->remove_connection(f_listener_connection);
        f_listener_connection.reset();
    }
}


/** \brief Remove services that are marked as terminated.
 *
 * Whenever we receive the SIGCHLD, a service is to be removed. This
 * function is called last to then remove the service from the list
 * of services (f_service_list).
 *
 * In some cases the service is kept as we want to give it another
 * change to run (especially the CRON services.)
 *
 * If all services are removed from the f_service_list, the function
 * then removes all the other connections from the snap_communicator
 * object. As a result, the run() function will return and snapinit
 * will exit.
 */
void snap_init::remove_terminated_services()
{
    // remove services that were terminated
    //
    auto if_stopped = [](service::pointer_t s)
    {
        return s->has_stopped();
    };
    service::vector_t stopped_services;
    std::copy_if( f_service_list.begin(), f_service_list.end(), std::back_inserter(stopped_services), if_stopped );
    f_service_list.erase(std::remove_if(f_service_list.begin(), f_service_list.end(), if_stopped), f_service_list.end());

    // Go through each stopped service and make sure anything that depends on it also has stopped
    //
    for( const auto& svc : stopped_services )
    {
        service::vector_t depends_on_list;
        get_depends_on_list( svc->get_service_name(), depends_on_list );
        for( const auto& dep_svc : depends_on_list )
        {
            if( !dep_svc->has_stopped() )
            {
                dep_svc->set_stopping();
            }
        }
    }

    if(f_service_list.empty())
    {
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
        }
    }
}


/** \brief Process a user termination signal.
 *
 * This funtion is called whenever the user presses Ctlr-C, Ctrl-?, or Ctrl-\
 * on their keyboard (SIGINT, SIGTERM, or SIGQUIT). This function makes sure
 * to stop the process cleanly in this case by calling the
 * terminate_services() function.
 *
 * \param[in] sig  The Unix signal that generated this call.
 */
void snap_init::user_signal_caught(int sig)
{
    std::stringstream ss;
    ss << "User signal caught: " << (sig == SIGINT
                                         ? "SIGINT"
                                         : (sig == SIGTERM
                                                ? "SIGTERM"
                                                : "SIGQUIT"));
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
 * \return A smart pointer to the connection service.
 */
service::pointer_t snap_init::get_connection_service() const
{
    if(!f_connection_service)
    {
        throw std::logic_error("connection service requested before it was defined.");
    }

    return f_connection_service;
}


/** \brief Retrieve the service used to connect to the Cassandra cluster.
 *
 * This function returns the information about the server that is
 * used to connect to the Cassandra cluster.
 *
 * This should be the snapdbproxy service.
 *
 * Because a computer may not run snapdbproxy, this function may
 * return a null pointer. (i.e. although snapdbproxy is marked
 * as required, it can still be disabled.)
 *
 * \return A smart pointer to the snapdbproxy service.
 */
service::pointer_t snap_init::get_snapdbproxy_service() const
{
    return f_snapdbproxy_service;
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
    //
    for( auto const & opt : f_service_list )
    {
        ss << " [" << opt->get_service_name() << "]";
    }
    //
    SNAP_LOG_INFO(ss.str());
}



/** \brief Find who depends on the named service
 *
 * \return list of services who depend on the named service
 */
void snap_init::get_depends_on_list( const QString& service_name, service::vector_t& ret_list ) const
{
    ret_list.clear();
    for( auto service : f_service_list )
    {
        if( service->is_dependency_of( service_name ) )
        {
            ret_list.push_back(service);
        }
    }
}


/** \brief Query a service by name
 */
service::pointer_t snap_init::get_service( const QString& service_name ) const
{
    auto iter = std::find_if( f_service_list.begin(), f_service_list.end(),
    [service_name]( const auto& svc )
    {
        return svc->get_service_name() == service_name;
    });

    if( iter == f_service_list.end() )
    {
        return service::pointer_t();
    }

    return (*iter);
}


/** \brief Ask all services to quit.
 *
 * In most cases, this function is called when the snapinit tool
 * receives the STOP signal. It, itself, propagates the STOP signal
 * to all the services it started.
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
    // make sure that any death from now on marks the services as
    // done
    //
    for( auto s : f_service_list )
    {
        s->set_stopping();
    }

    // set_stopping() immediately marks certain services as dead
    // if they were not running, remove them immediately in case
    // that were all of them! the function then removes all the
    // connections and the communicator will exit its run() loop.
    //
    remove_terminated_services();

    // if we still have at least one service it has to be the
    // snapcommunicator service so we can send a STOP command
    //
    if(!f_service_list.empty())
    {
        if(f_listener_connection)
        {
            // by sending UNREGISTER to snapcommunicator, it will also
            // assume that a STOP message was sent and thus it will
            // propagate STOP to all services, and a DISCONNECT is sent
            // to all neighbors.
            //
            // The reason we do not send an UNREGISTER and a STOP from
            // here is that once we sent an UNREGISTER, the line is
            // cut and thus we cannot 100% guarantee that the STOP
            // will make it. Also, we do not use the STOP because it
            // is used by all services and overloading that command
            // could be problematic in the future.
            //
            snap::snap_communicator_message unregister_self;
            unregister_self.set_command("UNREGISTER");
            unregister_self.add_parameter("service", "snapinit");
            f_listener_connection->send_message(unregister_self);
        }
        else
        {
            // this can happen if we were trying to start snapcommunicator
            // and it somehow failed too many times too quickly
            //
            SNAP_LOG_WARNING("snap_init::terminate_services() called without a f_listener_connection. STOP could not be propagated.");
            if(common::is_a_tty())
            {
                std::cerr << "warning: snap_init::terminate_services() called without a f_listener_connection. STOP could not be propagated." << std::endl; 
            }
        }
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

    // save our (child) PID in the lock file (useful for the stop() processus)
    // the correct Debian format is the PID followed by '\n'
    //
    // FHS Version 2.1+:
    //   > The file should consist of the process identifier in ASCII-encoded
    //   > decimal, followed by a newline character. For example, if crond was
    //   > process number 25, /var/run/crond.pid would contain three characters:
    //   > two, five, and newline.
    //
    f_lock_file.write(QString("%1\n").arg(getpid()).toUtf8());
    f_lock_file.flush();

    // check whether all executables are available
    //
    bool failed(false);
    for( auto s : f_service_list )
    {
        if(!s->exists())
        {
            failed = true;

            // This is a fatal error, but we want to give the user information
            // about all the missing binary (this is not really true anymore
            // because this check is done at the end of the service confiration
            // function and generate a fatal error there already)
            //
            QString msg(QString("binary for service \"%1\" was not found or is not executable. snapinit will exit without starting anything.")
                        .arg(s->get_service_name()));
            SNAP_LOG_FATAL(msg);
            syslog( LOG_CRIT, "%s", msg.toUtf8().data() );
        }
    }
    if(failed)
    {
        common::fatal_error(QString("Premature exit because one or more services cannot be started (their executable are not available.) This may be because you changed the binary path to an invalid location."));
        snap::NOTREACHED();
    }

    // Assuming we have a connection service, we want to wake that
    // service first and once that is dealt with, we wake up the
    // other services (i.e. on the ACCEPT call)
    //
    if(f_connection_service)
    {
        f_connection_service->set_timeout_date(snap::snap_child::get_current_date());
        f_connection_service->set_enable(true);
    }
    else
    {
        // this call wakes all the other services; it is also called
        // whenever the connection to snapcommunicator is accepted
        //
        wakeup_services();
    }

    // initialize a UDP server as a fallback in case you want to use
    // snapinit without a snapcommunicator server
    //
    {
        // just in case snapcommunicator does not get started, we still can
        // receive messages over a UDP port (mainly a STOP message)
        //
        f_ping_server.reset(new ping_impl(shared_from_this(), f_udp_addr.toUtf8().data(), f_udp_port));
        f_ping_server->set_name("snapinit UDP backup server");
        f_ping_server->set_priority(30);
        f_communicator->add_connection(f_ping_server);
    }

    // initialize the SIGCHLD signal
    //
    {
        f_child_signal.reset(new sigchld_impl(shared_from_this()));
        f_child_signal->set_name("snapinit SIGCHLD signal");
        f_child_signal->set_priority(55);
        f_communicator->add_connection(f_child_signal);
    }

    // initialize the SIGTERM signal
    //
    {
        f_term_signal.reset(new sigterm_impl(shared_from_this()));
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
        f_int_signal->set_priority(60);
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
            if(!ok)
            {
                // just in case, make 100% sure that we have -1 as the PID
                lock_file_pid = -1;
            }
        }
    }

    SNAP_LOG_INFO("Stop Snap! Websites services (pid = ")(lock_file_pid)(").");

    QString udp_addr;
    int udp_port;
    get_addr_port_for_snap_communicator(udp_addr, udp_port, true);

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


void snap_init::get_addr_port_for_snap_communicator(QString & udp_addr, int & udp_port, bool default_to_snap_init)
{
    // defaults UDP for direct snapinit STOP signal
    //
    if(default_to_snap_init)
    {
        // get default from the snapinit.conf file
        //
        udp_addr = f_udp_addr;
        udp_port = f_udp_port;
    }
    else
    {
        // default for snapcommunicator
        //
        udp_addr = "127.0.0.1";
        udp_port = 4041;
    }

    // if we have snapcommunicator in our services, then we can send
    // a signal to that process, in which case we want to gather the
    // IP and port from that configuration file
    //
    service::vector_t::const_iterator snapcommunicator(std::find_if(
            f_service_list.begin()
          , f_service_list.end()
          , [](service::pointer_t const & s)
            {
                return s->get_service_name() == "snapcommunicator";
            }));
    if(snapcommunicator != f_service_list.end())
    {
        // we can send a UDP message to snapcommunicator, only we need
        // the address and port and those are defined in the
        // snapcommunicator settings
        //
        QString snapcommunicator_config_filename((*snapcommunicator)->get_config_filename());
        if(snapcommunicator_config_filename.isEmpty())
        {
            // in case it was not defined, use the default
            snapcommunicator_config_filename = "/etc/snapwebsites/snapcommunicator.conf";
        }
        snap::snap_config snapcommunicator_config;
        snapcommunicator_config.read_config_file( snapcommunicator_config_filename.toUtf8().data() );
        tcp_client_server::get_addr_port(snapcommunicator_config["signal"], udp_addr, udp_port, "udp");
    }
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
    f_opt.usage( advgetopt::getopt::no_error, "snapinit" );
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

    {
        snap::snap_exception_base::output_stack_trace();
        QString msg(QString("Fatal signal caught: %1").arg(signame));
        SNAP_LOG_FATAL(msg);
        syslog( LOG_CRIT, "%s", msg.toUtf8().data() );
        if(common::is_a_tty())
        {
            std::cerr << "snapinit: fatal: " << msg.toUtf8().data() << std::endl;
        }
    }

    // Make sure the lock file has been removed
    //
    snap_init::pointer_t si( snap_init::instance() );
    si->remove_lock();

    // Exit with error status
    //
    ::exit( 1 );
    snap::NOTREACHED();
}


// vim: ts=4 sw=4 et
