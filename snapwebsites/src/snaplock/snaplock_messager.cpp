/*
 * Text:
 *      snaplock_messager.cpp
 *
 * Description:
 *      A daemon to synchronize processes between any number of computers
 *      by blocking all of them but one.
 *
 *      The messager implementation listens for messages from other
 *      services. It understands the basic messages as well as the
 *      LOCK and other messages implemented by the snaplock implementation
 *      (since snaplock daemons communicate between each others.)
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
#include "snaplock.h"

// our lib
//
#include "log.h"



/** \class snaplock_messager
 * \brief Handle messages from the Snap Communicator server.
 *
 * This class is an implementation of the TCP client message connection
 * so we can handle incoming messages.
 */



/** \brief The messager initialization.
 *
 * The messager is a connection to the snapcommunicator server.
 *
 * In most cases we receive BLOCK, STOP, and LOG messages from it. We
 * implement a few other messages too (HELP, READY...)
 *
 * We use a permanent connection so if the snapcommunicator restarts
 * for whatever reason, we reconnect automatically.
 *
 * \note
 * The messager connection used by the snapfirewall tool makes use
 * of a thread. You will want to change this initialization function
 * if you intend to fork() direct children of ours (i.e. not fork()
 * + execv() as we do to run iptables.)
 *
 * \param[in] sfw  The snap firewall server we are listening for.
 * \param[in] addr  The address to connect to. Most often it is 127.0.0.1.
 * \param[in] port  The port to listen on (4040).
 */
snaplock_messager::snaplock_messager(snaplock * sl, std::string const & addr, int port)
    : snap_tcp_client_permanent_message_connection(addr, port)
    , f_snaplock(sl)
{
    set_name("snaplock messager");
}


/** \brief Pass messages to the Snap Firewall.
 *
 * This callback is called whenever a message is received from
 * Snap! Communicator. The message is immediately forwarded to the
 * snap_firewall object which is expected to process it and reply
 * if required.
 *
 * \param[in] message  The message we just received.
 */
void snaplock_messager::process_message(snap::snap_communicator_message const & message)
{
    f_snaplock->process_message(message);
}


/** \brief The messager could not connect to snapcommunicator.
 *
 * This function is called whenever the messagers fails to
 * connect to the snapcommunicator server. This could be
 * because snapcommunicator is not running or because the
 * configuration information for the snaplock is wrong...
 *
 * With snapinit the snapcommunicator should always already
 * be running so this error should not happen once everything
 * is properly setup.
 *
 * \param[in] error_message  An error message.
 */
void snaplock_messager::process_connection_failed(std::string const & error_message)
{
    SNAP_LOG_ERROR("connection to snapcommunicator failed (")(error_message)(")");

    // also call the default function, just in case
    snap_tcp_client_permanent_message_connection::process_connection_failed(error_message);
}


/** \brief The connection was established with Snap! Communicator.
 *
 * Whenever the connection is established with the Snap! Communicator,
 * this callback function is called.
 *
 * The messager reacts by REGISTERing the snapdbproxy with the Snap!
 * Communicator.
 */
void snaplock_messager::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    snap::snap_communicator_message register_snapdbproxy;
    register_snapdbproxy.set_command("REGISTER");
    register_snapdbproxy.add_parameter("service", "snaplock");
    register_snapdbproxy.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_snapdbproxy);
}


// vim: ts=4 sw=4 et
