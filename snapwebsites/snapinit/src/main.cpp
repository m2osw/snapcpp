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

// ourselves
//
#include "snapinit.h"

// snapwebsites lib
//
#include "not_used.h"


int main(int argc, char *argv[])
{
    int retval = 0;

    // save the main process PID in case common::fatal_error() gets called.
    //
    snapinit::common::setup_fatal_pid();

    try
    {
        // First, create the static snap_init object
        //
        snapinit::snap_init::create_instance( argc, argv );

        // Now run our processes!
        //
        snapinit::snap_init::pointer_t init( snapinit::snap_init::instance() );
        init->run_processes();
    }
    catch( snap::snap_exception const & e )
    {
        snapinit::common::fatal_error(QString("snapinit: snap_exception caught! %1").arg(e.what()));
        snap::NOTREACHED();
    }
    catch( std::invalid_argument const & e )
    {
        snapinit::common::fatal_error(QString("snapinit: invalid argument: %1").arg(e.what()));
        snap::NOTREACHED();
    }
    catch( std::exception const & e )
    {
        snapinit::common::fatal_error(QString("snapinit: std::exception caught! %1").arg(e.what()));
        snap::NOTREACHED();
    }
    catch( ... )
    {
        snapinit::common::fatal_error("snapinit: unknown exception caught!");
        snap::NOTREACHED();
    }

    return retval;
}

// vim: ts=4 sw=4 et
