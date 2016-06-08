/*
 * Text:
 *      status_connection.cpp
 *
 * Description:
 *      The implementation of the status connection between the main
 *      application and the status thread (an inter-thread connection).
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
//#include "process.h"

// C++ lib
//
//#include <fstream>

// C lib
//
//#include <fcntl.h>
//#include <sys/file.h>


namespace snap_manager
{


status_connection::status_connection(manager_daemon * md)
    : f_manager_daemon(md)
{
}


void status_connection::set_thread_b(manager_status * ms)
{
    f_manager_status = ms;

    snap::snap_communicator_message thread_ready;
    thread_ready.set_command("THREADREADY");
    send_message(thread_ready);
}


void status_connection::process_message_a(snap::snap_communicator_message const & message)
{
    // here we just received a message from the thread...
    //
    // if that message is MANAGERSTATUS, then it is expected to
    // be sent to all the computers in the cluster, not just this
    // computer, only the inter-thread connection does not allow
    // for broadcasting (i.e. the message never leaves the
    // snapmanagerdaemon process with that type of connection!)
    //
    // so here we check for the name of the service to where the
    // message is expected to arrive; if not empty, we instead
    // send the message to snapcommunicator
    //
    if(message.get_service().isEmpty()
    || message.get_service() == "snapmanagerdaemon")
    {
        f_manager_daemon->process_message(message);
    }
    else
    {
        f_manager_daemon->forward_message(message);
    }
}


void status_connection::process_message_b(snap::snap_communicator_message const & message)
{
    f_manager_status->process_message(message);
}





} // namespace snap_manager
// vim: ts=4 sw=4 et
