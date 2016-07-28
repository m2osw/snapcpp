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

#pragma once

// ourselves
//
#include "service.h"

// snapwebsites
//
#include "snapwebsites.h"
#include "snap_communicator.h"

// our libs
//
#include <advgetopt/advgetopt.h>

// Qt lib
//
#include <QDomDocument>
#include <QFile>
#include <QString>

// C++ lib
//
#include <functional>
#include <memory>
#include <string>



namespace snapinit
{


/////////////////////////////////////////////////
// SNAP INIT (class declaration)               //
/////////////////////////////////////////////////

class snap_init
        : public std::enable_shared_from_this<snap_init>
{
public:
    typedef std::shared_ptr<snap_init>  pointer_t;
    typedef std::weak_ptr<snap_init>    weak_pointer_t;

    enum class command_t
    {
        COMMAND_UNKNOWN,
        COMMAND_START,
        COMMAND_STOP,
        COMMAND_RESTART,
        COMMAND_LIST,
        COMMAND_TREE
    };

    /** \brief Handle incoming messages from Snap Communicator server.
     *
     * This class is an implementation of the TCP client message connection
     * used to accept messages received via the Snap Communicator server.
     */
    class listener_impl
            : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
    {
    public:
        typedef std::shared_ptr<listener_impl>    pointer_t;

        /** \brief The listener initialization.
         *
         * The listener receives UDP messages from various sources (mainly
         * backends at this point.)
         *
         * Retry the connection every 3 seconds.
         *
         * \note
         * Note that we pass -3 seconds so that way the first connection
         * happens after 3 seconds instead of immediately. That way we
         * are much more likely to connect at once within 3 seconds.
         *
         * \param[in] si  The snap init server we are listening for.
         * \param[in] addr  The address to listen on. Most often it is 127.0.0.1.
         * \param[in] port  The port to listen on (4040).
         */
        listener_impl(snap_init::pointer_t si, std::string const & addr, int port)
            : snap_tcp_client_permanent_message_connection(addr, port, tcp_client_server::bio_client::mode_t::MODE_PLAIN, -3LL * common::SECONDS_TO_MICROSECONDS, false)
            , f_snap_init(si)
        {
        }

        // snap::snap_communicator::snap_server_connection implementation
        virtual void process_message(snap::snap_communicator_message const & message) override
        {
            // we can call the same function for UDP and TCP messages
            f_snap_init->process_message(message, false);
        }

        virtual void process_connected() override
        {
            snap_tcp_client_permanent_message_connection::process_connected();

            snap::snap_communicator_message register_snapinit;
            register_snapinit.set_command("REGISTER");
            register_snapinit.add_parameter("service", "snapinit");
            register_snapinit.add_parameter("version", snap::snap_communicator::VERSION);
            send_message(register_snapinit);
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
        virtual void process_message(snap::snap_communicator_message const & message) override
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
        virtual void process_signal() override
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
        virtual void process_signal() override
        {
            // we call the same function on SIGTERM, SIGQUIT and SIGINT
            f_snap_init->user_signal_caught("SIGTERM");
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
        virtual void process_signal() override
        {
            // we call the same function on SIGTERM, SIGQUIT and SIGINT
            f_snap_init->user_signal_caught("SIGQUIT");
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
        virtual void process_signal() override
        {
            // we call the same function on SIGTERM, SIGQUIT and SIGINT
            f_snap_init->user_signal_caught("SIGINT");
        }

    private:
        // this is owned by a server function so no need for a smart pointer
        snap_init::pointer_t f_snap_init;
    };

                                ~snap_init();

    static void                 create_instance( int argc, char * argv[] );
    static pointer_t            instance();
    [[noreturn]] void           exit(int code) const;

    void                        run_processes();
    void                        process_message(snap::snap_communicator_message const & message, bool udp);
    void                        service_died();
    void                        terminate_services();
    void                        remove_service(service::pointer_t s);
    void                        user_signal_caught(char const * sig_name);
    QString const &             get_spool_path() const;
    QString const &             get_server_name() const;
    bool                        get_debug() const;
    service::pointer_t          get_snapcommunicator_service() const;
    void                        send_message(snap::snap_communicator_message const & message);

    void                        get_prereqs_list( QString const & service_name, service::weak_vector_t & ret_list ) const;
    service::pointer_t          get_service( QString const & service_name ) const;

private:
    typedef std::function<void(snap::snap_communicator_message const &)>    message_func_t;
    typedef std::map<QString, message_func_t>                               message_func_map_t;

    enum class snapinit_state_t
    {
        SNAPINIT_STATE_READY,
        SNAPINIT_STATE_STOPPING
    };

                                snap_init( int argc, char * argv[] );
                                snap_init( snap_init const & ) = delete;
    snap_init &                 operator = ( snap_init const & ) = delete;

    void                        usage();
    void                        init();
    void                        init_message_functions();
    static void                 sighandler( int sig );
    bool                        is_running() const;
    void                        xml_to_service(QDomDocument doc, QString const & xml_services_filename, std::vector<QString> & common_options);
    void                        log_selected_servers() const;
    void                        start();
    void                        restart();
    void                        stop();
    void                        create_service_tree();
    void                        get_addr_port_for_snap_communicator( QString & udp_addr, int & udp_port ); // for UDP on "stop"
    void                        remove_lock(bool force = false) const;

    // some snapinit internal values
    //
    static pointer_t                    f_instance;
    message_func_map_t                  f_udp_message_map;
    message_func_map_t                  f_tcp_message_map;

    // snapinit current state
    snapinit_state_t                    f_snapinit_state = snapinit_state_t::SNAPINIT_STATE_READY;

    // command line and .conf configuration
    //
    advgetopt::getopt                   f_opt;
    snap::snap_config                   f_config;
    QString                             f_log_conf;
    command_t                           f_command = command_t::COMMAND_UNKNOWN;
    bool                                f_debug = false;
    QString                             f_server_name;
    QString                             f_lock_filename;
    QFile                               f_lock_file;
    QString                             f_spool_path;
    mutable bool                        f_spool_directory_created = false;
    service::vector_t                   f_service_list;
    int                                 f_stop_max_wait = 60;
    service::pointer_t                  f_snapinit_service;
    service::pointer_t                  f_snapcommunicator_service;

    // snap communicator
    snap::snap_communicator::pointer_t  f_communicator;
    listener_impl::pointer_t            f_listener_connection;
    ping_impl::pointer_t                f_ping_server;
    sigchld_impl::pointer_t             f_child_signal;
    sigterm_impl::pointer_t             f_term_signal;
    sigquit_impl::pointer_t             f_quit_signal;
    sigint_impl::pointer_t              f_int_signal;
    QString                             f_udp_addr;
    int                                 f_udp_port = 4039;
};


} // namespace snapinit
// vim: ts=4 sw=4 et
