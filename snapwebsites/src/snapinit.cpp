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
#include "snap_exception.h"
#include "log.h"
#include "snap_thread.h"
#include "not_reached.h"

#include <unistd.h>
#include <sys/wait.h>

#include <exception>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include <advgetopt/advgetopt.h>

#include <QFile>

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
            's',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "server",
            nullptr,
            "Start and keep snapserver running.",
            advgetopt::getopt::no_argument
        },
        {
            'm',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "sendmail",
            nullptr,
            "Start and keep sendmail backend running.",
            advgetopt::getopt::no_argument
        },
        {
            'p',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "pagelist",
            nullptr,
            "Start and keep pagelist backend running.",
            advgetopt::getopt::no_argument
        },
        {
            'b',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
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
            'l',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "logfile",
            "/var/log/snapwebsites/snapinit.log",
            "Full path to the snapinit logfile",
            advgetopt::getopt::optional_argument
        },
        {
            'n',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "nolog",
            nullptr,
            "Only output to the console, not the log.",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "start|restart|stop",
            advgetopt::getopt::default_argument
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

    process( const QString& command, const QStringList& arglist )
        : f_command(command)
        , f_arglist(arglist)
        , f_pid(0)
        , f_exit(0)
        , f_startcount(0)
    {
    }

    bool run();
    bool is_running();
    void kill();
    int  startcount() const { return f_startcount; }

private:
    QString     f_command;
    QStringList f_arglist;
    int         f_pid;
    int         f_exit;
    int         f_startcount;
};


bool process::run()
{
    f_startcount++;
    f_pid = fork();
    if( f_pid == 0 )
    {
        // child
        //
        std::string cmd(f_command.toUtf8().data());
        std::vector<std::string> args;
        std::vector<const char *> args_p;
        args.push_back( cmd.c_str() );
        for( auto arg : f_arglist )
        {
            args.push_back(arg.toUtf8().data());
            args_p.push_back(args.rbegin()->c_str());
        }
        args_p.push_back(NULL);

        execv(
            cmd.c_str(),
            const_cast<char * const *>(&args_p[0])
        );

        SNAP_LOG_FATAL("Child process \"") << cmd << " " << f_arglist.join(" ") << "\" failed to start!";
        exit(1);
    }

    sleep(1);
    return is_running();
}


bool process::is_running()
{
    if( f_pid == 0 )
    {
        return false;
    }

    int status = 0;
    const int pid = waitpid( f_pid, &status, WNOHANG );
    if( pid == 0 )
    {
        return true;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    if(WIFEXITED(status))
    {
        f_exit = WEXITSTATUS(status);
    }
    else
    {
        SNAP_LOG_FATAL() << "Command [" << f_command << "] terminated with error [" << WEXITSTATUS(status) << "]";
        f_exit = -1;
    }
#pragma GCC diagnostic pop
    f_pid = 0;

    return false;
}


void process::kill()
{
    if( f_pid != 0 )
    {
        ::kill( f_pid, SIGTERM );
        f_pid = 0;
    }
}


class snap_init
{
public:
    snap_init( int argc, char *argv[] );
    ~snap_init();

    void run_processes();
    bool is_running();

private:
    advgetopt::getopt   f_opt;
    typedef std::map<std::string,bool> map_t;
    map_t       f_opt_map;
    QFile       f_lock_file;

    typedef std::vector<process::pointer_t> process_list_t;
    process_list_t f_process_list;

	void usage();
    void validate();
    void show_selected_servers() const;
    void create_server_process();
    void create_backend_process( const QString& name );
    void monitor_processes();
    void terminate_processes();
    void start();
	void restart();
	void stop();
};



snap_init::snap_init( int argc, char *argv[] )
    : f_opt(argc, argv, g_snapinit_options, g_configuration_files, "SNAPINIT_OPTIONS")
    , f_lock_file( QString("/tmp/%1").arg(SNAPINIT_KEY) )
{
    if( f_opt.is_defined( "nolog" ) )
    {
        snap::logging::configureConsole();
    }
    else
    {
        snap::logging::configureLogfile( f_opt.get_string("logfile").c_str() );
    }
}


snap_init::~snap_init()
{
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
    f_opt_map = { {"server",false}, {"sendmail",false}, {"pagelist",false} };

    for( auto& opt : f_opt_map )
    {
        opt.second = f_opt.is_defined(opt.first);
    }

    if( std::find_if( f_opt_map.begin(), f_opt_map.end(), []( map_t::value_type& opt ) { return opt.second; } ) == f_opt_map.end() )
	{
        throw std::invalid_argument("Must specify at least one --server, --sendmail or --pagelist");
	}
}


void snap_init::show_selected_servers() const
{
    std::stringstream ss;
    ss << "Enabled servers: ";
    //
    for( const auto& opt : f_opt_map )
    {
        if( opt.second )
        {
            ss << "[" << opt.first << "] ";
        }
    }
    //
    SNAP_LOG_INFO() << ss.str();
}


void snap_init::create_server_process()
{
    const QString command( (f_opt.get_string("binary_path") + "/snapserver").c_str() );
    QStringList arglist;
    arglist << "-c" << f_opt.get_string("config").c_str();
    process::pointer_t p( new process( command, arglist ) );
    p->run();
    f_process_list.push_back( p );
}


void snap_init::create_backend_process( const QString& name )
{
    const QString command( (f_opt.get_string("binary_path") + "/snapbackend").c_str() );
    QStringList arglist;
    arglist << "-c" << f_opt.get_string("config").c_str() << "-a" << name;
    process::pointer_t p( new process( command, arglist ) );
    p->run();
    f_process_list.push_back( p );
}


void snap_init::monitor_processes()
{
    for( auto p : f_process_list )
    {
        if( !p->is_running() )
        {
            // TODO: need some kind of count and a timer to disable
            // processes that just won't run
            //
            // Restart process
            p->run();
        }
    }
}


void snap_init::terminate_processes()
{
    for( auto p : f_process_list )
    {
        p->kill();
    }
}


void snap_init::start()
{
	SNAP_LOG_INFO() << "Start servers";
    if( is_running() )
	{
        throw std::runtime_error("snap_init is already running!");
	}

	// fork(), then stay resident
	// Listen for STOP command on UDP port.

    const int pid = fork();
    if( pid == 0 )
    {
        f_lock_file.open( QFile::ReadWrite );
        for( auto opt : f_opt_map )
        {
            if( !opt.second ) continue;
            //
            if( opt.first == "server" )
            {
                create_server_process();
            }
            else
            {
                create_backend_process( opt.first.c_str() );
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
        }

        f_lock_file.close();
        f_lock_file.remove();

        SNAP_LOG_INFO("Normal shutdown.");
    }
    else
    {
        SNAP_LOG_INFO("Process started successfully!");
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
}


void snap_init::usage()
{
    f_opt.usage( advgetopt::getopt::no_error, "snapinit" );
    throw std::invalid_argument( "usage" );
}


int main(int argc, char *argv[])
{
	int retval = 0;

	try
	{
		snap_init init( argc, argv );
        init.run_processes();
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

// vim: ts=4 sw=4
