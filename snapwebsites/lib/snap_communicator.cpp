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

#include "snap_communicator.h"

#include "not_reached.h"

#include "poison.h"

namespace snap
{
namespace
{


}


/** \brief Retrieve the socket attached to this connection.
 *
 * This function returns the socket currently attached to
 * this connection.
 *
 * If no socket was attached yet, the function returns -1.
 *
 * \return The currently attached socket.
 */
int snap_communicator::snap_connection::get_socket() const
{
    return f_socket;
}


/** \brief Set the socket of this connection.
 *
 * This function is used to save the connection socket internally.
 *
 * The connect may be closed in which case the socket can be set
 * to -1. Note that before closing a connection you need to
 * detach it from your snap_communicator object if currently
 * attached there.
 *
 * \param[in] s  The new socket.
 */
void snap_communicator::snap_connection::set_socket(int s)
{
    f_socket = s;
}


/** \brief Initializes the client connection.
 *
 * This function creates a connection using the address, port, and mode
 * parameters. This is very similar to using the bio_client class to
 * create a connection, only the resulting connection can be used with
 * the snap_communicator object.
 */
snap_communicator::snap_client_connection::snap_client_connection(std::string const & addr, int port, mode_t mode)
    : bio_client(addr, port, mode)
{
}


/** \brief Initialize the snap communicator.
 *
 * This function initializes the libevent library so it is ready for use.
 * (this is probably mostly to initialize the SOCK library under MS-Windows
 * but the library may also initialize a few other things and discover
 * what interface it will be using.)
 *
 * \expcetion snap_communicator_initialization_error
 * If the library cannot be initialized, then this exception is raised.
 */
snap_communicator::snap_communicator()
    : f_event_base(event_init())
{
    // verify that the event library was indeed initialized
    if(!f_event_base)
    {
        throw snap_communicator_initialization_error("snap_communicator::snap_communicator(): libevent could not be initialized");
    }
}


/** \brief Attach a connection to the communicator.
 *
 * This function attaches a connection to the communicator. This allows
 * us to execute code for that connection by having the process_signal()
 * function called.
 *
 * \param[in] connection  The connection being added.
 */
void snap_communicator::add_connection(snap_connection * connection)
{
    if(connection->get_socket() == -1)
    {
        throw snap_communicator_parameter_error("snap_communicator::add_connection(): connection without a socket cannot be added to the snap_communicator object.");
    }

    auto it(f_connections.find(connection));
    if(it != f_connections.end())
    {
        // already added, can be added only once
        return;
    }

    f_connections.push_back(connection);
}


} // namespace snap
// vim: ts=4 sw=4 et
