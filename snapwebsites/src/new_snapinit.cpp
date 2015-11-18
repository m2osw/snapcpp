/////////////////////////////////////////////////////////////////////////////////
// Snap Init Server -- snap initialization server

// Copyright (C) 2011-2015  Made to Order Software Corp.
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
 *      <service name="snapcommunicator">
 *        <!-- we give this one a very low priority as it has to be started
 *             before anything else -->
 *        <priority>-10</priority>
 *        <config>/etc/snapwebsites/snapcommunicator.conf</config>
 *        <register wait="10">127.0.0.1:4040</register>
 *      </service>
 *      <service name="snapserver">
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
 *      <service name="pagelist">
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
 */

#include "snapwebsites.h"
#include "snap_cassandra.h"
#include "snap_config.h"
#include "snap_exception.h"
#include "snap_thread.h"
#include "log.h"
#include "not_reached.h"

#include <advgetopt/advgetopt.h>

#include <QtCassandra/QCassandraTable.h>

#include <QFile>
#include <QTime>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <exception>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace
{
    /** \brief Define whethre the standard output stream is a TTY.
     *
     * This function defines whether 'stderr' is a TTY or not. If not
     * we assume that we were started as a deamon and we do not spit
     * out errors in stderr. If it is a TTY, then we also print a
     * message in the console making it easier to right away know
     * that the tool detected an error and did not start in the
     * background.
     */
    bool g_isatty = false;

    /** \brief List of configuration files.
     *
     * This variable is used as a list of configuration files. It is
     * empty here because the configuration file may include parameters
     * that are not otherwise defined as command line options.
     */
    std::vector<std::string> const g_configuration_files;

    /** \brief Command line options.
     *
     * This table includes all the options supported by the server.
     */
    advgetopt::getopt::option const g_snapinit_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: %p [-<opt>] <start|restart|stop>",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "where -<opt> is one or more of:",
            advgetopt::getopt::help_argument
        },
        {
            'b',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "binary_path",
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
            "/var/lock/snapwebsites",
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
            's',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "services_config",
            nullptr,
            "Configuration file to pass into servers.",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "show the version of the snapinit executable",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            "start|restart|stop",
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


    const char* UDP_SERVER   = "127.0.0.1:4100";
    const int   BUFSIZE      = 256;
    const int   TIMEOUT      = 1000;
    const char* SNAPINIT_KEY = "snapinit-1846faf6-a02a-11e3-884b-206a8a420cb5";
}
//namespace




/////////////////////////////////////////////////
// PROCESS                                     //
/////////////////////////////////////////////////

class process
{
public:
    typedef std::shared_ptr<process>        pointer_t;
    typedef std::vector<pointer_t>          vector_t;
    typedef std::map<QString, pointer_t>    map_t;

    void                        configure(QDomElement e);
    void                        create_process();

    void                        set_path( QString const & path )     { f_path = path; }
    void                        set_debug( bool const debug )        { f_debug = debug; }

    bool                        exists() const;
    bool                        run();
    bool                        is_running();
    void                        stop_service();
    void                        kill_service();

    pid_t                       pid()        const { return f_pid;             }
    QString const &             name()       const { return f_name;            }
    int                         startcount() const { return f_startcount;      }
    int                         elapsed()    const { return f_timer.elapsed(); }

    void                        set_disabled( bool const val ) { f_disabled = val; }
    bool                        disabled()   const { return f_disabled;        }

    bool                        operator < (process const & rhs) const;

private:
    QString const &             get_full_path() const;
    void                        handle_status( int const pid, int const status );

    mutable QString             f_full_path;
    mutable QString             f_action;
    QString                     f_path;
    QString                     f_config_filename;
    QString                     f_name;
    QString                     f_command;
    pid_t                       f_pid = -1;
    int                         f_exit = -1;
    int                         f_startcount = 0;
    QTime                       f_timer;
    bool                        f_disabled = false;
    bool                        f_debug = false;

    QString                     f_register;         // to register with snapcommunicator
    int                         f_register_wait_interval = 3;
    int                         f_priority = 50;
    int                         f_cron = 0;         // off
};




/** \brief Retrieve parameters about this service from e.
 *
 * This function configures this service object from the data defined
 * by 'e'.
 *
 * \param[in] e  The element with configuration information for this service.
 */
void process::configure(QDomElement e)
{
    // first make sure we have a name for this service
    f_name = e.attribute("name");
    if(f_name.isEmpty())
    {
        SNAP_LOG_FATAL("the \"name\" parameter of a service must be defined and not empty.");
        exit(1);
    }

    // by default the command is one to one like the name of the service
    f_command = f_name;

    // check to see whether the user specifies a command
    QDomElement command_element(e.firstChildElement("command"));
    if(!command_element.isNull())
    {
        f_command = command_element.text();
        if(f_command.isEmpty())
        {
            SNAP_LOG_FATAL("the command tag of service \"")(f_name)("\" returned an empty string which does not represent a valid command.");
            exit(1);
        }
    }

    // check for a priority; the default is 50, the user can change it
    QDomElement priority_element(e.firstChildElement("priority"));
    if(!priority_element.isNull())
    {
        bool ok(false);
        f_priority = priority_element.text().toInt(&ok, 10);
        if(!ok)
        {
            SNAP_LOG_FATAL("priority \"")(priority_element.text())("\" of service \"")(f_name)("\" returned a string that does not represent a valid decimal number.");
            exit(1);
        }
        if(f_priority < -100 || f_priority > 100)
        {
            SNAP_LOG_FATAL("priority \"")(priority_element.text())("\" of service \"")(f_name)("\" is out of bounds, we accept a priority between -100 and +100.");
            exit(1);
        }
    }

    // filename of this service configuration file
    // (if not specified here, then we do not specify anything on the
    // command line in that regard)
    //
    QDomElement config_element(e.firstChildElement("config_filename"));
    if(!config_element.isNull())
    {
        f_config_filename = config_element.text();
        if(f_config_filename.isEmpty())
        {
            SNAP_LOG_FATAL("the config tag of service \"")(f_name)("\" returned an empty string which does not represent a valid configuration filename.");
            exit(1);
        }
    }

    // whether we should register ourselves after that service was started
    //
    QDomElement register_element(e.firstChildElement("register"));
    if(!register_element.isNull())
    {
        f_register = register_element.text();
        if(f_register.isEmpty())
        {
            SNAP_LOG_FATAL("the register tag of service \"")(f_name)("\" returned an empty string which does not represent a valid IP and port specification.");
            exit(1);
        }
        bool ok(false);
        f_register_wait_interval = register_element.attribute("wait").toInt(&ok, 10);
    }

    // tasks that need to be run once in a while uses a <cron> tag
    //
    QDomElement cron_element(e.firstChildElement("cron"));
    if(!cron_element.isNull())
    {
        if(cron_element.text() == "off")
        {
            f_cron = 0;
        }
        else
        {
            bool ok(false);
            f_cron = cron_element.text().toInt(&ok, 10);
            if(!ok)
            {
                SNAP_LOG_FATAL("the cron tag of service \"")(f_name)("\" must be a valid decimal number representing a number of seconds to wait between each execution.");
                exit(1);
            }
            // we function like anacron and know when we have to run
            // (i.e. whether we missed some prior runs) so very large
            // cron values will work just as expected (see /var/spool/snap/*)
            //
            // TBD: offer a similar syntax to crontab? frankly we are not
            //      trying to replace cron and at this time we have just
            //      one service that runs every 5 min. so here...
            //
            if(f_cron < 10 || f_cron > 86400 * 367)
            {
                SNAP_LOG_FATAL("the cron tag of service \"")(f_name)("\" must be a number between 10 and 31708800 (a little over 1 year in seconds).");
                exit(1);
            }
        }
    }
}


/** \brief Get the full path of the target executable that snapinit will launch/monitor
 *
 * This function generates the full path to the executable to use to
 * launch and monitor the binary in question. It calculates the name
 * by looking at the f_name member, and translating it into the full path to launch.
 * 
 * The special names are "server" and "backend". If "server", then the "snapserver" basename
 * is used. If "backend," then the "snapbackend" will be used. Any other name is considered to
 * be the f_action, which will be a paramter passed to the "snapbackend."
 *
 * \note this method is marked const, but it does modify two mutable member variables: f_full_path and f_action
 * I probably should move this support into its own class to take advantage of RAII.
 *
 * \return full path to the binary to launch
 */
QString const & process::get_full_path() const
{
    if( f_full_path.isEmpty() )
    {
        QString basename;
        if( f_name == "server" )
        {
            basename = "snapserver";
            f_action = "";
        }
        else if( f_name == "backend" )
        {
            basename = "snapbackend";
            f_action = "";
        }
        else
        {
            basename = "snapbackend";
            f_action = f_name;
        }
        //
        f_full_path = QString("%1/%2").arg(f_path).arg(basename);
    }
    return f_full_path;

}


/** \brief Verify that this executable exists.
 *
 * This function generates the full path to the executable to use to
 * start this process. If that full path represents an existing file
 * and that file has it's executable flag set, then the function
 * returns true. Otherwise it returns false.
 *
 * When the snapinit tool starts, it first checks whether all the
 * services that are required to start exist. If not then it fails
 * because if any one service is missing, something is awry anyway.
 *
 * \return true if the file exists and can be executed.
 */
bool process::exists() const
{
    QString const & full_path( get_full_path() );
    return access(full_path.toUtf8().data(), R_OK | X_OK) == 0;
}


/** \brief Start the process in the background.
 *
 * This function starts this process.
 *
 * \return true if the process started as expected.
 */
bool process::run()
{
    f_timer.start();
    f_startcount++;
    pid_t const parent_pid(getpid());
    f_pid = fork();
    if( f_pid == 0 )
    {
        // child
        //

        // make sure that the SIGHUP is sent to us if our parent dies
        //
        prctl(PR_SET_PDEATHSIG, SIGHUP);

        // always reconfigure the logger in the child
        //
        snap::logging::reconfigure();

        // the parent may have died just before the prctl() had time to set
        // up our child death wish...
        //
        if(parent_pid != getppid())
        {
            SNAP_LOG_FATAL("process::run() lost parent too soon and did not receive SIGHUP; quit immediately.");
            exit(1);
            snap::NOTREACHED();
        }

        QString const & full_path( get_full_path() );
        QStringList qargs;
        qargs << full_path;
        if(f_debug)
        {
            qargs << "--debug";
        }
        qargs << "--config" << f_config_filename;
        //
        if( f_name != "server" && f_name != "backend" )
        {
            qargs << "--action" << f_name;
        }

        std::vector<std::string> args;
        std::vector<char const *> args_p;
        //
        for( auto arg : qargs )
        {
            args.push_back(arg.toUtf8().data());
            args_p.push_back(args.rbegin()->c_str());
        }
        //
        args_p.push_back(NULL);

        // Quiet up the console by redirecting these from/to /dev/null
        // except in debug mode
        //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        if(!f_debug)
        {
            freopen( "/dev/null", "r", stdin  );
            freopen( "/dev/null", "w", stdout );
            freopen( "/dev/null", "w", stderr );
        }

        // Execute the child processes
        //
        execv(
            full_path.toUtf8().data(),
            const_cast<char * const *>(&args_p[0])
        );
#pragma GCC diagnostic pop

        SNAP_LOG_FATAL("Child process \"")(qargs.join(" "))("\" failed to start!");
        exit(1);
    }

    sleep(1);
    return is_running();
}


void process::handle_status( int const the_pid, int const status )
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    if(WIFEXITED(status))
    {
        f_exit = WEXITSTATUS(status);
    }
    else
    {
        f_exit = -1;
    }
#pragma GCC diagnostic pop

    if( the_pid == -1 )
    {
        SNAP_LOG_ERROR() << "Command [" << f_name << "] terminated abnormally with exit code [" << f_exit << "]";
    }
    else
    {
        SNAP_LOG_INFO() << "Command [" << f_name << "] terminated normally with exit code [" << f_exit << "]";
    }

    f_pid = 0;
}


/** \brief Check whether this process is running.
 *
 * This function checks whether this proecss is running by checking the
 * f_pid is zero or not.
 *
 * If the process is running, call waitpid() to see whether the process
 * stopped or not. That will remove zombies and allow the snapinit process
 * to restart those processes.
 *
 * \return true if the process is still running.
 */
bool process::is_running()
{
    if( f_pid == 0 )
    {
        return false;
    }

    int status = 0;
    int const the_pid = waitpid( f_pid, &status, WNOHANG );
    if( the_pid == 0 )
    {
        return true;
    }

    handle_status( the_pid, status );

    return false;
}


/** \brief Request a service to stop.
 *
 * This function sends the STOP signal to a process.
 *
 * The function does nothing if the process is already stopped.
 */
void process::stop_service()
{
    if( !is_running() )
    {
        // Do nothing if no process running...
        //
        return;
    }

    // run the corresponding snapsignal command to stop this process
    //
    // TODO: Note that when f_name is "server" the snapsignal is not really
    //       expected to understand; it should be "snapserver" instead
    //       I think that the snapinit tool should only use "snapserver"
    //       everywhere instead of "server"
    //
    QString const command( QString("%1/snapsignal -c %2 -a %3 STOP").arg(f_path).arg(f_config_filename).arg(f_name) );
    int const retval( system( command.toUtf8().data() ) );
    if( retval == -1 )
    {
        SNAP_LOG_ERROR("Cannot execute command '")(command)("', so ")(f_name)(" won't be halted properly!");
        return;
    }
}


void process::kill_service()
{
    if( f_pid == 0 )
    {
        // Do nothing if no process running...
        return;
    }

    // Wait for process to end, then set f_exit status appropriately.
    //
    int timeout = 5;
    while( is_running() )
    {
        if( timeout > 0 )
        {
            SNAP_LOG_INFO("process ")(f_name)(" is still running. Wating ")(timeout)(" more counts.");
        }
        // Once we have snapcommunicator, the wait could be reduced, although
        // some backend may take a long time to get out...
        //
        usleep( 400000 );
        --timeout;

        if( timeout == 0 || timeout == -1 )
        {
            int const signal((timeout == 0) ? SIGTERM : SIGKILL);
            SNAP_LOG_WARNING("process ")(f_name)(", pid=")(f_pid)(", failed to respond to signal, using -")(signal);
            int const retval(::kill( f_pid, signal ));
            if( retval == -1 )
            {
                SNAP_LOG_WARNING("Unable to kill process ")(f_name)(", pid=")(f_pid)("! errno=")(errno);
                break;
            }
            if( timeout == 0 )
            {
                sleep( 1 );
            }
        }
        else if( timeout < -1 )
        {
            // stop the loop
            //
            SNAP_LOG_WARNING("process ")(f_name)(", pid=")(f_pid)(", failed to terminate properly...");
            break;
        }
    }
}







/** \brief Services are expected to be sorted by priority.
 *
 * This function compares 'this' priority against the 'rhs'
 * priority and returns true if 'this' priority is smaller.
 *
 * \param[in] rhs  The right hand side service object.
 */
bool process::operator < (process const & rhs) const
{
    return f_priority < rhs.f_priority;
}






/////////////////////////////////////////////////
// SNAP INIT                                   //
/////////////////////////////////////////////////

class snap_init
{
public:
    typedef std::shared_ptr<snap_init> pointer_t;

                                ~snap_init();

    static void                 create_instance( int argc, char * argv[] );
    static pointer_t            instance();

    void                        run_processes();
    bool                        is_running();

    static void                 sighandler( int sig );

private:
                                snap_init( int argc, char * argv[] );
                                snap_init( snap_init const & ) = delete;
    snap_init &                 operator = ( snap_init const & ) = delete;

    void                        usage();
    void                        xml_to_services(QDomDocument doc);
    void                        validate();
    void                        show_selected_servers() const;
    process::pointer_t          get_process( QString const & name );
    void                        start_processes();
    void                        monitor_processes();
    void                        terminate_processes();
    void                        start();
    void                        restart();
    void                        stop();
    void                        remove_lock();

    static pointer_t            f_instance;
    advgetopt::getopt           f_opt;
    process::vector_t           f_services;
    process::map_t              f_services_by_name;
    QString                     f_lock_filename;
    QFile                       f_lock_file;
    snap::snap_config           f_config;
    QString                     f_log_conf;
    //snap::snap_cassandra        f_cassandra;
    QString                     f_spool_path;

    process::vector_t           f_process_list;
    process::map_t              f_process_list_by_name;
};


snap_init::pointer_t snap_init::f_instance;


snap_init::snap_init( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapinit_options, g_configuration_files, "SNAPINIT_OPTIONS")
    , f_lock_filename( QString("%1/%2")
                       .arg(f_opt.get_string("lockdir").c_str())
                       .arg(SNAPINIT_KEY)
                     )
    , f_lock_file( f_lock_filename )
    , f_log_conf( "/etc/snapwebsites/snapinit.properties" )
    , f_spool_path( "/var/spool/snap" )
{
    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
    }

    bool const list( f_opt.is_defined( "list" ) );

    f_config.read_config_file( f_opt.get_string("config").c_str() );

    // setup the logger
    if( f_opt.is_defined( "nolog" ) || f_opt.is_defined( "help" ) )
    {
        snap::logging::configure_console();
    }
    else if( f_opt.is_defined("logfile") )
    {
        snap::logging::configure_logfile( QString::fromUtf8(f_opt.get_string( "logfile" ).c_str()) );
    }
    else
    {
        if(!f_config.contains("log_config"))
        {
            // use .conf definition when available
            f_log_conf = f_config["log_config"];
        }
        snap::logging::configure_conffile( f_log_conf );
        if(!list)
        {
            SNAP_LOG_INFO("---------------- snapinit manager started");
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
            SNAP_LOG_FATAL("the xml_services parameter cannot be empty, it has to be a path to the snapinit.xml file.");
            exit(1);
        }
        QFile xml_services_file(xml_services_filename);
        if(!xml_services_file.open(QIODevice::ReadOnly))
        {
            // the XML services is a mandatory file we need to be able to read
            int const e(errno);
            SNAP_LOG_FATAL("the XML file \"")(xml_services_filename)("\" could not be opened (")(strerror(e))(").");
            exit(1);
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
                SNAP_LOG_FATAL("the XML file \"")(xml_services_filename)("\" could not be parse as valid XML (")
                    (xml_services_filename)(":")(error_line)(": ")(error_message)("; on column: ")(error_column)(").");
                exit(1);
            }
            xml_to_services(doc);
        }
    }

    if(list)
    {
        std::cout << "List of services to start on this server:" << std::endl;
        for(auto p : f_process_list)
        {
            std::cout << p->name() << std::endl;
        }
        // the --list command is over!
        exit(1);
    }
}


snap_init::~snap_init()
{
    remove_lock();
}


void snap_init::create_instance( int argc, char * argv[] )
{
    f_instance.reset( new snap_init( argc, argv ) );
    if(!f_instance)
    {
        // new should throw std::bad_alloc or some other error anyway
        throw std::runtime_error("snapinit failed to create an instance of a snap_init object");
    }
}


snap_init::pointer_t snap_init::instance()
{
    if( !f_instance )
    {
        throw std::invalid_argument( "snap_init instance must be created with create_instance()!" );
    }
    return f_instance;
}


void snap_init::xml_to_services(QDomDocument doc)
{
    QDomNodeList services(doc.elementsByTagName("service"));

    std::string const binary_path(f_opt.get_string("binary_path").c_str() );
    bool const debug( f_opt.is_defined("debug") );

    int const max_services(services.size());
    for(int idx(0); idx < max_services; ++idx)
    {
        QDomElement e(services.at( idx ).toElement());
        if(!e.isNull()) // this should always be true
        {
            process::pointer_t p( new process );
            p->set_path( QString::fromUtf8( binary_path.c_str() ) );
            p->set_debug( debug );
            p->configure( e );

            // avoid two services with the same name
            //
            if( f_process_list_by_name.find( p->name() ) == f_process_list_by_name.end() )
            {
                SNAP_LOG_FATAL("snapinit cannot start the same service more than once. It found \"")(p->name())("\" twice.");
                exit(1);
            }
            f_process_list_by_name[p->name()] = p;

            f_process_list.push_back(p);
        }
    }

    // make sure we have at least one service;
    //
    // TODO: we may want to require certain services such as:
    //       snapcommunicator and snapwatchdog?
    //
    if(f_process_list.size() == 0)
    {
        SNAP_LOG_FATAL("no services were specified for snapinit to manage.");
        exit(1);
    }

    std::sort(f_services.begin(), f_services.end());
}


void snap_init::run_processes()
{
    if( f_opt.is_defined( "help" ) )
    {
        usage();
    }
    //
    if( !f_opt.is_defined( "--" ) )
    {
        SNAP_LOG_ERROR() << "A command is required!";
        usage();
    }

    validate();
    show_selected_servers();

    const std::string command( f_opt.get_string("--") );
    if( command == "start" )
    {
        start();
    }
    else if( command == "stop" )
    {
        stop();
    }
    else if( command == "restart" )
    {
        restart();
    }
    else
    {
        SNAP_LOG_ERROR() << "Command '" << command << "' not recognized!";
        usage();
    }
}


bool snap_init::is_running()
{
    return f_lock_file.exists();
}


void snap_init::validate()
{
    const std::string command( f_opt.get_string("--") );

    if( ((command == "start") || (command == "restart")) && f_services.empty() )
    {
        throw std::invalid_argument("Must specify at least one service in the \"services=...\" parameter of the snapserver.conf configuration file");
    }
    else if( command == "stop" )
    {
        if( f_opt.is_defined("detach") )
        {
            SNAP_LOG_WARNING("The --detach option is ignored with the 'stop' command.");
        }
    }
}


void snap_init::show_selected_servers() const
{
    std::stringstream ss;
    ss << "Enabled servers:";
    //
    for( auto const & opt : f_process_list )
    {
        ss << " [" << opt << "]";
    }
    //
    SNAP_LOG_INFO(ss.str());
}





void snap_init::monitor_processes()
{
    for( auto p : f_process_list )
    {
        if( !p->is_running() )
        {
            if( (p->startcount() > 5) && (p->elapsed() < 5000) )
            {
                // Job has died too often and too soon between startups
                //
                p->set_disabled( true );
                continue;
            }

            // Restart process
            //
            p->run();
        }
    }

    // Remove all disabled jobs
    //
    while( true )
    {
        auto it = std::find_if( f_process_list.begin(), f_process_list.end(),
            []( process::pointer_t p )
            {
                return p->disabled();
            }
        );
        //
        if( it == f_process_list.end() )
        {
            break;
        }
        //
        SNAP_LOG_WARNING() << "Process [" << (*it)->name() << "] refused to start, so removed from list!";
        f_process_list.erase( it );
    }
}


void snap_init::terminate_processes()
{
    // first send a STOP to each process, all at once
    for( auto p : f_process_list )
    {
        p->stop_service();
    }

    // give them a second to exit
    sleep(1);

    // then wait on all the processes still running
    for( auto p : f_process_list )
    {
        p->kill_service();
    }
}


void snap_init::start_processes()
{
    // This does prevent a race attack; however, in this mode, the server cannot remove the lock file
    // when it closes. Thus "snapinit stop" hangs forever.
    //
    int const fd(::open( f_lock_file.fileName().toUtf8().data(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR ));
    if( fd == -1 )
    {
        if(errno == EEXIST)
        {
            SNAP_LOG_FATAL("Lock file \"")(f_lock_file.fileName())("\" exists! Is this a race condition?");
            if(g_isatty)
            {
                std::cerr << "Lock file \"" << f_lock_file.fileName() << "\" exists! Is this a race condition?" << std::endl;
            }
        }
        else
        {
            SNAP_LOG_FATAL("Lock file \"")(f_lock_file.fileName())("\" could not be created.");
            if(g_isatty)
            {
                std::cerr << "Lock file \"" << f_lock_file.fileName() << "\" could not be created." << std::endl;
            }
        }
        exit(1);
    }

    // save the fd of the lock in a QFile
    f_lock_file.open( fd, QFile::ReadWrite );

    // check whether all executable are available
    bool failed(false);
    for( auto p : f_process_list )
    {
        if(!p->exists())
        {
            failed = true;
            SNAP_LOG_FATAL("process for service \"")(p->name())("\" was not found. snapinit will stop without starting anything.");
        }
    }
    // also verify that the snapsignal tool is accessible
    QString const snapsignal( QString("%1/snapsignal").arg( QString::fromUtf8(f_opt.get_string("binary_path").c_str()) ) );
    if(access(snapsignal.toUtf8().data(), R_OK | X_OK) != 0)
    {
        failed = true;
        SNAP_LOG_FATAL("process for service \"snapsignal\" was not found. snapinit will stop without starting anything.");
    }
    if(failed)
    {
        SNAP_LOG_INFO("Premature exit because one or more services cannot be started (their executable are not available.) This may be because you changed the binary path to an invalid location.");
        // this shows the user if he's looking the screen, otherwise the
        // log are likely very silent!
        if(g_isatty)
        {
            std::cerr << "Premature exit because one or more services cannot be started (their executable are not available.) This may be because you changed the binary path to an invalid location. More information can be found in the snapinit.log file." << std::endl;
        }
        return;
    }

    // start all the services we can start at this time (it may just be
    // the server.)
    for( auto p : f_process_list )
    {
        p->run();
    }

    // sleep until stopped
    snap::server::udp_server_t udp_signals( snap::server::udp_get_server( UDP_SERVER ) );
    //
    for(;;)
    {
        monitor_processes();
        //
        const std::string word( udp_signals->timed_recv( BUFSIZE, TIMEOUT ) );
        if( word == "STOP" )
        {
            SNAP_LOG_INFO("STOP received, terminate processes.");
            terminate_processes();
            break;
        }
        //sleep( 1 ); -- we already sleep in the timed_recv() call
    }

    remove_lock();

    SNAP_LOG_INFO("Normal shutdown.");
}


void snap_init::start()
{
    SNAP_LOG_INFO() << "Start servers";
    if( is_running() )
    {
        throw std::runtime_error("snap_init is already running!");
    }

    if( f_opt.is_defined("detach") )
    {
        // fork(), then stay resident
        // Listen for STOP command on UDP port.
        //
        int const pid(fork());
        if( pid == 0 )
        {
            start_processes();
        }
        else
        {
            SNAP_LOG_INFO("Process started successfully!");
        }
    }
    else
    {
        // Keep in foreground
        //
        start_processes();
    }
}


void snap_init::restart()
{
    SNAP_LOG_INFO() << "Restart servers";
    if( is_running() )
    {
        stop();
    }

    start();
}


void snap_init::stop()
{
    SNAP_LOG_INFO("Stop services");
    if( !is_running() )
    {
        throw std::runtime_error("snap_init is not running!");
    }

    snap::server::udp_ping_server( UDP_SERVER, "STOP" );

    // TODO: add a timer, by default wait at most 60 seconds
    //       (add a parameter in the .conf to allow for shorter/longer waits)
    do
    {
        // We wait until the remote process removes the lockfile...
        //
        sleep(1);
    }
    while( f_lock_file.exists() );
}


void snap_init::usage()
{
    f_opt.usage( advgetopt::getopt::no_error, "snapinit" );
    throw std::invalid_argument( "usage" );
}


void snap_init::remove_lock()
{
    if( f_lock_file.isOpen() )
    {
        // We have to do it this way, otherwise the remove does not work.
        ::close( f_lock_file.handle() );
        QFile lock_file( f_lock_filename );
        lock_file.remove();
    }
}


/** \brief A static function to capture various signals.
 *
 * This function captures unwanted signals like SIGSEGV and SIGILL.
 *
 * The handler logs the information and then the process exists.
 * This is done mainly so we have a chance to debug problems even
 * when it crashes on a server.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void snap_init::sighandler( int sig )
{
    QString signame;
    bool user_terminated(false);
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

    case SIGTERM:
        signame = "SIGTERM";
        user_terminated = true;
        break;

    case SIGINT:
        signame = "SIGINT";
        user_terminated = true;
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    snap_init::pointer_t instance( snap_init::instance() );
    if( user_terminated )
    {
        instance->terminate_processes();
        SNAP_LOG_INFO("User signal caught: ")(signame);
    }
    else
    {
        snap::snap_exception_base::output_stack_trace();
        SNAP_LOG_FATAL("Fatal signal caught: ")(signame);
    }

    // Make sure the lock file has been removed
    //
    instance->remove_lock();

    // Exit with error status
    //
    exit( 1 );
}











int main(int argc, char *argv[])
{
    int retval = 0;
    g_isatty = isatty(STDERR_FILENO);

    try
    {
        // First, create the static snap_init object
        //
        snap_init::create_instance( argc, argv );

        // Stop on these signals, log them, then terminate.
        //
        // Note: the handler may access the snap_init instance
        //
        signal( SIGSEGV, snap_init::sighandler );
        signal( SIGBUS,  snap_init::sighandler );
        signal( SIGFPE,  snap_init::sighandler );
        signal( SIGILL,  snap_init::sighandler );
        signal( SIGTERM, snap_init::sighandler );
        signal( SIGINT,  snap_init::sighandler );

        // Now run our processes!
        //
        snap_init::pointer_t init( snap_init::instance() );
        init->run_processes();
    }
    catch( snap::snap_exception const& except )
    {
        SNAP_LOG_FATAL("snap_init: snap_exception caught! ")(except.what());
        retval = 1;
    }
    catch( std::invalid_argument const& std_except )
    {
        SNAP_LOG_FATAL("snap_init: invalid argument: ")(std_except.what());
        retval = 1;
    }
    catch( std::exception const& std_except )
    {
        SNAP_LOG_FATAL("snap_init: std::exception caught! ")(std_except.what());
        retval = 1;
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_init: unknown exception caught!");
        retval = 1;
    }

    return retval;
}

// vim: ts=4 sw=4 et
