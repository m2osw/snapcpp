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
 * \brief The fast journal client server.
 *
 * This file starts the Fast Journal Client service. This service connects
 * to the Fast Journal Daemon and sends it any batch requests as received
 * in a local file used as a very fast journal.
 */

// advgetopt
//
#include <advgetopt/advgetopt.h>



namespace fastjournal
{




class cli
{
public:
                            cli(int argc, char * argv[]);
                            cli(cli const &) = delete;

    cli &                   operator = (cli const &) = delete;

    int                     run();

private:
    advgetopt::getopt       f_opt;
};



} // fastjournal namespace
// vim: ts=4 sw=4 et
