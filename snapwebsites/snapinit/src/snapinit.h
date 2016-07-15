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

#include "snapwebsites.h"
#include "snap_communicator.h"
#include "service.h"

#include <advgetopt/advgetopt.h>

#include <QDomDocument>
#include <QFile>
#include <QString>

#include <functional>
#include <memory>
#include <string>

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
    bool                        is_running() const;
    QString const &             get_spool_path() const;
    QString const &             get_server_name() const;
    service::pointer_t          get_connection_service() const;
    service::pointer_t          get_snapdbproxy_service() const;

    void                        get_prereqs_list( const QString& service_name, service::vector_t& ret_list ) const;
    service::pointer_t          get_service( const QString& service_name ) const;

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
    void                        init_message_functions();
    void                        register_died_service( service::pointer_t svc );

    typedef std::function<void(snap::snap_communicator_message const&)> message_func_t;
    typedef std::map<QString,message_func_t>   message_func_map_t;

    message_func_map_t                  f_udp_message_map;
    message_func_map_t                  f_tcp_message_map;
    static pointer_t                    f_instance;
    advgetopt::getopt                   f_opt;
    bool                                f_debug = false;
    snap::snap_config                   f_config;
    QString                             f_log_conf;
    command_t                           f_command = command_t::COMMAND_UNKNOWN;
    QString                             f_server_name;
    QString                             f_lock_filename;
    QFile                               f_lock_file;
    QString                             f_spool_path;
    mutable bool                        f_spool_directory_created = false;
    service::vector_t                   f_service_list;
    service::pointer_t                  f_connection_service;
    service::pointer_t                  f_snapdbproxy_service;
    snap::snap_communicator::pointer_t  f_communicator;
    listener_impl::pointer_t            f_listener_connection;
    ping_impl::pointer_t                f_ping_server;
    sigchld_impl::pointer_t             f_child_signal;
    sigterm_impl::pointer_t             f_term_signal;
    sigquit_impl::pointer_t             f_quit_signal;
    sigint_impl::pointer_t              f_int_signal;
    QString                             f_server_type;
    QString                             f_udp_addr;
    int                                 f_udp_port = 4039;
    int                                 f_stop_max_wait = 60;
    QString                             f_expected_safe_message;
};

// vim: ts=4 sw=4 et
