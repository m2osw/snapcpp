// Snap Websites Server -- snap websites serving children
// Copyright (C) 2011-2015  Made to Order Software Corp.
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

#include "snap_backend.h"

#include "snapwebsites.h"
#include "log.h"
#include "not_reached.h"

#include <wait.h>
#include <fcntl.h>


namespace snap
{

namespace
{

// this is a special case handled internally so the STOP works
// as soon as possible (some backend may still take a little
// while before stopping, but this should help greatly.)
char const *g_stop_message = "STOP";

}
// no name namespace






/* \class udp_monitor
 *
 * This private class encapsulates a thread which monitors the UDP buffer.
 */


snap_backend::udp_monitor::udp_monitor()
    : snap_thread::snap_runner( "udp_monitor" )
{
}


snap_backend::udp_signal_t snap_backend::udp_monitor::get_signal() const
{
    snap_thread::snap_lock lock( f_mutex_and_message_fifo );
    return f_udp_signal;
}


void snap_backend::udp_monitor::set_signal( snap_backend::udp_signal_t signal )
{
    snap_thread::snap_lock lock( f_mutex_and_message_fifo );
    f_udp_signal = signal;
}


void snap_backend::udp_monitor::set_backend( zpsnap_backend_t backend )
{
    snap_thread::snap_lock lock( f_mutex_and_message_fifo );
    f_backend = backend;
}


bool snap_backend::udp_monitor::get_error() const
{
    snap_thread::snap_lock lock( f_mutex_and_message_fifo );
    return f_error;
}


bool snap_backend::udp_monitor::stop_received() const
{
    snap_thread::snap_lock lock( f_mutex_and_message_fifo );
    return f_stop_received;
}


bool snap_backend::udp_monitor::is_message_pending() const
{
    // Note: the message FIFO is its own mutex and the empty() function
    //       already calls lock() as required
    return !f_mutex_and_message_fifo.empty();
}


bool snap_backend::udp_monitor::pop_message( message_t& message, int const wait_msecs )
{
    // already received STOP? bypass the possible wait...
    if(stop_received()) // must lock, hence call the function
    {
        return false;
    }
    return f_mutex_and_message_fifo.pop_front(message, static_cast<int64_t>(wait_msecs) * 1000UL);
}




void snap_backend::udp_monitor::run()
{
    while( !get_thread()->is_stopping() )
    {
        snap_thread::snap_lock lock( f_mutex_and_message_fifo );
        if( f_udp_signal )
        {
            break;
        }

        // wait until the signal is hooked up
        lock.unlock();
        sleep(1);
    }

    while( !get_thread()->is_stopping() )
    {
        char buf[256];
        int const r( f_udp_signal->timed_recv( buf, sizeof(buf), 1000 ) );
        if( r != -1 || errno != EAGAIN )
        {
            if( r < 1 || r >= static_cast<int>(sizeof(buf) - 1) )
            {
                perror("udp_monitor::run(): f_udp_signal->timed_recv():");
                SNAP_LOG_FATAL("snap_backend::udp_monitor::run(): an error occurred in the UDP recv() call, returned size: ")(r);
                f_error = true;
                break;
            }
            buf[r] = '\0';
            //
            if(strcmp(buf, g_stop_message) == 0)
            {
                // this is a special case where we also mark the
                // backend as "stopping"
                //
                snap_thread::snap_lock lock( f_mutex_and_message_fifo );

                f_stop_received = true;

                // we have to push the STOP message anyway or the
                // listener is likely to continue to wait for minutes...
                // (i.e. that triggers the signal as required)
                //
                f_mutex_and_message_fifo.push_back( buf );
                break; // no need to listen for more
            }
            else
            {
                f_mutex_and_message_fifo.push_back( buf );
            }
        }
    }
}








/** \class backend
 * \brief Backend process class.
 *
 * This class handles backend processing for the snapserver.
 *
 * The process for backends works this way:
 *
 * \li Backend tool prepares the server
 * \li Backend tool creates a snap_backend object.
 * \li Backend tool calls run_backend()
 * \li run_backend() connects to the database
 * \li run_backend() checks whether the sites table exists
 * \li if not ready -- wait until the sites table exists
 * \li -- while waiting for the sites table, we also check UDP STOP and
 *        PING signals
 *
 * \note
 * The constructor initializes the monitor and thread objects, however,
 * the thread is only started when the child is called with an action.
 *
 * \todo
 * Add more documentation about the backend and how it works.
 *
 * \sa snap_child
 */
snap_backend::snap_backend( server_pointer_t s )
    : snap_child(s)
    , f_thread( "snap_backend", &f_monitor ) // start in process_backend_uri()
{
    f_monitor.set_backend( this );
}


snap_backend::~snap_backend()
{
    // empty
}




/** \brief Create an object to monitor UDP messages.
 *
 * This function creates the UDP signal and attaches it to the monitor.
 *
 * This signal is used to monitor signals from the front end servers
 * in an attempt to wake up the backends.
 *
 * \param[in] name  A name to identifier the object in memory.
 */
void snap_backend::create_signal( std::string const& name )
{
    if(!f_monitor.get_signal())
    {
        f_monitor.set_signal( udp_get_server( name.c_str() ) );
    }
}


/** \brief Check whether the monitor had a problem.
 *
 * This function returns true if the monitor detected an error and
 * returned prematuraly. This should be checked in your backend
 * loop because no more message will be received once this flag is
 * set to one and the backend should be restarted as soon as
 * possible.
 *
 * Note that it is not necessary to break your inner loops on an
 * error. Only the main loop that waits on messages needs to test
 * this flag and if through break free.
 *
 * \return true if an error occurred and the backend should restart
 *         to make sure it continues to receive messages.
 */
bool snap_backend::get_error() const
{
    return f_monitor.get_error();
}


/** \brief Check whether the STOP signal was received.
 *
 * This function checks whether the UDP signal thread received the
 * STOP message. If so the function returns true and you are
 * expected to return from your backend as soon as possible.
 *
 * \return true if the backend thread received the STOP signal.
 */
bool snap_backend::stop_received() const
{
    return f_monitor.stop_received();
}


/** \brief Check to see if there are any ping messages pending.
 *
 * \note This method does not block.
 *
 * \return true if pending messages that can be read.
 */
bool snap_backend::is_message_pending() const
{
    return f_monitor.is_message_pending();
}


/** \brief Pop received UDP message from top of queue.
 *
 * The snap_backend class creates a background thread which monitors
 * the backend action port. It uses a mutex to set the flag and message
 * after receipt.
 *
 * \note
 * This method does not currently block. However, it may wait wait_secs
 * if the queue is empty. We will change that in the future so the
 * function can block until the next message or for X seconds.
 *
 * \param [out] message  Top message of the FIFO if not empty.
 * \param [in] wait_msecs  Wait time in milliseconds if FIFO is empty.
 *
 * \return true if a message was read from the front of the message list.
 */
bool snap_backend::pop_message( message_t& message, int const wait_msecs )
{
    return f_monitor.pop_message(message, wait_msecs);
}


/** \brief Execute the backend processes after initialization.
 *
 * This function is somewhat similar to the process() function. It is used
 * to ready the server and then run the backend processes by sending a
 * signal.
 */
void snap_backend::run_backend()
{
    try
    {
        init_start_date();

        // somewhat fake being a child (we are not here)
        f_is_child = true;
        f_child_pid = getpid();
        f_socket = -1;

        connect_cassandra();

        // define a User-Agent for all backends (should that be a parameter?)
        f_env[snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT)] = "Snap! Backend";

        auto p_server = f_server.lock();
        if(!p_server)
        {
            throw snap_child_exception_no_server("snap_backend::run_backend(): The p_server weak pointer could not be locked");
        }

        // verify that the "sites" table exists and is ready
        // this is a loop, we wait until the table gets ready
        //
        // NOTE: This is somewhat considered a hack; the proper fix (to be
        //       created) will be to have a dry run of the server to create
        //       the tables before you run snapinit to start anything
        //       (i.e. something like snapsetup <uri>)
        QString const action(p_server->get_parameter("__BACKEND_ACTION"));
        QtCassandra::QCassandraTable::pointer_t sites_table;
        {
            std::unique_ptr<snap_thread::snap_thread_life> tl;

            bool emit_warning(true);
            for(;;)
            {
                sites_table = f_context->findTable(get_name(SNAP_NAME_SITES));
                if(sites_table)
                {
                    // table exists, we can move on for now
                    break;
                }

//std::cerr << "no sites?! action is [" << action << "]\n";
                if(action.isEmpty())
                {
                    // this applies to all the backends so we can as well exit
                    // immediately instead of testing again and again
                    SNAP_LOG_FATAL("snap_backend::run_backend(): The 'sites' table is still empty or nonexistent! Likely you have not set up the domains and websites tables, either. Exiting this backend immediate!");
                    exit(1);
                    NOTREACHED();
                }

                if(emit_warning)
                {
                    emit_warning = false;

                    // the whole table is still missing after 5 minutes!
                    // in this case it is an error instead of a fatal error
                    SNAP_LOG_WARNING("snap_backend::run_backend(): The 'sites' table is still empty or nonexistent! Waiting before fully starting the \"")(action)("\" backend.");
                }

                if(!tl)
                {
                    std::string const signal_name(get_signal_name_from_action(action));
                    if(signal_name.empty())
                    {
                        SNAP_LOG_FATAL("snap_backend::run_backend(): The 'sites' table is not ready, this backend cannot be run at this time.");
                        exit(1);
                        NOTREACHED();
                    }

                    tl.reset(new snap_thread::snap_thread_life(&f_thread));
                    create_signal(signal_name);
                }
                if(get_error())
                {
                    SNAP_LOG_FATAL("snap_backend::run_backend(): The 'sites' table is not ready and we got an error from the UDP server!");
                    exit(1);
                    NOTREACHED();
                }
                snap_backend::message_t message;
                pop_message(message, 10 * 1000); // wait up to 10 seconds or STOP
                if(stop_received())
                {
                    SNAP_LOG_INFO("snap_backend::run_backend(): Stopped while waiting for the 'sites' table to be ready.");
                    exit(1);
                    NOTREACHED();
                }

                // this applies to all the backends so we can as well exit
                // immediately instead of testing again and again
                //exit(1);
                //NOTREACHED();
                f_context->clearCache();
            }
        }
        // reset that signal server because otherwise the backend itself
        // will fail; in most cases it is already a nullptr anyway
        udp_signal_t null_pointer;
        f_monitor.set_signal( null_pointer );

        QString const uri( p_server->get_parameter("__BACKEND_URI") );
        if( !uri.isEmpty() )
        {
            process_backend_uri(uri);
        }
        else
        {
            // if a site exists then it has a "core::last_updated" entry
            QtCassandra::QCassandraColumnNamePredicate::pointer_t column_predicate(new QtCassandra::QCassandraColumnNamePredicate);
            column_predicate->addColumnName(get_name(SNAP_NAME_CORE_LAST_UPDATED));
            QtCassandra::QCassandraRowPredicate row_predicate;
            row_predicate.setColumnPredicate(column_predicate);
            for(;;)
            {
                sites_table->clearCache();
                uint32_t count(sites_table->readRows(row_predicate));
                if(count == 0)
                {
                    // we reached the end of the whole table
                    break;
                }
                QtCassandra::QCassandraRows const r(sites_table->rows());
                for(QtCassandra::QCassandraRows::const_iterator o(r.begin());
                    o != r.end(); ++o)
                {
                    QString const key(QString::fromUtf8(o.key().data()));
                    process_backend_uri(key);
                }
            }
        }

        // return normally if no exception occured
        return;
    }
    catch( snap_exception const& except )
    {
        SNAP_LOG_FATAL("snap_backend::run_backend(): exception caught: ")(except.what());
    }
    catch( std::exception const& std_except )
    {
        SNAP_LOG_FATAL("snap_backend::run_backend(): exception caught: ")(std_except.what())(" (there are mainly two kinds of exceptions happening here: Snap logic errors and Cassandra exceptions that are thrown by thrift)");
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_backend::run_backend(): unknown exception caught!");
    }
    exit(1);
    NOTREACHED();
}


std::string snap_backend::get_signal_name_from_action(QString const& action)
{
    // in order to retrieve the signal name, we need a complete list of
    // plugins as a child process gets, so here we create a child and let
    // it determine the signal name
    //
    // we make use of pipes to retrieve the result once the child got it
    int ps[2];
    if(pipe2(ps, O_CLOEXEC) != 0)
    {
        SNAP_LOG_FATAL("snap_backend::get_signal_name_from_action() could not create pipes.");
        // we do not try again, we just abandon the whole process
        exit(1);
        NOTREACHED();
    }

    const pid_t p = fork_child();
    if(p != 0)
    {
        // no need for the write side here
        close(ps[1]);

        // parent process
        if(p == -1)
        {
            SNAP_LOG_FATAL("snap_backend::get_signal_name_from_action() could not create a child process.");
            // we do not try again, we just abandon the whole process
            exit(1);
            NOTREACHED();
        }

        char buf[256];
        int sz(::read(ps[0], buf, sizeof(buf) - 1));
        if(sz < 0)
        {
            SNAP_LOG_FATAL("snap_backend::get_signal_name_from_action() failed while reading from pipe.");
            // we do not try again, we just abandon the whole process
            exit(1);
            NOTREACHED();
        }
        buf[sz] = '\0';
        close(ps[0]);

        // block until child is done
        //
        // XXX should we have a way to break the wait after a "long"
        //     while in the event the child locks up?
        int status(0);
        wait(&status);
        // TODO: check status?
        return buf;
    }

    // no need for the read on this side
    close(ps[0]);

    // child process initialization
    //connect_cassandra(); -- this is already done in process()...

    // WARNING: this call checks the sites table for additional plugins
    //          this should just fail with an empty string which is fine
    //          because at the start the website cannot already have
    //          additional plugins defined!
    init_plugins(true);

    auto p_server( f_server.lock() );
    if(!p_server)
    {
        throw snap_logic_exception("snap_backend::get_signal_name_from_action(): server pointer is NULL");
    }

    server::backend_action_map_t actions;
    p_server->register_backend_action(actions);
    if(actions.contains(action))
    {
        char const *name(actions[action]->get_signal_name(action));
        size_t const len(strlen(name));
        ssize_t r(::write(ps[1], name, len));
        if(r != static_cast<ssize_t>(len))
        {
            SNAP_LOG_ERROR("snap_backend::get_signal_name_from_action() failed while writing to pipe (wrote ")(r)(" instead of ")(len)(").");
        }
    }

    // the child just dies now, it served its purpose
    exit(0);

    // for C++ we need a valid return here...
    return "";
}


/** \brief Process a backend request on the specified URI.
 *
 * This function is called with each URI that needs to be processed by
 * the backend processes. It creates a child process that will allow
 * the Cassandra data to not be shared between all instances. Instead
 * each instance reads data and then drops it as the process ends.
 * Since the parent blocks until the child is done, the Cassandra library
 * is still only used by a single process at a time thus we avoid
 * potential conflicts reading/writing on the same network connection
 * (since the child inherits the parents Cassandra connection.)
 *
 * \note
 * Note that the child is created from Cassandra, the plugins, the
 * f_uri and all the resulting keys... so we gain an environment
 * very similar to what we get in the server with Apache.
 *
 * \note
 * If that site has an internal redirect then no processing is
 * performed because otherwise the destination would be processed
 * twice in the end.
 *
 * \todo
 * Add necessary code to break the child if (1) the child is very long
 * and (2) never contact us (i.e. watchdog signals.)
 *
 * \param[in] uri  The URI of the site to be checked.
 */
void snap_backend::process_backend_uri(QString const& uri)
{
    // create a child process so the data between sites does not get
    // shared (also the Cassandra data would remain in memory increasing
    // the foot print each time we run a new website,) but the worst
    // are the plugins; we can request a plugin to be unloaded but
    // frankly the system is not very well written to handle that case.
    pid_t const p(fork_child());
    if(p != 0)
    {
        // parent process
        if(p == -1)
        {
            SNAP_LOG_FATAL("snap_backend::process_backend_uri() could not create a child process.");
            // we do not try again, we just abandon the whole process
            exit(1);
            NOTREACHED();
        }
        // block until child is done
        //
        // XXX should we have a way to break the wait after a "long"
        //     while in the event the child locks up?
        int status(0);
        wait(&status);
        // TODO: check status?
        return;
    }

    // set the URI; if user supplied it, then it can fail!
    if(!f_uri.set_uri(uri))
    {
        SNAP_LOG_FATAL("snap_backend::process_backend_uri() called with invalid URI: \"")(uri)("\", URI ignored.");
        exit(1);
        NOTREACHED();
    }

    // child process initialization
    //connect_cassandra(); -- this is already done in run_backend()...

    // process the f_uri parameter
    canonicalize_domain();
    canonicalize_website();
    site_redirect();
    if(f_site_key != f_original_site_key)
    {
        return;
    }
    // same as in normal server process -- should it change for each iteration?
    // (i.e. we're likely to run the backend process for each website of this
    // Cassandra instance!)
    // TODO: make sure this is not used anywhere anymore and then remove
    //       it; it is a lot faster to use f_snap->get_start_date()
    f_uri.set_option("start_date", QString("%1").arg(f_start_date));

    init_plugins(true);

    canonicalize_options();

    f_ready = true;

    auto p_server( f_server.lock() );
    if(!p_server)
    {
        throw snap_logic_exception("snap_backend::process_backend_uri(): server pointer is NULL");
    }

    QString const action(p_server->get_parameter("__BACKEND_ACTION"));
    if(!action.isEmpty())
    {
        server::backend_action_map_t actions;
        p_server->register_backend_action(actions);
#ifdef DEBUG
        if(actions.contains("list"))
        {
            throw snap_logic_exception(QString("snap_backend::process_backend_uri(): plugin \"%1\" makes use of an action named \"list\" which is reserved to the system")
                                                .arg(dynamic_cast<plugins::plugin *>(actions["list"])->get_plugin_name()));
        }
#endif

        if( actions.contains(action) )
        {
            // RAII monitor for the background thread. Stops the thread
            // when it goes out of scope...
            //
            snap_thread::snap_thread_life tl( &f_thread );

            // this is a valid action, execute the corresponding function!
            actions[action]->on_backend_action(action);
        }
        else if(action == "list")
        {
            // the user wants to know what's supported
            // we add a "list" entry so it appears in the right place
            class fake : public server::backend_action
            {
                virtual void on_backend_action(QString const& action)
                {
                    static_cast<void>(action);
                }
            };
            fake foo;
            actions["list"] = &foo;
            for(server::backend_action_map_t::const_iterator it(actions.begin()); it != actions.end(); ++it)
            {
                std::cout << it.key() << std::endl;
            }
        }
        else
        {
            SNAP_LOG_ERROR("snap_backend::process_backend_uri(): unknown action \"")(action)("\"");
            exit(1);
            NOTREACHED();
        }
    }
    else
    {
        // "standalone" backend processes are not expected to block
        // because if they do most everything won't work as expected
        // thus we do not need a thread here
        p_server->backend_process();
    }
}


} // namespace snap


// vim: ts=4 sw=4 et
