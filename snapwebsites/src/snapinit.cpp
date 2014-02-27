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

#include "snap_exception.h"
#include "log.h"
#include "process.h"
#include "not_reached.h"

#include <map>

#include <advgetopt/advgetopt.h>

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
}
//namespace


class snap_init
{
public:
    snap_init( int argc, char *argv[] );
    ~snap_init() {}

private:
    advgetopt::getopt   f_opt;

	void usage();
	void validate();
	void start();
	void restart();
	void stop();
};


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
	const bool valid = ( f_opt.is_defined("server") || f_opt.is_defined("sendmail") || f_opt.is_defined("pagelist") );

	if( !valid )
	{
		throw snap::snap_exception("Must specify at least one --server, --sendmail or --pagelist");
	}
}


void snap_init::start()
{
	SNAP_LOG_INFO() << "Start servers";
}


void snap_init::restart()
{
	SNAP_LOG_INFO() << "Restart servers";
}


void snap_init::stop()
{
	SNAP_LOG_INFO() << "Stop servers";
}


void snap_init::usage()
{
    f_opt.usage( advgetopt::getopt::no_error, "snapinit" );
	throw snap::snap_exception( "usage" );
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
        SNAP_LOG_FATAL("snap_init: exception caught!")(except.what());
		retval = 1;
    }
    catch( std::exception const& std_except )
    {
        SNAP_LOG_FATAL("snap_init: exception caught!")(std_except.what());
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
