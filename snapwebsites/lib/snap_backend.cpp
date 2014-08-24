// Snap Websites Server -- snap websites serving children
// Copyright (C) 2011-2014  Made to Order Software Corp.
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


void snap_backend::udp_monitor::set_signal( snap_backend::udp_signal_t signal )
{
    snap_thread::snap_lock lock( f_mutex );
    f_udp_signal = signal;
}


void snap_backend::udp_monitor::set_backend( zpsnap_backend_t backend )
{
    snap_thread::snap_lock lock( f_mutex );
    f_backend = backend;
}


bool snap_backend::udp_monitor::get_error() const
{
    snap_thread::snap_lock lock( f_mutex );
    return f_error;
}


bool snap_backend::udp_monitor::stop_received() const
{
    snap_thread::snap_lock lock( f_mutex );
    return f_stop_received;
}


bool snap_backend::udp_monitor::is_message_pending() const
{
    snap_thread::snap_lock lock( f_mutex );
    return !f_message_fifo.empty();
}


bool snap_backend::udp_monitor::pop_message( message_t& message, int const wait_msecs )
{
    // already received STOP? bypass the possible wait...
    if(f_stop_received)
    {
        return false;
    }
    return f_message_fifo.pop_front(message, wait_msecs);
}




void snap_backend::udp_monitor::run()
{
    while( !get_thread()->is_stopping() )
    {
        snap_thread::snap_lock lock( f_mutex );
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
                SNAP_LOG_FATAL() << "error: an error occurred in the UDP recv() call, returned size: " << r;
                f_error = true;
                break;
            }
            buf[r] = '\0';
            //
            if(strcmp(buf, g_stop_message) == 0)
            {
                // this is a special case where we do not need to push
                // the message because all we want now is stop
                //
                snap_thread::snap_lock lock( f_mutex );

                f_stop_received = true;

                // we have to push that or the listener is likely to
                // continue to way for minutes...
                //
                f_message_fifo.push_back( buf );
                break; // no need to listen for more
            }
            else
            {
                snap_thread::snap_lock lock( f_mutex );

                f_message_fifo.push_back( buf );
            }
        }
    }
}








/** \class backend
 * \brief Backend process class.
 *
 * This class handles backend processing for the snapserver.
 *
 * \todo Add more documentation about the backend and how it works.
 *
 * \sa snap_child
 */
snap_backend::snap_backend( server_pointer_t s )
    : snap_child(s)
    , f_thread( "snap_backend", &f_monitor )
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
    f_monitor.set_signal( udp_get_server( name.c_str() ) );
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

        f_is_child = true;
        f_child_pid = getpid();
        f_socket = -1;

        connect_cassandra();

        // define a User-Agent for all backends (should that be a parameter?)
        f_env[snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT)] = "Snap! Backend";

        auto p_server = f_server.lock();
        if(!p_server)
        {
            throw snap_child_exception_no_server("snap_backend::process(): The p_server weak pointer could not be locked");
        }

        // verify that the "sites" table exists
        QtCassandra::QCassandraTable::pointer_t sites_table = f_context->findTable(get_name(SNAP_NAME_SITES));
        if(!sites_table)
        {
            // the whole table is still missing after 5 minutes!
            // in this case it is an error instead of a fatal error
            SNAP_LOG_ERROR("The 'sites' table is still empty or nonexistent! Likely you have not set up the domains and websites tables, either. Exiting this backend!");

            // this applies to all the backends so we can as well exit
            // immediately instead of testing again and again
            exit(1);
            NOTREACHED();
        }

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
        SNAP_LOG_FATAL("snap_backend::process(): exception caught: ")(except.what());
    }
    catch( std::exception const& std_except )
    {
        SNAP_LOG_FATAL("snap_backend::process(): exception caught: ")(std_except.what())(" (there are mainly two kinds of exceptions happening here: Snap logic errors and Cassandra exceptions that are thrown by thrift)");
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_backend::process(): unknown exception caught!");
    }
    exit(1);
    NOTREACHED();
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
 * \param[in] uri  The URI of the site to be checked.
 */
void snap_backend::process_backend_uri(QString const& uri)
{
    // create a child process so the data between sites doesn't get
    // shared (also the Cassandra data would remain in memory increasing
    // the foot print each time we run a new website,) but the worst
    // are the plugins; we can request a plugin to be unloaded but
    // frankly the system is not very well written to handle that case.
    const pid_t p = fork_child();
    if(p != 0)
    {
        // parent process
        if(p == -1)
        {
            SNAP_LOG_FATAL("snap_backend::process_backend_uri() could not create a child process.");
            // we don't try again, we just abandon the whole process
            exit(1);
        }
        // block until child is done
        int status;
        wait(&status);
        // TODO: check status?
        return;
    }

    f_uri.set_uri(uri);

    // child process initialization
    //connect_cassandra(); -- this is already done in process()...

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

    init_plugins();

    canonicalize_options();

    f_ready = true;

    auto p_server( f_server.lock() );
    if(!p_server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }

    QString const action(p_server->get_parameter("__BACKEND_ACTION"));
    if(!action.isEmpty())
    {
        server::backend_action_map_t actions;
        p_server->register_backend_action(actions);
#ifdef DEBUG
        if(actions.contains("list"))
        {
            throw snap_logic_exception(QString("plugin \"%1\" makes use of an action named \"list\" which is reserved to the system")
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
            SNAP_LOG_ERROR() << "error: unknown action \"" << action << "\"";
            exit(1);
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
