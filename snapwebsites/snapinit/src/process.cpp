/////////////////////////////////////////////////////////////////////////////////
// Snap Init Server -- handle actual processes in snapinit
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

// snapwebsites library
//
#include "join_strings.h"
#include "log.h"
#include "not_used.h"

// C library
//
#include <grp.h>
#include <proc/sysinfo.h>
#include <pwd.h>
#include <syslog.h>
#include <sys/prctl.h>


/** \file
 * \brief A service runs a process.
 *
 * A process object has a state which is handled here.
 *
 * The state manchine has mainly three states plus the error state:
 *
 * 1. Process is currently stopped
 * 2. Process is running but is not yet registered with snapcommunicator
 * 3. Process is running and is registered with snapcommunicator
 * 4. Process just died (error state)
 *
 * There is a graph representing the process various states below.
 *
 * The "process died too many times" event generates a callback to the
 * service object which can enter the "Service Paused" state or terminate
 * if the process is required.
 *
 * IMPORTANT: The "restart process" is actually driven by the service
 * because it should not happen if the service is in the STOPPING state
 * or a dependency is not running. Especially, the cron task does not
 * get restarted immediately.
 *
 * NOTE: there is no action_stop() function because you could not stop a
 * process by calling it. Instead, the service sends a process a STOP
 * message and here we just see are the results: "process unregistered"
 * and "process died". (although we may safely miss the "process
 * unregistered" step in this case and that happens if SIGTERM is
 * used instead of the STOP message.)
 *
 * \code
 *                                    O
 *                                    | create process
 *                                    |
 *                                    V
 *                                +---------------+
 *     +------------------------->|               |<------------------+
 *     |                          | Stopped       |                   |
 *     | process stopped          | Process       |                   |
 *     |                          |               |                   |
 *     |                          +---+-----------+                   |
 *     |                              |                               |
 *     |                              | start process         +-------+-------+
 *     |                              |                       |               |
 *     |                              |                       | Dead          |
 *     |                              |                       | Process       |<--------------------------+
 *     |                              V                       |               |                           |
 *   +-+---------+  process died  +---------------+           +---------------+                           |
 *   |           |  [exit != 0]   |               |                   ^                                   |
 *   | Error     |<---------------+ Unregistered  |                   |                                   |
 *   | State     |                | Process       +-------------------+                                   |
 *   |           |            +-->|               | process died [exit == 0]                              |
 *   +---------- +            |   +---------------+                                                       |
 *         ^                  |       |                                                                   |
 *         |                  |       | process registered [if safe message is empty]                     |
 *         |                  |       |     or                                                            |
 *         |                  |       | safe message received [if safe message is not empty and matches]  |
 *         |                  |       |                                                                   |
 *         |     process      |       V                                                                   |
 *         |     unregistered |   +---------------+                                                       |
 *         |                  |   |               |                                                       |
 *         |                  +---+ Registered    +-------------------------------------------------------+
 *         |                      | Process       | process died [exit == 0]
 *         +----------------------+               |
 *           process died         |               |
 *           [exit != 0]          +---------------+
 * \endcode
 */

namespace snapinit
{

/////////////////////////////////////////////////
// PROCESS (class implementation)              //
/////////////////////////////////////////////////


/** \brief Initialize the process
 *
 * This function saves the snap_init and service pointers.
 *
 * \todo
 * Change those pointers with a callback instead.
 *
 * \param[in] si  The snap_init pointer.
 * \param[in] s  The service pointer.
 */
process::process(std::shared_ptr<snap_init> si, service * s)
    : f_snap_init(si)
    , f_service(s)
{
}


/** \brief If we are to force the user on startup of a process.
 *
 * snapinit will automatically start child processes making them
 * owned by this user if defined.
 *
 * By default processes would end up being root if not forced to
 * some other user. In most cases, Snap! Websites daemons should
 * all be started as snapwebsites:snapwebsites.
 *
 * \param[in] user  The name of the user to switch to on startup.
 */
void process::set_user(QString const & user)
{
    f_user = user;
}


/** \brief If we are to force the group on startup of a process.
 *
 * snapinit will automatically start child processes making them
 * owned by this group if defined.
 *
 * By default processes would end up being part of the root group
 * if not forced to some other group. In most cases, Snap! Websites
 * daemons should all be started as snapwebsites:snapwebsites.
 *
 * \param[in] group  The name of the group to switch to on startup.
 */
void process::set_group(QString const & group)
{
    f_group = group;
}


/** \brief Setup the coredump size limit.
 *
 * snapinit offers a way to get the coredump of a process. By default
 * this feature is not modified so the setup from the shell gets
 * used. In most cases, under Linux the shell setup for coredumps
 * is to not create a coredump (i.e. size of zero.)
 *
 * This comes from the XMl file and can be defined on a per service
 * basis.
 *
 * \param[in] coredump_limit  The limit amount of data to core dump on a crash.
 */
void process::set_coredump_limit(rlim_t coredump_limit)
{
    f_coredump_limit = coredump_limit;
}


/** \brief Setup the command of this process.
 *
 * This function saves the command of the process. As it is at it, it
 * verifies that the command exists and can be executed. In normal
 * startup mode, snapinit will fail immdiately if a process cannot
 * be found by this function.
 *
 * The \p binary_path parameter is a list of colon separated paths
 * used to search for the binaries.
 *
 * \todo
 * Pre-split the \p binary_path.
 *
 * \param[in] binary_path  A list of paths to use to search for the command.
 * \param[in] command  The command as defined by the \<command> tag, or use
 *                     the name of the service.
 */
bool process::set_command(QString const & binary_path, QString const & command)
{
    if(command.isEmpty())
    {
        common::fatal_error("process::set_command() cannot be called with an empty string.");
        snap::NOTREACHED();
    }

    // keep a copy although at this time we are not using it anywhere...
    //
    f_command = command;

    // we have a special case for snapinit--we do not have to find it
    // because we are not going to use its f_full_path anyway
    //
    if(f_command == "snapinit")
    {
        f_full_path = f_command;
        return true;
    }

    // compute the full path to the binary
    //
    // note: command cannot be empty here
    //
    if(command[0] != '/')
    {
        // try with all the binary paths offerred
        //
        snap::snap_string_list paths(binary_path.split(':'));
        for(auto const & p : paths)
        {
            // sub-folder (for snapdbproxy and snaplock while doing development, maybe others later)
            {
                f_full_path = QString("%1/%2/%3").arg(p).arg(command).arg(command);
                if(exists())
                {
                    return true;
                }
            }
            // direct
            {
                f_full_path = QString("%1/%2").arg(p).arg(command);
                if(exists())
                {
                    return true;
                }
            }
        }
    }
    else
    {
        f_full_path = command;
        if(exists())
        {
            return true;
        }
    }

    // okay, we do not completely ignore the fact that we could
    // not find the service, but we do not generate a fatal error
    //
    SNAP_LOG_WARNING("could not find \"")
                    (f_service->get_service_name())
                    ("\" in any of the paths \"")
                    (binary_path)
                    ("\".");

    return false;
}


/** \brief Verify that this executable exists.
 *
 * This function checks the full path to the executable used to
 * start this service. If that full path represents an existing
 * readable and executable file, then the function returns true.
 * Otherwise it returns false.
 *
 * When snapinit starts, it first checks whether all the services
 * that are required to start exist. If not then it fails because
 * if any one service is missing, something is awry anyway.
 *
 * \return true if the file exists and can be executed.
 */
bool process::exists() const
{
    return access(f_full_path.toUtf8().data(), R_OK | X_OK) == 0;
}


/** \brief Save the configuration filename for this service.
 *
 * Whenever we start a service, we pass the --config command line
 * option with a full path to the configuration file if specified
 * in the XML file. This function saves that path.
 *
 * \param[in] config_filename  The path and filename of the configuration file.
 */
void process::set_config_filename(QString const & config_filename)
{
    f_config_filename = config_filename;
}


/** \brief Set additional command line options.
 *
 * This function saves the additional command line options as defined in
 * the service XML data.
 *
 * The command line options may include quotes to include arguments that
 * include spaces. The whole argument needs to be quoted. For example:
 *
 * \code
 *      <options>--debug "--force=overwrite settings"</options>
 * \endcode
 *
 * We do not currently support backslash escaping.
 *
 * \param[in] options  Additional command options.
 */
void process::set_options(QString const & options)
{
    f_options = options;
}


/** \brief Set common command line options.
 *
 * Various services may add a command line option that it and all
 * the other services will accept (although some may not use that
 * option parameter.)
 *
 * At this time the snapcommunicator and snapdbproxy add their
 * command line option that way.
 *
 * \param[in] common_options  Additional command options.
 */
void process::set_common_options(std::vector<QString> const & common_options)
{
    f_common_options = std::move(common_options);
}


/** \brief Save the safe message if this service requires such.
 *
 * This function saves the safe message required by this service.
 * If such is defined, then the process is marked as registered
 * only once that specific safe message is received.
 *
 * If an invalid safe message is received, the whole process
 * ends (i.e. it is considered a really bad error to have a
 * safe message mismatch.)
 *
 * \param[in] safe_message  The safe message this process will send
 *                          us once ready.
 */
void process::set_safe_message(QString const & safe_message)
{
    f_safe_message = safe_message;
}


/** \brief Change the nice value of this process.
 *
 * A process with a small nice value (0) has priority and thus gets more
 * processing time than a process with a large nice value (19).
 *
 * We do not accept negative nice values since there is no point in running
 * our process with such preemptive priorities.
 *
 * \param[in] nice  The new nice value for this process.
 */
void process::set_nice(int const nice)
{
    f_nice = nice;
}


/** \brief The service has to be started now.
 *
 * This function starts the service process now.
 *
 * Only a service that is currently STOPPED can be started in this way.
 */
void process::action_start()
{
    if(f_state != process_state_t::PROCESS_STATE_STOPPED)
    {
        throw std::runtime_error("attempt to start a process that is not currently STOPPED.");
    }

    if(start_service_process())
    {
        action_process_unregistered();
    }
    else
    {
        // this is as if the process died immediately
        //
        f_end_date = f_start_date;

        action_error(true);
    }
}


/** \brief Mark this process as dead.
 *
 * Whenever the snapinit service_died() function gets called, it searches
 * for which services died. Then it calls this function to signal that
 * the process is indeed dead.
 *
 * Note that this function cannot be called more than once.
 *
 * As a side effect, the process_status_changed() function of the
 * corresponding service will be called. This may send a STOP signal
 * to other processes.
 *
 * \param[in] termination  How the process terminated.
 */
void process::action_died(termination_t termination)
{
    if(f_state == process_state_t::PROCESS_STATE_STOPPED
    || f_state == process_state_t::PROCESS_STATE_ERROR)
    {
        throw std::runtime_error("a STOPPED or ERROR process cannot die.");
    }
    f_end_date = snap::snap_communicator::get_current_date();

    // if snapcommunicator already died, we cannot forward
    // the DIED or any other message
    //
    {
        SNAP_LOG_TRACE("process::action_died(): service '")(f_service->get_service_name())("' died.");
        snap::snap_communicator_message register_snapinit;
        register_snapinit.set_command("DIED");
        register_snapinit.set_service(".");
        register_snapinit.add_parameter("service", f_service->get_service_name());
        register_snapinit.add_parameter("pid", f_pid);
        snap_init_ptr()->send_message(register_snapinit);
    }

    if(termination == termination_t::TERMINATION_NORMAL)
    {
        action_dead();
    }
    else
    {
        // process died with an error, reflect that by calling action_error()
        //
        action_error(false);
    }

    f_service->process_status_changed();
}


void process::action_process_registered()
{
    if(f_state != process_state_t::PROCESS_STATE_UNREGISTERED)
    {
        throw std::runtime_error(std::string("only an UNREGISTERED process can become REGISTERED, right now process state is ") + state_to_string(f_state) + ".");
    }
    if(f_safe_message.isEmpty())
    {
        f_state = process_state_t::PROCESS_STATE_REGISTERED;

        f_service->process_status_changed();
    }
    //else -- wait on the safe message instead
}


void process::action_process_unregistered()
{
    if(f_state != process_state_t::PROCESS_STATE_STOPPED
    && f_state != process_state_t::PROCESS_STATE_REGISTERED)
    {
        throw std::runtime_error("only a STOPPED or REGISTERED process can become UNREGISTERED.");
    }
    f_state = process_state_t::PROCESS_STATE_UNREGISTERED;

    //if(f_command == "snapcommunicator")
    //{
    //    // auto-register the snapcommunicator
    //    //
    //    action_process_registered();
    //}
    //else
    //{
        f_service->process_status_changed();
    //}
}


/** \brief We just received a safe message, check whether this is valid.
 *
 * This function checks whether the safe message we just received matches
 * this process expected safe message.
 *
 * \todo
 * Verify that the source service name is also defined as expected
 * (i.e. service sent from.)
 *
 * \param[in] message  The safe message we just received.
 */
void process::action_safe_message(QString const & message)
{
    // make sure input is valid
    //
    if(message.isEmpty())
    {
        throw std::logic_error("action_safe_message() cannot be called with an empty message as input.");
    }

    if(f_safe_message != message)
    {
        // we want to terminate the existing services cleanly
        // so we do not use common::fatal_error() here
        //
        common::fatal_message(QString("received wrong SAFE message. We expected \"%1\" but we received \"%2\".")
                                        .arg(f_safe_message)
                                        .arg(message));

        // Simulate a STOP, we cannot continue safely
        //
        snap_init_ptr()->terminate_services();
        return;
    }

    if(f_state != process_state_t::PROCESS_STATE_UNREGISTERED)
    {
        throw std::runtime_error("only an UNREGISTERED process can become REGISTERED (on safe message received).");
    }

    f_state = process_state_t::PROCESS_STATE_REGISTERED;

    f_service->process_status_changed();
}


/** \brief Called whenever the process dies without errors.
 *
 * In most cases a process dies with an exit code of zero. In that case,
 * there is no error to manage and we want to reset the error counter.
 *
 * This action is called when the action_died() function go called with
 * a NORMAL termination.
 */
void process::action_dead()
{
    f_state = process_state_t::PROCESS_STATE_STOPPED;
    f_error_count = 0;

    // let the service know that we died, allow for the service
    // to start a timer to call action_start() soonish or if it is
    // in its STOPPING state to ignore the event
    //
    f_service->process_died();

    f_service->process_status_changed();
}


/** \brief Called whenever the process dies.
 *
 * This function gets called whenever a process dies with an error.
 * (i.e. not with exit code of zero.)
 *
 * \param[in] immediate_error  The child could not be started (i.e. in most
 * cases this means the fork() call itself failed.)
 */
void process::action_error(bool immediate_error)
{
    f_state = process_state_t::PROCESS_STATE_ERROR;

    f_service->process_status_changed();

    f_state = process_state_t::PROCESS_STATE_STOPPED;

    // did the process die too quickly?
    //
    // TBD: put the MAX_START_INTERVAL in the .conf?
    //
    if(immediate_error
    || f_end_date - f_start_date < MAX_START_INTERVAL)
    {
        ++f_error_count;
    }
    else
    {
        f_error_count = 0;
    }

    // if too many errors happened too quickly, then call the
    // process_pause() function
    //
    // TBD: put the MAX_START_COUNT in the .conf?
    //
    if(f_error_count >= MAX_START_COUNT
    || immediate_error)
    {
        // if too many errors occurred in a row, or fork() failed immediately
        // then we ask the service to pause for a while before calling
        // action_start() again
        //
        f_service->process_pause();

        // reset the counter now for next time
        //
        f_error_count = 0;
    }
    else
    {
        // let the service know that we died, allow for the service
        // to either call action_start() immediately or if it is
        // in its STOPPING state to ignore the event
        //
        f_service->process_died();
    }

    f_service->process_status_changed();
}


bool process::is_running() const
{
    return f_state == process_state_t::PROCESS_STATE_UNREGISTERED
        || f_state == process_state_t::PROCESS_STATE_REGISTERED;
}


bool process::is_registered() const
{
    return f_state == process_state_t::PROCESS_STATE_REGISTERED;
}


bool process::is_stopped() const
{
    return f_state == process_state_t::PROCESS_STATE_STOPPED;
}


pid_t process::get_pid() const
{
    return f_pid;
}


QString const & process::get_config_filename() const
{
    return f_config_filename;
}


/** \brief Send the specified signal to the proces..
 *
 * This function sends the specified signal to the process. We expect
 * the service implementation to call the function with SIGTERM and
 * SIGKILL whenever it is trying to stop the process and the STOP
 * message did not work.
 *
 * \note
 * In development mode, when the programmer runs snapinit as himself,
 * the function may fail because the destination process has more
 * privileges than snapinit (i.e. snapmanagerdaemon runs as root.)
 *
 * \param[in] signum  The signal number such as SIGTERM.
 *
 * \return true if the signal was sent successfully, false otherwise.
 */
bool process::kill_process(int signum)
{
    // TODO: verify pid?
    //
    int const retval(::kill( f_pid, signum ));
    if( retval == -1 )
    {
        // we consider this a fatal error, although if we could not
        // send SIGTERM, we will still try with SIGKILL and then
        // abort the process -- so you are likely to see this
        // error twice in a row...
        //
        int const e(errno);
        common::fatal_message(QString("Unable to kill service \"%1\", pid=%2, errno=%3 -- %4")
                .arg(f_service->get_service_name())
                .arg(f_pid)
                .arg(e)
                .arg(strerror(e)));
        return false;
    }

    return true;
}


std::shared_ptr<snap_init> process::snap_init_ptr()
{
    snap_init::pointer_t locked(f_snap_init.lock());
    if(!locked)
    {
        common::fatal_error("process::snap_init_ptr(): somehow we could not get a lock on f_snap_init from a process object.");
        snap::NOTREACHED();
    }
    return locked;
}


bool process::start_service_process()
{
    // mark when this service is started using the current system
    // time; that way we can see whether the run was very short
    // when the process dies and if so eventually mark the process
    // as failed
    //
    f_start_date = snap::snap_communicator::get_current_date();

    // if this is the snapinit service, then it is always running
    // (or this code would not be executed!)
    //
    if(f_command == "snapinit")
    {
        // this is us!
        //
        f_pid = getpid();
        return true;
    }

    pid_t const parent_pid(getpid());
    f_pid = fork();

    // child?
    //
    if(f_pid == 0)
    {
        exec_child(parent_pid);
        snap::NOTREACHED();
    }

    // error?
    //
    if(-1 == f_pid)
    {
        int const e(errno);
        SNAP_LOG_ERROR("fork() failed to create a child process to start service \"")(f_service->get_service_name())("\". (errno: ")(e)(" -- ")(strerror(e))(")");

        // request the proc library to read memory information
        meminfo();
        SNAP_LOG_INFO("memory total: ")(kb_main_total)(", free: ")(kb_main_free)(", swap_free: ")(kb_swap_free)(", swap_total: ")(kb_swap_total);

        return false;
    }

    // here we are considered started and running
    //
    return true;
}


void process::parse_options(std::vector<std::string> & args, char const * s)
{
    auto const push_arg([&args](char const * start, char const * end, bool const push_empty = false)
        {
            if(end > start
            || push_empty)
            {
                args.push_back(std::string(start, end - start));
            }
        });

    // TODO: refactor raw loop?
    //
    char const * start(s);
    while(*s != '\0')
    {
        if(*s == '"' || *s == '\'')
        {
            // quotes define options with special characters
            //
            push_arg(start, s);
            char const quote(*s++);

            // TODO: add support for escaping quotes within a string
            //
            start = s;
            for(; *s != quote && *s != '\0'; ++s);
            push_arg(start, s, true);

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
            // spaces separate options
            //
            push_arg(start, s);

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
            //
            ++s;
        }
    }

    // and potentially a last argument
    push_arg(start, s);
}


/** \brief This function is run by a child process to start a service.
 *
 * This function initializes the child process in various ways and
 * then calls execv(). The function never returns.
 */
void process::exec_child(pid_t parent_pid)
{
    // make sure that the SIGHUP is sent to us if our parent dies
    //
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    // unblock those signals we blocked in the main snapinit process
    // because the children should not have such a mask on startup
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

    if(f_nice >= 0)
    {
        SNAP_LOG_TRACE("set nice of ")(f_service->get_service_name())(" to ")(f_nice);
        setpriority(PRIO_PROCESS, 0, f_nice);
    }

    // if the user requested core dump files, we turn on the feature here
    //
    // We do not change it if f_coredump_limit is set to zero, that way
    // the shell `ulimit -c <size>` gets used
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

    std::vector<std::string> args;
    args.push_back(f_full_path.toUtf8().data());

    // various services may offer common options which are defined in
    // the <common-options> tag (i.e. snapcommunicator and snapdbproxy)
    //
    // note that the snapinit service is  given a few common options
    // of its own (See snapinit.cpp for details) even though it does
    // not come from an XML file
    //
    std::for_each(
            f_common_options.begin(),
            f_common_options.end(),
            [this, &args](auto const & options)
            {
                std::string const opts(options.toUtf8().data());
                this->parse_options(args, opts.c_str());
            });

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
        parse_options(args, opts.c_str());
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
    if(!snap_init_ptr()->get_debug())
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
            struct group * grp(getgrnam(f_group.toUtf8().data()));
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
            struct passwd * pswd(getpwnam(f_user.toUtf8().data()));
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

    // make sure we can have an idea of how the command looks like
    //
    std::string const command_line(snap::join_strings(args, " "));
    SNAP_LOG_TRACE(QString("starting service with command line: \"%1\"").arg(command_line.c_str()));

    // Execute the child processes
    //
    execv(
        args_p[0],
        const_cast<char * const *>(&args_p[0])
    );
#pragma GCC diagnostic pop

    // the command did not start...
    //
    int const e(errno);
    common::fatal_error(QString("service::run() child: process \"%1\" failed to start! (errno: %2, %3)")
                    .arg(command_line.c_str())
                    .arg(e)
                    .arg(strerror(e))
                    );
    snap::NOTREACHED();
}


/** \brief Transform the current state to a string for display.
 *
 * This function transforms the specified state into a string.
 *
 * \param[in] state  The state to convert to a string.
 *
 * \return A string naming the specified state.
 */
char const * process::state_to_string( process_state_t const state )
{
    switch( state )
    {
    case process_state_t::PROCESS_STATE_STOPPED:
        return "PROCESS_STATE_STOPPED";

    case process_state_t::PROCESS_STATE_UNREGISTERED:
        return "PROCESS_STATE_UNREGISTERED";

    case process_state_t::PROCESS_STATE_REGISTERED:
        return "PROCESS_STATE_REGISTERED";

    case process_state_t::PROCESS_STATE_ERROR:
        return "PROCESS_STATE_ERROR";

    }
    return "PROCESS_STATE_UNKNOWN";
}




} // namespace snapinit
// vim: ts=4 sw=4 et
