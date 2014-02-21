// Snap Websites Server -- snap websites server
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

#include "snapwebsites.h"
#include "snap_exception.h"
#include "log.h"
#include "not_reached.h"


int main(int argc, char *argv[])
{
	try
	{
		// create a server object
		snap::server::pointer_t s( snap::server::instance() );

		// parse the command line arguments
		s->config(argc, argv);

		// if possible, detach the server
		s->detach();
		// Only the child (server) process returns here

		// Now create the qt application instance
		//
		s->prepare_qtapp( argc, argv );

		// prepare the database
		s->prepare_cassandra();

		// listen to connections
		s->listen();

		// exit via the server so the server can clean itself up properly
		s->exit(0);
	}
    catch( const snap::snap_exception& except )
    {
        SNAP_LOG_FATAL("snap_child::process(): exception caught!")(except.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_child::process(): unknown exception caught!");
    }

	exit(1);
    snap::NOTREACHED();
    return 0;
}

// vim: ts=4 sw=4
