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

#include "service.h"
#include "snapinit.h"
#include "log.h"
#include "not_used.h"

#include <QFile>

#include <fcntl.h>
#include <grp.h>
#include <proc/sysinfo.h>
#include <pwd.h>
#include <syslog.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <exception>
#include <map>
#include <sstream>
#include <vector>


namespace common
{
    /** \brief Define whether the standard output stream is a TTY.
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

    void fatal_error(QString msg)
    {
        SNAP_LOG_FATAL(msg);
        QByteArray utf8(msg.toUtf8());
        syslog( LOG_CRIT, "%s", utf8.data() );
        if(common::is_a_tty())
        {
            std::cerr << "snapinit: fatal error: " << utf8.data() << std::endl;
        }
        exit(1);
        snap::NOTREACHED();
    }
}


namespace
{
    int64_t const SNAPINIT_START_DELAY     = 1000000LL;
    int64_t const SNAPINIT_STOP_DELAY      = 1000000LL;
    int64_t const SNAPINIT_LONG_STOP_DELAY = 10LL * SNAPINIT_STOP_DELAY;
}


/////////////////////////////////////////////////
// SERVICE (class implementation)              //
/////////////////////////////////////////////////

/** \brief Initialize the service object.
 *
 * The constructor initializes the service object. It saves the pointer
 * back to the snap_init object as a weak pointer.
 *
 * It also initializes the snapcommunicator timer which is used whenever
 * we want to wake up this service to run it. The timer is disabled by
 * default to avoid starting this up in the wrong order.
 *
 * \param[in] si  The parent snap_init object.
 */
service::service( std::shared_ptr<snap_init> si )
    : snap_timer(SNAPINIT_START_DELAY) // wake up once per second by default
    , f_snap_init(si)
{
    // by default our timer is turned off
    //
    set_enable(false);

    // timer has a low priority (runs last)
    //
    set_priority(100);

    // Set up the event-based function map
    //
    init_functions();

    // Queue up starting process...
    //
    set_starting();
}


/** \brief Shared from this to return the correct type of pointer_t.
 *
 * Reimplemented shared_from_this() so I can get the correct type
 * of pointer without having to cast every single time I use
 * that function.
 *
 * \return A shared pointer to a server::pointer_t.
 */
service::pointer_t service::shared_from_this() const
{
    return std::dynamic_pointer_cast<service>(const_cast<service *>(this)->snap_timer::shared_from_this());
}


/** \brief Retrieve parameters about this service from e.
 *
 * This function configures this service object from the data defined
 * in DOM element \p e.
 *
 * The \p binary_path parameter is used to calculate the f_full_path
 * parameter which is expected to represent the full path to the
 * binary to execute. By default that parameter is the empty string.
 * In general, it is only set by a developer to specify his
 * development directory with the --binary-path command line option.
 *
 * The \p debug flag defines whether debug should be turned on in
 * the service or not. By default debug is turned off. To turn it
 * on, use the --debug command line option.
 *
 * \param[in] e  The element with configuration information for this service.
 * \param[in] binary_path  The path to your binaries (practical for developers).
 * \param[in] debug  Whether to start the services in debug mode.
 * \param[in] ignore_path_check  Whether the path check generate a fatal
 *            error (false) or just a warning (true)
 */
void service::configure(QDomElement e, QString const & binary_path, bool const debug, bool const ignore_path_check)
{
    // the XML does not overwrite this flag, but it can force debug
    // by using --debug in the list of <options>
    //
    f_debug = debug;

    // first make sure we have a name for this service
    //
    f_service_name = e.attribute("name");
    if(f_service_name.isEmpty())
    {
        common::fatal_error("the \"name\" parameter of a service must be defined and not empty.");
        snap::NOTREACHED();
    }

    // if a required service fails then snapinit fails as a whole
    //
    f_required = e.attributes().contains("required");

    // by default the command is one to one like the name of the service
    f_command = f_service_name;

    // check to see whether the user specified a specific command
    {
        QDomElement const sub_element(e.firstChildElement("command"));
        if(!sub_element.isNull())
        {
            f_command = sub_element.text();
            if(f_command.isEmpty())
            {
                common::fatal_error(QString("the command tag of service \"%1\" returned an empty string which does not represent a valid command.")
                                .arg(f_service_name));
                snap::NOTREACHED();
            }
        }
    }

    // user may specify a wait to use before moving forward with the next
    // item (i.e. wait on snapcommunicator before trying to connect to it)
    //
    {
        QDomElement const sub_element(e.firstChildElement("wait"));
        if(!sub_element.isNull())
        {
            if(sub_element.text() == "none")
            {
                // this is the default anyway
                //
                f_wait_interval = 0;
            }
            else
            {
                bool ok(false);
                f_wait_interval = sub_element.text().toInt(&ok, 10);
                if(!ok)
                {
                    common::fatal_error(QString("the wait tag of service \"%1\" returned an invalid decimal number.").arg(f_service_name));
                    snap::NOTREACHED();
                }
                if(f_wait_interval < 0 || f_wait_interval > 3600)
                {
                    common::fatal_error(QString("the wait tag of service \"%1\" cannot be a negative number or more than 3600.").arg(f_service_name));
                    snap::NOTREACHED();
                }
            }
        }
    }

    // user may specify a wait to use before trying again after a "hard"
    // failure... if a service crashes, there is generally no point in
    // trying to run it again and again. So we do that only 5 times and
    // after the 5th attempt we instead lose that service. You may instead
    // setup a recovery parameter to sleep on it for a while and try yet
    // again...
    //
    {
        QDomElement const sub_element(e.firstChildElement("recovery"));
        if(!sub_element.isNull())
        {
            if(sub_element.text() == "none")
            {
                // this is the default anyway
                //
                f_recovery = 0;
            }
            else
            {
                bool ok(false);
                f_recovery = sub_element.text().toInt(&ok, 10);
                if(!ok)
                {
                    common::fatal_error(QString("the wait tag of service \"%1\" returned an invalid decimal number.").arg(f_service_name));
                    snap::NOTREACHED();
                }
                if(f_recovery < 60 || f_recovery > 86400 * 7)
                {
                    common::fatal_error(QString("the wait tag of service \"%1\" cannot be less than 60 or more than 604800 (about 1 week.) Used 'none' to turn off the recovery feature.").arg(f_service_name));
                    snap::NOTREACHED();
                }
            }
        }
    }

    // user may specify a safe tag, in that case we have to wait for
    // a SAFE message with the same name as the one specified in this
    // safe tag
    //
    {
        QDomElement const sub_element(e.firstChildElement("safe"));
        if(!sub_element.isNull())
        {
            f_safe_message = sub_element.text();
            if(f_safe_message == "none")
            {
                // "none" is equivalent to nothing which is the default
                //
                f_safe_message.clear();
            }
        }
    }

    // get the core dump file size limit
    //
    {
        QDomElement const sub_element(e.firstChildElement("coredump"));
        if(!sub_element.isNull())
        {
            if(sub_element.text() == "none")
            {
                // this is the default anyway
                //
                f_coredump_limit = 0;
            }
            if(sub_element.text() == "infinity")
            {
                // save the entire process data when the crash occurs
                //
                f_coredump_limit = RLIM_INFINITY;
            }
            else
            {
                // allow a size specification (kb, mb, gb)
                //
                int64_t multiplicator(1);
                QString size(sub_element.text().toLower());
                if(size.endsWith("kb"))
                {
                    size = size.mid(0, size.length() - 2);
                    multiplicator = 1024;
                }
                else if(size.endsWith("mb"))
                {
                    size = size.mid(0, size.length() - 2);
                    multiplicator = 1024 * 1024;
                }
                else if(size.endsWith("gb"))
                {
                    size = size.mid(0, size.length() - 2);
                    multiplicator = 1024 * 1024 * 1024;
                }
                bool ok(false);
                f_coredump_limit = size.toLongLong(&ok, 10) * multiplicator;
                if(!ok)
                {
                    common::fatal_error(QString("the coredump tag of service \"%1\" is not a valid decimal number, optionally followed by \"kb\", \"mb\", or \"gb\".").arg(f_service_name));
                    snap::NOTREACHED();
                }
                if(f_coredump_limit < 1024)
                {
                    // the size of 1024 is hard coded from Linux ulimit
                    // which has the following table:
                    //
                    //      static RESOURCE_LIMITS limits[] = {
                    //      #ifdef RLIMIT_CORE
                    //      { 'c',        RLIMIT_CORE,  1024,     "core file size",      "blocks" },
                    //      #endif
                    //      ...
                    //
                    // I tested and indeed blocks are 1024 bytes under Ubuntu
                    // 14.04 and is not likely to change. It is also in the
                    // bash documentation under ulimit:
                    //
                    //      Values are in 1024-byte increments, except for
                    //      -t, which is in seconds; -p, which is in units
                    //      of 512-byte blocks; and -T, -b, -n, and -u,
                    //      which are unscaled values.
                    //
                    // See: https://lists.gnu.org/archive/html/bug-bash/2007-10/msg00010.html
                    //
                    common::fatal_error(QString("the coredump tag of service \"%1\" cannot be less than one memory block (1024 bytes.) Right now it is set to: %2 bytes")
                                            .arg(f_service_name).arg(f_coredump_limit));
                    snap::NOTREACHED();
                }
                // keep the value in blocks, rounded up
                //
                f_coredump_limit = (f_coredump_limit + 1023) / 1024;
            }
        }
    }

    // check to see whether the user specified command line options
    //
    {
        QDomElement const sub_element(e.firstChildElement("options"));
        if(!sub_element.isNull())
        {
            f_options = sub_element.text();
        }
    }

    // check for a priority; the default is DEFAULT_PRIORITY (50), the user can change it
    //
    {
        QDomElement const sub_element(e.firstChildElement("priority"));
        if(!sub_element.isNull())
        {
            bool ok(false);
            f_priority = sub_element.text().toInt(&ok, 10);
            if(!ok)
            {
                common::fatal_error(QString("priority \"%1\" of service \"%2\" returned a string that does not represent a valid decimal number.")
                                .arg(sub_element.text()).arg(f_service_name));
                snap::NOTREACHED();
            }
            if(f_priority < -100 || f_priority > 100)
            {
                common::fatal_error(QString("priority \"%1\" of service \"%2\" is out of bounds, we accept a priority between -100 and +100.")
                                .arg(sub_element.text()).arg(f_service_name));
                snap::NOTREACHED();
            }
        }
    }

    // filename of this service configuration file
    // (if not specified here, then we do not specify anything on the
    // command line in that regard, so the default will be used)
    //
    {
        QDomElement sub_element(e.firstChildElement("config"));
        if(!sub_element.isNull())
        {
            f_config_filename = sub_element.text();
            if(f_config_filename.isEmpty())
            {
                common::fatal_error(QString("the config tag of service \"%1\" returned an empty string which does not represent a valid configuration filename.")
                                    .arg(f_service_name));
                snap::NOTREACHED();
            }
        }
    }

    // whether we should connect ourselves after that service was started
    //
    {
        QDomElement const sub_element(e.firstChildElement("connect"));
        if(!sub_element.isNull())
        {
            QString const addr_port(sub_element.text());
            if(addr_port.isEmpty())
            {
                common::fatal_error(QString("the <connect> tag of service \"%1\" returned an empty string which does not represent a valid IP and port specification.")
                                .arg(f_service_name));
                snap::NOTREACHED();
            }
            f_snapcommunicator_addr = "127.0.0.1";
            f_snapcommunicator_port = 4040;
            tcp_client_server::get_addr_port(addr_port, f_snapcommunicator_addr, f_snapcommunicator_port, "tcp");
            if(f_snapcommunicator_addr != "127.0.0.1")
            {
                SNAP_LOG_WARNING("the address to connect to snapcommunicator is always expected to be 127.0.0.1 and not ")(f_snapcommunicator_addr)(".");
            }

            // at this time the snapcommunicator is immediately considered
            // registered
            //
            f_registered = true;
        }
    }

    // whether we are running a snapdbproxy
    //
    {
        QDomElement const sub_element(e.firstChildElement("snapdbproxy"));
        if(!sub_element.isNull())
        {
            QString const addr_port(sub_element.text());
            if(addr_port.isEmpty())
            {
                common::fatal_error(QString("the <snapdbproxy> tag of service \"%1\" returned an empty string which does not represent a valid IP and port specification.")
                                .arg(f_service_name));
                snap::NOTREACHED();
            }
            f_snapdbproxy_addr = "127.0.0.1";
            f_snapdbproxy_port = 4042;
            tcp_client_server::get_addr_port(addr_port, f_snapdbproxy_addr, f_snapdbproxy_port, "tcp");
        }
    }

    // tasks that need to be run once in a while uses a <cron> tag
    //
    {
        QDomElement sub_element(e.firstChildElement("cron"));
        if(!sub_element.isNull())
        {
            if(sub_element.text() == "off")
            {
                f_cron = 0;
            }
            else
            {
                bool ok(false);
                f_cron = sub_element.text().toInt(&ok, 10);
                if(!ok)
                {
                    common::fatal_error(QString("the cron tag of service \"%1\" must be a valid decimal number representing a number of seconds to wait between each execution.")
                                          .arg(f_service_name));
                    snap::NOTREACHED();
                }
                // we function like anacron and know when we have to run
                // (i.e. whether we missed some prior runs) so very large
                // cron values will work just as expected (see /var/spool/snap/*)
                //
                // TBD: offer a similar syntax to crontab? frankly we are not
                //      trying to replace cron and at this time we have just
                //      one service that runs every 5 min. so here...
                //
                if(f_cron < 60
                || f_cron > 86400 * 367)
                {
                    common::fatal_error(QString("the cron tag of service \"%1\" must be a number between 60 (1 minute) and 31708800 (a little over 1 year in seconds).")
                                  .arg(f_service_name));
                    snap::NOTREACHED();
                }
            }
        }
    }

    // non-priv user to drop to after child has forked
    // (if empty, then we stay at the user level we were at)
    //
    {
        QDomElement sub_element(e.firstChildElement("user"));
        if(!sub_element.isNull())
        {
            f_user = sub_element.text();
            if(f_user.isEmpty())
            {
                common::fatal_error(QString("the config tag of service \"%1\" returned an empty string which does not represent a valid configuration filename.")
                                    .arg(f_service_name));
                snap::NOTREACHED();
            }
        }
    }

    // non-priv group to drop to after child has forked
    // (if empty, then we stay at the user level we were at)
    //
    {
        QDomElement sub_element(e.firstChildElement("group"));
        if(!sub_element.isNull())
        {
            f_group = sub_element.text();
            if(f_group.isEmpty())
            {
                common::fatal_error(QString("the config tag of service \"%1\" returned an empty string which does not represent a valid configuration filename.")
                                    .arg(f_service_name));
                snap::NOTREACHED();
            }
        }
    }

    // Get the list of dependencies that must be started first.
    //
    {
        f_dep_name_list.clear();
        //
        QDomElement sub_element(e.firstChildElement("dependencies"));
        if( !sub_element.isNull() )
        {
            QDomNode n( sub_element.firstChild() );
            while( !n.isNull() )
            {
                if( n.isElement() )
                {
                    QDomElement subelm(n.toElement());
                    if( subelm.tagName() == "dependency" )
                    {
                        f_dep_name_list << subelm.text();
                    }
                }
                //
                n = n.nextSibling();
            }
        }
    }

    // compute the full path to the binary
    //
    // note: f_command cannot be empty here
    //
    if(f_command[0] != '/')
    {
        snap::snap_string_list paths(binary_path.split(':'));
        for(auto const & p : paths)
        {
            // sub-folder (for snapdbproxy and snaplock while doing development, maybe others later)
            {
                f_full_path = QString("%1/%2/%3").arg(p).arg(f_command).arg(f_command);
                QFile file(f_full_path);
                if(file.exists())
                {
                    goto done;
                }
            }
            // direct
            {
                f_full_path = QString("%1/%2").arg(p).arg(f_command);
                QFile file(f_full_path);
                if(file.exists())
                {
                    goto done;
                }
            }
        }

        if(!ignore_path_check)
        {
            common::fatal_error(QString("could not find \"%1\" in any of the paths \"%2\".")
                          .arg(f_service_name)
                          .arg(binary_path));
            snap::NOTREACHED();
        }

        // okay, we do not completely ignore the fact that we could
        // not find the service, but we do not generate a fatal error
        //
        SNAP_LOG_WARNING("could not find \"")
                      (f_service_name)
                      ("\" in any of the paths \"")
                      (binary_path)
                      ("\".");
done:;
    }
    else
    {
        f_full_path = f_command;
    }

    // the XML configuration worked, create a timer too
    //
    set_name(f_service_name + " timer");

    if(cron_task())
    {
        compute_next_tick(false);
    }
}


void service::get_prereqs_list()
{
    if( f_prereqs_list.empty() )
    {
        const auto snap_init( f_snap_init.lock() );
        snap_init->get_prereqs_list( f_service_name, f_prereqs_list );
    }
}


void service::get_depends_list()
{
    if( f_dep_name_list.isEmpty() )
    {
        return;
    }

    if( f_depends_list.empty() )
    {
        auto const snap_init( f_snap_init.lock() );
        for( auto const & service_name : f_dep_name_list )
        {
            auto const svc( snap_init->get_service( service_name ) );
            if( !svc )
            {
                common::fatal_error( QString("Dependency service '%1' not found!").arg(service_name) );
                snap::NOTREACHED();
            }
            f_depends_list.push_back( svc );
        }
    }
}


void service::push_state( const state_t state )
{
    if( f_queue.front().first == state )
    {
        // No need to constantly queue states already queued...
        return;
    }

    SNAP_LOG_TRACE("service::push_state() state '")(state_to_string(state))("' for service: ")(f_service_name)(", queue size b4 push=")(f_queue.size());
    f_previous_state = f_current_state;
    f_current_state  = state;
    f_queue.emplace( *(f_func_map.find(state)) );
}


/** \brief Process a timeout on a connection.
 *
 * This function should probably be cut into a few sub-functions. It
 * handles all the time out callbacks from snapcommunicator. These
 * are used to start and stop services.
 *
 * \li Start process
 *
 * If a connection is required, then that service is started and
 * then a connection setup. Once the connection is available, we
 * send a CONNECT message and wait on the ACCEPT response. If all
 * of that worked, we wake up all the other processes. In this case
 * we use the timer twice: once to start the connection process
 * and once to attempt to connect with a TCP socket. If the TCP
 * connection fails, the timer quicks in again and we attempt
 * that connection again.
 *
 * When the process to start is not one that requires a connection,
 * we just call run() on them.
 *
 * Once started a process generally does not require a timer so
 * it gets disabled. However, a cron task will instead get a
 * timeout setup to its next tick. If that tick happens while
 * the process is still running, then the tick is skipped and
 * the next one is calculated.
 *
 * \li Stop process
 *
 * When the process was asked to stop (i.e. the snapinit process
 * sent a STOP message to the snapcommunicator,) this function
 * sends the signal f_stopping using kill(). At first, the signal
 * is SIGTERM and then SIGKILL. If both signals fail to stop
 * the process, we ignore the failure and quit anyway.
 */
void service::process_timeout()
{
    if( f_queue.empty() )
    {
        //SNAP_LOG_TRACE("service::process_timeout() queue empty for service '")(f_service_name)("'");
        return;
    }

    auto f( f_queue.front() );
    SNAP_LOG_TRACE("service::process_timeout() processing state '")(state_to_string(f.first))("' for service: ")(f_service_name)(", size=")(f_queue.size());
    f_queue.pop();
    f.second();
}


/** \brief Check to see if the service has stopped running.
 *
 * \note if the service never started, this will return false.
 *
 * \sa mark_service_as_dead()
 */
bool service::service_may_have_died() const
{
    // if this process was not even started, it could not have died
    //
    if( !f_started )
    {
        return false;
    }

    // no matter what, if we are still running, there is nothing
    // for us to do here
    //
    if( is_running() )
    {
        return false;
    }

    return true;
}


/** \brief Take a service out which is no longer running.
 *
 * \sa service_may_have_died()
 */
void service::mark_service_as_dead()
{
    SNAP_LOG_TRACE("service::mark_service_as_stopped(): service='")(f_service_name)("'.");

    f_started = false;

    // if this was a service with a connection (snapcommunicator) then
    // we indicate that it died
    //
    if( is_connection_required() )
    {
        snap_init::pointer_t si(f_snap_init.lock());
        if(!si)
        {
            // Sanity check...TODO: perhaps this should throw?
            SNAP_LOG_ERROR("cron service \"")(f_service_name)("\" lost its parent snapinit object.");
            return;
        }

        si->service_down(shared_from_this());
    }

    mark_process_as_dead();
}


bool service::is_dependency_of( const QString& service_name )
{
    return f_dep_name_list.contains(service_name);
}


void service::mark_process_as_stopped( const bool from_set_stopping )
{
    SNAP_LOG_TRACE("service::mark_process_as_stopped(): service='")(f_service_name)("' has been terminated; removing from communicator. from_set_stopping=")(from_set_stopping);

    // clearly mark that the service is dead
    //
    f_stopping = SIGCHLD;

    // if we are not running anymore,
    // remove self (timer) from snapcommunicator
    //
    remove_from_communicator();

    if( from_set_stopping )
    {
        const auto snap_init( f_snap_init.lock() );
        if(!snap_init)
        {
            common::fatal_error("somehow we could not get a lock on f_snap_init from a service object.");
            snap::NOTREACHED();
        }
        snap_init->remove_terminated_services();
    }
}


void service::mark_process_as_dead()
{
    SNAP_LOG_TRACE("marking service '")(f_service_name)("' as dead.");

    const auto snap_init( f_snap_init.lock() );
    if(!snap_init)
    {
        common::fatal_error("somehow we could not get a lock on f_snap_init from a service object.");
        snap::NOTREACHED();
    }

    // do we know we sent the STOP signal? if so, remove outselves
    // from snapcommunicator
    //
    if( f_stopping != 0 )
    {
        if( f_restart_requested )
        {
            f_stopping = 0;
            f_failed = 0;
            push_state( state_t::waiting_for_deps );
        }
        else
        {
            mark_process_as_stopped( false );
        }
        return;
    }

    // if it is the cron task, that is normal, the timer of the
    // cron task is already set as expected so ignore too
    //
    if( cron_task() )
    {
        SNAP_LOG_TRACE("service='")(f_service_name)("' is a cron task. Ignoring.");
        return;
    }

    // if the service is not yet marked as failed, check whether
    // we have to increase the short run count
    //
    if( !failed() )
    {
        int64_t const now(snap::snap_child::get_current_date());
//std::cerr << "*** process " << f_service_name << " died, now/start " << now << "/" << f_start_date << ", time interval is " << (now - f_start_date) << ", counter: " << f_short_run_count << "\n";
        if(now - f_start_date < MAX_START_INTERVAL)
        {
            ++f_short_run_count;

            // too many short runs means this service failed
            //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
            f_failed = f_short_run_count >= MAX_START_COUNT;
#pragma GCC diagnostic pop
        }
        else
        {
            f_short_run_count = 0;
        }
    }

    // if the service died too many times then it is marked as
    // a failed service; in that case we ignore the call unless the
    // service has a recovery "plan"...
    //
    if( failed() )
    {
        SNAP_LOG_TRACE("service='")(f_service_name)("' has died too many times.");
        int64_t const recovery(get_recovery());
        if( recovery <= 0 )
        {
            SNAP_LOG_TRACE("service='")(f_service_name)("' cannot recover! Removing from communicator.");

#if 0
            // this service cannot recover...

            // make sure the timer is stopped
            // (should not be required since we remove self from
            // snapcommunicator anyway...)
            //
            //set_enable(false);

            // remove self (timer) from snapcommunicator
            //
            remove_from_communicator();

            // we are already at a full stop so we can directly
            // mark ourselves as stopped
            //
            f_stopping = SIGCHLD;
#else
            mark_process_as_stopped( false );
#endif
            return;
        }

        // starting recovery process so reset the failed status
        //
        f_failed = false;
        f_short_run_count = 0;

        // we may wake up later and try again as specified by
        // the user in the XML file (at least 1 minute wait in
        // this case)
        //
        set_timeout_delay(recovery * 1000000LL);

        // Make sure prereqs have stopped (except snapcommunicator)
        //
        get_prereqs_list();
        for( auto const& prereq : f_prereqs_list )
        {
            //SNAP_LOG_TRACE("   service::mark_process_as_dead(): Examining service '")(prereq->get_service_name())("'.");
            if( !prereq->has_stopped() && !prereq->is_connection_required() )
            {
                SNAP_LOG_TRACE("   service::mark_process_as_dead(): Taking service '")(prereq->get_service_name())("' down.");
                prereq->set_restarting();
            }
        }
    }
    else
    {
        SNAP_LOG_TRACE("setting big delay for service='")(f_service_name)("'.");

        // in this case we use a default delay of one second to
        // avoid swamping the CPU with many restart all at once
        //
        set_timeout_delay(1000000LL);
    }
    //
    push_state( state_t::waiting_for_deps );
}


/** \brief For a CRON task, we have to compute the next tick.
 *
 * CRON tasks run when a specific tick happens. If the process
 * is still running when the tick happens, then the service
 * ignores that tick, which is considered lost.
 */
void service::compute_next_tick(bool just_ran)
{
    snap_init::pointer_t si(f_snap_init.lock());
    if(!si)
    {
        SNAP_LOG_ERROR("cron service \"")(f_service_name)("\" lost its parent snapinit object.");
        return;
    }

    // when the cron task does not start properly, we set a timeout
    // delay of three seconds that needs to be reset
    //
    set_timeout_delay(-1);

    // compute the tick exactly on 'now' or just before now
    //
    // current time
    int64_t const now(snap::snap_child::get_current_date() / SNAPINIT_START_DELAY);
    // our hard coded start date
    int64_t const start_date(SNAP_UNIX_TIMESTAMP(2012, 1, 1, 0, 0, 0));
    // number of seconds from the start
    int64_t const diff(now - start_date);
    // number of ticks from the start
    int64_t const ticks(diff / f_cron);
    // time using exact ticks
    int64_t latest_tick(start_date + ticks * f_cron); // latest_tick <= now (rounded down)

    // check whether the spool file exists, if so read it
    //
    QString const spool_path(si->get_spool_path());
    QString const spool_filename(QString("%1/%2.txt").arg(spool_path).arg(f_service_name));
    QFile spool_file(spool_filename);
    if(!spool_file.open(QIODevice::ReadWrite))
    {
        // since we open in R/W it has to succeed, although it could be empty
        //
        SNAP_LOG_ERROR("cron service \"")(f_service_name)("\" could not open its spool file \"")(spool_filename)("\".");
        return;
    }
    QByteArray data(spool_file.readAll());
    bool update(true);
    bool ok(false);
    int64_t last_tick(0);
    if(!data.isEmpty())
    {
        // the file exists and has a tick defined
        //
        QString const last_time(QString::fromUtf8(data.data()));
        last_tick = last_time.toLongLong(&ok, 10);
    }
    if(ok)
    {
        if(just_ran && last_tick == latest_tick)
        {
            // this one is the case when we have to move forward
            //
            //    latest_tick + f_cron > now  (i.e. in the future)
            //
            latest_tick += f_cron;
            set_timeout_date(latest_tick * SNAPINIT_START_DELAY);
        }
        else if(last_tick >= latest_tick)
        {
            // last_tick is now or in the future so we can keep it
            // as is (happen often when starting snapinit)
            //
            set_timeout_date(last_tick * SNAPINIT_START_DELAY);
            update = false;
        }
        else
        {
            // this looks like we may have missed a tick or two
            // so this task already timed out...
            //
            set_timeout_date(latest_tick * SNAPINIT_START_DELAY);
        }
    }
    else
    {
        // never ran, use this latest tick so we run that process
        // once as soon as possible.
        //
        set_timeout_date(latest_tick * SNAPINIT_START_DELAY);
    }

    if(update)
    {
        // reset the file
        spool_file.seek(0);
        spool_file.resize(0);

        // then write the new tick timestop
        spool_file.write(QString("%1").arg(latest_tick).toUtf8());
    }
}


/** \brief Verify that this executable exists.
 *
 * This function generates the full path to the executable to use to
 * start this service. If that full path represents an existing file
 * and that file has it's executable flag set, then the function
 * returns true. Otherwise it returns false.
 *
 * When the snapinit tool starts, it first checks whether all the
 * services that are required to start exist. If not then it fails
 * because if any one service is missing, something is awry anyway.
 *
 * \return true if the file exists and can be executed.
 */
bool service::exists() const
{
    return access(f_full_path.toUtf8().data(), R_OK | X_OK) == 0;
}


/** \brief Start the service in the background.
 *
 * This function starts this service in the background. It uses a fork()
 * and execv() to do so.
 *
 * This function counts the number of time it gets called for each
 * specific service so it can mark the service as a failure if it
 * gets started too many times in a row in a short amount of time.
 *
 * If the service was already marked as failed, then run() always
 * return false unless a \<recovery> tag was defined for that
 * service in which case it will be recovered at some point.
 *
 * \return true if the service started as expected.
 */
bool service::run()
{
    // make sure we did not try too often in a very short time
    // because if so, we want to kill this loop and thus not
    // try again for a while or even remove that service from
    // the list of services
    //
    // also if the service is already marked as stopping, we do
    // not restart it
    //
    if( failed() || is_stopping() )
    {
        return false;
    }

    SNAP_LOG_TRACE("service::run() attempting to start service '")(f_service_name)("'.");

    // Check to make sure dependent processes have started first.
    // Defer if not.
    //
    get_depends_list();
    if( !f_depends_list.empty() )
    {
        auto const snap_init( f_snap_init.lock() );
        for( auto const & svc : f_depends_list )
        {
            if( !svc->is_running() || !svc->is_registered() )
            {
                // Return at this point, since dependent services are not
                // started and registered yet...
                //
                SNAP_LOG_WARNING("Dependency service '")(svc->get_service_name())("' has not yet started for dependent service '")(f_service_name)("'. Deferring start.");
                return false;
            }
        }
    }

    // mark when this service is started using the current system
    // time; that way we can see whether the run was very short
    // when the process dies and if so eventually mark the process
    // as failed
    //
    f_start_date = snap::snap_child::get_current_date();

    pid_t const parent_pid(getpid());
    f_pid = fork();
    if( f_pid == 0 )
    {
        // child
        //

        // make sure that the SIGHUP is sent to us if our parent dies
        //
        prctl(PR_SET_PDEATHSIG, SIGHUP);

        // unblock those signals we blocked in this process because
        // the children should not have such a mask on startup
        //
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGCHLD);
        sigaddset(&set, SIGTERM);
        sigaddset(&set, SIGQUIT);
        sigaddset(&set, SIGINT);
        sigprocmask(SIG_UNBLOCK, &set, nullptr);

        // TBD: should we really "fix" the group of the child so Ctrl-C on
        //      snapinit does not kill all the children? Without doing
        //      so a SIGINT kills all the processes instead of giving
        //      the snapinit tool a chance to turn off those processes
        //      cleanly.
        //
        setpgid(0, 0);

        // always reconfigure the logger in the child
        //
        snap::logging::reconfigure();

        // the parent may have died just before the prctl() had time to set
        // up our child death wish...
        //
        if(parent_pid != getppid())
        {
            common::fatal_error("service::run():child: lost parent too soon and did not receive SIGHUP; quit immediately.");
            snap::NOTREACHED();
        }

        // if the user requested core dump files, we turn on the feature here
        //
        // We do not change it if f_coredump_limit is set to zero, that way
        // the shell `ulimit -c <size>`
        //
        if(f_coredump_limit != 0)
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
            // -Wpedantic prevent the named fields g++ extension when
            // initializing a structure
            //
            struct rlimit core_limits =
            {
                rlim_cur: f_coredump_limit,
                rlim_max: f_coredump_limit
            };
#pragma GCC diagnostic pop
            setrlimit(RLIMIT_CORE, &core_limits);
        }

        std::shared_ptr<snap_init> si(f_snap_init.lock());
        if(!si)
        {
            common::fatal_error("service::run():child: somehow we could not get a lock on f_snap_init from a service object.");
            snap::NOTREACHED();
        }

        std::vector<std::string> args;
        args.push_back(f_full_path.toUtf8().data());
        args.push_back("--server-name");
        args.push_back(si->get_server_name().toUtf8().data());
        args.push_back("--connect");
        args.push_back(si->get_connection_service()->get_connect_string());

        // this server may not have a snapdbproxy, so we have to verify first
        service::pointer_t snapdbproxy_service(si->get_snapdbproxy_service());
        if(snapdbproxy_service)
        {
            args.push_back("--snapdbproxy");
            args.push_back(snapdbproxy_service->get_snapdbproxy_string());
        }

        if( f_debug )
        {
            args.push_back("--debug");
        }
        if( !f_config_filename.isEmpty() )
        {
            args.push_back("--config");
            args.push_back(f_config_filename.toUtf8().data());
        }
        if( !f_options.isEmpty() )
        {
            // **** TODO: REFACTOR RAW LOOP ****
            //
            // f_options is one long string, we need to break it up in
            // arguments paying attention to quotes
            //
            // XXX: we could implement a way to avoid a second --debug
            //      if it was defined in the f_options and on snapinit's
            //      command line
            //
            std::string const opts(f_options.toUtf8().data());
            char const * s(opts.c_str());
            char const * start(s);
            while(*s != '\0')
            {
                if(*s == '"' || *s == '\'')
                {
                    if(s > start)
                    {
                        args.push_back(std::string(start, s - start));
                    }
                    char const quote(*s++);
                    start = s;
                    // TODO: add support for escaping quotes
                    for(; *s != quote && *s != '\0'; ++s);
                    args.push_back(std::string(start, s - start));
                    if(*s != quote)
                    {
                        SNAP_LOG_ERROR("service_run():child: arguments to child process have a quoted string which is not closed properly");
                    }
                    else
                    {
                        // skip the quote
                        ++s;
                    }
                    start = s;
                }
                else if(isspace(*s))
                {
                    if(s > start)
                    {
                        args.push_back(std::string(start, s - start));
                    }
                    // skip all the spaces at once (and avoid empty
                    // arguments too!)
                    //
                    do
                    {
                        ++s;
                    }
                    while(isspace(*s));
                    start = s;
                }
                else
                {
                    // other characters are part of the options
                    ++s;
                }
            }
            // a last argument?
            if(s > start)
            {
                args.push_back(std::string(start, s - start));
            }
        }

        // execv() needs plain string pointers
        std::vector<char const *> args_p;
        std::transform( std::begin(args), std::end(args), std::back_inserter(args_p),
            [&](const auto& a)
            {
                return a.c_str();
            });
        //
        args_p.push_back(nullptr);

        // Quiet up the console by redirecting these from/to /dev/null
        // except in debug mode
        //
        // TODO: handle returned results...
        //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        if(!f_debug)
        {
            freopen( "/dev/null", "r", stdin  );
            freopen( "/dev/null", "w", stdout );
            freopen( "/dev/null", "w", stderr );
        }

        // drop to non-priv user/group if f_user and f_group are set
        //
        if( getuid() == 0 )
        {
            // Group first, then user. Otherwise you lose privs to change your group!
            //
            if( !f_group.isEmpty() )
            {
                struct group* grp(getgrnam(f_group.toUtf8().data()));
                if( nullptr == grp )
                {
                    common::fatal_error( QString("Cannot locate group '%1'! Create it first, then run the server.").arg(f_group) );
                    exit(1);
                }
                const int sw_grp_id = grp->gr_gid;
                //
                if( setgid( sw_grp_id ) != 0 )
                {
                    common::fatal_error( QString("Cannot drop to group '%1'!").arg(f_group) );
                    exit(1);
                }
            }
            //
            if( !f_user.isEmpty() )
            {
                struct passwd* pswd(getpwnam(f_user.toUtf8().data()));
                if( nullptr == pswd )
                {
                    common::fatal_error( QString("Cannot locate user '%1'! Create it first, then run the server.").arg(f_user) );
                    exit(1);
                }
                const int sw_usr_id = pswd->pw_uid;
                //
                if( setuid( sw_usr_id ) != 0 )
                {
                    common::fatal_error( QString("Cannot drop to user '%1'!").arg(f_user) );
                    exit(1);
                }
            }
        }

        // Execute the child processes
        //
        execv(
            args_p[0],
            const_cast<char * const *>(&args_p[0])
        );
#pragma GCC diagnostic pop

        // the command did not start...
        //
        std::string command_line;
        bool first(true);
        for(auto const & a : args)
        {
            if(first)
            {
                first = false;
            }
            else
            {
                command_line += " ";
            }
            command_line += a;
        }
        common::fatal_error(QString("service::run() child: process \"%1\" failed to start!").arg(command_line.c_str()));
        snap::NOTREACHED();
    }

    if( -1 == f_pid )
    {
        int const e(errno);
        SNAP_LOG_ERROR("fork() failed to create a child process to start service \"")(f_service_name)("\". (errno: ")(e)(" -- ")(strerror(e))(")");

        // request the proc library to read memory information
        meminfo();
        SNAP_LOG_INFO("memory total: ")(kb_main_total)(", free: ")(kb_main_free)(", swap_free: ")(kb_swap_free)(", swap_total: ")(kb_swap_total);

        // process never started, but it is considered as a short run
        // and the counter for a short run is managed in the
        // mark_process_as_stopped() function (so unfortunately we may
        // fail a service if the OS takes too much time to resolve
        // the memory issue.)
        //
        mark_process_as_stopped();

        f_pid = 0;

        return false;
    }
    else
    {
        // here we are considered started and running
        //
        f_started = true;
        return true;
    }
}


/** \brief Generate the addr:port information of the connection service.
 *
 * This function gives us the address and port used to connect to the
 * connection service.
 *
 * This is generally the snapcommunicator service. The default IP and
 * port are 127.0.0.1:4040.
 *
 * The function returns a string based on those two parameters. The
 * string is passed to all the services when they are started by the
 * snapinit daemon.
 *
 * \return The address and port of the connection service.
 */
std::string service::get_connect_string() const
{
    std::stringstream ss;
    ss << f_snapcommunicator_addr
       << ":"
       << f_snapcommunicator_port;
    return ss.str();
}


/** \brief Generate the addr:port information of the snapdbproxy service.
 *
 * This function gives us the address and port used to connect to the
 * snapdbproxy service.
 *
 * The default IP and port are 127.0.0.1:4042. It is defined in your
 * snapinit.xml file.
 *
 * The function returns a string based on those two parameters. The
 * string is passed to all the services when they are started by the
 * snapinit daemon.
 *
 * \return The address and port of the snapdbproxy service.
 */
std::string service::get_snapdbproxy_string() const
{
    std::stringstream ss;
    ss << f_snapdbproxy_addr
       << ":"
       << f_snapdbproxy_port;
    return ss.str();
}


/** \brief Check whether this service is running.
 *
 * This function checks whether this proecss is running by checking the
 * f_pid is zero or not.
 *
 * If the service is running, call waitpid() to see whether the service
 * stopped or not. That will remove zombies and allow the snapinit service
 * to restart those processes.
 *
 * \return true if the service is still running.
 */
bool service::is_running() const
{
    // is this service running at all?
    //
    if( f_pid == 0 )
    {
        return false;
    }

    // check whether the process is still running
    //
    int status = 0;
    pid_t const the_pid(waitpid( f_pid, &status, WNOHANG ));
    if( the_pid == 0 )
    {
        return true;
    }

    // process is not running anymore
    //
    // IMPORTANT NOTE: however, we keep f_started as true because the
    //                 service_may_have_died() requires it that way
    //
    const_cast<service *>(this)->f_old_pid = f_pid;
    const_cast<service *>(this)->f_pid = 0;

    if( the_pid == -1 )
    {
        int const e(errno);
        SNAP_LOG_ERROR("waitpid() returned an error (")(strerror(e))(").");
    }
    else
    {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if(WIFEXITED(status))
        {
            int const exit_code(WEXITSTATUS(status));

            if( exit_code == 0 )
            {
                // when this happens there is not really anything to tell about
                SNAP_LOG_DEBUG("Service \"")(f_service_name)("\" terminated normally.");
            }
            else
            {
                SNAP_LOG_INFO("Service \"")(f_service_name)("\" terminated normally, but with exit code ")(exit_code);
            }
        }
        else if(WIFSIGNALED(status))
        {
            int const signal_code(WTERMSIG(status));
            bool const has_code_dump(!!WCOREDUMP(status));

            SNAP_LOG_ERROR("Service \"")
                          (f_service_name)
                          ("\" terminated because of OS signal \"")
                          (strsignal(signal_code))
                          ("\" (")
                          (signal_code)
                          (")")
                          (has_code_dump ? " and a core dump was generated" : "")
                          (".");
        }
        else
        {
            // I do not think we can reach here...
            //
            SNAP_LOG_ERROR("Service \"")(f_service_name)("\" terminated abnormally in an unknown way.");
        }
#pragma GCC diagnostic pop

    }

    return false;
}


/** \brief Whether this service is marked as required.
 *
 * Some service are marked as required meaning that if they fail
 * to start, or break and cannot be restarted, the whole system
 * is hosed. In other words, snapinit will stop everything and
 * quit if the failed() function of a required service returns
 * true and no recovery is offered.
 *
 * \return true if the service was marked as required.
 */
bool service::is_service_required()
{
    return f_required;
}


void service::init_functions()
{
    /*********************************************************************
     * Start service
     *********************************************************************/
    f_func_map =
    {
        {
            state_t::starting_with_listener,
            [&]()
            {
                SNAP_LOG_TRACE("state_t::starting_with_listener() for service '")(f_service_name)("'.");

                // the connection is done in the snap_init class so
                // we have to call a function there once the process
                // is running
                //
                if( !is_running() )
                {
                    SNAP_LOG_TRACE("state_t::starting_with_listener() service '")(f_service_name)("' NOT RUNNING!");

                    // keep the timer in place to try again a little later
                    //
                    // wait for a few seconds before attempting to connect
                    // with the snapcommunicator service
                    //
                    set_timeout_delay(std::max(get_wait_interval(), 3) * SNAPINIT_START_DELAY);

                    // start the process
                    //
                    // in this case we ignore the return value since the
                    // time is still in place and we will be called back
                    // and try again a few times
                    //
                    snap::NOTUSED( run() );
                    push_state( state_t::starting_with_listener );
                    return;
                }

                std::shared_ptr<snap_init> si(f_snap_init.lock());
                if(!si)
                {
                    common::fatal_error("somehow we could not get a lock on f_snap_init from a service object.");
                    snap::NOTREACHED();
                }

                if( !si->connect_listener(f_service_name, f_snapcommunicator_addr, f_snapcommunicator_port) )
                {
                    SNAP_LOG_TRACE("state_t::starting_with_listener() UNABLE TO CONNECT to listener service '")(f_service_name)("'!");
                    push_state( state_t::starting_with_listener );
                    return;
                }

                // TODO: later we may want to try the CONNECT event
                //       more than once; although over TCP on the
                //       local network it should not fail... but who
                //       knows (note that if the snapcommunicator
                //       crashes then we get a SIGCHLD and the
                //       is_running() function returns false.)
                //
            }
        },
        {
            state_t::cron_running,
            [&]()
            {
                if( !cron_task() ) return;

                SNAP_LOG_TRACE("state_t::cron_running() for service '")(f_service_name)("'.");

                if( is_running() )
                {
                    // Run forever, or until process is terminated
                    //
                    compute_next_tick(true);
                }
                else
                {
                    // Try the run the process...
                    //
                    if( run() )
                    {
                        compute_next_tick(true);
                    }
                    else
                    {
                        // give the OS a little time to get its shit back together
                        // (we may have run out of memory for a little while)
                        //
                        set_timeout_delay(3 * SNAPINIT_START_DELAY);
                    }
                }
                //
                push_state( state_t::cron_running );
            }
        },
        {
            state_t::starting_without_listener,
            [&]()
            {
                if( is_running() ) return;

                SNAP_LOG_TRACE("state_t::starting_without_listener() for service '")(f_service_name)("'.");

                // Try running the process. On failure, try again.
                if( run() ) return;

                // give the OS a little time to get its shit back together
                // (we may have run out of memory for a little while)
                //
                // Then...try again.
                //
                set_timeout_delay(3 * SNAPINIT_START_DELAY);
                push_state( state_t::starting_without_listener );
            }
        },
        {
            state_t::starting,
            [&]()
            {
                SNAP_LOG_TRACE("state_t::starting() for service '")(f_service_name)("'.");
                if( is_connection_required() )
                {
                    push_state( state_t::starting_with_listener );
                }
                else
                {
                    if( cron_task() )
                    {
                        push_state( state_t::cron_running );
                    }
                    else
                    {
                        push_state( state_t::starting_without_listener );
                    }
                }
            }
        },
        {
            state_t::waiting_for_deps,
            [&]()
            {
                SNAP_LOG_TRACE("state_t::waiting_for_deps() for service '")(f_service_name)("'.");
                get_depends_list();
                auto iter = std::find_if( std::begin(f_depends_list), std::end(f_depends_list),
                [&]( const auto& svc )
                {
                    return svc->has_stopped();
                });
                if( iter == std::end(f_depends_list) )
                {
                    push_state( state_t::starting );
                }
                else
                {
                    SNAP_LOG_TRACE("state_t::waiting_for_deps() set starting on dependency service '")((*iter)->get_service_name())("'.");
                    // Stop if the service has failed...
                    if( (*iter)->failed() )
                    {
                        SNAP_LOG_INFO("Service ")((*iter)->get_service_name())(" has failed, so will stop waiting.");
                        return;
                    }

                    set_timeout_delay( SNAPINIT_STOP_DELAY );
                    push_state( state_t::waiting_for_deps );
                }
            }
        },

        /*********************************************************************
         * Stop service
         *********************************************************************/
        {
            state_t::stopping,
            [&]()
            {
                SNAP_LOG_TRACE("state_t::stopping() for service '")(f_service_name)("'.");
                if( !is_running() )
                {
                    if( f_restart_requested )
                    {
                        SNAP_LOG_TRACE("state_t::stopping(): restart requested!");
                        push_state( state_t::waiting_for_deps );
                        f_restart_requested = false;
                    }
                    else
                    {
                        SNAP_LOG_TRACE("state_t::stopping(): stopping service!");
                        // use SIGCHLD to show that we are done with signals
                        // and also make sure we get removed from the main
                        // object otherwise the snap_communicator::run()
                        // function would block forever
                        //
                        mark_process_as_stopped();
                    }
                    return;
                }

                // sig is the signal we want to send to the service
                //
                SNAP_LOG_WARNING
                ("service ")(f_service_name)
                (", pid=")(f_pid)
                (", failed to respond to ")(f_stopping == SIGTERM ? "STOP" : "SIGTERM")
                (" signal, using `kill -")(f_stopping)("`.");
                int const retval(::kill( f_pid, f_stopping ));
                if( retval == -1 )
                {
                    // This is marked as FATAL because we are about to kill
                    // that service for good (i.e. we are going to disable
                    // it and never try to start it again); however snapinit
                    // itself will continue to run...
                    //
                    int const e(errno);
                    QString msg(QString("Unable to kill service \"%1\", pid=%2! errno=%3 -- %4")
                    .arg(f_service_name)
                    .arg(f_pid)
                    .arg(e)
                    .arg(strerror(e)));
                    SNAP_LOG_FATAL(msg);
                    syslog( LOG_CRIT, "%s", msg.toUtf8().data() );
                    return;
                }

                if(f_stopping == SIGKILL)
                {
                    // we send SIGKILL once and stop...
                    // then we should receive the SIGCHLD pretty quickly
                    //
                    //set_enable(false);

                    // use SIGCHLD to show that we are done with signals
                    //
                    f_stopping = SIGCHLD;
                }
                else
                {
                    f_stopping = SIGKILL;

                    // reduce the time for SIGTERM to act to half a second
                    // instead of 2 seconds
                    //
                    set_timeout_delay(500000LL);
                    push_state( state_t::stopping );
                }
            }
        },
        {
            state_t::stopping_prereqs,
            [&]()
            {
                SNAP_LOG_TRACE("state_t::stopping_prereqs() for service '")(f_service_name)("'.");
                SNAP_LOG_TRACE("f_restart_requested='")(f_restart_requested)("'");

                // Make sure the services which depend on this have stopped first...
                //
                get_prereqs_list();
                auto iter = std::find_if( std::begin(f_prereqs_list), std::end(f_prereqs_list),
                [&]( const auto& svc )
                {
                    return !svc->has_stopped() && !svc->is_connection_required();
                });
                if( iter == std::end(f_prereqs_list) )
                {
                    SNAP_LOG_TRACE("state_t::stopping_prereqs(): no prereqs left, stopping service.");
                    set_timeout_delay(SNAPINIT_STOP_DELAY);
                    push_state( state_t::stopping );
                }
                else
                {
                    SNAP_LOG_TRACE("state_t::stopping_prereqs(): stopping '")((*iter)->get_service_name())("'");
                    //
                    // Leave timer running and come back again if any service has not yet stopped...
                    //
                    set_timeout_delay(SNAPINIT_STOP_DELAY);
                    push_state( state_t::stopping_prereqs );
                }
            }
        },


#if 0
        /*********************************************************************
         * Mark service as dead
         *********************************************************************/
        {
            state_t::waiting_stop_prereqs,
            [&]()
            {
                SNAP_LOG_TRACE("state_t::waiting_stop_prereqs() for service '")(f_service_name)("'.");
                // Make sure the services which depend on this have stopped first...
                //
                get_prereqs_list();
                auto iter = std::find_if( std::begin(f_prereqs_list), std::end(f_prereqs_list),
                [&]( const auto& svc )
                {
                    return !svc->has_stopped() && !svc->is_connection_required();
                });
                if( iter == std::end(f_prereqs_list) )
                {
                    SNAP_LOG_TRACE("state_t::waiting_stop_prereqs(): no prereqs left, stopping service.");

                    get_prereqs_list();
                    for( auto const& svc : f_prereqs_list )
                    {
                        if( !svc->is_running() ) { svc->set_starting(); }
                    }
                    set_timeout_delay(SNAPINIT_STOP_DELAY);
                    //
                    push_state( state_t::waiting_for_deps );
                }
                else
                {
                    SNAP_LOG_TRACE("state_t::waiting_stop_prereqs(): stopping '")((*iter)->get_service_name())("'");
#if 0
                    SNAP_LOG_INFO("Dependency service '")
                        ((*iter)->get_service_name())
                        ("' has not yet stopped, checking again on the next timeout.");
#endif
                    //
                    // Leave timer running and come back again if any service has not yet stopped...
                    //
                    set_timeout_delay(SNAPINIT_STOP_DELAY);
                    push_state( state_t::waiting_stop_prereqs );
                }
            }
        },
#endif
#if 0
        {
            state_t::failed,
            [&]()
            {
                SNAP_LOG_TRACE("state_t::failed() service='")(f_service_name)("' has died too many times.");
                int64_t const recovery(get_recovery());
                if( recovery <= 0 )
                {
                    SNAP_LOG_TRACE("service='")(f_service_name)("' cannot recover! Removing from communicator.");

                    // this service cannot recover...

                    // make sure the timer is stopped
                    // (should not be required since we remove self from
                    // snapcommunicator anyway...)
                    //
                    //set_enable(false);

                    // remove self (timer) from snapcommunicator
                    //
                    remove_from_communicator();

                    // we are already at a full stop so we can directly
                    // mark ourselves as stopped
                    //
                    f_stopping = SIGCHLD;
                    return;
                }

                // starting recovery process so reset the failed status
                //
                f_failed = false;
                f_short_run_count = 0;

                // we may wake up later and try again as specified by
                // the user in the XML file (at least 1 minute wait in
                // this case)
                //
                set_timeout_delay(recovery * SNAPINIT_STOP_DELAY);

                push_state( state_t::failing );
            }
        },
        {
            state_t::failing_wait,
            [&]()
            {
                SNAP_LOG_TRACE("state_t::failing_wait(): setting big delay for service='")(f_service_name)("'.");

                // in this case we use a default delay of one second to
                // avoid swamping the CPU with many restart all at once
                //
                set_timeout_delay(SNAPINIT_STOP_DELAY);

                if( failed() )
                {
                    push_state( state_t::failed );
                }
                else
                {
                    push_state( state_t::failing_wait );
                }
            }
        },
        {
            state_t::failing,
            [&]()
            {
                SNAP_LOG_TRACE("state_t::failing() for service '")(f_service_name)("'.");

                // Stop all prereqs and dependencies
                //
                get_prereqs_list();
                for( auto const& svc : f_prereqs_list )
                {
                    if( !svc->has_stopped() )
                    {
                        svc->set_stopping();
                    }
                }
                //
                get_depends_list();
                for( auto const & svc : f_depends_list )
                {
                    if( !svc->has_stopped() )
                    {
                        svc->set_stopping();
                    }
                }

                int64_t const now(snap::snap_child::get_current_date());
                //std::cerr << "*** process " << f_service_name << " died, now/start " << now << "/" << f_start_date << ", time interval is " << (now - f_start_date) << ", counter: " << f_short_run_count << "\n";
                if(now - f_start_date < MAX_START_INTERVAL)
                {
                    ++f_short_run_count;

                    // too many short runs means this service failed
                    //
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wstrict-overflow"
                    f_failed = f_short_run_count >= MAX_START_COUNT;
                #pragma GCC diagnostic pop
                }
                else
                {
                    f_short_run_count = 0;
                }

                if( failed() )
                {
                    push_state( state_t::failed );
                }
                else
                {
                    push_state( state_t::failing_wait );
                }
            }
        }
#endif
    };
}


/** \brief Mark this service as starting.
 *
 * Mark a service as starting, and start the timer. Any processes that are
 * dependent on this process running will likewise be marked as starting. They
 * will not start until this process has fully started.
 *
 * \sa process_timeout()
 */
void service::set_starting()
{
    if( is_running() )
    {
        SNAP_LOG_TRACE("service::set_starting() already running service '")(f_service_name)("'!");
        return;
    }

    if( is_stopping() )
    {
        SNAP_LOG_INFO("service::set_starting() service '")(f_service_name)("' is stopping, so returning.");
        return;
    }

    SNAP_LOG_TRACE("service::set_starting() for service '")(f_service_name)("'.");

    f_stopping = 0;

    //set_enable(true);
    set_timeout_delay(SNAPINIT_START_DELAY);
    set_timeout_date(-1); // ignore any date timeout

#if 0
    // Make sure all prereqs are started
    //
    for( auto const& svc : f_prereqs_list )
    {
        if( !svc->is_running() )
        {
            svc->set_starting();
        }
    }
#endif

    push_state( state_t::waiting_for_deps );
}


QString service::state_to_string( state_t const state )
{
    QString retval;
    switch( state )
    {
        case state_t::not_started               : retval = "not_started";               break;
        case state_t::starting                  : retval = "starting";                  break;
        case state_t::starting_without_listener : retval = "starting_without_listener"; break;
        case state_t::starting_with_listener    : retval = "starting_with_listener";    break;
        case state_t::waiting_for_deps          : retval = "waiting_for_deps";          break;
        case state_t::running                   : retval = "running";                   break;
        case state_t::cron_running              : retval = "cron_running";              break;
        case state_t::stopping                  : retval = "stopping";                  break;
        case state_t::stopping_prereqs          : retval = "stopping_prereqs";          break;
        case state_t::stopped                   : retval = "stopped";                   break;
        //case state_t::waiting_stop_prereqs      : retval = "waiting_stop_prereqs";      break;
    }
    return retval;
}


void service::set_restarting()
{
    SNAP_LOG_TRACE("service::set_restarting() for service '")(f_service_name)("'.");
    f_restart_requested = true;
    set_stopping();
}

/** \brief Mark this service as stopping.
 *
 * This service is marked as being stopped. This happens when quitting
 * or a fatal error occurs.
 *
 * The function marks the service as stopping and setup the service
 * timeout so it can be killed with a SIGTERM and after the SIGTERM:
 * a SIGKILL.
 *
 * The process_timeout() function is in charge of sending those signals.
 *
 * This function does NOT send the STOP signal to the service. This is
 * left to the caller (see terminate_services() in snap_init), which
 * has all the necessary information to send the signal to the
 * snapcommunicator, which in turn will send the signals to each
 * service.
 */
void service::set_stopping()
{
    SNAP_LOG_TRACE("service::set_stopping() stopping service '")(f_service_name)("'");

    if(is_running())
    {
        // on the next timeout, use SIGTERM
        //
        f_stopping = SIGTERM;

        // give the STOP signal 10 seconds, note that all services are sent
        // the STOP signal at the same time so 10 seconds should be more
        // than enough for all to quit (only those running a really heavy
        // job and do not check their signals often enough...)
        //
        // TODO: we probably want that 10 seconds to be a variable in the
        //       .conf file, that way users can decide how long we should
        //       be waiting here.
        //
        // the test before the set_enable() and set_timeout_delay()
        // is there because set_stopping() could be called multiple times.
        //
        //set_enable(true);
        set_timeout_delay(SNAPINIT_LONG_STOP_DELAY);
        set_timeout_date(-1); // ignore any date timeout

        // Stop prereqs
        //
        get_prereqs_list();
        for( auto const& prereq : f_prereqs_list )
        {
            if( !prereq->has_stopped() && !prereq->is_connection_required() ) { prereq->set_stopping(); }
        }

        // Now set stopping on all processes which depend on this service:
        //
        push_state( state_t::stopping_prereqs );
    }
    else
    {
        // stop process complete, mark so with SIGCHLD
        //
        f_stopping = SIGCHLD;

        // no need to timeout anymore, this service will not be restarted
        //
        //set_enable(false);
    }
}


/** \brief Check whether a service is stopping or was stopped.
 *
 * This function returns true if the service was requested to stop
 * normally or has already stopped.
 *
 * If this function returns true, the service was already stopped or
 * was at least sent the STOP signal. Later on (2 seconds) it may
 * also have been sent a SIGTERM or (another 0.5 seconds later)
 * SIGKILL system signal.
 *
 * \return true if the service termination process is over.
 */
bool service::is_stopping() const
{
    return f_stopping != 0;
}


/** \brief Check whether a service was stopped.
 *
 * This function returns true if the service has stopped normally.
 * If this function returns true, the service was stopped either with
 * the STOP signal, the SIGTERM, or the SIGKILL system signals.
 *
 * To know whether the service is currently running, you want to
 * call the is_running() function instead. This one is called to
 * know that the service is gone and snapinit can exit.
 *
 * \return true if the service termination process is over.
 */
bool service::has_stopped() const
{
    return f_stopping == SIGCHLD;
}


/** \brief Determine whether this service requires us to connect to it.
 *
 * snapinit starts the snapcommunicator and it is expected to
 * connect to it (connect with a client and send a CONNECT message.)
 *
 * This function returns true if the necessary information was defined
 * so we can actually connect. Note that the \<connect> tag is required
 * since it is used to distinguish the snapcommunicator without
 * actually checking the name of the service.
 *
 * \return true if there are connection address and port defined here.
 */
bool service::is_connection_required() const
{
    return !f_snapcommunicator_addr.isEmpty();
}


/** \brief Determine whether this service is the snapdbproxy.
 *
 * snapinit starts the snapdbproxy and it is expected to let other
 * services connect to the database used by Snap! The snapdbproxy
 * may not run on all computers in a cluster, but it has to run on
 * any computer that has services requiring access to the database.
 *
 * This function returns true if this service represents the snapdbproxy
 * service (i.e. is has a \<snapdbproxy> tag.)
 *
 * \return true if there are snapdbproxy address and port defined here.
 */
bool service::is_snapdbproxy() const
{
    return !f_snapdbproxy_addr.isEmpty();
}



/** \brief returns the registration status with snapcommunicator
 */
bool service::is_registered() const
{
    return f_registered;
}


/** \brief sets the registration status with snapcommunicator
 */
void service::set_registered( bool const val )
{
    f_registered = val;
}


/** \brief Determine whether this service requires us to wait on a SAFE message.
 *
 * snapinit starts the snapfirewall and wait for the SAFE message it
 * sends to let us know that the firewall is up and running and that
 * it is now safe to start the snapserver.
 *
 * Obviously, the main firewall setup should already be up by the time
 * we start the snapfirewall. The snapfirewall service only adds a
 * set of rules blocking IP addresses that were received from various
 * anti-hacker and anti-spam plugins and tools.
 *
 * \return true if this module requires a SAFE message.
 */
bool service::is_safe_required() const
{
    return !f_safe_message.isEmpty();
}


/** \brief Retrieve the safe message.
 *
 * This function returns a copy of the expected safe message from the
 * last service that we started and required us to wait on such a
 * safe message before starting even more services.
 *
 * \return The safe message as defined in the safe tag.
 */
QString const & service::get_safe_message() const
{
    return f_safe_message;
}


/** \brief Determine whether this is a cron task or not.
 *
 * At this time we have one service (backend) which we want to run on
 * a regular basis. This is considered a cron task as it does not
 * run forever but it needs to be run at given intervals (at a given
 * tick).
 *
 * \return true if this service is considered a cron task.
 */
bool service::cron_task() const
{
    return f_cron != 0;
}


/** \brief Retrieve the filename of this service configuration file.
 *
 * This function returns the configuration filename as defined in
 * the \<config> tag.
 *
 * \return The configuration filename.
 */
QString const & service::get_config_filename() const
{
    return f_config_filename;
}


/** \brief Return the name of the service.
 *
 * This function returns the name of the server.
 *
 * Note that since we derive from a snap_connection, you also have
 * a get_name() function, which returns the connection name instead.
 *
 * \return The service name as defined in the name attribute of
 *         the \<service> tag found in the snapinit.xml file.
 */
QString const & service::get_service_name() const
{
    return f_service_name;
}


/** \brief Return the PID of service before it died.
 *
 * This function returns 0 if the process never ran and died.
 * After a first death, this returns the PID of the previous
 * process.
 *
 * \return The old PID of this service.
 */
pid_t service::get_old_pid() const
{
    return f_old_pid;
}


/** \brief Check whether this service failed to start.
 *
 * This function is called before we start a service again. It checks
 * whether the number of times we already tried is larger than
 * MAX_START_COUNT and that this number of retries each happened
 * in an interval of MAX_START_INTERVAL or less.
 *
 * Note that the interval is calculated from the last time run()
 * was called. In other words, any one run needs to last at least
 * MAX_START_INTERVAL microseconds or this function triggers the
 * fail status.
 *
 * Most failed processes will be removed from the list of services.
 * You may mark a service with the \<recovery> tag in which case
 * that service will not die. Instead, snapinit will sleep for the
 * number of seconds specified in that \<recovery> tag and when
 * it wakes up, reset the failed state and try to start that process
 * again. By then, the possibly problematic data will be gone and
 * the backend will work as expected.
 *
 * \note
 * If you call failed() before you ever called run(), then f_start_date
 * is still zero meaning that the second part if the condition will
 * always be false, which is the expected result in this case (i.e. it
 * is not a failed service if it never ran.)
 *
 * \return true if the service is considered to have failed too many times.
 */
bool service::failed() const
{
    return f_failed;
}


/** \brief Retrieve the wait interval.
 *
 * This function returns the wait interval value as found in the XML
 * file. This is the value defined in the \<wait> tag.
 *
 * For a service which we have to connect with, this represents the
 * period of time we want to wait before attempting a connection.
 *
 * For other services, this is a delay between this service (with a
 * wait interval larger than 0) and the next service.
 *
 * \return The wait interval of this service.
 */
int service::get_wait_interval() const
{
    return f_wait_interval;
}


/** \brief Retrieve the recovery wait time.
 *
 * Whenever a backend crashes, it is very much possible that it can
 * be restarted a little later as the database will have recovered
 * or the page it was working on is now marked as done, somehow.
 *
 * So restarting that backend process later will generally work
 * (it happened quite a few times to me at least.) So we offer
 * a recovery feature which tells the snapinit tool to leave that
 * process sleeping for a while. After that pause, snapinit will
 * try to start the process again. This will go on until snapinit
 * ends.
 *
 * While recovering, a service is not running at all.
 *
 * \note
 * Our backends are not unlikely to have some recovering
 * mechanism already implemented in the snap_backend class
 * making the recovery of snapinit services redundant.
 *
 * \todo
 * Count the number of recoveries and whether the process ever
 * recovers (i.e. if after a recovery pause the process still
 * dies immediately (i.e. failed() returns true again) then
 * we probably cannot recover that way...)
 *
 * \return The number of seconds to sleep as recovery time.
 */
int service::get_recovery() const
{
    return f_recovery;
}


/** \brief Retrieve the priority of this service.
 *
 * Before we attempt to wake up each service, we sort them
 * by priority. This is the value being used.
 *
 * The smaller a priority is, the sooner a service gets started.
 *
 * \return The priority, a number between -100 and +100.
 */
int service::get_priority() const
{
    return f_priority;
}


/** \brief Services are expected to be sorted by priority.
 *
 * This function compares 'this' priority against the 'rhs'
 * priority and returns true if 'this' priority is smaller.
 *
 * \param[in] rhs  The right hand side service object.
 *
 * \return true if the priority of this service is smaller
 *         that the one of \p rhs (smaller means higher
 *         priority, starts first.)
 */
bool service::operator < (service const & rhs) const
{
    return f_priority < rhs.f_priority;
}


// vim: ts=4 sw=4 et
