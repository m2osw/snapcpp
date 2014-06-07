/////////////////////////////////////////////////////////////////////////////////
// Snap Init Server -- snap initialization server

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
//
// This server reads in a configuration file and keeps specified services running.
// When signaled, it will terminate those services cleanly.
/////////////////////////////////////////////////////////////////////////////////

#include "snapwebsites.h"
#include "snap_cassandra.h"
#include "snap_config.h"
#include "snap_exception.h"
#include "snap_thread.h"
#include "log.h"
#include "not_reached.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <exception>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include <advgetopt/advgetopt.h>

#include <QtCassandra/QCassandraTable.h>

#include <QFile>
#include <QTime>

namespace
{
    /** \brief List of configuration files.
     *
     * This variable is used as a list of configuration files. It may be
     * empty.
     */
    std::vector<std::string> const g_configuration_files = 
    {
        "/etc/snapwebsites/snapinit.conf"
    };

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
            "/etc/snapwebsites/snapserver.conf",
            "Configuration file to pass into servers.",
            advgetopt::getopt::optional_argument
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
        // TODO: We should allow for a log filename definition
        //       in the snapserver.conf file too
        {
            'l',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "logfile",
            "/var/log/snapwebsites/snapinit.log",
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

class process
{
public:
    typedef std::shared_ptr<process> pointer_t;

    typedef enum { Server, Backend } type_t;

    process( const QString& n )
        : f_type(Backend)
        , f_name(n)
        , f_pid(0)
        , f_exit(0)
        , f_startcount(0)
        , f_disabled(false)
    {
    }

    process()
        : f_type(Server)
        , f_name("snapserver")
        , f_pid(0)
        , f_exit(0)
        , f_startcount(0)
        , f_disabled(false)
    {
    }

    void set_path( const QString& path )     { f_path = path; }
    void set_config( const QString& config ) { f_config_filename = config; }

    bool   run();
    bool   is_running();
    void   kill();
    //
    pid_t   pid()        const { return f_pid;             }
    QString name()       const { return f_name;            }
    int     startcount() const { return f_startcount;      }
    type_t  type()       const { return f_type;            }
    int     elapsed()    const { return f_timer.elapsed(); }
    bool    disabled()   const { return f_disabled;        }
    //
    void   set_disabled( const bool val ) { f_disabled = val; }

private:
    type_t   f_type;
    QString  f_path;
    QString  f_config_filename;
    QString  f_name;
    int      f_pid;
    int      f_exit;
    int      f_startcount;
    QTime    f_timer;
    bool     f_disabled;

    void handle_status( const int pid, const int status );
};


bool process::run()
{
    f_timer.start();
    f_startcount++;
    f_pid = fork();
    if( f_pid == 0 )
    {
        // child
        //
        const QString cmd( QString("%1/%2").arg(f_path).arg( (f_type == Server)? "snapserver": "snapbackend") );
        QStringList qargs;
        qargs << cmd
              << "-c" << f_config_filename;
        //
        if( f_type == Backend )
        {
            qargs << "-a" << f_name;
        }

        std::vector<std::string> args;
        std::vector<const char *> args_p;
        //
        for( auto arg : qargs )
        {
            args.push_back(arg.toUtf8().data());
            args_p.push_back(args.rbegin()->c_str());
        }
        //
        args_p.push_back(NULL);

        // Quiet up the console by redirecting these from/to /dev/null
        //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        freopen( "/dev/null", "r", stdin  );
        freopen( "/dev/null", "w", stdout );
        freopen( "/dev/null", "w", stderr );

        // Execute the child processes
        //
        execv(
            cmd.toUtf8().data(),
            const_cast<char * const *>(&args_p[0])
        );
#pragma GCC diagnostic pop

        SNAP_LOG_FATAL("Child process \"")(qargs.join(" "))("\" failed to start!");
        exit(1);
    }

    sleep(1);
    return is_running();
}


void process::handle_status( const int the_pid, const int status )
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


bool process::is_running()
{
    if( f_pid == 0 )
    {
        return false;
    }

    int status = 0;
    const int the_pid = waitpid( f_pid, &status, WNOHANG );
    if( the_pid == 0 )
    {
        return true;
    }

    handle_status( the_pid, status );

    return false;
}


void process::kill()
{
    if( f_pid == 0 )
    {
        // Do nothing if no process running...
        return;
    }

    const QString command( QString("%1/snapsignal -c %2 -a %3 STOP").arg(f_path).arg(f_config_filename).arg(f_name) );
    const int retval = system( command.toUtf8().data() );
    if( retval == -1 )
    {
        SNAP_LOG_ERROR() << "Cannot execute command '" << command << "', so " << f_name << " has not been halted!";
        return;
    }

    // Wait for process to end, then set f_exit status appropriately.
    //
    int timeout = 5;
    while( is_running() )
    {
        if( timeout > 0 )
        {
            SNAP_LOG_INFO() << "process " << f_name << " is still running. Wating " << timeout << " more counts.";
        }
        //
        sleep( 5 );
        --timeout;

        if( timeout == 0 )
        {
            SNAP_LOG_WARNING() << "process " << f_name << ", pid=" << f_pid << ", failed to respond to signal, using SIGTERM...";
            ::kill( f_pid, SIGTERM );
            sleep(10);
        }
        else if( timeout == -1 )
        {
            SNAP_LOG_WARNING() << "process " << f_name << ", pid=" << f_pid << ", failed to respond to signal, using SIGKILL...";
            ::kill( f_pid, SIGKILL );
        }
        else if( timeout < -1 )
        {
            // stop the loop
            //
            SNAP_LOG_WARNING() << "process " << f_name << ", pid=" << f_pid << ", failed to terminate properly...";
            break;
        }
    }
}


class snap_init
{
public:
    typedef std::shared_ptr<snap_init> pointer_t;

    static void create_instance( int argc, char * argv[] );
    static pointer_t instance();
    ~snap_init();

    void run_processes();
    bool is_running();

    static void sighandler( int sig );

private:
    typedef std::vector<std::string>        services_t;

    static pointer_t     f_instance;
    advgetopt::getopt    f_opt;
    services_t           f_services;
    QFile                f_lock_file;
    snap::snap_config    f_config;
    snap::snap_cassandra f_cassandra;

    typedef std::vector<process::pointer_t> process_list_t;
    process_list_t f_process_list;

    snap_init( int argc, char *argv[] );

    void usage();
    void validate();
    void show_selected_servers() const;
    bool backend_ready();
    void create_server_process();
    void create_backend_process( QString const& name );
    void start_processes();
    void monitor_processes();
    void terminate_processes();
    void start();
    void restart();
    void stop();
    void remove_lock();
};


snap_init::pointer_t snap_init::f_instance;


snap_init::snap_init( int argc, char *argv[] )
    : f_opt(argc, argv, g_snapinit_options, g_configuration_files, "SNAPINIT_OPTIONS")
    , f_lock_file( QString("/tmp/%1").arg(SNAPINIT_KEY) )
{
    if( f_opt.is_defined( "nolog" ) || f_opt.is_defined( "help" ) )
    {
        snap::logging::configureConsole();
    }
    else
    {
        snap::logging::configureLogfile( f_opt.get_string("logfile").c_str() );
    }

    f_config.read_config_file( f_opt.get_string("config").c_str() );
    if(!f_config.contains("services"))
    {
        SNAP_LOG_FATAL() << "the configuration file must list the services to start (services=server,images,pagelist,sendmail)";
        exit( 1 );
    }

    f_cassandra.connect( &f_config );
    //
    QtCassandra::QCassandraContext::pointer_t context( f_cassandra.get_snap_context() );
    Q_ASSERT(context);
    //
    QtCassandra::QCassandraTable::pointer_t domains_table  ( context->findTable(snap::get_name(snap::SNAP_NAME_DOMAINS))  );
    QtCassandra::QCassandraTable::pointer_t websites_table ( context->findTable(snap::get_name(snap::SNAP_NAME_WEBSITES)) );
    //
    if( !(domains_table && websites_table) )
    {
        SNAP_LOG_FATAL() << "You must create both the 'domains' and the 'websites' tables before you can run snapserver!";
        exit( 1 );
    }

    bool const list( f_opt.is_defined( "list" ) );
    if(list)
    {
        std::cout << "List of services to start on this server:" << std::endl;
    }
    QString services(f_config["services"]);
    QStringList service_list(services.split(","));
    int const max_services(service_list.size());
    for(int idx(0); idx < max_services; ++idx)
    {
        QString service(service_list[idx].trimmed());
        f_services.push_back(service.toStdString());

        if(list)
        {
            std::cout << service.toStdString() << std::endl;
        }
    }
    if(list)
    {
        // the --list command is over!
        exit(1);
    }
}


snap_init::~snap_init()
{
}


void snap_init::create_instance( int argc, char * argv[] )
{
    f_instance.reset( new snap_init( argc, argv ) );
    Q_ASSERT(f_instance);
}


snap_init::pointer_t snap_init::instance()
{
    if( !f_instance )
    {
        throw std::invalid_argument( "snap_init instance must be created with create_instance()!" );
    }
    return f_instance;
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
        throw std::invalid_argument("Must specify at least one service in the services parameter in the configuration file");
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
    for( const auto& opt : f_services )
    {
        ss << " [" << opt << "]";
    }
    //
    SNAP_LOG_INFO(ss.str());
}


/** \brief Check if backend is ready to start.
 *
 * If the sites table has not yet been created, you don't want to
 * continue with backend processing.
 *
 * \return false means the sites table does not exist.
 */
bool snap_init::backend_ready()
{
    QtCassandra::QCassandraContext::pointer_t context( f_cassandra.get_snap_context() );
    Q_ASSERT( context );
    if( !context )
    {
        SNAP_LOG_FATAL() << "snap_websites context does not exist! Exiting.";
        exit(1);
    }

    QtCassandra::QCassandraTable::pointer_t sites_table( context->findTable( snap::get_name(snap::SNAP_NAME_SITES) ) );
    return static_cast<bool>(sites_table);
}


void snap_init::create_server_process()
{
    process::pointer_t p( new process() );
    p->set_path( f_opt.get_string("binary_path").c_str() );
    p->set_config( f_opt.get_string("config").c_str() );
    p->run();
    f_process_list.push_back( p );
}


void snap_init::create_backend_process( QString const& name )
{
    if( !backend_ready() )
    {
        SNAP_LOG_ERROR() << "The 'sites' table does not yet exist. Disabling backend--restart snapinit when the database is ready.";
        return;
    }

    process::pointer_t p( new process( name ) );
    p->set_path( f_opt.get_string("binary_path").c_str() );
    p->set_config( f_opt.get_string("config").c_str() );
    p->run();
    f_process_list.push_back( p );
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
    for( auto p : f_process_list )
    {
        p->kill();
    }
}


void snap_init::start_processes()
{
    f_lock_file.open( QFile::ReadWrite );
    for( auto opt : f_services )
    {
        //
        if( opt == "server" )
        {
            create_server_process();
        }
        else
        {
            create_backend_process( opt.c_str() );
        }
    }

    snap::server::udp_server_t udp_signals( snap::server::udp_get_server( UDP_SERVER ) );
    //
    while( true )
    {
        monitor_processes();
        //
        const std::string word( udp_signals->timed_recv( BUFSIZE, TIMEOUT ) );
        if( word == "STOP" )
        {
            terminate_processes();
            break;
        }
        sleep( 1 );
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
        const int pid = fork();
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
    SNAP_LOG_INFO() << "Stop servers";
    if( !is_running() )
    {
        throw std::runtime_error("snap_init is not running!");
    }

    snap::server::udp_ping_server( UDP_SERVER, "STOP" );

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
        f_lock_file.close();
        f_lock_file.remove();
    }
}


void snap_init::sighandler( int sig )
{
    QString signame;
    bool user_terminated = false;
    switch( sig )
    {
        case SIGSEGV : signame = "SIGSEGV"; break;
        case SIGBUS  : signame = "SIGBUS";  break;
        case SIGFPE  : signame = "SIGFPE";  break;
        case SIGILL  : signame = "SIGILL";  break;
        case SIGTERM : signame = "SIGTERM"; user_terminated = true; break;
        case SIGINT  : signame = "SIGINT";  user_terminated = true; break;
        default      : signame = "UNKNOWN";
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

    try
    {
        // First, create the static snap_init object
        //
        snap_init::create_instance( argc, argv );

        // Stop on these signals, log them, then terminate.
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

    return 0;
}

// vim: ts=4 sw=4 et
