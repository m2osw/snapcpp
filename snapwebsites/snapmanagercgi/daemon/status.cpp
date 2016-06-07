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
#include "process.h"


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
    f_manager_daemon->process_message(message);
}


void status_connection::process_message_b(snap::snap_communicator_message const & message)
{
    f_manager_status->process_message(message);
}


/** \brief Initialize the manager status.
 *
 * This constructor names the runner object "manager_status". It also
 * saves a reference to the status connection object which is used
 * to (1) send new MANAGERSTATUS and (2) receive STOP when we are done
 * and the thread needs to quit.
 */
manager_status::manager_status(status_connection::pointer_t sc)
    : snap_runner("manager_status")
    , f_status_connection(sc)
{
    f_status_connection->set_thread_b(this);
}


/** \brief Thread used to permanently gather this server status.
 *
 * Each computer in the Snap! cluster should be running an instance
 * of the snapmanagerdaemon system. This will gather basic information
 * about the state of the each system and send the information to
 * all the computers who have snapmanager.cgi active.
 */
void manager_status::run()
{
    // run as long as the parent thread did not ask us to quit
    //
    for(;;)
    {
        // we may be asked to wake up immediately and at that point
        // we may notice that we are not expected to continue working
        //
        if(!continue_running() || !f_running)
        {
            return;
        }

        // wait for messages or 1 minute
        //
        f_status_connection->poll(60 * 1000000LL);

        // gather status information

        QString status("this is status from manager_status!");


        // generate a message to send the snapmanagerdaemon
        //
        snap::snap_communicator_message status_message;
        status_message.set_command("MANAGERSTATUS");
        status_message.add_parameter("status", status);
        f_status_connection->send_message(status_message);
    }
}


/** \brief Process a message sent to us by our "parent".
 *
 * This function gets called whenever the manager_daemon object
 * sends us a message.
 *
 * \param[in] message  The message to process.
 */
void manager_status::process_message(snap::snap_communicator_message const & message)
{
    QString const & command(message.get_command());

    switch(command[0].unicode())
    {
    case 'S':
        if(command == "STOP")
        {
            // this will stop the manager_status thread as soon as possible
            //
            f_running = false;
        }
        break;

    }
}






/** \brief Installs one Debian package.
 *
 * This function installs ONE package as specified by \p package_name.
 *
 * \param[in] package_name  The name of the package to install.
 * \param[in] add_info_only_if_present  Whether to add information about
 *            non installed packages in the output.
 *
 * \return The exit code of the apt-get install command.
 */
int manager_daemon::package_status(QString const & package_name, bool add_info_only_if_present)
{
    snap::process p("check status");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
    p.set_command("dpkg-query");
    p.add_argument("-W");
    p.add_argument(package_name);
    int r(p.run());

    // the output is saved so we can send it to the user and log it...
    if(r == 0)
    {
        QString const output(p.get_output(true));
        f_output += output;
        SNAP_LOG_TRACE("package status:\n")(output);
    }
    else if(!add_info_only_if_present)
    {
        // in this case the output is likely empty (i.e. we do not read
        // stderr...), so we ignore it
        //
        f_output += package_name + " is not installed";
        SNAP_LOG_TRACE("package named \"")(package_name)("\" isnot installed.");
    }

    return r;
}




void manager_daemon::status(snap::snap_communicator_message const & message)
{
    // we just send the most current status we have
    snap::snap_communicator_message reply;
    reply.set_command("MANAGEREPLY");
    reply.reply_to(message);

    // the status is actually the status of all the servers in one
    // message; we probably will want to rethink that if the number
    // of servers grows to thousands...
    //
    QString status;
    int size(0);
    for(auto const & s : f_status)
    {
        size += s.first.length() + s.second.length() + 2;
    }
    if(size == 0)
    {
        // asked too soon, we do not have any status information yet!
        //
        status = "not-available";
    }
    else
    {
        status.reserve(size);

        // Now we can create the final status
        //
        // Note that the server name (s.first) is already known to
        // include only safe characters (a-z0-9_) although we do
        // check in set_manager_status(), just in case.
        //
        // However, the status value (s.second) is checked in
        // the set_manager_status and made valid there if it
        // includes any pipe (|) character, those will have been
        // escaped with a backslash (as in "\|").
        //
        for(auto const & s : f_status)
        {
            if(status.isEmpty())
            {
                status += QString("%1=%2").arg(s.first).arg(s.second);
            }
            else
            {
                status += QString("|%1=%2").arg(s.first).arg(s.second);
            }
        }
    }

    reply.add_parameter("result", status);
    f_messenger->send_message(reply);
}


/** \brief Function called whenever the MANAGERSTATUS message is received.
 *
 * Whenever the status of a snapmanagerdaemon changes, it is sent to all
 * the other snapmanagerdaemon (and this daemon itself.)
 *
 * \param[in] message  The actual MANAGERSTATUS message.
 */
void manager_daemon::set_manager_status(snap::snap_communicator_message const & message)
{
    // TBD: should we check that the name of the sending service is one of us?
    //
    QString const server(message.get_sent_from_server());
    QString value(message.get_parameter("status"));
    value.replace("|", "\\|");

    if(server.indexOf('|') != -1
    || server.indexOf('=') != -1)
    {
        SNAP_LOG_ERROR("the name of a server cannot include the '|' or the '=' characters, \"")
                      (server)
                      ("\" is considered invalid. The message will be ignored.");
        return;
    }

    f_status[server] = value;
}


/*
 * This needs to be 100% dynamic
 *
# server_types=<list of names>
#
# List what functionality this server offers as a list of names as defined
# below.
#
# The "Comp." column (Computer) defines where such and such service should
# be running. Each set of services can run on the same computer (often does
# while doing development.) However, running everything on a single computer
# is not how Snap! was designed. The idea is to be able to run the websites
# on any number of computers in a cluster. This is why we have Cassandra,
# but really all parts can be running on any number of computers.
#
# +===============+===========================================+=======+
# |    Name       |              Description                  | Comp. |
# +===============+===========================================+=======+
# | anti-virus    | clamav along snapbackend                  | Back  |
# | application   | snapserver and clamav                     | Back  |
# | backend       | one or more snapbackend                   | Back  |
# | base          | snamanager.cgi and dependencies           | All   |
# | cassandra     | cassandra database                        | Back  |
# | firewall      | snapfirewall                              | All   |
# | frontend      | apache with snap.cgi                      | Front |
# | mailserver    | postfix with snapbounce                   | Front |
# | ntp           | time server                               | All   |
# | vpn           | tinc / openvpn                            | All   |
# | logserver     | loggingserver from log4cplus              | Back  |
# +===============+===========================================+=======+
#
# Note that the snapserver plugins come with clamav.
#
# Default: base
server_types=base
*/

} // namespace snap_manager
// vim: ts=4 sw=4 et
