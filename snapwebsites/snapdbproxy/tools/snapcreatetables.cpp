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
        snap::logging::set_progname(argv[0]);
        snap::logging::configure_console();

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
