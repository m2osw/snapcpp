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

#include "snapwebsites.h"
#include "snap_communicator.h"
#include "log.h"
#include "mkdir_p.h"
#include "not_used.h"

#include <QFile>

#include <fcntl.h>
#include <proc/sysinfo.h>
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

/** \file
 * \brief Initialize a Snap! server on your server.
 *
 * This tool is the snapserver controller, used to start and stop the
 * server and backend processes.
 *
 * The tool is actually in charge of starting all the elements that can
 * be started on a Snap! server:
 *
 * \li snapinit -- snapinit gets started by script /etc/init.d/snapserver
 *     (we will later make it compatible with the new boot system, though)
 * \li snapcommunicator -- the RPC system used by snap to communicate
 *     between all the servers used by snap.
 * \li snapserver -- the actual snap server listening for incoming client
 *     connections (through Apache2 and snap.cgi for now)
 * \li snapbackend -- various backends to support working on slow tasks
 *     so front ends do not have to do those slow task and have the client
 *     wait for too long... (i.e. images, pagelist, sendmail, ...)
 * \li snapwatchdogserver -- a server which checks various things to
 *     determine the health of the server it is running on
 * \li "snapcron" -- this task actually makes use of snapbackend without
 *     the --action command line option; it runs tasks that are to be
 *     run once in a while (by default every 5 minutes) such as clean ups,
 *     aggregation, etc.
 *
 * The snapinit tool reads a snapinit.xml file, by default it is expected
 * to be found under /etc/snapwebsites. That file declares any number of
 * parameters as required by the snapinit tool to start the service.
 *
 * A sample XML is briefly shown here:
 *
 * \code{.xml}
 *    <?xml version="1.0"?>
 *    <snapservices>
 *      <!-- Snap Communicator is started as a service -->
 *      <service name="snapcommunicator" required="required">
 *        <!-- we give this one a very low priority as it has to be started
 *             before anything else -->
 *        <priority>-10</priority>
 *        <config>/etc/snapwebsites/snapcommunicator.conf</config>
 *        <connect>127.0.0.1:4040</connect>
 *        <wait>10</wait>
 *      </service>
 *      <service name="snapserver" required="required">
 *        <!-- the server should be started before the other backend
 *             services because the first time it initializes things
 *             that other backends need to run as expected; although
 *             some of that will change -->
 *        <priority>0</priority>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *      </service>
 *      <!-- various other services; these get a priority of 50 (default)
 *           and may use the same configuration file as the snapserver
 *           service -->
 *      <service name="sendmail">
 *        <command>/usr/bin/snapbackend</command>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *      </service>
 *      <!-- you may use the disabled flag to prevent starting this
 *           service. -->
 *      <service name="pagelist" disabled="disabled">
 *        <!-- commands are generally defined using a full path, while
 *             doing development, though, you may use just the name of
 *             the binary and use the --binary-path <path> on the
 *             command line -->
 *        <command>/usr/bin/snapbackend</command>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *      </service>
 *      <service name="images">
 *        <command>/usr/bin/snapbackend</command>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *      </service>
 *      <service name="snapwatchdog">
 *        <command>/usr/bin/snapwatchdogserver</command>
 *        <priority>90</priority>
 *        <config>/etc/snapwebsites/snapwatchdog.conf</config>
 *      </service>
 *      <service name="backend">
 *        <priority>75</priority>
 *        <config>/etc/snapwebsites/snapserver.conf</config>
 *        <cron>300</cron>
 *      </service>
 *    </snapservices>
 * \endcode
 *
 * TBD: since each backend service can be run only once, we may want to
 *      look in having this XML file as a common file and the definitions
 *      would include server names where the services are expected to run
 *      when things are normal and which server to use as fallbacks when
 *      something goes wrong. Right now, I think that I will keep it simpler.
 *      The sharing of the XML could be done via snapcommunitor or Cassandra
 *      but then that would mean snapinit would have to know how to start
 *      snapcommunicator without the XML...
 *
 * The following is an attempt at describing the process messages used
 * to start everything and stop everything:
 *
 * \msc
 * hscale = "2";
 * a [label="snapinit"],
 * b [label="snapcommunicator"],
 * c [label="snapserver"],
 * d [label="snapbackend (permanent)"],
 * e [label="snapbackend (cron)"],
 * f [label="neighbors"],
 * g [label="snapsignal"];
 *
 * d note d [label="images, page_list, sendmail,snapwatchdog"];
 *
 * #
 * # snapinit initialization
 * #
 * a=>a [label="init()"];
 * a=>a [label="--detach (optional)"];
 * |||;
 * ... [label="pause (0 seconds)"];
 * |||;
 * a=>>a [label="connection timeout"];
 * a=>b [label="start (fork+execv)"];
 * |||;
 * b>>a;
 *
 * #
 * # snapcommunicator initialization
 * #
 * b=>b [label="open socket to neighbor"];
 * b->f [label="CONNECT type=frontend ..."];
 * f->b [label="ACCEPT type=backend ..."];
 * ... [label="or"];
 * f->b [label="REFUSE type=backend"];
 * |||;
 * ... [label="neighbors may try to connect too"];
 * |||;
 * f=>f [label="open socket to neighbor"];
 * f->b [label="CONNECT type=backend ..."];
 * b->f [label="ACCEPT type=frontend ..."];
 * ... [label="or"];
 * b->f [label="REFUSE type=frontend"];
 *
 * #
 * # snapinit registers with snapcommunicator
 * #
 * |||;
 * ... [label="pause (10 seconds)"];
 * |||;
 * a=>a [label="open socket to snapcommunicator"];
 * a->b [label="REGISTER service=snapinit;version=<version>"];
 * b->a [label="READY"];
 * a->b [label="SERVICES list=...depends on snapinit.xml..."];
 * a=>a [label="wakeup services"];
 * |||;
 * b->a [label="HELP"];
 * a->b [label="COMMANDS list=HELP,QUITTING,READY,STOP"];
 *
 * #
 * # snapinit starts snapserver which registers with snapcommunicator
 * #
 * |||;
 * ... [label="pause (0 seconds)"];
 * |||;
 * --- [label="...start snapserver..."];
 * a=>>a [label="connection timeout"];
 * a=>c [label="start (fork+execv)"];
 * c>>a;
 * c=>c [label="open socket to snapcommunicator"];
 * c->b [label="REGISTER service=snapserver;version=<version>"];
 * b->c [label="READY"];
 *
 * #
 * # snapinit starts various backends (images, sendmail, ...)
 * #
 * |||;
 * ... [label="pause (<wait> seconds, at least 1 second)"];
 * |||;
 * --- [label="...(start repeat for each backend)..."];
 * a=>>a [label="connection timeout"];
 * a=>d [label="start (fork+execv)"];
 * d>>a;
 * d=>d [label="open socket to snapcommunicator"];
 * d->b [label="REGISTER service=<service name>;version=<version>"];
 * b->d [label="READY"];
 * b->d [label="STATUS service=snapwatchdog"];
 * |||;
 * ... [label="pause (<wait> seconds, at least 1 second)"];
 * |||;
 * --- [label="...(end repeat)..."];
 *
 * #
 * # snapinit starts snapback (CRON task)
 * #
 * |||;
 * ... [label="...cron task, run once per timer tick event..."];
 * |||;
 * a=>>a [label="CRON timer tick"];
 * a=>a [label="if CRON tasks still running, return immediately"];
 * a=>e [label="start (fork+execv)"];
 * e>>a;
 * e=>e [label="open socket to snapcommunicator"];
 * e->b [label="REGISTER service=snapbackend;version=<version>"];
 * b->e [label="READY"];
 * |||;
 * e=>>e [label="run CRON task 1"];
 * e=>>e [label="run CRON task 2"];
 * ...;
 * e=>>e [label="run CRON task n"];
 * |||;
 * e->b [label="UNREGISTER service=snapbackend"];
 * |||;
 * ... [label="...(end of cron task)..."];
 *
 * #
 * # STOP process
 * #
 * |||;
 * --- [label="snapinit STOP process with: 'snapinit stop' or 'snapsignal snapinit/STOP'"];
 *
 * |||;
 * g->b [label="'snapsignal snapinit/STOP' command sends STOP to snapcommunicator"];
 * b->a [label="STOP"];
 * ... [label="...or..."];
 * a->a [label="'snapinit stop' command sends STOP to snapinit"];
 * ...;
 * a->b [label="UNREGISTER service=snapinit"];
 * a->b [label="STOP"];
 * b->c [label="snapserver/STOP"];
 * b->d [label="<service name>/STOP"];
 * b->e [label="snapbackend/STOP"];
 * c->b [label="UNREGISTER service=snapserver"];
 * c->c [label="exit(0)"];
 * d->b [label="UNREGISTER service=<service name>"];
 * d->d [label="exit(0)"];
 * e->b [label="UNREGISTER service=snapbackend (if still running at the time)"];
 * e->e [label="exit(0)"];
 * ... [label="once all services are unregistered"];
 * b->f [label="DISCONNECT"];
 * \endmsc
 */

namespace
{


/** \brief Define whether the logger was initialized.
 *
 * This variable defines whether the logger was already initialized.
 */
bool g_logger_ready = false;


/** \brief Define whether the standard output stream is a TTY.
 *
 * This function defines whether 'stderr' is a TTY or not. If not
 * we assume that we were started as a deamon and we do not spit
 * out errors in stderr. If it is a TTY, then we also print a
 * message in the console making it easier to right away know
 * that the tool detected an error and did not start in the
 * background.
 */
bool g_isatty = false;


/** \brief List of configuration files.
 *
 * This variable is used as a list of configuration files. It is
 * empty here because the configuration file may include parameters
 * that are not otherwise defined as command line options.
 */
std::vector<std::string> const g_configuration_files;


/** \brief Command line options.
 *
 * This table includes all the options supported by the server.
 */
advgetopt::getopt::option const g_snapinit_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "Usage: %p [-<opt>] <start|restart|stop>",
        advgetopt::getopt::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "where -<opt> is one or more of:",
        advgetopt::getopt::help_argument
    },
    {
        'b',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "binary-path",
        "/usr/bin",
        "Path where snap! binaries can be found (e.g. snapserver and snapbackend).",
        advgetopt::getopt::optional_argument
    },
    {
        'c',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        "/etc/snapwebsites/snapinit.conf",
        "Configuration file to initialize snapinit.",
        advgetopt::getopt::optional_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "debug",
        nullptr,
        "Start the server and backend services in debug mode.",
        advgetopt::getopt::no_argument
    },
    {
        'd',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "detach",
        nullptr,
        "Background the snapinit server.",
        advgetopt::getopt::no_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show usage and exit.",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "list",
        nullptr,
        "Display the list of services and exit.",
        advgetopt::getopt::no_argument
    },
    {
        'k',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "lockdir",
        "/run/lock/snapwebsites",
        "Full path to the snapinit lockdir.",
        advgetopt::getopt::optional_argument
    },
    {
        'l',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "logfile",
        nullptr,
        "Full path to the snapinit logfile.",
        advgetopt::getopt::optional_argument
    },
    {
        'n',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "nolog",
        nullptr,
        "Only output to the console, not the log file.",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "remove-lock",
        nullptr,
        "For the removal of an existing lock (useful if a spurious lock still exists).",
        advgetopt::getopt::optional_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "running",
        nullptr,
        "test whether snapinit is running; exit with 0 if so, 1 otherwise.",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "show the version of the snapinit executable.",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        "start|restart|stop",
        advgetopt::getopt::default_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::end_of_options
    }
};



void fatal_error(QString msg) __attribute__ ((noreturn));
void fatal_error(QString msg)
{
    SNAP_LOG_FATAL(msg);
    QByteArray utf8(msg.toUtf8());
    syslog( LOG_CRIT, "%s", utf8.data() );
    if(g_isatty)
    {
        std::cerr << "snapinit: fatal error: " << utf8.data() << std::endl;
    }
    exit(1);
    snap::NOTREACHED();
}



}
//namespace


// passed as the "parent" of each service
class snap_init;




/////////////////////////////////////////////////
// SERVICE (class declaration)                 //
/////////////////////////////////////////////////

class service
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<service>        pointer_t;
    typedef std::vector<pointer_t>          vector_t;
    typedef std::map<QString, pointer_t>    map_t;

    static int const            MAX_START_COUNT = 5;
    static int64_t const        MAX_START_INTERVAL = 60LL * 1000000LL; // 1 minute in microseconds
    static int const            DEFAULT_PRIORITY = 50;

                                service( std::shared_ptr<snap_init> si );

    void                        configure(QDomElement e, QString const & binary_path, bool const debug, bool const ignore_path_check);

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout();

    bool                        exists() const;
    bool                        run();
    bool                        is_running() const;
    bool                        is_service_required();
    void                        set_stopping();
    bool                        is_stopping() const;
    bool                        has_stopped() const;
    bool                        is_connection_required() const;
    bool                        is_snapdbproxy() const;
    std::string                 get_connect_string() const;
    std::string                 get_snapdbproxy_string() const;
    bool                        is_safe_required() const;
    QString const &             get_safe_message() const;
    bool                        cron_task() const;
    QString const &             get_config_filename() const;
    QString const &             get_service_name() const;
    pid_t                       get_old_pid() const;
    bool                        failed() const;
    int                         get_wait_interval() const;
    int                         get_recovery() const;
    bool                        service_may_have_died();

    bool                        operator < (service const & rhs) const;

protected:
    pointer_t                   shared_from_this() const;

private:
    void                        compute_next_tick(bool just_ran);
    void                        mark_process_as_dead();

    std::weak_ptr<snap_init>    f_snap_init;
    QString                     f_full_path;
    QString                     f_config_filename;
    QString                     f_service_name;
    QString                     f_command;
    QString                     f_options;
    pid_t                       f_pid = 0;
    pid_t                       f_old_pid = 0;
    int                         f_short_run_count = 0;
    int64_t                     f_start_date = 0;       // in microseconds, to calculate an interval
    int                         f_wait_interval = 0;    // in seconds
    int                         f_recovery = 0;         // in seconds
    QString                     f_safe_message;
    rlim_t                      f_coredump_limit = 0;   // avoid core dump files by default
    bool                        f_started = false;
    bool                        f_failed = false;
    bool                        f_debug = false;
    bool                        f_required = false;
    int                         f_stopping = 0;
    QString                     f_snapcommunicator_addr;         // to connect with snapcommunicator
    int                         f_snapcommunicator_port = 0;     // to connect with snapcommunicator
    QString                     f_snapdbproxy_addr;     // to connect with snapdbproxy
    int                         f_snapdbproxy_port = 0; // to connect with snapdbproxy
    int                         f_priority = DEFAULT_PRIORITY;
    int                         f_cron = 0;             // if 0, then off (i.e. not a cron task)
};






/////////////////////////////////////////////////
// SNAP INIT (class declaration)               //
/////////////////////////////////////////////////

class snap_init
        : public std::enable_shared_from_this<snap_init>
{
public:
    typedef std::shared_ptr<snap_init> pointer_t;

    enum class command_t
    {
        COMMAND_UNKNOWN,
        COMMAND_START,
        COMMAND_STOP,
        COMMAND_RESTART,
        COMMAND_LIST
    };

    /** \brief Handle incoming messages from Snap Communicator server.
     *
     * This class is an implementation of the TCP client message connection
     * used to accept messages received via the Snap Communicator server.
     */
    class listener_impl
            : public snap::snap_communicator::snap_tcp_client_message_connection
    {
    public:
        typedef std::shared_ptr<listener_impl>    pointer_t;

        /** \brief The listener initialization.
         *
         * The listener receives UDP messages from various sources (mainly
         * backends at this point.)
         *
         * \param[in] si  The snap init server we are listening for.
         * \param[in] addr  The address to listen on. Most often it is 127.0.0.1.
         * \param[in] port  The port to listen on (4040).
         */
        listener_impl(snap_init::pointer_t si, std::string const & addr, int port)
            : snap_tcp_client_message_connection(addr, port)
            , f_snap_init(si)
        {
        }

        // snap::snap_communicator::snap_server_connection implementation
        virtual void process_message(snap::snap_communicator_message const & message)
        {
            // we can call the same function for UDP and TCP messages
            f_snap_init->process_message(message, false);
        }

    private:
        // this is owned by a server function so no need for a smart pointer
        snap_init::pointer_t f_snap_init;
    };

    /** \brief Handle new connections from clients.
     *
     * This class is an implementation of the snap server connection so we can
     * handle new connections from various clients.
     */
    class ping_impl
            : public snap::snap_communicator::snap_udp_server_message_connection
    {
    public:
        typedef std::shared_ptr<ping_impl>    pointer_t;

        /** \brief The messager initialization.
         *
         * The messager receives UDP messages from various sources (mainly
         * backends at this point.)
         *
         * \param[in] si  The snap init server we are listening for.
         * \param[in] addr  The address to listen on. Most often it is 127.0.0.1.
         *                  for the UDP because we currently only allow for
         *                  local messages.
         * \param[in] port  The port to listen on.
         */
        ping_impl(snap_init::pointer_t si, std::string const & addr, int port)
            : snap_udp_server_message_connection(addr, port)
            , f_snap_init(si)
        {
        }

        // snap::snap_communicator::snap_udp_server_message_connection implementation
        virtual void process_message(snap::snap_communicator_message const & message)
        {
            // we can call the same function for UDP and TCP messages
            f_snap_init->process_message(message, true);
        }

    private:
        // this is owned by a server function so no need for a smart pointer
        snap_init::pointer_t f_snap_init;
    };

    /** \brief Handle the death of a child process.
     *
     * This class is an implementation of the snap signal connection so we can
     * get an event whenever one of our children dies.
     */
    class sigchld_impl
            : public snap::snap_communicator::snap_signal
    {
    public:
        typedef std::shared_ptr<sigchld_impl>    pointer_t;

        /** \brief The SIGCHLD signal initialization.
         *
         * The constructor defines this signal connection as a listener for
         * the SIGCHLD signal.
         *
         * \param[in] si  The snap init server we are listening for.
         */
        sigchld_impl(snap_init::pointer_t si)
            : snap_signal(SIGCHLD)
            , f_snap_init(si)
        {
        }

        // snap::snap_communicator::snap_signal implementation
        virtual void process_signal()
        {
            // we can call the same function
            f_snap_init->service_died();
        }

    private:
        // this is owned by a server function so no need for a smart pointer
        snap_init::pointer_t f_snap_init;
    };

    /** \brief Handle the SIGTERM cleanly.
     *
     * This class is an implementation of the snap signal connection so we can
     * get an event whenever the user does `kill snapinit` (which sends a
     * SIGTERM by default.)
     */
    class sigterm_impl
            : public snap::snap_communicator::snap_signal
    {
    public:
        typedef std::shared_ptr<sigterm_impl>    pointer_t;

        /** \brief The SIGTERM signal initialization.
         *
         * The constructor defines this signal connection as a listener for
         * the SIGTERM signal.
         *
         * \param[in] si  The snap init server we are listening for.
         */
        sigterm_impl(snap_init::pointer_t si)
            : snap_signal(SIGTERM)
            , f_snap_init(si)
        {
        }

        // snap::snap_communicator::snap_signal implementation
        virtual void process_signal()
        {
            // we call the same function on SIGTERM, SIGQUIT and SIGINT
            f_snap_init->user_signal_caught(SIGTERM);
        }

    private:
        // this is owned by a server function so no need for a smart pointer
        snap_init::pointer_t f_snap_init;
    };

    /** \brief Handle the Ctrl-\ cleanly.
     *
     * This class is an implementation of the snap signal connection so we can
     * get an event whenever the user presses Ctrl-\ (which sends a SIGQUIT).
     */
    class sigquit_impl
            : public snap::snap_communicator::snap_signal
    {
    public:
        typedef std::shared_ptr<sigquit_impl>    pointer_t;

        /** \brief The SIGQUIT signal initialization.
         *
         * The constructor defines this signal connection as a listener for
         * the SIGQUIT signal.
         *
         * \param[in] si  The snap init server we are listening for.
         */
        sigquit_impl(snap_init::pointer_t si)
            : snap_signal(SIGQUIT)
            , f_snap_init(si)
        {
        }

        // snap::snap_communicator::snap_signal implementation
        virtual void process_signal()
        {
            // we call the same function on SIGTERM, SIGQUIT and SIGINT
            f_snap_init->user_signal_caught(SIGQUIT);
        }

    private:
        // this is owned by a server function so no need for a smart pointer
        snap_init::pointer_t f_snap_init;
    };

    /** \brief Handle Ctrl-C cleanly.
     *
     * This class is an implementation of the snap signal connection so we can
     * get an event whenever a user presses Ctrl-C (which sends a SIGINT).
     */
    class sigint_impl
            : public snap::snap_communicator::snap_signal
    {
    public:
        typedef std::shared_ptr<sigint_impl>    pointer_t;

        /** \brief The SIGINT signal initialization.
         *
         * The constructor defines this signal connection as a listener for
         * the SIGINT signal.
         *
         * \param[in] si  The snap init object we are listening for.
         */
        sigint_impl(snap_init::pointer_t si)
            : snap_signal(SIGINT)
            , f_snap_init(si)
        {
        }

        // snap::snap_communicator::snap_signal implementation
        virtual void process_signal()
        {
            // we call the same function on SIGTERM, SIGQUIT and SIGINT
            f_snap_init->user_signal_caught(SIGINT);
        }

    private:
        // this is owned by a server function so no need for a smart pointer
        snap_init::pointer_t f_snap_init;
    };

                                ~snap_init();

    static void                 create_instance( int argc, char * argv[] );
    static pointer_t            instance();
    __attribute__ ((noreturn)) void exit(int code) const;

    bool                        connect_listener(QString const & service_name, QString const & host, int port);
    void                        run_processes();
    void                        process_message(snap::snap_communicator_message const & message, bool udp);
    void                        service_died();
    void                        service_down(service::pointer_t s);
    void                        remove_terminated_services();
    void                        user_signal_caught(int sig);
    bool                        is_running();
    QString const &             get_spool_path() const;
    QString const &             get_server_name() const;
    service::pointer_t          get_connection_service() const;
    service::pointer_t          get_snapdbproxy_service() const;

    static void                 sighandler( int sig );

private:
                                snap_init( int argc, char * argv[] );
                                snap_init( snap_init const & ) = delete;
    snap_init &                 operator = ( snap_init const & ) = delete;

    void                        usage();
    void                        init();
    void                        xml_to_services(QDomDocument doc, QString const & xml_services_filename);
    void                        wakeup_services();
    void                        log_selected_servers() const;
    service::pointer_t          get_process( QString const & name );
    void                        start_processes();
    void                        terminate_services();
    void                        start();
    void                        restart();
    void                        stop();
    void                        get_addr_port_for_snap_communicator(QString & udp_addr, int & udp_port, bool default_to_snap_init);
    void                        remove_lock(bool force = false) const;

    static pointer_t            f_instance;
    advgetopt::getopt           f_opt;
    bool                        f_debug = false;
    snap::snap_config           f_config;
    QString                     f_log_conf;
    command_t                   f_command = command_t::COMMAND_UNKNOWN;
    QString                     f_server_name;
    QString                     f_lock_filename;
    QFile                       f_lock_file;
    QString                     f_spool_path;
    mutable bool                f_spool_directory_created = false;
    service::vector_t           f_service_list;
    service::pointer_t          f_connection_service;
    service::pointer_t          f_snapdbproxy_service;
    snap::snap_communicator::pointer_t  f_communicator;
    listener_impl::pointer_t    f_listener_connection;
    ping_impl::pointer_t        f_ping_server;
    sigchld_impl::pointer_t     f_child_signal;
    sigterm_impl::pointer_t     f_term_signal;
    sigquit_impl::pointer_t     f_quit_signal;
    sigint_impl::pointer_t      f_int_signal;
    QString                     f_server_type;
    QString                     f_udp_addr;
    int                         f_udp_port = 4039;
    int                         f_stop_max_wait = 60;
    QString                     f_expected_safe_message;
};




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
    : snap_timer(1000000LL) // wake up once per second by default
    , f_snap_init(si)
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
        fatal_error("the \"name\" parameter of a service must be defined and not empty.");
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
                fatal_error(QString("the command tag of service \"%1\" returned an empty string which does not represent a valid command.")
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
                    fatal_error(QString("the wait tag of service \"%1\" returned an invalid decimal number.").arg(f_service_name));
                    snap::NOTREACHED();
                }
                if(f_wait_interval < 0 || f_wait_interval > 3600)
                {
                    fatal_error(QString("the wait tag of service \"%1\" cannot be a negative number or more than 3600.").arg(f_service_name));
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
                    fatal_error(QString("the wait tag of service \"%1\" returned an invalid decimal number.").arg(f_service_name));
                    snap::NOTREACHED();
                }
                if(f_recovery < 60 || f_recovery > 86400 * 7)
                {
                    fatal_error(QString("the wait tag of service \"%1\" cannot be less than 60 or more than 604800 (about 1 week.) Used 'none' to turn off the recovery feature.").arg(f_service_name));
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
                    fatal_error(QString("the coredump tag of service \"%1\" is not a valid decimal number, optionally followed by \"kb\", \"mb\", or \"gb\".").arg(f_service_name));
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
                    fatal_error(QString("the coredump tag of service \"%1\" cannot be less than one memory block (1024 bytes.) Right now it is set to: %2 bytes")
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
                fatal_error(QString("priority \"%1\" of service \"%2\" returned a string that does not represent a valid decimal number.")
                                .arg(sub_element.text()).arg(f_service_name));
                snap::NOTREACHED();
            }
            if(f_priority < -100 || f_priority > 100)
            {
                fatal_error(QString("priority \"%1\" of service \"%2\" is out of bounds, we accept a priority between -100 and +100.")
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
                fatal_error(QString("the config tag of service \"%1\" returned an empty string which does not represent a valid configuration filename.")
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
                fatal_error(QString("the <connect> tag of service \"%1\" returned an empty string which does not represent a valid IP and port specification.")
                                .arg(f_service_name));
                snap::NOTREACHED();
            }
            f_snapcommunicator_addr = "127.0.0.1";
            f_snapcommunicator_port = 4040;
            tcp_client_server::get_addr_port(addr_port, f_snapcommunicator_addr, f_snapcommunicator_port, "tcp");
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
                fatal_error(QString("the <snapdbproxy> tag of service \"%1\" returned an empty string which does not represent a valid IP and port specification.")
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
                    fatal_error(QString("the cron tag of service \"%1\" must be a valid decimal number representing a number of seconds to wait between each execution.")
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
                    fatal_error(QString("the cron tag of service \"%1\" must be a number between 60 (1 minute) and 31708800 (a little over 1 year in seconds).")
                                  .arg(f_service_name));
                    snap::NOTREACHED();
                }
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
        for(auto p : paths)
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
            fatal_error(QString("could not find \"%1\" in any of the paths \"%2\".")
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


/** \brief Process a timeout on a connection.
 *
 * This function should probably be cut in a few sub-functions. It
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
    // if we are stopping we enter a completely different mode that
    // allows us to send SIGTERM and SIGKILL to the Unix process
    //
    if(f_stopping != 0)
    {
        if(is_running())
        {
            // f_stopping is the signal we want to send to the service
            //
            SNAP_LOG_WARNING("service ")(f_service_name)(", pid=")(f_pid)(", failed to respond to ")(f_stopping == SIGTERM ? "STOP" : "SIGTERM")(" signal, using `kill -")(f_stopping)("`.");
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

                // I do not foresee retrying as a solution to this error...
                // (it should not happen anyway...)
                //
                set_enable(false);
                return;
            }
            if(f_stopping == SIGKILL)
            {
                // we send SIGKILL once and stop...
                // then we should receive the SIGCHLD pretty quickly
                //
                set_enable(false);

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
            }
        }
        else
        {
            // Unix process stopped, we are all good now
            //
            set_enable(false);

            // use SIGCHLD to show that we are done with signals
            //
            f_stopping = SIGCHLD;
        }
        return;
    }

    if(is_connection_required())
    {
        // the connection is done in the snap_init class so
        // we have to call a function there once the process
        // is running
        //
        if(is_running())
        {
            std::shared_ptr<snap_init> si(f_snap_init.lock());
            if(!si)
            {
                fatal_error("somehow we could not get a lock on f_snap_init from a service object.");
                snap::NOTREACHED();
            }

            if(si->connect_listener(f_service_name, f_snapcommunicator_addr, f_snapcommunicator_port))
            {
                // TODO: later we may want to try the CONNECT event
                //       more than once; although over TCP on the
                //       local network it should not fail... but who
                //       knows (note that if the snapcommunicator
                //       crashes then we get a SIGCHLD and the
                //       is_running() function returns false.)
                //
                set_enable(false);
            }
            // else -- keep the timer in place to try again a little later
        }
        else
        {
            // wait for a few seconds before attempting to connect
            // with the snapcommunicator service
            //
            set_timeout_delay(std::max(get_wait_interval(), 3) * 1000000LL);

            // start the process
            //
            // in this case we ignore the return value since the
            // time is still in place and we will be called back
            // and try again a few times
            //
            snap::NOTUSED( run() );
        }
    }
    else if(is_running())
    {
        if(cron_task())
        {
            compute_next_tick(true);
        }
        else
        {
            // spurious timer?
            //
            SNAP_LOG_DEBUG("service::process_timeout() called when a regular process is still running.");

            // now really turn it off!
            //
            set_enable(false);
        }
    }
    else
    {
        // process needs to be started, do that now
        //
        if(run())
        {
            if(cron_task())
            {
                compute_next_tick(true);
            }
            else
            {
                set_enable(false);
            }
        }
        else
        {
            // give the OS a little time to get it shit back together
            // (we may have run out of memory for a small while)
            //
            set_timeout_delay(3 * 1000000LL);
        }
    }
}


bool service::service_may_have_died()
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

    f_started = false;

    // if this was a service with a connection (snapcommunicator) then
    // we indicate that it died
    //
    if( is_connection_required() )
    {
        snap_init::pointer_t si(f_snap_init.lock());
        if(!si)
        {
            SNAP_LOG_ERROR("cron service \"")(f_service_name)("\" lost its parent snapinit object.");
            return true;
        }

        si->service_down(shared_from_this());
    }

    mark_process_as_dead();

    return true;
}


void service::mark_process_as_dead()
{
    // do we know we sent the STOP signal? if so, remove outselves
    // from snapcommunicator
    //
    if( f_stopping != 0 )
    {
        // clearly mark that the service is dead
        //
        f_stopping = SIGCHLD;

        // if we are not running anymore,
        // remove self (timer) from snapcommunicator
        //
        remove_from_communicator();
        return;
    }

    // if it is the cron task, that is normal, the timer of the
    // cron task is already set as expected so ignore too
    //
    if( cron_task() )
    {
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
        int64_t const recovery(get_recovery());
        if( recovery <= 0 )
        {
            // this service cannot recover...

            // make sure the timer is stopped
            // (should not be required since we remove self from
            // snapcommunicator anyway...)
            //
            set_enable(false);

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
        set_timeout_delay(recovery * 1000000LL);
    }
    else
    {
        // in this case we use a default delay of one second to
        // avoid swamping the CPU with many restart all at once
        //
        set_timeout_delay(1000000LL);
    }

    set_enable(true);
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
    int64_t const now(snap::snap_child::get_current_date() / 1000000LL);
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
            set_timeout_date(latest_tick * 1000000LL);
        }
        else if(last_tick >= latest_tick)
        {
            // last_tick is now or in the future so we can keep it
            // as is (happen often when starting snapinit)
            //
            set_timeout_date(last_tick * 1000000LL);
            update = false;
        }
        else
        {
            // this looks like we may have missed a tick or two
            // so this task already timed out...
            //
            set_timeout_date(latest_tick * 1000000LL);
        }
    }
    else
    {
        // never ran, use this latest tick so we run that process
        // once as soon as possible.
        //
        set_timeout_date(latest_tick * 1000000LL);
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
            fatal_error("service::run():child: lost parent too soon and did not receive SIGHUP; quit immediately.");
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
            fatal_error("service::run():child: somehow we could not get a lock on f_snap_init from a service object.");
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
            args.push_back(si->get_snapdbproxy_service()->get_snapdbproxy_string());
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
                    if(start != s)
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
                    args.push_back(std::string(start, s - start));
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
        //
        for( auto arg : args )
        {
            args_p.push_back(arg.c_str());
        }
        //
        args_p.push_back(nullptr);

        // Quiet up the console by redirecting these from/to /dev/null
        // except in debug mode
        //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        if(!f_debug)
        {
            freopen( "/dev/null", "r", stdin  );
            freopen( "/dev/null", "w", stdout );
            freopen( "/dev/null", "w", stderr );
        }

        // Execute the child processes
        //
        execv(
            args[0].c_str(),
            const_cast<char * const *>(&args_p[0])
        );
#pragma GCC diagnostic pop

        // the command did not start...
        //
        std::string command_line;
        bool first(true);
        for(auto a : args)
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
        fatal_error(QString("service::run() child: process \"%1\" failed to start!").arg(command_line.c_str()));
        snap::NOTREACHED();
    }

    if( f_pid == -1 )
    {
        int const e(errno);
        SNAP_LOG_ERROR("fork() failed to create a child process to start service \"")(f_service_name)("\". (errno: ")(e)(" -- ")(strerror(e))(")");

        // request the proc library to read memory information
        meminfo();
        SNAP_LOG_INFO("memory total: ")(kb_main_total)(", free: ")(kb_main_free)(", swap_free: ")(kb_swap_free)(", swap_total: ")(kb_swap_total);

        // process never started, but it is considered as a short run
        // and the counter for a short run is managed in the
        // mark_process_as_dead() function (so unfortunately we may
        // fail a service if the OS takes too much time to resolve
        // the memory issue.)
        //
        mark_process_as_dead();

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


/** \brief Mark this service as stopping.
 *
 * This service is marked as being stopped. This happens when quitting
 * or a fatal error occurs.
 *
 * The function marks the service as stopping and setup the service
 * timeout so it can be killed with a SIGTERM and after the SIGTERM,
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
    if(is_running())
    {
        // on the next timeout, use SIGTERM
        //
        f_stopping = SIGTERM;

        // give the STOP signal 2 seconds, note that all services are sent
        // the STOP signal at the same time so 2 seconds should be more
        // than enough for all to quit (only those running a really heavy
        // job and do not check their signals often enough...)
        //
        // the test before the set_enable() and set_timeout_delay()
        // is there because set_stopping() could be called multiple times.
        //
        int64_t const SNAPINIT_STOP_DELAY = 2LL * 1000000LL;
        set_enable(true);
        set_timeout_delay(SNAPINIT_STOP_DELAY);
        set_timeout_date(-1); // ignore any date timeout
    }
    else
    {
        // stop process complete, mark so with SIGCHLD
        //
        f_stopping = SIGCHLD;

        // no need to timeout anymore, this service will not be restarted
        //
        set_enable(false);
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






/////////////////////////////////////////////////
// SNAP INIT (class implementation)            //
/////////////////////////////////////////////////


snap_init::pointer_t snap_init::f_instance;


snap_init::snap_init( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapinit_options, g_configuration_files, "SNAPINIT_OPTIONS")
    , f_log_conf( "/etc/snapwebsites/snapinit.properties" )
    , f_lock_filename( QString("%1/snapinit-lock.pid")
                       .arg(f_opt.get_string("lockdir").c_str())
                     )
    , f_lock_file( f_lock_filename )
    , f_spool_path( "/var/spool/snap/snapinit" )
    , f_communicator(snap::snap_communicator::instance())
{
    // commands that return immediately
    //
    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    if( f_opt.is_defined( "help" ) )
    {
        usage();
        snap::NOTREACHED();
    }
    if(f_opt.is_defined("running"))
    {
        // WARNING: shell true/false are inverted compared to C++
        exit(is_running() ? 0 : 1);
        snap::NOTREACHED();
    }
    if(f_opt.is_defined("remove-lock"))
    {
        // exit() does not force the lock removal so we have to call
        // it here...
        //
        remove_lock(true);
        exit(0);
        snap::NOTREACHED();
    }

    f_debug = f_opt.is_defined("debug");

    // read the configuration file
    //
    f_config.read_config_file( f_opt.get_string("config").c_str() );

    // get the server name
    // (we do it early so the logs can make use of it)
    //
    if(f_config.contains("server_name"))
    {
        f_server_name = f_config["server_name"];
    }
    if(f_server_name.isEmpty())
    {
        // use hostname by default if undefined in configuration file
        //
        char host[HOST_NAME_MAX + 1];
        host[HOST_NAME_MAX] = '\0';
        if(gethostname(host, sizeof(host)) != 0
        || strlen(host) == 0)
        {
            fatal_error("server_name is not defined in your configuration file and hostname is not available as the server name,"
                            " snapinit not started. (in snapinit.cpp/snap_init::snap_init())");
            snap::NOTREACHED();
        }
        f_server_name = host;
    }

    // setup the logger
    //
    if( f_opt.is_defined( "nolog" ) )
    {
        snap::logging::configure_console();
    }
    else if( f_opt.is_defined("logfile") )
    {
        snap::logging::configure_logfile( QString::fromUtf8(f_opt.get_string( "logfile" ).c_str()) );
    }
    else
    {
        if( f_config.contains("log_config") )
        {
            // use .conf definition when available
            f_log_conf = f_config["log_config"];
        }
        snap::logging::configure_conffile( f_log_conf );
    }

    if( f_debug )
    {
        // Force the logger level to DEBUG
        // (unless already lower)
        //
        snap::logging::reduce_log_output_level( snap::logging::log_level_t::LOG_LEVEL_DEBUG );
    }

    g_logger_ready = true;

    // do not do too much in the constructor or we may get in
    // trouble (i.e. calling shared_from_this() from the
    // constructor fails)
}


/** \brief Actually initialize this snap_init object.
 *
 * This function checks all the parameters and services and initializes
 * them all.
 */
void snap_init::init()
{
    if( f_opt.is_defined( "list" ) )
    {
        // use a default command name
        //
        f_command = command_t::COMMAND_LIST;
    }
    else
    {
        SNAP_LOG_INFO("---------------- snapinit manager started on ")(f_server_name);

        if( f_opt.is_defined( "--" ) )
        {
            std::string const command( f_opt.get_string("--") );

            // make sure we accept this command
            //
            if(command == "start")
            {
                f_command = command_t::COMMAND_START;
            }
            else if(command == "stop")
            {
                f_command = command_t::COMMAND_STOP;

                // `snapinit --detach stop` is not supported, --detach is ignore then
                //
                if( f_opt.is_defined("detach") )
                {
                    SNAP_LOG_WARNING("The --detach option is ignored with the 'stop' command.");
                }
            }
            else if(command == "restart")
            {
                f_command = command_t::COMMAND_RESTART;
            }
            else
            {
                SNAP_LOG_FATAL("Unknown command \"")(command)("\".");
                usage();
                snap::NOTREACHED();
            }
        }
        else
        {
            SNAP_LOG_FATAL("A command is required!");
            usage();
            snap::NOTREACHED();
        }
    }

    // user can change were the "cron" data managed by snapinit gets saved
    if(f_config.contains("spool_path"))
    {
        f_spool_path = f_config["spool_path"];
    }

    // make sure we can load the XML file with the various service
    // definitions
    //
    {
        QString const xml_services_filename(f_config.contains("xml_services")
                                        ? f_config["xml_services"]
                                        : "/etc/snapwebsites/snapinit.xml");
        if(xml_services_filename.isEmpty())
        {
            // the XML services is mandatory (it cannot be set to an empty string)
            fatal_error("the xml_services parameter cannot be empty, it has to be a path to the snapinit.xml file.");
            snap::NOTREACHED();
        }
        QFile xml_services_file(xml_services_filename);
        if(!xml_services_file.open(QIODevice::ReadOnly))
        {
            // the XML services is a mandatory file we need to be able to read
            int const e(errno);
            fatal_error(QString("the XML file \"%1\" could not be opened (%2).")
                            .arg(xml_services_filename)
                            .arg(strerror(e)));
            snap::NOTREACHED();
        }
        {
            QString error_message;
            int error_line;
            int error_column;
            QDomDocument doc;
            if(!doc.setContent(&xml_services_file, false, &error_message, &error_line, &error_column))
            {
                // the XML is probably not valid, setContent() returned false...
                // (it could also be that the file could not be read and we
                // got some I/O error.)
                //
                fatal_error(QString("the XML file \"%1\" could not be parse as valid XML (%2:%3: %4; on column: %5).")
                            .arg(xml_services_filename)
                            .arg(xml_services_filename)
                            .arg(error_line)
                            .arg(error_message)
                            .arg(error_column));
                snap::NOTREACHED();
            }
            xml_to_services(doc, xml_services_filename);
        }
    }

    // retrieve the direct listen information for the UDP port
    // on which we listen as a fallback in case snapcommunicator
    // is not available
    //
    {
        QString direct_listen;
        if(f_config.contains("direct_listen"))
        {
            // use .conf definition when available
            direct_listen = f_config["direct_listen"];
        }
        f_udp_addr = "127.0.0.1";
        f_udp_port = 4039;
        tcp_client_server::get_addr_port(direct_listen, f_udp_addr, f_udp_port, "udp");
    }

    if(f_config.contains("stop_max_wait"))
    {
        bool ok(false);
        f_stop_max_wait = f_config["stop_max_wait"].toInt(&ok, 10);
        if(!ok)
        {
            fatal_error(QString("the stop_max_wait parameter must be a number of seconds, \"%1\" is not valid.")
                                .arg(f_config["stop_max_wait"]));
            snap::NOTREACHED();
        }
        if(f_stop_max_wait < 10)
        {
            fatal_error(QString("the stop_max_wait parameter must be at least 10 seconds, \"%1\" is too small. The default value is 60.")
                                .arg(f_config["stop_max_wait"]));
            snap::NOTREACHED();
        }
    }

    if(f_command == command_t::COMMAND_LIST)
    {
        // TODO: add support for --verbose and print much more than just
        //       the service name
        //
        std::cout << "List of services to start on this server:" << std::endl;
        for(auto s : f_service_list)
        {
            std::cout << s->get_service_name() << std::endl;
        }
        // the --list command is over!
        exit(1);
        snap::NOTREACHED();
    }

    // if not --list we still write the list of services but in log file only
    log_selected_servers();

    // make sure the path to the lock file exists
    //
    snap::mkdir_p(f_lock_filename, true);

    // Stop on these signals, log them, then terminate.
    //
    // Note: the handler may access the snap_init instance
    //
    signal( SIGSEGV, sighandler );
    signal( SIGBUS,  sighandler );
    signal( SIGFPE,  sighandler );
    signal( SIGILL,  sighandler );
}


/** \brief Clean up the snap_init object.
 *
 * The destructor makes sure that the snapinit lock file gets removed
 * before exiting the process.
 */
snap_init::~snap_init()
{
    remove_lock();
}


/** \brief Exiting requires the removal of the lock.
 *
 * This function stops snapinit with an exit() call. The problem with
 * a direct exit() we do not get the destructor called and thus that
 * means the lock file does not get deleted.
 *
 * We over load the exit() command so that way we can make sure that
 * at least the lock gets destroyed.
 *
 * \param[in] code  The exit code, to be returned to the parent process.
 */
void snap_init::exit(int code) const
{
    remove_lock();

    ::exit(code);
}


void snap_init::create_instance( int argc, char * argv[] )
{
    f_instance.reset( new snap_init( argc, argv ) );
    if(!f_instance)
    {
        // new should throw std::bad_alloc or some other error anyway
        throw std::runtime_error("snapinit failed to create an instance of a snap_init object");
    }
    f_instance->init();
}


snap_init::pointer_t snap_init::instance()
{
    if( !f_instance )
    {
        throw std::invalid_argument( "snapinit instance must be created with create_instance()!" );
    }
    return f_instance;
}


void snap_init::xml_to_services(QDomDocument doc, QString const & xml_services_filename)
{
    QDomNodeList services(doc.elementsByTagName("service"));

    QString const binary_path( QString::fromUtf8(f_opt.get_string("binary-path").c_str()) );

    // use a map to make sure that each service has a distinct name
    //
    service::map_t service_list_by_name;

    int const max_services(services.size());
    for(int idx(0); idx < max_services; ++idx)
    {
        QDomElement e(services.at( idx ).toElement());
        if(!e.isNull()      // it should always be an element
        && !e.attributes().contains("disabled"))
        {
            service::pointer_t s(std::make_shared<service>(shared_from_this()));
            s->configure( e, binary_path, f_debug, f_command == command_t::COMMAND_LIST );

            // avoid two services with the same name
            //
            if( service_list_by_name.find( s->get_service_name() ) != service_list_by_name.end() )
            {
                fatal_error(QString("snapinit cannot start the same service more than once on \"%1\". It found \"%2\" twice in \"%3\".")
                              .arg(f_server_name)
                              .arg(s->get_service_name())
                              .arg(xml_services_filename));
                snap::NOTREACHED();
            }
            service_list_by_name[s->get_service_name()] = s;

            // we currently only support one snapcommunicator connection
            // mechanism, snapinit does not know anything about connecting
            // with any other service; so if we find more than one connection
            // service, we fail early
            //
            if(s->is_connection_required())
            {
                if(f_connection_service)
                {
                    fatal_error(QString("snapinit only supports one connection service at this time on \"%1\". It found two: \"%2\" and \"%3\" in \"%4\".")
                                  .arg(f_server_name)
                                  .arg(s->get_service_name())
                                  .arg(f_connection_service->get_service_name())
                                  .arg(xml_services_filename));
                    snap::NOTREACHED();
                }
                f_connection_service = s;
            }

            // we are starting the snapdbproxy system which offers an
            // address and port to connect to (itself, it listens to
            // those) and we have to send that information to all the
            // children we start so we need to save that pointer
            //
            if(s->is_snapdbproxy())
            {
                if(f_snapdbproxy_service)
                {
                    fatal_error(QString("snapinit only supports one snapdbproxy service at this time on \"%1\". It found two: \"%2\" and \"%3\" in \"%4\".")
                                  .arg(f_server_name)
                                  .arg(s->get_service_name())
                                  .arg(f_snapdbproxy_service->get_service_name())
                                  .arg(xml_services_filename));
                    snap::NOTREACHED();
                }
                f_snapdbproxy_service = s;
            }

            // make sure to add all services as a timer connection
            // to the communicator so we can wake a service on its
            // own (especially to support the <recovery> feature.)
            //
            f_communicator->add_connection( s );

            f_service_list.push_back( s );
        }
    }

    // make sure we have at least one service;
    //
    // TODO: we may want to require certain services such as:
    //       snapcommunicator and snapwatchdog?
    //
    if(f_service_list.empty())
    {
        fatal_error(QString("no services were specified in \"%1\" for snapinit to manage.")
                      .arg(xml_services_filename));
        snap::NOTREACHED();
    }

    // sort those services by priority
    //
    std::sort(f_service_list.begin(), f_service_list.end());
}


/** \brief Nuge services so they wake up.
 *
 * This function enables the timer of all the services that are
 * not requiring a connection (i.e. snapcommunicator.)
 *
 * The function also defines a timeout delay if some service
 * wants a bit of time to themselves to get started before
 * others (following) services get kicked in.
 *
 * Note that cron tasks do not get their tick date and time modified
 * here since it has to start exactly on their specific tick date
 * and time.
 *
 * \todo
 * Redesign the waking up to have a current state instead of having
 * special, rather complicated rules as we have now.
 */
void snap_init::wakeup_services()
{
    SNAP_LOG_TRACE("Wake Up Services called. (Total number of services: ")(f_service_list.size())(")");

    int64_t timeout_date(snap::snap_child::get_current_date());
    for(auto s : f_service_list)
    {
        // ignore the connection service, it already got started when
        // this function is called
        //
        // TODO: as noted in the documentation above, we need to redisign
        //       this "wake up services" for several reasons, but here
        //       the "is_running()" call is actually absolutely incorrect
        //       since the process could have died in between and thus
        //       we would get false when we would otherwise expect true.
        //
        if(s->is_connection_required()
        || s->is_running())
        {
            continue;
        }

        // cron tasks handle their own timeout as a date to have ticks
        // at a very specific date and time; avoid changing that timer!
        //
        if(!s->cron_task())
        {
            s->set_timeout_date(timeout_date);
        }

        // now this task timer is enabled; when we receive that callback
        // we can check whether the process is running and if not start
        // it as required by the current status
        //
        s->set_enable(true);

        // if we just started a service that has to send us a SAFE message
        // then we cannot start anything more at this point
        //
        if(s->is_safe_required())
        {
            f_expected_safe_message = s->get_safe_message();
            break;
        }

        // this service may want the next service to start later
        // (notice how that will not affect a cron task...)
        //
        // we put a minimum of 1 second so that way we do not start
        // many tasks all at once which the OS does not particularly
        // like and it makes nearly no difference on our services
        //
        timeout_date += std::max(1, s->get_wait_interval()) * 1000000LL;
    }
}


/** \brief Start a process depending on the command line command.
 *
 * This function is called once the snap_init object was initialized.
 * The function calls the corresponding function.
 *
 * At this time only three commands are supported:
 *
 * \li start
 * \li stop
 * \li restart
 *
 * The restart first calls stop() if snapinit is still running.
 * Then it calls start().
 */
void snap_init::run_processes()
{
    if( f_command == command_t::COMMAND_START )
    {
        start();
    }
    else if( f_command == command_t::COMMAND_STOP )
    {
        stop();
    }
    else if( f_command == command_t::COMMAND_RESTART )
    {
        restart();
    }
    else
    {
        SNAP_LOG_ERROR("Command '")(f_opt.get_string("--"))("' not recognized!");
        usage();
        snap::NOTREACHED();
    }
}


/** \brief Connect the listener to snapcommunicator.
 *
 * This function starts a connection with the snapcommunicator and
 * sends a CONNECT message.
 *
 * \note
 * The listener is created in the main thread, meaning that the thread
 * dies out until the connection either succeeds or fails. This is done
 * by design since at this point the only service running is expected
 * to be snapcommunicator and there is no other event we can receive
 * unless the connection fails (i.e. snapcommunicator can crash and
 * we want to know about that, but the connection will fail if the
 * snapcommunicator crashed.) Since this is local connection, it should
 * be really fast anyway.
 *
 * \param[in] service_name  Name of the service requesting a listener connection.
 * \param[in] host  The host to connect to.
 * \param[in] port  The port to use to connect to.
 *
 * \return true if the connection succeeded.
 */
bool snap_init::connect_listener(QString const & service_name, QString const & host, int port)
{
    // TODO: count attempts and after X attempts, fail completely
    try
    {
        // this is snapcommunicator, connect to it
        //
        f_listener_connection.reset(new listener_impl(shared_from_this(), host.toUtf8().data(), port));
        f_listener_connection->set_name("snapinit listener");
        f_listener_connection->set_priority(0);
        f_communicator->add_connection(f_listener_connection);

        // and now connect to it
        //
        snap::snap_communicator_message register_snapinit;
        register_snapinit.set_command("REGISTER");
        register_snapinit.add_parameter("service", "snapinit");
        register_snapinit.add_parameter("version", snap::snap_communicator::VERSION);
        f_listener_connection->send_message(register_snapinit);

        return true;
    }
    catch(tcp_client_server::tcp_client_server_runtime_error const & e)
    {
        // this can happen if we try too soon and the snapconnection
        // listening socket is not quite ready yet
        //
        SNAP_LOG_WARNING("connection to service \"")(service_name)("\" failed.");

        // clean up the listener connection
        //
        if(f_listener_connection)
        {
            f_communicator->remove_connection(f_listener_connection);
            f_listener_connection.reset();
        }
    }

    return false;
}


/** \brief Process a message.
 *
 * Once started, snapinit accepts messages on a UDP port. This is offered so
 * one can avoid starting snapcommunicator. Only the STOP command should be
 * sent through the UDP port.
 *
 * When snapcommunicator is a service that snapinit is expected to start
 * (it should be in almost all cases), then this function is also called
 * as soon as the snapcommunicator system is in place.
 *
 * \param[in] message  The message to be handled.
 * \param[in] udp  Whether the message was received via the UDP listener.
 */
void snap_init::process_message(snap::snap_communicator_message const & message, bool udp)
{
    SNAP_LOG_TRACE("received message [")(message.to_message())("]");

    QString const command(message.get_command());

// ******************* TCP and UDP messages

    // someone sent "snapinit/STOP" to snapcommunicator
    // or "[whatever/]STOP" directly to snapinit (via UDP)
    //
    if(command == "STOP")
    {
        // someone asking us to stop snap_init; this means we want to stop
        // all the services that snap_init started; if we have a
        // snapcommunicator, then we use that to send the STOP signal to
        // all services at once
        //
        terminate_services();
        return;
    }

    // UDP messages that we accept are very limited...
    // (especially since we cannot send a reply)
    //
    if(udp)
    {
        SNAP_LOG_ERROR("command \"")(command)("\" is not supported on the UDP connection.");
        return;
    }

// ******************* TCP only messages

    switch(command[0].unicode())
    {
    case 'H':
        // all have to implement the HELP command
        //
        if(command == "HELP")
        {
            snap::snap_communicator_message reply;
            reply.set_command("COMMANDS");

            // list of commands understood by snapinit
            //
            reply.add_parameter("list", "HELP,LOG,QUITTING,READY,SAFE,STOP,UNKNOWN");

            f_listener_connection->send_message(reply);
            return;
        }
        break;

    case 'L':
        if(command == "LOG")
        {
            SNAP_LOG_INFO("Logging reconfiguration.");
            snap::logging::reconfigure();
            return;
        }
        break;

    //case 'N':
    //    if(command == "NEWSERVICE")
    //    {
    //    ... TODO: check whether we are waiting on the service to be started ...
    //    }
    //    break;

    case 'Q':
        if(command == "QUITTING")
        {
            // it looks like we sent a message after a STOP was received
            // by snapcommunicator; this means we should receive a STOP
            // shortly too, but we just react the same way to QUITTING
            // than to STOP.
            //
            terminate_services();
            return;
        }
        break;

    case 'R':
        if(command == "READY")
        {
            // now we can start all the other services (except CRON tasks)
            //
            wakeup_services();

            // send the list of local services to the snapcommunicator
            //
            snap::snap_communicator_message reply;
            reply.set_command("SERVICES");

            // generate the list of services as a string of comma names
            //
            snap::snap_string_list services;
            services << "snapinit";
            for(auto s : f_service_list)
            {
                services << s->get_service_name();
            }
            reply.add_parameter("list", services.join(","));

            f_listener_connection->send_message(reply);
            return;
        }
        break;

    case 'S':
        if(command == "SAFE")
        {
            // we received a "we are safe" message so we can move on and
            // start the next service
            //
            if(f_expected_safe_message != message.get_parameter("name"))
            {
                // we need to terminate the existing services cleanly
                // so we do not use fatal_error() here
                //
                QString msg(QString("received wrong SAFE message. We expected \"%1\" but we received \"%2\".")
                                    .arg(f_expected_safe_message)
                                    .arg(message.get_parameter("name")));
                SNAP_LOG_FATAL(msg);
                syslog( LOG_CRIT, "%s", msg.toUtf8().data() );

                // Simulate a STOP, we cannot continue safely
                //
                terminate_services();
                return;
            }

            // wakeup other services
            //
            wakeup_services();
            return;
        }
        break;

    case 'U':
        if(command == "UNKNOWN")
        {
            SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            return;
        }
        break;

    }

    // unknown command is reported and process goes on
    //
    SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received on the TCP connection.");
    {
        snap::snap_communicator_message reply;
        reply.set_command("UNKNOWN");
        reply.add_parameter("command", command);
        f_listener_connection->send_message(reply);
    }
}


/** \brief This callback gets called on a SIGCHLD signal.
 *
 * Whenever a child dies, we receive a SIGCHLD. The snapcommunicator
 * library knows how to handle those signals and ends up calling this
 * function when one happens. Only, at this point the snapcommunicator
 * does not tell us which child died. So we quickly look through our
 * list (in comparison to having a timer and poll the list once a
 * second, this is still way faster since 99.9% of the time our
 * processes do not just die!)
 *
 * In most cases, this process will restart the service. Only if the
 * service was restarted many times in a very short period of time
 * it may actually be removed from the list instead or put to sleep
 * for a while ("put to sleep" means not restarted at all...)
 *
 * \warning
 * This function will call itself if it detects that a process dies
 * and it has to terminate snapinit itself.
 */
void snap_init::service_died()
{
    // first go through the list and allow any service which is
    // not died and should not have to be restarted (i.e. all
    // services except CRON services for now)
    //
    for(auto s : f_service_list)
    {
        if(s->service_may_have_died())
        {
            if(!f_listener_connection)
            {
                // snapcommunicator already died, we cannot forward
                // the DIED or any other message
                break;
            }

            snap::snap_communicator_message register_snapinit;
            register_snapinit.set_command("DIED");
            register_snapinit.set_service("*");
            register_snapinit.add_parameter("service", s->get_service_name());
            register_snapinit.add_parameter("pid", s->get_old_pid());
            f_listener_connection->send_message(register_snapinit);
        }
    }

    // check whether a service failed and is marked as required
    // although if recovery is not zero we ignore the situation...
    //
    {
        auto required_and_failed = [](service::pointer_t s)
        {
            // no need to test whether recovery == 0 since it would
            // not be in the failed state if recovery != 0
            //
            return s->failed() && s->is_service_required();
        };
        service::vector_t::const_iterator required_failed(std::find_if(f_service_list.begin(), f_service_list.end(), required_and_failed));
        if(required_failed != f_service_list.end())
        {
            // we need to terminate the existing services cleanly
            // so we do not use fatal_error() here
            //
            QString msg(QString("service \"%1\" failed and since it is required, we are stopping snapinit now.")
                        .arg((*required_failed)->get_service_name()));
            SNAP_LOG_FATAL(msg);
            syslog( LOG_CRIT, "%s", msg.toUtf8().data() );

            // terminate snapinit
            //
            terminate_services();
            return;
        }
    }

    // completely forget about failed services with
    // no possible recovery fallback
    //
    {
        auto if_failed = [](service::pointer_t s)
        {
            return s->failed() && s->get_recovery() == 0;
        };
        f_service_list.erase(std::remove_if(f_service_list.begin(), f_service_list.end(), if_failed), f_service_list.end());
    }

    remove_terminated_services();
}


/** \brief Detected that a connection service dropped.
 *
 * This function is called whenever the listener connection service
 * is down. It is not unlikely that we already received a hang up
 * callback on that connection though.
 *
 * \param[in] s  The the service that just died.
 */
void snap_init::service_down(service::pointer_t s)
{
    NOTUSED(s);

    if(f_listener_connection)
    {
        f_communicator->remove_connection(f_listener_connection);
        f_listener_connection.reset();
    }
}


/** \brief Remove services that are marked as terminated.
 *
 * Whenever we receive the SIGCHLD, a service is to be removed. This
 * function is called last to then remove the service from the list
 * of services (f_service_list).
 *
 * In some cases the service is kept as we want to give it another
 * change to run (especially the CRON services.)
 *
 * If all services are removed from the f_service_list, the function
 * then removes all the other connections from the snap_communicator
 * object. As a result, the run() function will return and snapinit
 * will exit.
 */
void snap_init::remove_terminated_services()
{
    // remove services that were terminated
    //
    {
        auto if_stopped = [](service::pointer_t s)
        {
            return s->has_stopped();
        };
        f_service_list.erase(std::remove_if(f_service_list.begin(), f_service_list.end(), if_stopped), f_service_list.end());

        if(f_service_list.empty())
        {
            // no more services, also remove our other connections so
            // we exit the snapcommunicator loop
            //
            f_communicator->remove_connection(f_ping_server);
            f_communicator->remove_connection(f_child_signal);
            f_communicator->remove_connection(f_term_signal);
            f_communicator->remove_connection(f_quit_signal);
            f_communicator->remove_connection(f_int_signal);
            if(f_listener_connection)
            {
                f_communicator->remove_connection(f_listener_connection);
            }
        }
    }
}


/** \brief Process a user termination signal.
 *
 * This funtion is called whenever the user presses Ctlr-C, Ctrl-?, or Ctrl-\
 * on their keyboard (SIGINT, SIGTERM, or SIGQUIT). This function makes sure
 * to stop the process cleanly in this case by calling the
 * terminate_services() function.
 *
 * \param[in] sig  The Unix signal that generated this call.
 */
void snap_init::user_signal_caught(int sig)
{
    std::stringstream ss;
    ss << "User signal caught: " << (sig == SIGINT
                                         ? "SIGINT"
                                         : (sig == SIGTERM
                                                ? "SIGTERM"
                                                : "SIGQUIT"));
    SNAP_LOG_INFO(ss.str());
    if(g_isatty)
    {
        std::cerr << "snapinit: " << ss.str() << std::endl;
    }

    // by calling this function, snapinit will quit once all the
    // services stopped
    //
    terminate_services();
}


/** \brief Check whether snapinit is running (has a lock file in place.)
 *
 * The snapinit process creates a lock file on the 'start' command.
 * If that lock file exists, then it is viewed as locked and that
 * snapinit is already running. This prevents you from starting
 * multiple instances of the snapinit server. It is still possible
 * to start snapinit with other commands, especially the 'stop'
 * and 'restart' commands, but also the --version and --list
 * command line options work just fine even when the lock is in
 * place.
 *
 * \return true if the snapinit process lock file exists.
 */
bool snap_init::is_running()
{
    return f_lock_file.exists();
}


/** \brief Retrieve the path to the spool directory.
 *
 * The spool directory is used by the anacron tool and we do the
 * same thing. We save the time in seconds when we last ran a
 * CRON process in a file under that directory.
 *
 * This function makes sure that the spool directory exists
 * the first time it is called. After that, it is assumed
 * that the path never changes so it does not try to recreate
 * the path.
 *
 * \return The path to the spool file.
 */
QString const & snap_init::get_spool_path() const
{
    if(!f_spool_directory_created)
    {
        f_spool_directory_created = true;

        // make sure that the directory exists
        //
        if(snap::mkdir_p(f_spool_path, false) != 0)
        {
            fatal_error(QString("snapinit could not create directory \"%1\" to save spool data.")
                                .arg(f_spool_path));
            snap::NOTREACHED();
        }
    }

    return f_spool_path;
}


/** \brief Retrieve the name of the server.
 *
 * This parameter returns the value of the server_name=... parameter
 * defined in the snapinit configuration file or the hostname if
 * the server_name=... parameter was not defined.
 *
 * \return The name of the server this snapinit instance is running on.
 */
QString const & snap_init::get_server_name() const
{
    return f_server_name;
}


/** \brief Retrieve the service used to inter-connect services.
 *
 * This function returns the information about the server that is
 * used to inter-connect services together. This should be the
 * snapcommunicator service.
 *
 * \exception std::logic_error
 * The function raises that exception if it gets called too soon
 * (i.e. before a connection service is found in the XML file.)
 *
 * \return A smart pointer to the connection service.
 */
service::pointer_t snap_init::get_connection_service() const
{
    if(!f_connection_service)
    {
        throw std::logic_error("connection service requested before it was defined.");
    }

    return f_connection_service;
}


/** \brief Retrieve the service used to inter-connect services.
 *
 * This function returns the information about the server that is
 * used to inter-connect services together. This should be the
 * snapcommunicator service.
 *
 * \exception std::logic_error
 * The function raises that exception if it gets called too soon
 * (i.e. before a connection service is found in the XML file.)
 *
 * \return A smart pointer to the connection service.
 */
service::pointer_t snap_init::get_snapdbproxy_service() const
{
    if(!f_snapdbproxy_service)
    {
        throw std::logic_error("connection service requested before it was defined.");
    }

    return f_snapdbproxy_service;
}


/** \brief List the servers we are starting to the log.
 *
 * This function prints out the list of services that this instance
 * of snapinit is managing.
 *
 * The list may be shorten as time goes if some services die too
 * many times. This gives you an exact list on startup.
 *
 * Note that services marked as disabled in the snapinit.xml file
 * are not loaded at all so they will not make it to the log from
 * this function.
 */
void snap_init::log_selected_servers() const
{
    std::stringstream ss;
    ss << "Enabled servers:";
    //
    for( auto const & opt : f_service_list )
    {
        ss << " [" << opt->get_service_name() << "]";
    }
    //
    SNAP_LOG_INFO(ss.str());
}






/** \brief Ask all services to quit.
 *
 * In most cases, this function is called when the snapinit tool
 * receives the STOP signal. It, itself, propagates the STOP signal
 * to all the services it started.
 *
 * This is done by marking all the services as stopping and then
 * sending the STOP signal to the snapcommunicator.
 *
 * If all the services were already stopped, then the function
 * does not send a STOP (since snapcommunicator would not even
 * be running.)
 *
 * \warning
 * This function does NOT block. Instead it sends messages and
 * then returns.
 *
 * \bug
 * At this time we have no clue whether the service is already
 * connected to the snapcommunicator or not. Although we have
 * a SIGTERM + SIGKILL fallback anyway, in reality we end up
 * having an ugly termination if the service was not yet
 * connected at the top we send the STOP signal. That being
 * said, if that happens, it is not unlikely that the process
 * was not doing much yet. On the other hand, I prefer correctness
 * and I think that accepting the snapcommunicator STATUS signal
 * would give us a way to know where we are and send the SIGTERM
 * immediately preventing the child process from starting a real
 * task (because until connected to the snapcommunicator it
 * should not be any important work.) Also all children could
 * have the SIGTERM properly handle a quit.
 */
void snap_init::terminate_services()
{
    // make sure that any death from now on marks the services as
    // done
    //
    for( auto s : f_service_list )
    {
        s->set_stopping();
    }

    // set_stopping() immediately marks certain services as dead
    // if they were not running, remove them immediately in case
    // that were all of them! the function then removes all the
    // connections and the communicator will exit its run() loop.
    //
    remove_terminated_services();

    // if we still have at least one service it has to be the
    // snapcommunicator service so we can send a STOP command
    //
    if(!f_service_list.empty())
    {
        if(f_listener_connection)
        {
            // by sending UNREGISTER to snapcommunicator, it will also
            // assume that a STOP message was sent and thus it will
            // propagate STOP all services, and a DISCONNECT is sent
            // to all neighbors.
            //
            // The reason we do not send an UNREGISTER and a STOP from
            // here is that once we sent an UNREGISTER, the line is
            // cut and thus we cannot 100% guarantee that the STOP
            // will make it. Also, we do not use the STOP because it
            // is used by all services and overloading that command
            // could be problematic in the future.
            //
            snap::snap_communicator_message unregister_self;
            unregister_self.set_command("UNREGISTER");
            unregister_self.add_parameter("service", "snapinit");
            f_listener_connection->send_message(unregister_self);
        }
        else
        {
            // this can happen if we were trying to start snapcommunicator
            // and it somehow failed too many times too quickly
            //
            SNAP_LOG_WARNING("snap_init::terminate_services() called without a f_listener_connection. STOP could not be propagated.");
            if(g_isatty)
            {
                std::cerr << "warning: snap_init::terminate_services() called without a f_listener_connection. STOP could not be propagated." << std::endl; 
            }
        }
    }
}


/** \brief Start the snapinit services.
 *
 * This function starts the Snap! Websites services.
 *
 * If the --detach command line option was used, then the function
 * calls fork() to detach the process from the calling shell.
 */
void snap_init::start()
{
    // The following open() prevents race conditions
    //
    int const fd(::open( f_lock_file.fileName().toUtf8().data(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR ));
    if( fd == -1 )
    {
        int const e(errno);
        if(e == EEXIST)
        {
            int lock_file_pid(-1);
            {
                if(f_lock_file.open(QFile::ReadOnly))
                {
                    QByteArray const data(f_lock_file.readAll());
                    f_lock_file.close();
                    QString const pid_string(QString::fromUtf8(data).trimmed());
                    bool ok(false);
                    lock_file_pid = pid_string.toInt(&ok, 10);
                    if(!ok)
                    {
                        // just in case, make 100% sure that we have -1 as
                        // the PID when invalid
                        lock_file_pid = -1;
                    }
                }
            }

            if(lock_file_pid != -1)
            {
                if(getpgid(lock_file_pid) < 0)
                {
                    // although the lock file is in place, the PID defined in
                    // it does not exist, change the error message accordingly
                    //
                    // TODO: look into implementing a delete, but for that we
                    //       need to open the file locked, otherwise we may
                    //       have a race condition!
                    //       (see SNAP-133 which is closed)
                    //
                    fatal_error(QString("Lock file \"%1\" exists! However, process with PID %2 is not running. To delete the lock, use `snapinit --remove-lock`.")
                                  .arg(f_lock_filename)
                                  .arg(lock_file_pid));
                }
                else
                {
                    // snapinit is running
                    //
                    fatal_error(QString("Lock file \"%1\" exists! snapinit is already running as PID %2.")
                                  .arg(f_lock_filename)
                                  .arg(lock_file_pid));
                }
            }
            else
            {
                // snapinit is running
                //
                fatal_error(QString("Lock file \"%1\" exists! Is this a race condition? (errno: %2 -- %3)")
                              .arg(f_lock_filename)
                              .arg(e)
                              .arg(strerror(e)));
            }
        }
        else
        {
            fatal_error(QString("Lock file \"%1\" could not be created. (errno: %2 -- %3)")
                            .arg(f_lock_filename)
                            .arg(e)
                            .arg(strerror(e)));
        }
        snap::NOTREACHED();
    }

    // save fd in the QFile object
    //
    // WARNING: this call removes the filename from the QFile
    //          hence, we generally use the f_lock_filename instead of
    //          the f_lock_file.fileName() function
    //
    if(!f_lock_file.open( fd, QFile::ReadWrite ))
    {
        fatal_error(QString("Lock file \"%1\" could not be registered with Qt.")
                            .arg(f_lock_filename));
        snap::NOTREACHED();
    }

    if( f_opt.is_defined("detach") )
    {
        // fork(), then stay resident
        // Listen for STOP command on UDP port.
        //
        int const pid(fork());
        if( pid != 0 )
        {
            // the parent
            //
            if( pid < 0 )
            {
                // the child did not actually start
                //
                int const e(errno);
                fatal_error(QString("fork() failed, snapinit could not detach itself. (errno: %1).")
                                .arg(strerror(e)));
                snap::NOTREACHED();
            }

            // in this case we MUST keep the lock in place,
            // which is done by closing that file; if the file
            // is closed whenever we hit the remove_lock()
            // function, then the file does not get deleted
            //
            f_lock_file.close();
            return;
        }

        // the child goes on
    }

    // save our (child) PID in the lock file (useful for the stop() processus)
    // the correct Debian format is the PID followed by '\n'
    //
    // FHS Version 2.1+:
    //   > The file should consist of the process identifier in ASCII-encoded
    //   > decimal, followed by a newline character. For example, if crond was
    //   > process number 25, /var/run/crond.pid would contain three characters:
    //   > two, five, and newline.
    //
    f_lock_file.write(QString("%1\n").arg(getpid()).toUtf8());
    f_lock_file.flush();

    // check whether all executables are available
    //
    bool failed(false);
    for( auto s : f_service_list )
    {
        if(!s->exists())
        {
            failed = true;

            // This is a fatal error, but we want to give the user information
            // about all the missing binary (this is not really true anymore
            // because this check is done at the end of the service confiration
            // function and generate a fatal error there already)
            //
            QString msg(QString("binary for service \"%1\" was not found or is not executable. snapinit will exit without starting anything.")
                        .arg(s->get_service_name()));
            SNAP_LOG_FATAL(msg);
            syslog( LOG_CRIT, "%s", msg.toUtf8().data() );
        }
    }
    if(failed)
    {
        fatal_error(QString("Premature exit because one or more services cannot be started (their executable are not available.) This may be because you changed the binary path to an invalid location."));
        snap::NOTREACHED();
    }

    // Assuming we have a connection service, we want to wake that
    // service first and once that is dealt with, we wake up the
    // other services (i.e. on the ACCEPT call)
    //
    if(f_connection_service)
    {
        f_connection_service->set_timeout_date(snap::snap_child::get_current_date());
        f_connection_service->set_enable(true);
    }
    else
    {
        // this call wakes all the other services; it is also called
        // whenever the connection to snapcommunicator is accepted
        //
        wakeup_services();
    }

    // initialize a UDP server as a fallback in case you want to use
    // snapinit without a snapcommunicator server
    //
    {
        // just in case snapcommunicator does not get started, we still can
        // receive messages over a UDP port (mainly a STOP message)
        //
        f_ping_server.reset(new ping_impl(shared_from_this(), f_udp_addr.toUtf8().data(), f_udp_port));
        f_ping_server->set_name("snapinit UDP backup server");
        f_ping_server->set_priority(30);
        f_communicator->add_connection(f_ping_server);
    }

    // initialize the SIGCHLD signal
    //
    {
        f_child_signal.reset(new sigchld_impl(shared_from_this()));
        f_child_signal->set_name("snapinit SIGCHLD signal");
        f_child_signal->set_priority(55);
        f_communicator->add_connection(f_child_signal);
    }

    // initialize the SIGTERM signal
    //
    {
        f_term_signal.reset(new sigterm_impl(shared_from_this()));
        f_term_signal->set_name("snapinit SIGTERM signal");
        f_term_signal->set_priority(65);
        f_communicator->add_connection(f_term_signal);
    }

    // initialize the SIGQUIT signal
    //
    {
        f_quit_signal.reset(new sigquit_impl(shared_from_this()));
        f_quit_signal->set_name("snapinit SIGQUIT signal");
        f_quit_signal->set_priority(65);
        f_communicator->add_connection(f_quit_signal);
    }

    // initialize the SIGINT signal
    //
    {
        f_int_signal.reset(new sigint_impl(shared_from_this()));
        f_int_signal->set_name("snapinit SIGINT signal");
        f_int_signal->set_priority(60);
        f_communicator->add_connection(f_int_signal);
    }

    // run the event loop until we receive a STOP message
    //
    f_communicator->run();

    remove_lock();

    SNAP_LOG_INFO("Normal shutdown.");
}


/** \brief Attempts to restart Snap! Websites services.
 *
 * This function stops the existing snapinit instance and waits for it
 * to be done. If that succeeds, then it attempts to restart the
 * services immediately after that. The restart does not return
 * until itself stopped unless the detach option is used.
 */
void snap_init::restart()
{
    SNAP_LOG_INFO("Restart Snap! Websites services.");

    // call stop only if the server is running
    //
    if( is_running() )
    {
        stop();
    }

    // start and block unless "detach" is true
    //
    start();
}


/** \brief Run the 'stop' command of snapinit.
 *
 * This function runs the stop command, which attempts to stop the
 * existing / running snapinit process.
 *
 * If snapinit is not currently running, the function returns immediately
 * after logging and informational message about the feat.
 *
 * \return true if snapinit successfully stopped.
 */
void snap_init::stop()
{
    if( !is_running() )
    {
        // if not running, is this an error?
        //
        SNAP_LOG_INFO("'snapinit stop' called while snapinit is not running.");
        if( g_isatty )
        {
            std::cerr << "snapinit: info: 'snapinit stop' called while snapinit is not running."
                      << std::endl;
        }
        return;
    }

    // read the PID of the locking process so we can wait on its PID
    // and not just the lock (because in case it is restarted immediately
    // we would not see the lock file disappear...)
    //
    int lock_file_pid(-1);
    {
        if(f_lock_file.open(QFile::ReadOnly))
        {
            QByteArray const data(f_lock_file.readAll());
            f_lock_file.close();
            QString const pid_string(QString::fromUtf8(data).trimmed());
            bool ok(false);
            lock_file_pid = pid_string.toInt(&ok, 10);
            if(!ok)
            {
                // just in case, make 100% sure that we have -1 as the PID
                lock_file_pid = -1;
            }
        }
    }

    SNAP_LOG_INFO("Stop Snap! Websites services (pid = ")(lock_file_pid)(").");

    QString udp_addr;
    int udp_port;
    get_addr_port_for_snap_communicator(udp_addr, udp_port, true);

    // send the UDP message now
    //
    snap::snap_communicator_message stop_message;
    stop_message.set_service("snapinit");
    stop_message.set_command("STOP");
    if(!snap::snap_communicator::snap_udp_server_message_connection::send_message(udp_addr.toUtf8().data(), udp_port, stop_message))
    {
        fatal_error("'snapinit stop' failed to send the STOP message to the running instance.");
        snap::NOTREACHED();
    }

    // wait for the processes to end and snapinit to delete the lock file
    //
    // if it takes too long, we will exit the loop and things will
    // eventually still be running...
    //
    for(int idx(0); idx < f_stop_max_wait; ++idx)
    {
        sleep(1);

        // the lock_file_pid should always be >= 0
        //
        if(lock_file_pid >= 0)
        {
            if(getpgid(lock_file_pid) < 0)
            {
                // errno == ESRCH -- the process does not exist anymore
                return;
            }
        }
        else
        {
            if( !f_lock_file.exists() )
            {
                // it worked!
                return;
            }
        }
    }

    // it failed...
    fatal_error(QString("snapinit waited for %1 seconds and the running version did not return.")
                    .arg(f_stop_max_wait));
    snap::NOTREACHED();
}


void snap_init::get_addr_port_for_snap_communicator(QString & udp_addr, int & udp_port, bool default_to_snap_init)
{
    // defaults UDP for direct snapinit STOP signal
    //
    if(default_to_snap_init)
    {
        // get default from the snapinit.conf file
        //
        udp_addr = f_udp_addr;
        udp_port = f_udp_port;
    }
    else
    {
        // default for snapcommunicator
        //
        udp_addr = "127.0.0.1";
        udp_port = 4041;
    }

    // if we have snapcommunicator in our services, then we can send
    // a signal to that process, in which case we want to gather the
    // IP and port from that configuration file
    //
    service::vector_t::const_iterator snapcommunicator(std::find_if(
            f_service_list.begin()
          , f_service_list.end()
          , [](service::pointer_t const & s)
            {
                return s->get_service_name() == "snapcommunicator";
            }));
    if(snapcommunicator != f_service_list.end())
    {
        // we can send a UDP message to snapcommunicator, only we need
        // the address and port and those are defined in the
        // snapcommunicator settings
        //
        QString snapcommunicator_config_filename((*snapcommunicator)->get_config_filename());
        if(snapcommunicator_config_filename.isEmpty())
        {
            // in case it was not defined, use the default
            snapcommunicator_config_filename = "/etc/snapwebsites/snapcommunicator.conf";
        }
        snap::snap_config snapcommunicator_config;
        snapcommunicator_config.read_config_file( snapcommunicator_config_filename.toUtf8().data() );
        tcp_client_server::get_addr_port(snapcommunicator_config["signal"], udp_addr, udp_port, "udp");
    }
}


/** \brief Print out the usage information for snapinit.
 *
 * This function returns the snapinit usage information to the user whenever
 * an invalid command line option is used or --help is used explicitly.
 *
 * The function does not return.
 */
void snap_init::usage()
{
    f_opt.usage( advgetopt::getopt::no_error, "snapinit" );
    snap::NOTREACHED();
}


/** \brief Remove the lock file.
 *
 * This function is called to remove the lock file so that way
 * a server can restart the snapinit tool on the next run.
 *
 * \todo
 * At this time this is not 100% RAII because we have many
 * fatal errors that call exit(1) directly.
 *
 * \param[in] force  Force the removal, whether the file was opened or not.
 */
void snap_init::remove_lock(bool force) const
{
    if( f_lock_file.isOpen() || force )
    {
        // We first have to close the handle, otherwise the remove does not work.
        //
        if( f_lock_file.isOpen() )
        {
            ::close( f_lock_file.handle() );

            // the Qt close() by itself does not work right, but
            // we want the QFile to be marked as closed
            //
            const_cast<QFile &>(f_lock_file).close();
        }

        QFile lock_file( f_lock_filename );
        lock_file.remove();
    }
}


/** \brief A static function to capture various signals.
 *
 * This function captures unwanted signals like SIGSEGV and SIGILL.
 *
 * The handler logs the information and then the service exists.
 * This is done mainly so we have a chance to debug problems even
 * when it crashes on a server.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void snap_init::sighandler( int sig )
{
    QString signame;
    switch( sig )
    {
    case SIGSEGV:
        signame = "SIGSEGV";
        break;

    case SIGBUS:
        signame = "SIGBUS";
        break;

    case SIGFPE:
        signame = "SIGFPE";
        break;

    case SIGILL:
        signame = "SIGILL";
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    {
        snap::snap_exception_base::output_stack_trace();
        QString msg(QString("Fatal signal caught: %1").arg(signame));
        SNAP_LOG_FATAL(msg);
        syslog( LOG_CRIT, "%s", msg.toUtf8().data() );
        if(g_isatty)
        {
            std::cerr << "snapinit: fatal: " << msg.toUtf8().data() << std::endl;
        }
    }

    // Make sure the lock file has been removed
    //
    snap_init::pointer_t si( snap_init::instance() );
    si->remove_lock();

    // Exit with error status
    //
    ::exit( 1 );
    snap::NOTREACHED();
}











int main(int argc, char *argv[])
{
    int retval = 0;
    g_isatty = isatty(STDERR_FILENO);

    try
    {
        // First, create the static snap_init object
        //
        snap_init::create_instance( argc, argv );

        // Now run our processes!
        //
        snap_init::pointer_t init( snap_init::instance() );
        init->run_processes();
    }
    catch( snap::snap_exception const & e )
    {
        fatal_error(QString("snapinit: snap_exception caught! %1").arg(e.what()));
        snap::NOTREACHED();
    }
    catch( std::invalid_argument const & e )
    {
        fatal_error(QString("snapinit: invalid argument: %1").arg(e.what()));
        snap::NOTREACHED();
    }
    catch( std::exception const & e )
    {
        fatal_error(QString("snapinit: std::exception caught! %1").arg(e.what()));
        snap::NOTREACHED();
    }
    catch( ... )
    {
        fatal_error("snapinit: unknown exception caught!");
        snap::NOTREACHED();
    }

    return retval;
}

// vim: ts=4 sw=4 et
