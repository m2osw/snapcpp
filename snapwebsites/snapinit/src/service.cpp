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
#include "snapinit.h"

// snapwebsites lib
//
#include "log.h"
#include "not_used.h"

// C++ lib
//
#include <sstream>


/** \file
 * \brief Services is an object that allows us to run one service.
 *
 * The following are the states as the services understands them.
 *
 * A service in the "Ready Service" state runs its process whenever
 * it can. Whenever a process gets registered, an event occurs and
 * services that are ready and now got all of their dependencies
 * satisfied get started.
 *
 * Note: the "Paused Service" is the same as "Ready Service" with a
 * longer pause interval (timer) and the action of forcing all
 * pre-requisits to stop running (i.e. go back to "Ready Service").
 *
 * Note: the "Stopped Service" status is actually a service that
 * removed itself from the snap_init class.
 *
 * \todo
 * The following does not correctly support the case where the
 * snapcommunicator crashes.
 *
 * \code
 *                           O
 *                           | create service
 *                           |
 *                           V
 *                    +---------------------+
 *       stop service |                     |
 *   +----------------+ Disabled Service    |
 *   |                |                     |
 *   |                +---------------------+
 *   |                       |
 *   |                       | initialize service
 *   |                       |
 *   |                       V
 *   |                +---------------------+
 *   |   stop service |                     | start process [if all dependencies are registered]
 *   +----------------+ Ready Service       +---------------->[see process]
 *   |                |                     |
 *   |                +---------------------+
 *   |                       ^     ^
 *   |                       |     |          process died [if service state "Ready Service" or "Service Go Down"]
 *   |                       |     +--------------------------[see process]
 *   |                       |     |
 *   |                       |     |          process pause [if service state "Service Go Down"]
 *   |                       |     +--------------------------[see process]
 *   |                       |
 *   |                       |
 *   |                       | restart service [if pause timed out]
 *   |                       |
 *   |                +---------------------+
 *   |   stop service |                     |  process pause [if service state "Ready Service"]
 *   +----------------+ Paused Service      |<----------------[see process]
 *   |                |                     |
 *   |                |                     |
 *   |                |                     |
 *   |                +---------------------+
 *   |                       |
 *   |                       | stop pre-requirements
 *   |                       |
 *   |                       V
 *   |                +---------------------+
 *   |   stop service |                     |
 *   +----------------+ Service Go Down     |
 *   |                |                     |
 *   |                +---------------------+
 *   |
 *   |
 *   |                +---------------------+
 *   |                |                     |  process pause / process died
 *   +--------------->| Stopping Service    |<----------------[see process]
 *                    |                     |
 *                    +---------------------+
 *                           |
 *                           | remove service from snapinit [if process is not running]
 *                           |
 *                           O
 *
 * \endcode
 *
 * The process of stopping a service is a sub-state machine described below.
 * This process is in active mode is the current service state is
 * STOPPING or PAUSING.
 *
 * It can be resumed in a few steps: send a STOP command (if registered),
 * if still running, send a SIGTERM, if still running, send a SIGKILL.
 * In all cases, whether a process is still running is determined by the
 * receipt of the SIGCHLD signal. If the SIGKILL fails, then snapinit
 * attempts to exit.
 *
 *
 * \code
 *              O
 *              |
 *              | create service
 *              |
 *              V
 *       +---------------------+
 *       |                     |  process died
 *       | Stop Idle           |<------------------[process]
 *       |                     |
 *       +------+--------+-----+
 *              |        |
 *              |        |  send SIGTERM [if process is running and unregistered]
 *              |        +---------------------------------------------------+
 *              |                                                            |
 *              |                                                            |
 *              | send STOP message [if process is running and registered]   |
 *              |                                                            |
 *              V                                                            |
 *       +---------------------+                                             |
 *       |                     |                                             |
 *       | Stop Service        |                                             |
 *       |                     |                                             |
 *       +------+--------------+                                             |
 *              |                                                            |
 *              | send SIGTERM [if timed out]                                |
 *              |                                                            |
 *              V                                                            |
 *       +---------------------+                                             |
 *       |                     |                                             |
 *       | Terminate Service   |<--------------------------------------------+
 *       |                     |
 *       +------+--------------+
 *              |
 *              | send SIGKILL [if timed out]
 *              |
 *              V
 *       +---------------------+
 *       |                     |
 *       | Kill Service        |
 *       |                     |
 *       +------+--------------+
 *              |
 *              | fail [if timed out]
 *              |
 *              V
 *       +---------------------+
 *       |                     |
 *       | Abort Process       |
 *       |                     |
 *       +------+--------------+
 *              |
 *              |
 *              O
 *
 * \endcode
 */



namespace snapinit
{



namespace
{


/** \brief When the next process can be started.
 *
 * In order to manage the RAM and CPU (especially on small VPS machines)
 * we make sure to not start two services "simultaneously". Instead we
 * allow a minimum of 1 second delay between each startup.
 *
 * This variable defines the time when the next startup is allowed.
 * If action_ready() gets called and the time is too short, then
 * we instead setup a timer which will receive in a call of
 * the process_ready() function a little later.
 *
 * \note
 * This variable is global because it is common to all services.
 * Although it could be defined as a static variable, there is
 * no point in doing so. It can just be a static here.
 */
//int64_t             g_next_start_on = 0;



auto is_process_running = [](auto const & s)
{
    auto const svc(s.lock());
    if(!svc)
    {
        return false;
    }
    return svc->is_running();
};



} // no name namespace





service::dependency_t::dependency_t()
{
}


service::dependency_t::dependency_t(QString const & service_name, dependency_type_t const type)
    : f_service_name(service_name)
    , f_type(type)
{
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
    : snap_timer(-1)
    , f_snap_init(si)
    , f_process(si, this)
{
    // by default our timer is turned off
    //
    set_enable(false);

    // timer has a low priority (runs last)
    //
    set_priority(100);
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


/** \brief Create ourselves as as service.
 *
 * We do not offer an XML file for snapinit itself because we do not
 * want end users to be able to change our settings as a service.
 * (They can make changes to snapinit.conf, however.)
 *
 * This function is used to configure that specific service.
 */
void service::configure_as_snapinit()
{
    f_service_name = "snapinit";
    f_required = true;
    QString const ignored_binary_path;
    snap::NOTUSED(f_process.set_command(ignored_binary_path, "snapinit"));
    //f_wait_interval = 1;
    //f_recovery = 0;
    //f_process.set_safe_message("");
    //f_process.set_nice(0);
    //f_process.set_coredump_limit(0);
    //f_process.set_options("");
    //common_options.push_bacl(...); -- this is done in snapinit.cpp
    f_priority = -80;
    f_process.set_config_filename("/etc/snapwebsites/snapinit.conf");
    //f_snapcommunicator_addr.clear();
    //f_snapcommunicator_port = 4040;
    //f_snapdbproxy_addr.clear();
    //f_snapdbproxy_port = 4042;
    //f_cron = 0;
    f_process.set_user("root");
    f_process.set_group("root");
    f_dep_name_list.clear();
    f_dep_name_list.push_back(dependency_t("snapcommunicator", dependency_t::dependency_type_t::DEPENDENCY_TYPE_STRONG));

    // set snap_communicator connection name to help with debugging
    //
    set_name(f_service_name + " timer");
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
 */
void service::configure(QDomElement e, QString const & binary_path, std::vector<QString> & common_options)
{
    // first make sure we have a name for this service
    //
    f_service_name = e.attribute("name");
    if(f_service_name.isEmpty())
    {
        common::fatal_error("the \"name\" parameter of a service must be defined and not empty.");
        snap::NOTREACHED();
    }

    // for the --list and --tree command options, save the disabled state
    //
    f_disabled = e.attributes().contains("disabled");

    // if a required service fails then snapinit fails as a whole
    //
    f_required = e.attributes().contains("required");

    // by default the command is one to one like the name of the service
    //
    {
        QString command(f_service_name);

        // check to see whether the user specified a specific command in XML
        //
        QDomElement const sub_element(e.firstChildElement("command"));
        if(!sub_element.isNull())
        {
            command = sub_element.text();
            if(command.isEmpty())
            {
                common::fatal_error(QString("the command tag of service \"%1\" returned an empty string which does not represent a valid command.")
                                .arg(f_service_name));
                snap::NOTREACHED();
            }
        }

        if(!f_process.set_command(binary_path, command))
        {
            // we could not find the command, mark it as if it were disabled
            //
            f_disabled = true;
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
                f_wait_interval = 1;
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
        // minimum of 1 second between f_process.action_start() calls
        //
        if(f_wait_interval < 1)
        {
            f_wait_interval = 1;
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
                // this is the default, no recovery, these disappear
                // if they fail too quickly (i.e. on pause_process())
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
            QString const safe_message(sub_element.text());
            if(!safe_message.isEmpty()
            && safe_message != "none") // "none" is equivalent to nothing which is the default
            {
                f_process.set_safe_message(safe_message);
            }
        }
    }

    // get the nice value if defined
    //
    {
        QDomElement const sub_element(e.firstChildElement("nice"));
        if(!sub_element.isNull())
        {
            if(sub_element.text() != "default")
            {
                QString nice_string(sub_element.text());
                bool ok(false);
                int const nice(nice_string.toLongLong(&ok, 10));
                if(!ok)
                {
                    common::fatal_error(QString("the nice tag of service \"%1\" is not a valid decimal number nor \"default\".").arg(f_service_name));
                    snap::NOTREACHED();
                }
                if(nice < 0 || nice > 19)
                {
                    // See `man setpriority`
                    //
                    common::fatal_error(QString("the nice tag of service \"%1\" cannot be a value under 0 or larger than 19.")
                                            .arg(f_service_name));
                    snap::NOTREACHED();
                }
                f_process.set_nice(nice);
            }
        }
    }

    // get the core dump file size limit
    //
    {
        rlim_t coredump_limit(0);

        QDomElement const sub_element(e.firstChildElement("coredump"));
        if(!sub_element.isNull())
        {
            if(sub_element.text() == "none")
            {
                // this is the default anyway
                //
                coredump_limit = 0;
            }
            if(sub_element.text() == "infinity")
            {
                // save the entire process data when the crash occurs
                //
                coredump_limit = RLIM_INFINITY;
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
                coredump_limit = size.toLongLong(&ok, 10) * multiplicator;
                if(!ok)
                {
                    common::fatal_error(QString("the coredump tag of service \"%1\" is not a valid decimal number, optionally followed by \"kb\", \"mb\", or \"gb\".").arg(f_service_name));
                    snap::NOTREACHED();
                }
                if(coredump_limit < 1024)
                {
                    // the size of 1024 is hard coded from Linux ulimit
                    // which has the following table:
                    //
                    //      static RESOURCE_LIMITS limits[] = {
                    //      #ifdef RLIMIT_CORE
                    //      { 'c',        RLIMIT_CORE,  1024,     "core file size",      "blocks" },
                    //      #endif
                    //      ...
                    //      };
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
                                            .arg(f_service_name).arg(coredump_limit));
                    snap::NOTREACHED();
                }
                // keep the value in blocks, rounded up
                //
                coredump_limit = (coredump_limit + 1023) / 1024;
            }
        }

        f_process.set_coredump_limit(coredump_limit);
    }

    // check to see whether the user specified command line options
    //
    {
        QDomElement const sub_element(e.firstChildElement("options"));
        if(!sub_element.isNull())
        {
            f_process.set_options(sub_element.text());
        }
    }

    // check to see whether the service defines an option that is to be
    // used on the command line of all the other services
    //
    {
        QDomElement const sub_element(e.firstChildElement("common-options"));
        if(!sub_element.isNull())
        {
            common_options.push_back(sub_element.text());
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
            QString const config_filename(sub_element.text());
            if(config_filename.isEmpty())
            {
                common::fatal_error(QString("the config tag of service \"%1\" returned an empty string which does not represent a valid configuration filename.")
                                    .arg(f_service_name));
                snap::NOTREACHED();
            }
            f_process.set_config_filename(config_filename);
        }
    }

    // whether we should connect ourselves after that service was started
    //
    {
        QDomElement const sub_element(e.firstChildElement("snapcommunicator"));
        if(!sub_element.isNull())
        {
            QString const addr_port(sub_element.text());
            if(addr_port.isEmpty())
            {
                common::fatal_error(QString("the <snapcommunicator> tag of service \"%1\" returned an empty string which does not represent a valid IP and port specification.")
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
            QString const user(sub_element.text());
            if(user.isEmpty())
            {
                common::fatal_error(QString("the config tag of service \"%1\" returned an empty string which does not represent a valid configuration filename.")
                                    .arg(f_service_name));
                snap::NOTREACHED();
            }
            f_process.set_user(user);
        }
    }

    // non-priv group to drop to after child has forked
    // (if empty, then we stay at the user level we were at)
    //
    {
        QDomElement sub_element(e.firstChildElement("group"));
        if(!sub_element.isNull())
        {
            QString const group(sub_element.text());
            if(group.isEmpty())
            {
                common::fatal_error(QString("the config tag of service \"%1\" returned an empty string which does not represent a valid configuration filename.")
                                    .arg(f_service_name));
                snap::NOTREACHED();
            }
            f_process.set_group(group);
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
            QDomElement n( sub_element.firstChildElement() );
            while( !n.isNull() )
            {
                if( n.tagName() == "dependency" )
                {
                    QString const dep_name(n.text());
                    if(dep_name.isEmpty())
                    {
                        common::fatal_error(QString("the name of a dependency cannot be the empty string in \"%1\" service definition.")
                                            .arg(f_service_name));
                        snap::NOTREACHED();
                    }
                    f_dep_name_list.push_back(
                            dependency_t(
                                dep_name,
                                n.attribute("type") == "weak" ? dependency_t::dependency_type_t::DEPENDENCY_TYPE_WEAK
                                                              : dependency_t::dependency_type_t::DEPENDENCY_TYPE_STRONG
                            )
                        );
                }
                //
                n = n.nextSiblingElement();
            }
        }
    }

    // the XML configuration worked, create a timer too
    //
    set_name(f_service_name + " timer");

    if(is_cron_task())
    {
        compute_next_tick(false);
    }
}


void service::finish_configuration(std::vector<QString> & common_options)
{
    f_process.set_common_options(common_options);

    init_prereqs_list();
    init_depends_list();
}


void service::init_prereqs_list()
{
    snap_init_ptr()->get_prereqs_list( f_service_name, f_prereqs_list );

    // sort those services by DESCENDING priority
    //
    // unfortunately, the following would sort items by pointer were
    // we to not specifying our own sort function
    //
    std::sort(
            f_prereqs_list.begin(),
            f_prereqs_list.end(),
            [](auto const & a, auto const & b)
            {
                service::pointer_t const svc_a(a.lock());
                service::pointer_t const svc_b(b.lock());
                if(!svc_a || !svc_b)
                {
                    throw std::logic_error("it is not possible for the weak pointers to have been set to null by now.");
                }
                // WARNING: here the test is inverted, that way we can
                //          stop services in the inverse order, the
                //          exact opposite from the order in which they
                //          were started
                //
                return !(*svc_a < *svc_b);
            });
}


void service::init_depends_list()
{
    snap_init::pointer_t si(snap_init_ptr());
    std::for_each(
            std::begin(f_dep_name_list),
            std::end(f_dep_name_list),
            [this, &si]( auto const & dependency )
            {
                auto const svc( si->get_service( dependency.f_service_name ) );
                if( !svc )
                {
                    if(dependency.f_type == dependency_t::dependency_type_t::DEPENDENCY_TYPE_WEAK)
                    {
                        // ignore the fact that the dependency is missing
                        // because it is weak
                        //
                        return;
                    }

                    common::fatal_error( QString("Strong dependency service '%1' not found!").arg(dependency.f_service_name) );
                    snap::NOTREACHED();
                }
                f_depends_list.push_back( svc );
            });

    // sort those services by priority
    //
    // unfortunately, the following would sort items by pointer were
    // we to not specifying our own sort function
    //
    std::sort(
            f_depends_list.begin(),
            f_depends_list.end(),
            [](auto const & a, auto const & b)
            {
                service::pointer_t const svc_a(a.lock());
                service::pointer_t const svc_b(b.lock());
                if(!svc_a || !svc_b)
                {
                    throw std::logic_error("it is not possible for the weak pointers to have been set to null by now.");
                }
                return *svc_a < *svc_b;
            });
}


process & service::get_process()
{
    return f_process;
}


service::weak_vector_t const & service::get_depends_list() const
{
    return f_depends_list;
}


/** \brief Pause this service if it is running.
 *
 * If the process of this service is not running, then nothing happens.
 *
 * If the process is running, then a "stop to pause" is initiated. This
 * is a normal stopping process, except that we do not change the state
 * of the service to SERVICE_STATE_STOPPING.
 */
void service::action_ready()
{
    // change the state to READY
    //
    if(f_service_state == service_state_t::SERVICE_STATE_STOPPING)
    {
        throw std::runtime_error("a service cannot go from STOPPING to READY.");
    }
    f_service_state = service_state_t::SERVICE_STATE_READY;

    process_ready();

    // we cannot be sure that the order in which action_ready() is going
    // to be correct... (i.e. it uses the priority sorted order and not
    // the tree + priority sorted order as expected)
    //
    //int64_t const start_on(snap::snap_communicator::get_current_date());
    //if(start_on > g_next_start_on)
    //{
    //    g_next_start_on = start_on + f_wait_interval * common::SECONDS_TO_MICROSECONDS;
    //
    //    process_ready();
    //}
    //else
    //{
    //    // this case happens if more than one process can be started
    //    //
    //    // we put a minimum of 1 second between startup of processes
    //    // so that way we do not start many tasks all at once which
    //    // the OS does not particularly like and it makes nearly no
    //    // difference on our services
    //    //
    //    set_enable(true);
    //    set_timeout_date(g_next_start_on);
    //
    //    g_next_start_on += f_wait_interval * common::SECONDS_TO_MICROSECONDS;
    //}
}


/** \brief Stop this service as it is a pre-requirement of another that died.
 *
 * If the process of this service is not running, then nothing happens.
 *
 * If the process is currently running, then a "stop" process is initiated.
 * This is a normal stopping process, except that once the process is indeed
 * stopped, the status goes back to SERVICE_STATE_READY instead of
 * disappearing.
 */
void service::action_godown()
{
    if(f_service_state == service_state_t::SERVICE_STATE_STOPPING)
    {
        SNAP_LOG_FATAL("service \"")(f_service_name)("\" cannot go from STOPPING to GOINGDOWN.");
        throw std::runtime_error("a service cannot go from STOPPING to GOINGDOWN.");
    }
    if(f_service_state == service_state_t::SERVICE_STATE_PAUSED)
    {
        SNAP_LOG_FATAL("service \"")(f_service_name)("\" cannot go from PAUSED to GOINGDOWN.");
        throw std::runtime_error("a service cannot go from PAUSED to GOINGDOWN.");
    }

    // if already going down, do nothing
    //
    if(f_service_state == service_state_t::SERVICE_STATE_GOINGDOWN)
    {
        return;
    }

    f_service_state = service_state_t::SERVICE_STATE_GOINGDOWN;

    process_stop();
}


/** \brief Pause this service if it is running.
 *
 * If the process of this service is not running, then nothing happens.
 *
 * If the process is running, then a "stop to pause" is initiated. This
 * is a normal stopping process, except that we do not change the state
 * of the service to SERVICE_STATE_STOPPING.
 */
void service::action_stop()
{
    // only switch to STOP if we are not already in that mode
    //
    if(f_service_state != service_state_t::SERVICE_STATE_STOPPING)
    {
        f_service_state = service_state_t::SERVICE_STATE_STOPPING;

        process_stop();
    }
}


/** \brief The stopping process was aborted or ended.
 *
 * Whenever the stopping process ends, it becomes idle again. This
 * happens whenever a process death is detected since the timer
 * associated with that service can now also be stopped.
 */
void service::action_idle()
{
    if(f_stopping_state != stopping_state_t::STOPPING_STATE_IDLE)
    {
        f_stopping_state = stopping_state_t::STOPPING_STATE_IDLE;

        // stop this timer since we avoided the stopping timeout
        // by detecting that the process stopped early enough
        //
        set_enable(false);
    }
}



void service::process_ready()
{
    // well that process is not stopped so we cannot start it anyway
    //
    if(!f_process.is_stopped())
    {
        return;
    }

    // verify that all dependencies are registered
    // 
    for( auto const & s : f_depends_list )
    {
        auto const & svc(s.lock());
        if(svc
        && !svc->is_registered())
        {
            // Not quite ready to start... wait next event and check again
            //
            SNAP_LOG_TRACE("Dependency service '")
                          (svc->get_service_name())
                          ("' has not yet started for dependent service '")
                          (f_service_name)
                          ("'. Deferring start.");
            return;
        }
    }

    // the process can be started now, do so
    //
    // Note: if the following call fails, a callback will automatically
    //       be called so we have nothing to do here to handle error cases
    //
    f_process.action_start();
}


/** \brief Execute the stop process.
 *
 * This function initiates the stop process. This is a sub-state machine
 * that handles the stopping of a process. It has a few states as
 * defined at the top of the file (i.e. idle, stop, terminate, kill).
 *
 * It gets called by various functions that want to stop the process
 * such as the action_stop() and action_godown() functions.
 *
 * Further calls after the timers time out are redirected to the
 * process_stop_timeout() function instead which knows how to escalate
 * the state to the next level.
 */
void service::process_stop()
{
    // already dead?
    //
    if(!is_running())
    {
        process_died();
        return;
    }

    // service already in the stop process?
    //
    if(f_stopping_state != stopping_state_t::STOPPING_STATE_IDLE)
    {
        return;
    }

    // before we can stop this process, all its pre-requirements
    // need to have stopped, check those first
    //
    service::weak_vector_t running_prereqs;
    std::copy_if(
            f_prereqs_list.begin(),
            f_prereqs_list.end(),
            std::back_inserter(running_prereqs),
            is_process_running);

    if(!running_prereqs.empty())
    {
        // there are pre-requirements, stop them first, when one dies
        // we get a callback called (process_died() or process_pause())
        // and we can react by trying to initiate the stop process
        // from that point if required
        //
        std::for_each(
                running_prereqs.begin(),
                running_prereqs.end(),
                [this](auto const & s)
                {
                    auto const & svc(s.lock());
                    if(!svc)
                    {
                        // this should not happen since we do not include
                        // entries that cannot be locked
                        //
                        return;
                    }
                    switch(this->f_service_state)
                    {
                    case service_state_t::SERVICE_STATE_DISABLED:
                    case service_state_t::SERVICE_STATE_PAUSED:
                        common::fatal_error( QString("service::process_stop(): service \"%1\" was selected as a running_prereqs even though the state is DISABLED or PAUSED.").arg(f_service_name) );
                        snap::NOTREACHED();
                        break;

                    case service_state_t::SERVICE_STATE_READY:
                        // although this could be action_stop(), it is not
                        // because the only one initiating a full stop is
                        // the snapinit process (see terminate_services())
                        //
                        // so in this case we have a dependency which is
                        // PAUSED and thus need to go down
                        //
                        svc->action_godown();
                        break;

                    case service_state_t::SERVICE_STATE_GOINGDOWN:
                    case service_state_t::SERVICE_STATE_STOPPING:
                        if(svc->f_service_state == service_state_t::SERVICE_STATE_STOPPING)
                        {
                            // if still IDLE then we need to give it a kick
                            //
                            svc->process_stop();
                        }
                        else
                        {
                            // not yet marked as stopping, make sure it is now
                            //
                            svc->action_stop();
                        }
                        break;

                    }
                });

        // we cannot yet initiate this service stop process, just return
        //
        return;
    }

    process_stop_initiate();
}


void service::process_stop_initiate()
{
    // the snapinit service "dies" immediately (at least
    // figuratively as far as the service object is concerned)
    //
    if(f_service_name == "snapinit")
    {
        // we cannot directly call the f_process.ation_died()
        // because doing so we start a recursive call which
        // somehow breaks the whole STOPPING process
        //
        // so instead we use our timer to return to the
        // snapcommunicator::run() function and get
        // called back through the process_timeout() function
        // (and that better matches what happens with other
        // processes.)
        //
        f_stopping_state = stopping_state_t::STOPPING_STATE_STOP;

        set_enable(true);
        set_timeout_date(snap::snap_communicator::get_current_date());
        return;
    }

    if(is_registered())
    {
        f_stopping_state = stopping_state_t::STOPPING_STATE_STOP;

        // process is registered so we can attempt to send a STOP
        // command to get started
        //
        snap::snap_communicator_message stop_message;
        if(is_snapcommunicator())
        {
            // for the snapcommunicator we need to send UNREGISTER
            // instead of a STOP message
            //
            stop_message.set_command("UNREGISTER");
            stop_message.add_parameter("service", "snapinit");
        }
        else
        {
            stop_message.set_service(f_service_name);
            stop_message.set_command("STOP");
        }
        snap_init_ptr()->send_message( stop_message );

        // this may not work so we use the timer to know what to do next
        //
        set_enable(true);
        set_timeout_date(snap::snap_communicator::get_current_date() + SERVICE_STOP_DELAY);
    }
    else
    {
        // the process is not registered so attempting to send a STOP
        // message would be futile, instead send a SIGTERM immediately
        //
        process_stop_terminate();
    }
}


void service::process_stop_timeout()
{
    if(f_service_name == "snapinit")
    {
        // the snapinit service "dies" after a process_timeout()
        // which ends up calling this function
        //
        f_process.action_died(termination_t::TERMINATION_NORMAL);
        return;
    }

    switch(f_stopping_state)
    {
    case stopping_state_t::STOPPING_STATE_IDLE:
        // on the timeout we cannot still be in IDLE mode...
        // (i.e. if we "properly died," then the timer is off, or we
        // have been removed from the list of services, or we went
        // back to "Ready Service".
        //
        common::fatal_error( QString("service::process_stop_timeout(): service \"%1\" got a timeout on a stop while in IDLE mode.").arg(f_service_name) );
        snap::NOTREACHED();

    case stopping_state_t::STOPPING_STATE_STOP:
        process_stop_terminate();
        break;

    case stopping_state_t::STOPPING_STATE_TERMINATE:
        process_stop_kill();
        break;

    case stopping_state_t::STOPPING_STATE_KILL:
        // there is nothing more to do...
        //
        // and even the exit() from the fatal_error() function will probably
        // not help in this case (i.e. we will be stuck until administrator
        // intervention)
        //
        common::fatal_error( QString("service::process_stop_timeout(): could not stop process for service \"%1\" even with a SIGKILL...").arg(f_service_name) );
        snap::NOTREACHED();

    }
}


void service::process_stop_terminate()
{
    f_stopping_state = stopping_state_t::STOPPING_STATE_TERMINATE;

    if(!f_process.kill_process(SIGTERM))
    {
        // could not send SIGTERM, try again with the SIGKILL which is
        // likely to fail just the same
        //
        process_stop_kill();
        return;
    }

    // this may not work so we use the timer to know what to do next
    //
    set_enable(true);
    set_timeout_date(snap::snap_communicator::get_current_date() + SERVICE_TERMINATE_DELAY);
}


void service::process_stop_kill()
{
    f_stopping_state = stopping_state_t::STOPPING_STATE_KILL;

    if(!f_process.kill_process(SIGKILL))
    {
        // we are stuck in this case (i.e. snapinit cannot kill
        // snapmanagerdaemon if it runs as root and did not accept
        // the STOP message)
        //
        common::fatal_error( QString("service::process_stop_kill(): could not send SIGKILL to process of service \"%1\".").arg(f_service_name) );
        snap::NOTREACHED();
    }

    // this may not work so we use the timer to know what to do next
    //
    set_enable(true);
    set_timeout_date(snap::snap_communicator::get_current_date() + SERVICE_TERMINATE_DELAY);
}


/** \brief The process died.
 *
 * This function gets called whenever the process of this service dies.
 *
 * The function decides what is the next action to perform:
 *
 * \li cron task -- in this case, we simply re-enable the timer and
 *                  will re-run the task again on the next tick
 *
 * \li other task -- in all other cases, we wait a little while
 *                   (QUICK_RETRY_INTERVAL) and try to start
 *                   the process again
 *
 * At this point, we do not do anything about the services that depend
 * on this service because the retry will happen very quickly.
 */
void service::process_died()
{
    // this service process is now dead, reflect that in the stopping state
    //
    action_idle();

    //SNAP_LOG_TRACE("service received call to process_died() for \"")(f_service_name)("\" when service status is \"")(state_to_string(f_service_state))("\"");

    // if service is still READY, restart the timer and let it go
    // to the next timeout
    //
    switch(f_service_state)
    {
    case service_state_t::SERVICE_STATE_DISABLED:
        // this is not considered a valid state in this case
        //
        common::fatal_error( QString("service::process_died() was called when service \"%1\" is in DISABLED state.").arg(f_service_name) );
        snap::NOTREACHED();
        return;

    case service_state_t::SERVICE_STATE_READY:
        // state remains the same, pause for a while and then will
        // restart whenever we get awaken
        //
        if(is_cron_task())
        {
            // this is the normal way the cron process is expected to die
            //
            // setup the next tick and re-enable the timer
            //
            compute_next_tick(true);
            set_enable(true);
            return;
        }

        // wait a little bit and try to start the process again
        //
        set_enable(true);
        set_timeout_date(snap::snap_communicator::get_current_date() + QUICK_RETRY_INTERVAL);
        break;

    case service_state_t::SERVICE_STATE_PAUSED:
        // we already are in the PAUSED state
        //
        common::fatal_error( QString("service::process_died() was called when service \"%1\" is in PAUSED state.").arg(f_service_name) );
        snap::NOTREACHED();
        return;

    case service_state_t::SERVICE_STATE_GOINGDOWN:
        // a service that was asked to go down is now down
        //
        process_wentdown();
        break;

    case service_state_t::SERVICE_STATE_STOPPING:
        // we were stopped, remove ourselves from the snapinit environment
        //
        snap_init_ptr()->remove_service(shared_from_this());

        // check whether other processes can now be stopped; when we
        // are in this state, all the services are already set to
        // state STOPPING so we know we can directly call process_stop()
        //
        std::for_each(
                f_depends_list.begin(),
                f_depends_list.end(),
                [](auto const & s)
                {
                    auto const svc(s.lock());
                    if(!svc)
                    {
                        return;
                    }
                    if(svc->f_service_state == service_state_t::SERVICE_STATE_STOPPING)
                    {
                        // if still IDLE then we need to give it a kick
                        //
                        svc->process_stop();
                    }
                    else
                    {
                        // not yet marked as stopping, make sure it is now
                        //
                        svc->action_stop();
                    }
                });
        break;

    }
}


/** \brief Pause this service for a while.
 *
 * In this case, the process died too quickly (within
 * process::MAX_START_INTERVAL between the start and end)
 * and too many times
 * (i.e. process::MAX_START_COUNT times in a row), so the
 * process is asking us to take a break.
 *
 * The following function react differently depending on the
 * type of service that died too quickly:
 *
 * \li cron task -- raise an exception because this should never happen;
 *                  the cron task is expected to always be handled by the
 *                  process_died() callback
 * \li required task -- in this case we ask snapinit to terminate immediately;
 *                      required task just are cannot be paused
 * \li no recovery task -- this service gets removed from the list of
 *                         services; it won't run until snapinit gets
 *                         restarted
 * \li other tasks -- all the other tasks get paused for a while, for an
 *                    amount of seconds as defined by f_recovery; in this
 *                    case, the tasks pre-requirements are asked to STOP
 */
void service::process_pause()
{
    // this service process is now dead, reflect that in the stopping state
    //
    action_idle();

    // whatever the state, it is never legal to get a pause on the CRON
    // service (it should have died first and not get restarted so quickly)
    //
    if(is_cron_task())
    {
        common::fatal_error( QString("service::process_pause() was called with the CRON task (\"%1\").").arg(f_service_name) );
        snap::NOTREACHED();
    }

    // whatever the state, required services cannot be paused for
    // any amount of time
    //
    if(f_required)
    {
        // first remove ourselves
        //
        f_service_state = service_state_t::SERVICE_STATE_STOPPING;
        snap_init_ptr()->remove_service(shared_from_this());

        // then make sure to terminate snapinit
        //
        snap_init_ptr()->terminate_services();
        return;
    }

    // whatever the state, when there is no recovery time, it means we
    // forget about that service altogether--it will be restarted the
    // next time snapinit starts
    //
    if(f_recovery == 0)
    {
        // just remove ourselves
        //
        f_service_state = service_state_t::SERVICE_STATE_STOPPING;
        snap_init_ptr()->remove_service(shared_from_this());
        return;
    }

    // now the current state may be important, we cannot just pause
    // from any state to any state
    //
    switch(f_service_state)
    {
    case service_state_t::SERVICE_STATE_DISABLED:
        // this is not considered a valid state in this case
        //
        common::fatal_error( QString("service::process_pause() was called when service \"%1\" is in DISABLED state.").arg(f_service_name) );
        snap::NOTREACHED();
        return;

    case service_state_t::SERVICE_STATE_READY:
        // we continue with the code below; this is the expected state
        // when we receive this event
        //
        break;

    case service_state_t::SERVICE_STATE_PAUSED:
        // this is not considered a valid state in this case
        //
        common::fatal_error( QString("service::process_pause() was called when service \"%1\" is in PAUSED state.").arg(f_service_name) );
        snap::NOTREACHED();
        return;

    case service_state_t::SERVICE_STATE_GOINGDOWN:
        // this is not expected, but if it occurs, we want to go
        // to PAUSED just like a READY process
        //
        // that being said, we also may need to wake up our pre-required
        // services if we were the last process to die and pause
        //
        process_wentdown();

        // so here we go on!
        //
        break;

    case service_state_t::SERVICE_STATE_STOPPING:
        // we were stopped, remove ourselves from the snapinit environment
        //
        snap_init_ptr()->remove_service(shared_from_this());
        return;

    }

    // we are PAUSED for now
    //
    // this means we do not go back to being READY right away, instead we
    // stop our pre-required services if any and then sleep for a while
    // before trying to restart ourselves
    //
    f_service_state = service_state_t::SERVICE_STATE_PAUSED;

    // check whether we have pre-required servics still running
    //
    service::weak_vector_t running_prereqs;
    auto is_process_running_and_has_a_strong_connection = [this](auto const & s)
    {
        auto const svc(s.lock());
        if(!svc)
        {
            // no service at that index
            return false;
        }
        if(!svc->is_running())
        {
            // service is not running, ignore
            return false;
        }

        // service is running, we need to nkow whether it is a weak
        // dependency because if so we can ignore it (i.e. return false)
        //
        // WARNING: we are testing prereqs, so the weak dependency test
        //          looks "swapped" (i.e. 
        //
        return !svc->is_weak_dependency( this->get_service_name() );
    };
    std::copy_if(
            f_prereqs_list.begin(),
            f_prereqs_list.end(),
            std::back_inserter(running_prereqs),
            is_process_running_and_has_a_strong_connection);

    if(running_prereqs.empty())
    {
        // setup a long pause and on the next tick try to start
        // the process anew
        //
        start_pause_timer();
    }
    else
    {
        // a long pause means that pre-requirements need to be stopped
        // (forced into a "long pause" themselves, although they really
        // go to "Go Down" until stopped and then go back to "Ready Service"
        // and wait for their dependency to wake up again!)
        //
        std::for_each(
                running_prereqs.begin(),
                running_prereqs.end(),
                [](auto const & s)
                {
                    auto const svc(s.lock());
                    if(!svc)
                    {
                        // this should not happen, we already checked in
                        // the copy_if() above
                        //
                        return;
                    }
                    svc->action_godown();
                });
    }
}


/** \brief Start the service timer to wait for its long pause.
 *
 * This function is called After all pre-required services of a service
 * are down. It is used to know when to try to restart.
 *
 * \note
 * This pause is not initiated at the time the process_pause() gets
 * called because it's pre-required services may take a long time
 * to come down and we do not want the timer to timeout before we
 * are down with the "Service GoDown" process.
 *
 * \sa process_pause()
 * \sa process_prereqs_down()
 */
void service::start_pause_timer()
{
    set_enable(true);
    set_timeout_date(snap::snap_communicator::get_current_date() + f_recovery * common::SECONDS_TO_MICROSECONDS);
}


/** \brief Check whether all pre-required services are down.
 *
 * After a process_pause() callback is received, pre-requirements of
 * that service are asked to go down. If all were already down, then
 * we do not reach here.
 *
 * This function sets up the recovery timeout of this process if all
 * of its pre-required services were down.
 */
void service::process_prereqs_down()
{
    auto const it(std::find_if(
            f_prereqs_list.begin(),
            f_prereqs_list.end(),
            is_process_running));
    if(it != f_prereqs_list.end())
    {
        // all pre-required services are down, we can now start the
        // pause timer
        //
        start_pause_timer();
        return;
    }

    // some pre-required services are still running...
}


/** \brief This service process died while in its "Service GoDown" state.
 *
 * The process was asked to go down and successfully did so. Now the may
 * have to wake up the paused process (which initiated this process for
 * us to go down.)
 *
 * The service also changes back to "Ready Service".
 */
void service::process_wentdown()
{
    std::for_each(
            f_depends_list.begin(),
            f_depends_list.end(),
            [](auto const & s)
            {
                // any of our dependencies which are paused need us to
                // call the process_prereqs_down() to make sure we
                // start the pause timer if necessary
                //
                auto const & svc(s.lock());
                if(svc
                && svc->f_service_state == service_state_t::SERVICE_STATE_PAUSED)
                {
                    svc->process_prereqs_down();
                }
            });

    // Note: we have to call action_ready() here because the state of
    //       the paused process may have changed in such a way that
    //       this service can be restarted now (it should not happen
    //       but this is cleaner, none the less.)
    //
    action_ready();
}


/** \brief Act on the fact that the process changed status.
 *
 * Whenever a process changes its status, we want to make sure that we
 * react accordingly. This means:
 *
 * \li the service is in the SERVICE_STATE_STOPPING -- make
 *     sure that all pre-requirements know they have to stop
 *
 * \li process was registered -- then we want to check whether this
 *     process has pre-requirements, if so, then mark them as ready
 *     meaning that it will start these processes if they finally
 *     got all their dependencies registered
 *
 * \li process was unregistered, died, etc. -- then we want to either
 *     stop or pause the pre-requirements
 */
void service::process_status_changed()
{
    if(is_registered())
    {
        // Going to registered means we need to give a little kick to
        // the sleeping services waiting on a dependency to be ready.
        // In many cases nothing happens, in many cases a new process
        // gets started
        //
        std::for_each(
                f_prereqs_list.begin(),
                f_prereqs_list.end(),
                [](auto const & s)
                {
                    auto const & svc(s.lock());
                    if(!svc)
                    {
                        return;
                    }
                    switch(svc->f_service_state)
                    {
                    case service_state_t::SERVICE_STATE_READY:
                        svc->process_ready();
                        break;

                    default:
                        // do nothing to the other services, they will
                        // wake up in a different way
                        break;

                    }
                });
    }
}


void service::set_service_index(int index)
{
    f_service_index = index;
}


int service::get_service_index() const
{
    return f_service_index;
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
 * sends the signal f_stopping_signal using kill(). At first, the signal
 * is SIGTERM and then SIGKILL. If both signals fail to stop
 * the process, we ignore the failure and quit anyway.
 */
void service::process_timeout()
{
    // always disable the service timer on receipt of a timeout
    // we will re-enable it as required
    //
    set_enable(false);

    switch(f_service_state)
    {
    case service_state_t::SERVICE_STATE_DISABLED:
        // do nothing when disabled
        break;

    case service_state_t::SERVICE_STATE_READY:
        // try to start the process if not already running
        //
        process_ready();
        break;

    case service_state_t::SERVICE_STATE_PAUSED:
        // the pause timed out, we may be able to go back to ready
        // for some time (until next crash at least...)
        //
        action_ready();
        break;

    case service_state_t::SERVICE_STATE_GOINGDOWN:
    case service_state_t::SERVICE_STATE_STOPPING:
        // make sure the process gets stopped
        // i.e. if the STOP timed out, send SIGTERM,
        //      if SIGTERM timed out, send SIGKILL,
        //      if SIGKILL timed out, ???
        //
        process_stop_timeout();
        break;

    }
}




bool service::is_dependency_of( QString const & service_name )
{
    return f_dep_name_list.end() != std::find_if(
                                        f_dep_name_list.begin(),
                                        f_dep_name_list.end(),
                                        [&service_name](auto const & dependency)
                                        {
                                            return dependency.f_service_name == service_name;
                                        });
}


/** \brief For a CRON task, we have to compute the next tick.
 *
 * CRON tasks run when a specific tick happens. If the process
 * is still running when the tick happens, then the service
 * ignores that tick, which is considered lost.
 */
void service::compute_next_tick(bool just_ran)
{
    // when the cron task does not start properly, we set a timeout
    // delay of a few seconds, which needs to be reset
    //
    set_timeout_delay(-1);

    // compute the tick exactly on 'now' or just before now
    //
    // current time
    int64_t const now(snap::snap_child::get_current_date() / common::SECONDS_TO_MICROSECONDS);
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
    QString const spool_path(snap_init_ptr()->get_spool_path());
    QString const spool_filename(QString("%1/%2.txt").arg(spool_path).arg(f_service_name));
    QFile spool_file(spool_filename);
    if(!spool_file.open(QIODevice::ReadWrite))
    {
        // since we open in R/W it has to succeed, although it could be empty
        //
        SNAP_LOG_ERROR("cron service \"")
                      (f_service_name)
                      ("\" could not open its spool file \"")
                      (spool_filename)
                      ("\".");
        return;
    }
    QByteArray const data(spool_file.readAll());
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
    int64_t timestamp(0);
    if(ok)
    {
        if(just_ran && last_tick == latest_tick)
        {
            // this one is the case when we have to move forward
            //
            //    latest_tick + f_cron > now  (i.e. in the future)
            //
            latest_tick += f_cron;
            timestamp = latest_tick * common::SECONDS_TO_MICROSECONDS;
        }
        else if(last_tick >= latest_tick)
        {
            // last_tick is now or in the future so we can keep it
            // as is (happen often when starting snapinit)
            //
            timestamp = last_tick * common::SECONDS_TO_MICROSECONDS;
            update = false;
        }
        else
        {
            // this looks like we may have missed a tick or two
            // so this task already timed out...
            //
            timestamp = latest_tick * common::SECONDS_TO_MICROSECONDS;
        }
    }
    else
    {
        // never ran, use this latest tick so we run that process
        // once as soon as possible.
        //
        timestamp = latest_tick * common::SECONDS_TO_MICROSECONDS;
    }

    if(update)
    {
        // reset the file
        spool_file.seek(0);
        spool_file.resize(0);

        // then write the new tick timestop
        spool_file.write(QString("%1").arg(latest_tick).toUtf8());
    }

    SNAP_LOG_TRACE("service::compute_next_tick(): timestamp = ")(timestamp);
    set_timeout_date(timestamp);
}



std::shared_ptr<snap_init> service::snap_init_ptr()
{
    snap_init::pointer_t locked(f_snap_init.lock());
    if(!locked)
    {
        common::fatal_error("service::snap_init_ptr(): somehow we could not get a lock on f_snap_init from a service object.");
        snap::NOTREACHED();
    }
    return locked;
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
std::string service::get_snapcommunicator_string() const
{
    std::stringstream ss;
    ss << f_snapcommunicator_addr
       << ":"
       << f_snapcommunicator_port;
    return ss.str();
}


QString const & service::get_snapcommunicator_addr() const
{
    return f_snapcommunicator_addr;
}


int service::get_snapcommunicator_port() const
{
    return f_snapcommunicator_port;
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
    return f_process.is_running();
}




/** \brief Check whether a link between two services is weak.
 *
 * This function check wether the user defined the dependency
 * between this service and its \p servive_name dependency
 * to know whether it is a weak dependency or not. Most
 * dependencies are not weak.
 *
 * At this time this function is only used when building the
 * dependency tree (See the --tree command line option.)
 *
 * \param[in] service_name  The name of the dependency to check out.
 *
 * \return true if the named dependency is marked as weak.
 */
bool service::is_weak_dependency( QString const & service_name )
{
    auto const & dependency(std::find_if(
            f_dep_name_list.begin(),
            f_dep_name_list.end(),
            [service_name](auto const & dep)
            {
                return dep.f_service_name == service_name;
            }));

    if(dependency == f_dep_name_list.end())
    {
        throw std::logic_error("somehow we could not find a service within its row dependency definition list.");
    }

    return dependency->f_type == dependency_t::dependency_type_t::DEPENDENCY_TYPE_WEAK;
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
bool service::is_snapcommunicator() const
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



/** \brief Returns the registration status with snapcommunicator.
 *
 * A process that snapinit starts has to register itself with
 * snapcommunicator. When that happens, the snapinit system
 * receives a STATUS message with the status set to "up",
 * meaning that the process is registered, or "down", meaning
 * that the process was unregistered.
 *
 * This function returns the current status of that flag (which
 * is a process state.)
 *
 * A registered process is expected to accept messages sent to
 * it. However, a status of 'true' does not guarantee that the
 * process will receive your messages. However, a status of false
 * definitely means that messages cannot be sent to that service.
 *
 * \return true if the process is currently considered regitered.
 */
bool service::is_registered() const
{
    return f_process.is_registered();
}


/** \brief Check whether the process is currently paused.
 *
 * A process that failed too many times in a raw gets paused for
 * a while. This function checks whether the srvice is in that
 * state.
 *
 * It is current used by the tree generator to create a run time
 * tree for snapmanager.cgi.
 *
 * \return true if the process is currently paused.
 */
bool service::is_paused() const
{
    return f_service_state == service_state_t::SERVICE_STATE_PAUSED;
}


/** \brief Let you know whether th service was marked as being disabled.
 *
 * At this time this flag is only available when the --list or --tree
 * command line options are used. Later we may want to always load all
 * the possible services and make the distinction at run time in order
 * to allow runtime enabling services.
 *
 * \note
 * It could be useful to show the current status and see that some
 * services died. Yet again some features do not support that yet.
 *
 * \return true if the service was marked as disabled.
 */
bool service::is_disabled() const
{
    return f_disabled;
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
bool service::is_cron_task() const
{
    return f_cron != 0;
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


/** \brief Transform the current state to a string for display.
 *
 * This function transforms the specified state into a string.
 *
 * \param[in] state  The state to convert to a string.
 *
 * \return A string naming the specified state.
 */
char const * service::state_to_string( service_state_t const state )
{
    switch( state )
    {
    case service_state_t::SERVICE_STATE_DISABLED:
        return "SERVICE_STATE_DISABLED";

    case service_state_t::SERVICE_STATE_READY:
        return "SERVICE_STATE_READY";

    case service_state_t::SERVICE_STATE_PAUSED:
        return "SERVICE_STATE_PAUSED";

    case service_state_t::SERVICE_STATE_GOINGDOWN:
        return "SERVICE_STATE_GOINGDOWN";

    case service_state_t::SERVICE_STATE_STOPPING:
        return "SERVICE_STATE_STOPPING";

    }
    return "SERVICE_STATE_UNKNOWN";
}



} // namespace snapinit
// vim: ts=4 sw=4 et
