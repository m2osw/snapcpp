// Snap Communicator -- classes to ease handling communication between processes
// Copyright (C) 2012-2015  Made to Order Software Corp.
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
#pragma once

#include "snap_exception.h"

#include <event.h>


namespace snap
{

class snap_communicator_parameter_error : public snap_logic_exception
{
public:
    snap_communicator_parameter_error(std::string const & whatmsg) : snap_logic_exception(whatmsg) {}
};

class snap_communicator_initialization_error : public snap_exception
{
public:
    snap_communicator_initialization_error(std::string const & whatmsg) : snap_exception(whatmsg) {}
};




class snap_communicator
{
public:
    class snap_connection
    {
    public:
        typedef std::shared_ptr<snap_connection>    pointer_t;
        typedef std::vector<pointer_t>              vector_t;

        // virtual classes must have a virtual destructor
        virtual                     ~snap_connection() {}

        virtual int                 get_socket() const;

        virtual void                process_signal() = 0;

    private:
    };

    class snap_client_connection
            : public bio_client
            , public snap_connection
    {
    public:
                                    snap_client_connection(std::string const & addr, int port, mode_t mode = mode_t::MODE_PLAIN);

        virtual int                 get_socket() const;
    };

    // TODO: switch the tcp_server to a bio_server once available
    class snap_server_connection
            : public tcp_server
            , public snap_connection
    {
    public:
                                    snap_server_connection(std::string const & addr, int port, int max_connections = -1, bool reuse_addr = false, bool auto_close = false);

        virtual int                 get_socket() const;
    };

                                snap_communicator();

    void                        reinit(); // after a fork()

    void                        add_connection(snap_connection::pointer_t connection);

private:
    struct event_base *         f_event_base;
    snap_connection::vector_t   f_connections;
};



} // namespace snap
// vim: ts=4 sw=4 et
