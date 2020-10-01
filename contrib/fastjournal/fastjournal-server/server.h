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
 * \brief The fast journal backend server.
 *
 * This file starts the Fast Journal Backend service. This server accepts
 * connections from clients to receive requests for batch work.
 */

// advgetopt
//
#include    <advgetopt/advgetopt.h>


// eventdispatcher lib
//
#include    <eventdispatcher/signal_handler.h>



namespace fastjournal
{




class server
{
public:
                            server(int argc, char * argv[]);
                            server(server const &) = delete;

    server &                operator = (server const &) = delete;

    int                     run();

private:
    advgetopt::getopt               f_opt;
    ed::signal_handler::pointer_t   f_signal_handler = ed::signal_handler::pointer_t();
};



} // fastjournal namespace
// vim: ts=4 sw=4 et
