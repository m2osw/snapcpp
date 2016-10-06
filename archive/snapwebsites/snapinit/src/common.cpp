/////////////////////////////////////////////////////////////////////////////////
// Snap Init Server -- snap initialization server
// Copyright (C) 2011-2016  Made to Order Software Corp.
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
//
// This server reads in a configuration file and keeps specified services running.
// When signaled, it will terminate those services cleanly.
/////////////////////////////////////////////////////////////////////////////////

// ourselves
//
#include "common.h"
#include "snapinit.h"

// our library
//
#include <log.h>

// C++ library
//
#include <iostream>

// C library
//
#include <syslog.h>


namespace snapinit
{
namespace common
{


/** \brief The PID of the main snapinit application.
 *
 * When in the fork()'ed process, we may end up calling the fatal_error()
 * function, which is fatal for the child, but not for the snapinit process
 * itself.
 *
 * However, as a result, we would delete the PID file which is also our
 * file used to lock the snapinit process (prevent two instances from
 * running simultaneously on the same computer.) So we have this PID
 * to check and make sure we call the snapinit->exit() function only
 * if we are in the main process, not a child.
 */
pid_t g_main_snapinit_pid = -1;



/** \brief Check whether the standard output stream is a TTY.
 *
 * This function defines whether 'stderr' is a TTY or not. If not
 * we assume that we were started as a deamon and we do not spit
 * out errors in stderr. If it is a TTY, then we also print a
 * message in the console making it easier to right away know
 * that the tool detected an error and did not start in the
 * background.
 */
bool is_a_tty()
{
    return ::isatty(STDERR_FILENO);
}


/** \brief Output a fatal error message.
 *
 * In most cases you want to call fatal_error() which prints out
 * a fatal error message and then exits the snapinit process.
 *
 * This function can be called whenever the fatal error is followed
 * by a terminate_services() call because we assume that we can still
 * cleanly terminate snapinit.
 *
 * \param[in] msg  The fatal message to output.
 */
void fatal_message(QString const & msg)
{
    // output in regular logs
    //
    SNAP_LOG_FATAL(msg);

    QByteArray const utf8(msg.toUtf8());

    // output in syslog as this is a rather important problem in snapinit
    //
    syslog( LOG_CRIT, "%s", utf8.data() );

    // if stderr is a TTY, also send it to stderr
    //
    if(common::is_a_tty())
    {
        std::cerr << "snapinit: fatal error: " << utf8.data() << std::endl;
    }
}


/** \brief Generate a fatal error.
 *
 * This function prints out the specified message using the fatal_message()
 * function and then calls exit() to stop the process.
 *
 * If snap_init has an instance, then the snap_init->exit() function is
 * used to make sure we get everything cleaned up as expected (i.e. the
 * lockfile needs to be unlocked.)
 *
 * This function never returns.
 *
 * \param[in] msg  The fatal error message to print out before exiting.
 */
void fatal_error(QString const & msg)
{
    fatal_message(msg);
    snap_init::pointer_t si( snap_init::instance() );
    if(si && getpid() == g_main_snapinit_pid)
    {
        // call this one only if we are the main snapinit process
        // and an instance was created
        //
        si->exit(1);
    }
    else
    {
        // direct exit!
        //
        exit(1);
    }
    snap::NOTREACHED();
}


/** \brief Called from the main() function to save the main snapinit PID.
 *
 * This function is called by the main() function. It saves the process
 * PID in a variable which is reused by the fatal_error() function to
 * make sure that the snapinit->exit() function gets called only by
 * the snapinit process.
 *
 * That means a child process as created by the process.cpp implementation
 * will not inadvertendly call that function which has the side effect
 * of deleting the PID file used to lock the snapinit process.
 */
void setup_fatal_pid()
{
    g_main_snapinit_pid = getpid();
}



} // namespace common
} // namespace snapinit
// vim: ts=4 sw=4 et
