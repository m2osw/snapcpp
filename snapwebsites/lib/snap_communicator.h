// Snap Communicator -- classes to ease handling communication between processes
// Copyright (C) 2012-2015  Made to Order Software Corp.
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
#pragma once

#include "snap_exception.h"
#include "tcp_client_server.h"
#include "udp_client_server.h"

#include <QMap>


namespace snap
{

class snap_communicator_parameter_error : public snap_logic_exception
{
public:
    snap_communicator_parameter_error(std::string const & whatmsg) : snap_logic_exception(whatmsg) {}
};

class snap_communicator_exception : public snap_exception
{
public:
    snap_communicator_exception(std::string const & what_msg) : snap_exception("snap_communicator", what_msg) {}
};

class snap_communicator_initialization_error : public snap_communicator_exception
{
public:
    snap_communicator_initialization_error(std::string const & whatmsg) : snap_communicator_exception(whatmsg) {}
};

class snap_communicator_runtime_error : public snap_communicator_exception
{
public:
    snap_communicator_runtime_error(std::string const & whatmsg) : snap_communicator_exception(whatmsg) {}
};

class snap_communicator_invalid_message : public snap_communicator_exception
{
public:
    snap_communicator_invalid_message(std::string const & whatmsg) : snap_communicator_exception(whatmsg) {}
};



class snap_communicator_message
{
public:
    typedef QMap<QString, QString>  parameters_t;

    bool                    from_message(QString const & message);
    QString                 to_message() const;

    QString                 get_name() const;
    void                    set_name(QString const & name);
    QString                 get_command() const;
    void                    set_command(QString const & command);
    void                    add_parameter(QString const & name, QString const & value);
    bool                    has_parameter(QString const & name) const;
    QString                 get_parameter(QString const & name);
    int64_t                 get_integer_parameter(QString const & name);
    parameters_t const &    get_all_parameters() const;

private:
    void                    verify_parameter_name(QString const & name) const;

    QString                 f_name;
    QString                 f_command;
    parameters_t            f_parameters;
};




class snap_communicator
{
public:
    typedef std::shared_ptr<snap_communicator>      pointer_t;

    typedef int                                     what_event_t;

    static what_event_t const EVENT_TIMEOUT         = 0x01; // receive only
    static what_event_t const EVENT_READ            = 0x02;
    static what_event_t const EVENT_WRITE           = 0x04;
    static what_event_t const EVENT_SIGNAL          = 0x08;
    //static what_event_t const EVENT_PERSIST         = 0x10; -- internal
    //static what_event_t const EVENT_EDGE_TRIGGERED  = 0x20; -- not supported
    static what_event_t const EVENT_ACCEPT          = 0x40;

    class priority_t
    {
    public:
        static int              get_maximum_number_of_priorities();
        int                     get_priorities() const;
        void                    set_priorities(int n_priorities);
        int64_t                 get_timeout() const;
        void                    set_timeout(int64_t timeout_us);
        int64_t                 get_max_callbacks() const;
        void                    set_max_callbacks(int max_callbacks);
        int64_t                 get_min_priority() const;
        void                    set_min_priority(int min_priority);

        void                    validate() const;

    private:
        int                     f_priorities = 1;
        int64_t                 f_timeout = -1;
        int                     f_max_callbacks = -1;
        int                     f_min_priority = 0;
    };

    class snap_connection
    {
    public:
        typedef std::shared_ptr<snap_connection>    pointer_t;
        typedef std::vector<pointer_t>              vector_t;

                                    snap_connection();

        // virtual classes must have a virtual destructor
        virtual                     ~snap_connection();

        std::string const &         get_name() const;
        void                        set_name(std::string const & name);

        virtual bool                is_listener() const;
        virtual int                 get_socket() const = 0;
        virtual what_event_t        get_events() const = 0;
        virtual void                process_signal(what_event_t we, pointer_t new_client) = 0;
        virtual pointer_t           create_new_connection(int socket);
        virtual bool                valid_socket() const;

        int                         get_priority() const;
        void                        set_priority(int priority);

        int64_t                     get_timeout() const;
        void                        set_timeout(int64_t timeout_us);

        void                        non_blocking();

        void                        save_new_connection_info(int s, struct sockaddr * addr, int len);

    private:
        struct snap_connection_impl;

        friend snap_communicator;

        std::string                 f_name;
        int                         f_priority = 0;
        int64_t                     f_timeout = -1; // in microseconds
        std::unique_ptr<snap_connection_impl>   f_impl;
        int                         f_new_connection_socket = -1;
        struct sockaddr             f_new_connection_addr;
        int                         f_new_connection_len = 0;
    };

    class snap_timer
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_timer>    pointer_t;

                                    snap_timer(int64_t timeout_us);

        virtual int                 get_socket() const;
        virtual what_event_t        get_events() const;
        virtual bool                valid_socket() const;
    };

    class snap_signal
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_signal>    pointer_t;

                                    snap_signal(int posix_signal);

        // snap_connection implementation
        virtual int                 get_socket() const;
        virtual what_event_t        get_events() const;

    private:
        int                         f_signal; // i.e. SIGHUP, SIGTERM...
    };

    class snap_client_connection
        : public tcp_client_server::bio_client
        , public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_client_connection>    pointer_t;

                                    snap_client_connection(std::string const & addr, int port, mode_t mode = mode_t::MODE_PLAIN);

        // snap_connection implementation
        virtual int                 get_socket() const;
        virtual what_event_t        get_events() const;
    };

    // TODO: switch the tcp_server to a bio_server once available
    class snap_tcp_server_connection
        : public tcp_client_server::tcp_server
        , public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_connection>    pointer_t;

                                    snap_tcp_server_connection(std::string const & addr, int port, int max_connections = -1, bool reuse_addr = false, bool auto_close = false);

        // snap_connection implementation
        virtual bool                is_listener() const;
        virtual int                 get_socket() const;
        virtual what_event_t        get_events() const;
    };

    class snap_tcp_server_client_connection
        //: public tcp_client_server::tcp_client -- this will not work without some serious re-engineering of the tcp_client class
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_client_connection>    pointer_t;

                                    snap_tcp_server_client_connection(int socket);
        virtual                     ~snap_tcp_server_client_connection();

        // snap_connection implementation
        virtual int                 get_socket() const;
        virtual what_event_t        get_events() const;

        void                        set_address(struct sockaddr * address, size_t length);
        size_t                      get_address(struct sockaddr & address) const;
        std::string                 get_addr() const;

        void                        keep_alive() const;

    private:
        int                         f_socket;
        struct sockaddr             f_address;
        size_t                      f_length;
    };

    class snap_udp_server_connection
        : public udp_client_server::udp_server
        , public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_udp_server_connection>    pointer_t;

                                    snap_udp_server_connection(std::string const & addr, int port);

        // snap_connection implementation
        virtual int                 get_socket() const;
        virtual what_event_t        get_events() const;
    };

                                        snap_communicator(priority_t const & priority);

    void                                reinit(); // after a fork()

    snap_connection::vector_t const &   get_connections() const;
    bool                                add_connection(snap_connection::pointer_t connection);
    bool                                remove_connection(snap_connection::pointer_t connection);
    virtual bool                        run();

private:
    struct snap_communicator_impl;

    std::shared_ptr<snap_communicator_impl> f_impl;
    snap_connection::vector_t               f_connections;
    priority_t                              f_priority;
};



} // namespace snap
// vim: ts=4 sw=4 et
