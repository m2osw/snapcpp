/*
 * Copyright (c) 2020  Made to Order Software Corp.  All Rights Reserved
 *
 * https://snapwebsites.org/
 * contact@m2osw.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/** \file
 * \brief The Fast Journal Client server.
 *
 * This file starts the Fast Journal Client service. This server runs
 * on the client side and is used to send the bacth data  to the backend.
 * The service detects when the file(s) get updated, reads that data,
 * and sends it to the Fast Journal Backend service which in turn
 * sends it to a Backend for actual process once the timestamp date
 * is reached.
 */

// self
//
#include    "client.h"


// C++ lib
//
#include    <iostream>


// advgetopt lib
//
#include    <advgetopt/exception.h>


// snapdev lib
//
#include    <snapdev/poison.h>


int main(int argc, char * argv[])
{
    try
    {
        fastjournal::client s(argc, argv);

        return s.run();
    }
    catch(advgetopt::getopt_exit const & except)
    {
        return except.code();
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception occurred: " << e.what() << std::endl;
    }

    return 1;
}

// vim: ts=4 sw=4 et
