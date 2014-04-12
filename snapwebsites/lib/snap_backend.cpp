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


class thread_life
{
public:
    thread_life( snap_thread* thread )
        : f_thread(thread)
    {
        assert(f_thread);
        f_thread->start();
    }

    ~thread_life()
    {
        f_thread->stop();
    }

private:
    snap_thread* f_thread;
};


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


void snap_backend::udp_monitor::run()
{
    while( !get_thread()->is_stopping() )
    {
        snap_thread::snap_lock lock( f_mutex );
        if( !f_udp_signal )
        {
            // wait until the signal is hooked up
            sleep(1);
            continue;
        }

        char buf[256];
        int const r( f_udp_signal->timed_recv( buf, sizeof(buf), 5 * 60 * 1000 ) ); // wait for up to 5 minutes (x 60 seconds)
        if( r != -1 || errno != EAGAIN )
        {
            if( r < 1 || r >= static_cast<int>(sizeof(buf) - 1) )
            {
                perror("udp_monitor::run(): f_udp_signal->timed_recv():");
                SNAP_LOG_FATAL() << "error: an error occured in the UDP recv() call, returned size: " << r;
                f_error = true;
                break;
            }
            buf[r] = '\0';
            //
            f_backend->push_message( buf );
        }
    }
}


/** \brief Create the UDP signal that will be monitored by the background thread.
 */
void snap_backend::create_signal( const std::string& name )
{
    snap_thread::snap_lock lock( f_mutex );
    f_monitor.set_signal( udp_get_server( name.c_str() ) );
}


/** \brief Check for received ping in background monitoring process.
 *
 * The snap_backend class creates a background thread which monitors
 * the backend action port. It uses a mutex to set the flag and message
 * after receipt.
 *
 */
std::string snap_backend::pop_message()
{
    snap_thread::snap_lock lock( f_mutex );

    if( f_message_list.empty() )
    {
        return std::string();
    }

    const std::string msg( f_message_list.front() );
    f_message_list.pop_front();
    return msg;
}


void snap_backend::push_message( const std::string& msg )
{
    snap_thread::snap_lock lock( f_mutex );

    f_message_list.push_back( msg );
}


bool snap_backend::get_error() const
{
    snap_thread::snap_lock lock( f_mutex );

    return f_monitor.get_error();
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

        const QString uri( p_server->get_parameter("__BACKEND_URI") );
        //
        QtCassandra::QCassandraTable::pointer_t sites_table = f_context->findTable(get_name(SNAP_NAME_SITES));
#if 0
        // TODO: this code needs to be moved to snapinit
        //
        if( !sites_table )
        {
            if( !uri.isEmpty())
            {
                // if it failed and the backend was specific, then we've got a
                // real problem so we have to stop with a fatal error
                //
                // the whole table is still empty
                SNAP_LOG_FATAL("The 'sites' table is empty or nonexistent! Likely you have not set up the domains and websites tables, either. Exiting!");
                return;
            }

            // TODO: we probably want to move the PING and STOP events sent via
            //       UDP to this function because it can be used in the wait of
            //       the 'sites' table and we could also make use of the loop
            //       over the 'sites' table in the backends...
            //       This would also offer all backends to run instead of having
            //       some such as sendmail and pagelist capturing the action and
            //       having their own infinite loops.
            //       Finally, we would have one place where we can easily define
            //       the amount of time we should wait and other similar
            //       parameters in link with the loop.

            // try up to 60 times or 5 minutes
            for(int i(0); i < 60; ++i)
            {
                // try getting the sites_table
                sites_table = f_context->findTable(get_name(SNAP_NAME_SITES));
                if(sites_table)
                {
                    break;
                }

                if(i == 0)
                {
                    SNAP_LOG_TRACE("The 'sites' table does not exist yet. Waiting...");
                }

                // otherwise wait 5 seconds before trying again
                sleep(5);
                f_context->clearCache();
            }
            //
            SNAP_LOG_TRACE("Got the 'sites' table, processing...");

            // TODO: how do we know that all the tables were created?
            //       actually, that could happen at any time, we may end up
            //       installing a new plugin that creates a table that's
            //       required by the backend at some point and the table
            //       won't exist in the f_context... at least here we
            //       wait 60 seconds to increase changes that our
            //       backends won't crash too soon after the first
            //       initialization of the server.
            //
            //       However, the libQtCassandra library is NOT currently
            //       testing to see whether a table exists after the first
            //       initialization. That is probably the worst problem at
            //       this point.
            //
            //       Finally, right now we've been testing with environment
            //       using 1 node. That's definitively not enough as a 3
            //       local nodes environment functions quite differently
            //       already, and then an environment with separate clusters
            //       where updates take even more time between nodes.
            sleep(60);
            f_context->clearCache();
            sites_table = f_context->findTable(get_name(SNAP_NAME_SITES));
            if(!sites_table)
            {
                // the whole table is still missing after 5 minutes!
                // in this case it is an error instead of a fatal error
                SNAP_LOG_ERROR("Lost the 'sites' table when re-resetting the context cache.");
                return;
            }
        }
#endif
        //
        if(!sites_table)
        {
            // the whole table is still missing after 5 minutes!
            // in this case it is an error instead of a fatal error
            SNAP_LOG_ERROR("After five minutes wait, the 'sites' table is still empty or nonexistent! Likely you have not set up the domains and websites tables, either. Exiting!");
            return;
        }

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
    assert( p_server );

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

        // RAII monitor for the background thread. Stops the thread
        // when it goes out of scope...
        //
        thread_life tl( &f_thread );

        if( actions.contains(action) )
        {
            // this is a valid action, execute the corresponding function!
            actions[action]->on_backend_action(action);
        }
        else if(action == "list")
        {
            // the user wants to know what's supported
            // we add a "list" entry so it appears in the right place
            class fake : public server::backend_action
            {
                virtual void on_backend_action(QString const& /*action*/)
                {
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
        p_server->backend_process();
    }
}


} // namespace snap


// vim: ts=4 sw=4 et
