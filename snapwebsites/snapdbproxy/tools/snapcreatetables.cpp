// Snap Websites Server -- create the snap websites tables
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

#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/snap_cassandra.h>
#include <snapwebsites/snapwebsites.h>






int main(int argc, char * argv[])
{
    snap::NOTUSED(argc);

    try
    {
        // TODO: get a function in the library so we can support a common
        //       way to setup the logger (and always support the various
        //       command line options, the logging server, etc.)
        //
        snap::logging::set_progname(argv[0]);
        if(isatty(STDERR_FILENO))
        {
            snap::logging::configure_console();
        }
        else
        {
            // as a background process use the snapserver setup
            // (it is always available because it is in snapbase)
            //
            snap::snap_config config("snapserver");
            //if(f_opt->is_defined("config"))
            //{
            //    config.set_configuration_path(f_opt->get_string("config"));
            //}
            QString const log_config(config["log_config"]);
            if(log_config.isEmpty())
            {
                snap::logging::configure_console();
            }
            else
            {
                snap::logging::configure_conffile(log_config);
            }
        }

        snap::snap_cassandra cassandra;
        cassandra.connect();

        // Create all the missing tables from all the plugins which
        // packages are currently installed
        cassandra.create_table_list();
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception was raised: \"" << e.what() << "\"" << std::endl;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "error: an unknown exception was raised." << std::endl;
        exit(1);
    }

    return 0;
}


// vim: ts=4 sw=4 et
