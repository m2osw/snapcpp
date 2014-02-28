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
#include "process.h"
#include "snap_thread.h"
#include "not_reached.h"

#include <unistd.h>

#include <exception>
#include <map>
#include <memory>
#include <vector>

#include <advgetopt/advgetopt.h>

#include <QSharedMemory>

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
            "config_file",
            "/etc/snapwebsites/snapserver.conf",
            "Configuration file to pass into servers.",
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


    const char* UDP_SERVER = "127.0.0.1:4100";
    const int BUFSIZE = 256;
    const int TIMEOUT = 1000;
}
//namespace


class snap_init
{
public:
    snap_init( int argc, char *argv[] );
    ~snap_init() {}

	static bool is_running();

private:
    advgetopt::getopt   f_opt;
    typedef std::map<std::string,bool> map_t;
    map_t f_opt_map;

    typedef std::shared_ptr<snap::process> process_t;
    typedef std::vector<process_t> process_list_t;
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


bool snap_init::is_running()
{
    // Attempt to create shared memory segment--if created, then there is another instance running...
    //  
    QSharedMemory sm("snap_init-1846faf6-a02a-11e3-884b-206a8a420cb5");
#ifdef DEBUG
    std::cout << "Native key=" << sm.nativeKey().toStdString() << std::endl;
#endif
    if( sm.create( 1, QSharedMemory::ReadOnly ) ) 
    {   
        return false;
    }   

	if( sm.error() != QSharedMemory::AlreadyExists )
	{   
		SNAP_LOG_FATAL() << "Cannot create shared memory!";
        throw std::runtime_error("Cannot create shared memory!");
	}   
	if( sm.isAttached() )
	{
		sm.detach();
	}

	return true;
}



snap_init::snap_init( int argc, char *argv[] )
    : f_opt(argc, argv, g_snapinit_options, g_configuration_files, "SNAPINIT_OPTIONS")
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
    std::cout << "Enabled servers: ";
    for( const auto& opt : f_opt_map )
    {
        if( opt.second )
        {
            std::cout << "[" << opt.first << "] ";
        }
    }
    std::cout << std::endl;
}


void snap_init::create_server_process()
{
    process_t p( new snap::process( "process::server" ) );
    p->set_mode( snap::process::PROCESS_MODE_INOUT );
    p->set_command( (f_opt.get_string("binary_path") + "/snapserver").c_str() );
    p->add_argument("-c");
    p->add_argument( f_opt.get_string("config_file").c_str() );
    p->run( false /*wait*/ );

    f_process_list.push_back( p );
}


void snap_init::create_backend_process( const QString& name )
{
    process_t p( new snap::process( "process::backend::" + name) );
    p->set_mode( snap::process::PROCESS_MODE_INOUT );
    p->set_command( (f_opt.get_string("binary_path") + "/snapbackend").c_str() );
    p->add_argument("-c");
    p->add_argument( f_opt.get_string("config_file").c_str() );
    p->add_argument("-a");
    p->add_argument( name );
    p->run( false /*wait*/ );

    f_process_list.push_back( p );
}


void snap_init::monitor_processes()
{
    for( auto p : f_process_list )
    {
        if( !p->is_running() )
        {
            // Restart process
            p->run( false /*wait*/ );
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
	if( snap_init::is_running() )
	{
        throw std::runtime_error("snap_init is already running!");
	}

	// fork(), then stay resident
	// Listen for STOP command on UDP port.

    const int pid = fork();
    if( pid == 0 )
    {
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
    }
    else
    {
        SNAP_LOG_INFO("Process started successfully!");
    }
}


void snap_init::restart()
{
	SNAP_LOG_INFO() << "Restart servers";
	if( snap_init::is_running() )
	{
		stop();
	}

	start();
}


void snap_init::stop()
{
	SNAP_LOG_INFO() << "Stop servers";
	if( !snap_init::is_running() )
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
		snap::logging::configureConsole();
		snap_init init( argc, argv );
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
