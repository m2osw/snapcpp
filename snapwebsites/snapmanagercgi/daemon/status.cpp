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
    reply.set_command("SERVERSTATUS");
    reply.reply_to(message);
    reply.add_parameter("status", "TODO");
    f_messenger->send_message(reply);
    return;
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
# +===============+===========================================+=======+
#
# Note that the snapserver plugins come with clamav.
#
# Default: base
server_types=base
*/

} // namespace snap_manager
// vim: ts=4 sw=4 et
