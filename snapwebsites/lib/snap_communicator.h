// Snap Communicator -- classes to ease handling communication between processes
// Copyright (C) 2012-2016  Made to Order Software Corp.
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

#include <signal.h>
#include <sys/signalfd.h>


namespace snap
{

class snap_communicator_parameter_error : public snap_logic_exception
{
public:
    snap_communicator_parameter_error(std::string const & what_msg) : snap_logic_exception(what_msg) {}
};

class snap_communicator_exception : public snap_exception
{
public:
    snap_communicator_exception(std::string const & what_msg) : snap_exception("snap_communicator", what_msg) {}
};

class snap_communicator_initialization_error : public snap_communicator_exception
{
public:
    snap_communicator_initialization_error(std::string const & what_msg) : snap_communicator_exception(what_msg) {}
};

class snap_communicator_runtime_error : public snap_communicator_exception
{
public:
    snap_communicator_runtime_error(std::string const & what_msg) : snap_communicator_exception(what_msg) {}
};

class snap_communicator_invalid_message : public snap_communicator_exception
{
public:
    snap_communicator_invalid_message(std::string const & what_msg) : snap_communicator_exception(what_msg) {}
};



class snap_communicator_message
{
public:
    typedef QMap<QString, QString>                  parameters_t;
    typedef std::vector<snap_communicator_message>  vector_t;

    bool                    from_message(QString const & message);
    QString                 to_message() const;

    QString const &         get_sent_from_server() const;
    void                    set_sent_from_server(QString const & server);
    QString const &         get_sent_from_service() const;
    void                    set_sent_from_service(QString const & service);
    QString const &         get_server() const;
    void                    set_server(QString const & server);
    QString const &         get_service() const;
    void                    set_service(QString const & service);
    void                    reply_to(snap_communicator_message const & message);
    QString const &         get_command() const;
    void                    set_command(QString const & command);
    void                    add_parameter(QString const & name, QString const & value);
    void                    add_parameter(QString const & name, int64_t value);
    bool                    has_parameter(QString const & name) const;
    QString const           get_parameter(QString const & name) const;
    int64_t                 get_integer_parameter(QString const & name) const;
    parameters_t const &    get_all_parameters() const;

private:
    void                    verify_name(QString const & name, bool can_be_empty = false, bool can_be_lowercase = true) const;

    QString                 f_sent_from_server;
    QString                 f_sent_from_service;
    QString                 f_server;
    QString                 f_service;
    QString                 f_command;
    parameters_t            f_parameters;
    mutable QString         f_cached_message;
};



// forward class declaration
class snap_tcp_client_permanent_message_connection_impl;



// WARNING: a snap_communicator object must be allocated and held in a shared pointer (see pointer_t)
class snap_communicator
        : public std::enable_shared_from_this<snap_communicator>
{
public:
    typedef std::shared_ptr<snap_communicator>      pointer_t;

    // this version defines the protocol version, it should really rarely
    // change if ever
    static int const                                VERSION = 1;

    typedef int                                     priority_t;

    static priority_t const                         EVENT_MAX_PRIORITY = 255;

    class snap_connection
        : public std::enable_shared_from_this<snap_connection>
    {
    public:
        typedef std::shared_ptr<snap_connection>    pointer_t;
        typedef std::vector<pointer_t>              vector_t;

                                    snap_connection();

        // prevent copies
                                    snap_connection(snap_connection const & connection) = delete;
        snap_connection &           operator = (snap_connection const & connection) = delete;

        // virtual classes must have a virtual destructor
        virtual                     ~snap_connection();

        void                        remove_from_communicator();

        QString const &             get_name() const;
        void                        set_name(QString const & name);

        virtual bool                is_listener() const;
        virtual bool                is_signal() const;
        virtual bool                is_reader() const;
        virtual bool                is_writer() const;
        virtual int                 get_socket() const = 0;
        virtual bool                valid_socket() const;

        bool                        is_enabled() const;
        void                        set_enable(bool enabled);

        int                         get_priority() const;
        void                        set_priority(priority_t priority);
        static bool                 compare(pointer_t const & lhs, pointer_t const & rhs);

        int64_t                     get_timeout_delay() const;
        void                        set_timeout_delay(int64_t timeout_us);
        void                        calculate_next_tick();
        int64_t                     get_timeout_date() const;
        void                        set_timeout_date(int64_t date_us);
        int64_t                     get_timeout_timestamp() const;

        void                        non_blocking() const;
        void                        keep_alive() const;

        // callbacks
        virtual void                process_timeout();
        virtual void                process_signal();
        virtual void                process_read();
        virtual void                process_write();
        virtual void                process_accept();
        virtual void                process_error();
        virtual void                process_hup();
        virtual void                process_invalid();
        virtual void                connection_added();
        virtual void                connection_removed();

    private:
        friend snap_communicator;

        int64_t                     save_timeout_timestamp();
        int64_t                     get_saved_timeout_timestamp() const;

        QString                     f_name;
        bool                        f_enabled = true;
        bool                        f_done = false;
        priority_t                  f_priority = 100;
        int64_t                     f_timeout_delay = -1;       // in microseconds
        int64_t                     f_timeout_next_date = -1;   // in microseconds, when we use the f_timeout_delay
        int64_t                     f_timeout_date = -1;        // in microseconds
        int64_t                     f_saved_timeout_stamp = -1; // in microseconds
        int                         f_fds_position = -1;
    };

    class snap_timer
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_timer>    pointer_t;

                                    snap_timer(int64_t timeout_us);

        virtual int                 get_socket() const;
        virtual bool                valid_socket() const;
    };

    class snap_signal
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_signal>    pointer_t;

                                    snap_signal(int posix_signal);
                                    ~snap_signal();

        // snap_connection implementation
        virtual bool                is_signal() const;
        virtual int                 get_socket() const;

        pid_t                       get_child_pid() const;

    private:
        friend snap_communicator;

        void                        process();

        int                         f_signal = 0;   // i.e. SIGHUP, SIGTERM...
        int                         f_socket = -1;  // output of signalfd()
        struct signalfd_siginfo     f_signal_info = signalfd_siginfo();
    };

    class snap_thread_done_signal
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_thread_done_signal>    pointer_t;

                                    snap_thread_done_signal();
                                    ~snap_thread_done_signal();

        // snap_connection implementation
        virtual bool                is_reader() const;
        virtual int                 get_socket() const;
        virtual void                process_read();

        void                        thread_done();

    private:
        int                         f_pipe[2];      // pipes
    };

    class snap_pipe_connection
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_pipe_connection>    pointer_t;

                                    snap_pipe_connection();
                                    ~snap_pipe_connection();

        virtual ssize_t             read(void * buf, size_t count);
        virtual ssize_t             write(void const * buf, size_t count);
        void                        close();

        // snap_connection implementation
        virtual bool                is_reader() const;
        virtual int                 get_socket() const;

    private:
        pid_t                       f_parent;       // the process that created these pipes (read/write to 0 if getpid() == f_parent, read/write to 1 if getpid() != f_parent)
        int                         f_socket[2];    // socket pair
    };

    class snap_pipe_buffer_connection
        : public snap_pipe_connection
    {
    public:
        typedef std::shared_ptr<snap_pipe_buffer_connection>    pointer_t;

        // snap::snap_communicator::snap_connection
        virtual bool                is_writer() const;

        // snap::snap_communicator::snap_pipe_connection implementation
        virtual ssize_t             write(void const * data, size_t length);
        virtual void                process_read();
        virtual void                process_write();
        virtual void                process_hup();

        // new callback
        virtual void                process_line(QString const & line) = 0;

    private:
        std::string                 f_line; // do NOT use QString because UTF-8 would break often... (since we may only receive part of messages)
        std::vector<char>           f_output;
        size_t                      f_position = 0;
    };

    class snap_pipe_message_connection
        : public snap_pipe_buffer_connection
    {
    public:
        typedef std::shared_ptr<snap_pipe_message_connection>    pointer_t;

        void                        send_message(snap_communicator_message const & message);

        // snap_tcp_server_client_buffer_connection implementation
        virtual void                process_line(QString const & line);

        // new callback
        virtual void                process_message(snap_communicator_message const & message) = 0;

    private:
    };

    class snap_tcp_client_connection
        : public snap_connection
        , public tcp_client_server::bio_client
    {
    public:
        typedef std::shared_ptr<snap_tcp_client_connection>    pointer_t;

                                    snap_tcp_client_connection(std::string const & addr, int port, mode_t mode = mode_t::MODE_PLAIN);

        QString const &             get_remote_address() const;

        virtual ssize_t             read(void * buf, size_t count);
        virtual ssize_t             write(void const * buf, size_t count);

        // snap_connection implementation
        virtual bool                is_reader() const;
        virtual int                 get_socket() const;

    private:
        QString const               f_remote_address;
    };

    // TODO: switch the tcp_server to a bio_server once available
    class snap_tcp_server_connection
        : public snap_connection
        , public tcp_client_server::tcp_server
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_connection>    pointer_t;

                                    snap_tcp_server_connection(std::string const & addr, int port, int max_connections = -1, bool reuse_addr = false, bool auto_close = false);

        // snap_connection implementation
        virtual bool                is_listener() const;
        virtual int                 get_socket() const;
    };

    class snap_tcp_server_client_connection
        : public snap_connection
        //, public tcp_client_server::tcp_client -- this will not work without some serious re-engineering of the tcp_client class
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_client_connection>    pointer_t;

                                    snap_tcp_server_client_connection(int socket);
        virtual                     ~snap_tcp_server_client_connection();

        virtual ssize_t             read(void * buf, size_t count);
        virtual ssize_t             write(void const * buf, size_t count);
        void                        close();
        size_t                      get_client_address(struct sockaddr_storage & address) const;
        std::string                 get_client_addr() const;

        // snap_connection implementation
        virtual bool                is_reader() const;
        virtual int                 get_socket() const;

    private:
        bool                        define_address();

        int                         f_socket = -1;
        struct sockaddr_storage     f_address = sockaddr_storage();
        socklen_t                   f_length = 0;
    };

    class snap_tcp_server_client_buffer_connection
        : public snap_tcp_server_client_connection
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_client_buffer_connection>    pointer_t;

                                    snap_tcp_server_client_buffer_connection(int socket);

        // snap::snap_communicator::snap_connection
        virtual bool                is_writer() const;

        // snap::snap_communicator::snap_tcp_server_client_connection implementation
        virtual ssize_t             write(void const * data, size_t length);
        virtual void                process_read();
        virtual void                process_write();
        virtual void                process_hup();

        // new callback
        virtual void                process_line(QString const & line) = 0;

    private:
        std::string                 f_line; // do NOT use QString because UTF-8 would break often... (since we may only receive part of messages)
        std::vector<char>           f_output;
        size_t                      f_position = 0;
    };

    class snap_tcp_server_client_message_connection
        : public snap_tcp_server_client_buffer_connection
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_client_message_connection>    pointer_t;

                                    snap_tcp_server_client_message_connection(int socket);

        void                        send_message(snap_communicator_message const & message);
        QString const &             get_remote_address() const;

        // snap_tcp_server_client_buffer_connection implementation
        virtual void                process_line(QString const & line);

        // new callback
        virtual void                process_message(snap_communicator_message const & message) = 0;

    private:
        QString                     f_remote_address;
    };

    class snap_tcp_client_buffer_connection
        : public snap_tcp_client_connection
    {
    public:
        typedef std::shared_ptr<snap_tcp_client_buffer_connection>    pointer_t;

                                    snap_tcp_client_buffer_connection(std::string const & addr, int port, mode_t const mode = mode_t::MODE_PLAIN, bool const blocking = false);

        // snap::snap_communicator::snap_tcp_client_connection implementation
        virtual ssize_t             write(void const * data, size_t length);
        virtual bool                is_writer() const;
        virtual void                process_read();
        virtual void                process_write();
        virtual void                process_hup();

        virtual void                process_line(QString const & line) = 0;

    private:
        std::string                 f_line; // do NOT use QString because UTF-8 would break often... (since we may only receive part of messages)
        std::vector<char>           f_output;
        size_t                      f_position = 0;
    };

    class snap_tcp_client_message_connection
        : public snap_tcp_client_buffer_connection
    {
    public:
        typedef std::shared_ptr<snap_tcp_client_message_connection>    pointer_t;

                                    snap_tcp_client_message_connection(std::string const & addr, int port, mode_t const mode = mode_t::MODE_PLAIN, bool const blocking = false);

        void                        send_message(snap_communicator_message const & message);

        // snap_tcp_client_reader_connection implementation
        virtual void                process_line(QString const & line);

        // new callback
        virtual void                process_message(snap_communicator_message const & message) = 0;
    };

    class snap_tcp_client_permanent_message_connection
        : public snap_timer
    {
    public:
        typedef std::shared_ptr<snap_tcp_client_permanent_message_connection>    pointer_t;

        static int64_t const        DEFAULT_PAUSE_BEFORE_RECONNECTING = 60LL * 1000000LL;  // 1 minute

                                    snap_tcp_client_permanent_message_connection(std::string const & address, int port, tcp_client_server::bio_client::mode_t mode = tcp_client_server::bio_client::mode_t::MODE_PLAIN, int64_t const pause = DEFAULT_PAUSE_BEFORE_RECONNECTING, bool const use_thread = true);

        bool                        send_message(snap_communicator_message const & message, bool cache = false);
        void                        mark_done();
        size_t                      get_client_address(struct sockaddr_storage & address) const;
        std::string                 get_client_addr() const;

        // snap_connection implementation
        virtual void                process_timeout();
        virtual void                process_error();
        virtual void                process_hup();
        virtual void                process_invalid();
        virtual void                connection_removed();

        // new callbacks
        virtual void                process_message(snap_communicator_message const & message) = 0;
        virtual void                process_connection_failed(std::string const & error_message);
        virtual void                process_connected();

    private:
        std::shared_ptr<snap_tcp_client_permanent_message_connection_impl>
                                    f_impl;
        int64_t                     f_pause = -1;
        bool const                  f_use_thread = true;
        bool                        f_done = false;
    };

    class snap_udp_server_connection
        : public snap_connection
        , public udp_client_server::udp_server
    {
    public:
        typedef std::shared_ptr<snap_udp_server_connection>    pointer_t;

                                    snap_udp_server_connection(std::string const & addr, int port);

        // snap_connection implementation
        virtual bool                is_reader() const;
        virtual int                 get_socket() const;
    };

    class snap_udp_server_message_connection
        : public snap_udp_server_connection
    {
    public:
        typedef std::shared_ptr<snap_udp_server_message_connection>    pointer_t;

        static size_t const         DATAGRAM_MAX_SIZE = 1024;

                                    snap_udp_server_message_connection(std::string const & addr, int port);

        static bool                 send_message(std::string const & addr, int port, snap_communicator_message const & message);

        // snap_connection implementation
        virtual void                process_read();

        // new callback
        virtual void                process_message(snap_communicator_message const & message) = 0;

    private:
    };

    class snap_tcp_blocking_client_message_connection
        : public snap_tcp_client_message_connection
    {
    public:
                                    snap_tcp_blocking_client_message_connection(std::string const & addr, int port, mode_t mode = mode_t::MODE_PLAIN);

        void                        run();
        void                        done();

        bool                        send_message(snap_communicator_message const & message);

        // snap_connection callback
        virtual void                process_error();

        // new callback
        virtual void                process_message(snap_communicator_message const & message) = 0;

    private:
        bool                        f_done = false;
    };

    static pointer_t                    instance();

    // prevent copies
                                        snap_communicator(snap_communicator const & communicator) = delete;
    snap_communicator &                 operator = (snap_communicator const & communicator) = delete;

    snap_connection::vector_t const &   get_connections() const;
    bool                                add_connection(snap_connection::pointer_t connection);
    bool                                remove_connection(snap_connection::pointer_t connection);
    virtual bool                        run();

    static int64_t                      get_current_date();

private:
                                        snap_communicator();

    snap_connection::vector_t           f_connections;
    bool                                f_force_sort = true;
};



} // namespace snap
// vim: ts=4 sw=4 et
