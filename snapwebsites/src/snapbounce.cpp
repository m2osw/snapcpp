/////////////////////////////////////////////////////////////////////////////////
// Snap Bounced Email Processor

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

#include "snapwebsites.h"
#include "snap_cassandra.h"
#include "snap_config.h"
#include "snap_exception.h"
#include "snap_thread.h"
#include "log.h"
#include "not_reached.h"

#include <advgetopt/advgetopt.h>

#include <QtCassandra/QCassandraContext.h>
#include <QtCassandra/QCassandraTable.h>

#include <QFile>
#include <QTime>

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <uuid/uuid.h>

#define USE_OPEN_FD
#ifdef USE_OPEN_FD
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#endif

#include <exception>
#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "qstring_stream.h"

using namespace QtCassandra;

namespace
{
    /** \brief List of configuration files.
     *
     * This variable is used as a list of configuration files. It may be
     * empty.
     */
    std::vector<std::string> const g_configuration_files = 
    {
        "/etc/snapwebsites/snapbounce.conf"
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
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "[optional] Show usage and exit.",
            advgetopt::getopt::no_argument
        },
        {
            'n',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "nolog",
            nullptr,
            "[optional] Only output to the console, not the syslog.",
            advgetopt::getopt::no_argument
        },
        {
            'c',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "config",
            "/etc/snapwebsites/snapserver.conf",
            "[optional] Configuration file from which to get cassandra server details.",
            advgetopt::getopt::optional_argument
        },
        {
            'v',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "[optional] show the version of the snapinit executable",
            advgetopt::getopt::no_argument
        },
        {
            's',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "sender",
            nullptr,
            "[required] Sender of the email.",
            advgetopt::getopt::required_argument
        },
        {
            'r',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "recipient",
            nullptr,
            "[required] Intended recipient of the email.",
            advgetopt::getopt::required_argument
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


class snap_bounce
{
public:
    typedef std::shared_ptr<snap_bounce> pointer_t;

    static void create_instance( int argc, char * argv[] );
    static pointer_t instance();
    ~snap_bounce();

    void read_stdin();
    void store_email();

private:
    static pointer_t     f_instance;
    advgetopt::getopt    f_opt;
    snap::snap_config    f_config;
    snap::snap_cassandra f_cassandra;
    //std::string          f_recipient;
    QStringList          f_email_body;

    snap_bounce( int argc, char *argv[] );

    void usage();
};


snap_bounce::pointer_t snap_bounce::f_instance;


snap_bounce::snap_bounce( int argc, char *argv[] )
    : f_opt(argc, argv, g_snapinit_options, g_configuration_files, "SNAPBOUNCE_OPTIONS")
    , f_cassandra( f_config )
{
    if(f_opt.is_defined("version"))
    {
        std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
    }

    if( f_opt.is_defined( "help" ) || !f_opt.is_defined( "sender" ) || !f_opt.is_defined( "recipient" ) )
    {
        usage();
        exit(1);
    }

    if( f_opt.is_defined( "nolog" ) || f_opt.is_defined( "help" ) )
    {
        snap::logging::configure_console();
    }
    else
    {
        snap::logging::configure_sysLog();
    }

    f_config.read_config_file( f_opt.get_string("config").c_str() );
}


snap_bounce::~snap_bounce()
{
    // Empty
}


void snap_bounce::create_instance( int argc, char * argv[] )
{
    f_instance.reset( new snap_bounce( argc, argv ) );
    Q_ASSERT(f_instance);
}


snap_bounce::pointer_t snap_bounce::instance()
{
    if( !f_instance )
    {
        throw std::invalid_argument( "snap_bounce instance must be created with create_instance()!" );
    }
    return f_instance;
}


void snap_bounce::usage()
{
    f_opt.usage( advgetopt::getopt::no_error, "snapinit" );
    //throw std::invalid_argument( "usage" );
}


void snap_bounce::read_stdin()
{
    //const std::string final_recipient( "Final-Recipient:" );
    //std::cout << "read_stdin():" << std::endl;
    f_email_body << QString("Sender:    %1").arg(f_opt.get_string("sender").c_str());
    f_email_body << QString("Recipient: %1").arg(f_opt.get_string("recipient").c_str());
    while( (std::cin.rdstate() & std::ifstream::eofbit) == 0 )
    {
        std::string line;
        std::getline( std::cin, line );
        f_email_body << line.c_str();

        #if 0
        // Attempt to extract Final-Recipient.
        // For example "Final-Recipient: rfc822; pleasebounce@dooglio.net"
        if( line.substr( 0, final_recipient.size() ) == final_recipient )
        {
            auto semicolon_it = std::find_if( line.begin(), line.end(), []( const char ch ) { return ch == ';'; } );
            if( semicolon_it != line.end() )
            {
                semicolon_it++;
                std::for_each( semicolon_it, line.end(), [this]( const char ch ) { if( ch != ' ' ) f_recipient.push_back(ch); } );
            }
        }
        #endif
    }

    #if 0
    std::cout << "recipient=" << f_recipient << std::endl;
    std::cout << std::endl << "f_email_body:" << std::endl;
    for( const QString line : f_email_body )
    {
        std::cout << "\t" << line << std::endl;
    }
    #endif
}


namespace
{
    QString generate_uuid()
    {
        uuid_t uuid;
        uuid_generate_random( uuid );
        char unique_key[37];
        uuid_unparse( uuid, unique_key );
        return unique_key;
    }
}


void snap_bounce::store_email()
{
    f_cassandra.connect();
    if( !f_cassandra.is_connected() )
    {
        throw snap::snap_exception( "Cannot connect to Cassandra!" );
    }

    // Send f_email_body's contents to cassandra, specifically in the "emails/bounced" table/row.
    //
    QCassandraContext::pointer_t context( f_cassandra.get_snap_context() );

    QCassandraTable::pointer_t table(context->findTable("emails"));
    if(!table)
    {
        // We don't want to bother with trying to create the "emails" table.
        // If it isn't there, then we'll just have to lose this email for now.
        return;
    }

    auto& bounced( (*table)["bounced"] );
    bounced[generate_uuid()] = f_email_body.join("\n");
}


int main(int argc, char *argv[])
{
    int retval = 0;

    try
    {
        // First, create the static snap_bounce object
        //
        snap_bounce::create_instance( argc, argv );

        // Now run our processes!
        //
        snap_bounce::pointer_t bounce( snap_bounce::instance() );
        bounce->read_stdin();
        bounce->store_email();
    }
    catch( snap::snap_exception const& except )
    {
        SNAP_LOG_FATAL("snap_bounce: snap_exception caught! ")(except.what());
        retval = 1;
    }
    catch( std::invalid_argument const& std_except )
    {
        SNAP_LOG_FATAL("snap_bounce: invalid argument: ")(std_except.what());
        retval = 1;
    }
    catch( std::exception const& std_except )
    {
        SNAP_LOG_FATAL("snap_bounce: std::exception caught! ")(std_except.what());
        retval = 1;
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_bounce: unknown exception caught!");
        retval = 1;
    }

    return 0;
}

// vim: ts=4 sw=4 et
