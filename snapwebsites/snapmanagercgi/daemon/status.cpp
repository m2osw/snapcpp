/*
 * Text:
 *      status.cpp
 *
 * Description:
 *      The implementation of the STATUS function.
 *
 * License:
 *      Copyright (c) 2016 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


// ourselves
//
#include "snapmanagerdaemon.h"

// our lib
//
#include "log.h"

// C lib
//
#include <sys/file.h>


namespace snap_manager
{


namespace
{

// TODO: make a common header file...
char const status_file_magic[] = "Snap! Status v1\n";

}
// no name namespace






/** \brief Function called whenever the MANAGERSTATUS message is received.
 *
 * Whenever the status of a snapmanagerdaemon changes, it is sent to all
 * the other snapmanagerdaemon (and this daemon itself.)
 *
 * \param[in] message  The actual MANAGERSTATUS message.
 */
void manager_daemon::set_manager_status(snap::snap_communicator_message const & message)
{
    QString const server(message.get_sent_from_server());
    QString const status(message.get_parameter("status"));

    server_status s(f_data_path, server);

    // get this snapcommunicator status in our server_status object
    //
    if(!s.from_string(status))
    {
        return;
    }

    // convert a few parameter to header parameters so they can be loaded
    // first without having to load the entire file (which can become
    // really big with time and additional packages to manage)
    //
    snap_manager::status_t header_status(s.get_field_status("self", "status"));
    header_status.set_plugin_name("header");
    s.set_field(header_status);

    snap_manager::status_t header_ip(s.get_field_status("self", "ip"));
    header_ip.set_plugin_name("header");
    s.set_field(header_ip);

    if(!s.write())
    {
        return;
    }

    // keep a copy of our own information
    //
    //if(server == f_server_name)
    //{
    //    f_status = s;
    //}
}



} // namespace snap_manager
// vim: ts=4 sw=4 et
