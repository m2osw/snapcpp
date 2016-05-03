/*
 * Text:
 *      snapdbproxy.h
 *
 * Description:
 *      Proxy database access for two main reasons:
 *
 *      1. keep connections between this computer and the database
 *         computer open (i.e. opening remote TCP connections taken
 *         "much" longer than opening local connections.)
 *
 *      2. remove threads being forced on us by the C/C++ driver from
 *         cassandra (this causes problems with the snapserver that
 *         uses fork() to create the snap_child processes.)
 *
 * License:
 *      Copyright (c) 2016 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// our lib
//
#include "snapwebsites.h"
#include "snap_communicator.h"
#include "snap_thread.h"

// 3rd party libs
//
//#include <QtCore>
#include <QtCassandra/QCassandraQuery.h>
#include <QtCassandra/QCassandraSession.h>
#include <QtCassandra/QCassandraOrder.h>
#include <QtCassandra/QCassandraProxy.h>
//#include <QtCassandra/QCassandraOrderResult.h>
#include <advgetopt/advgetopt.h>

// C++ libs
//

// system
//
#include <poll.h>


class snapdbproxy;


class snapdbproxy_messager
        : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<snapdbproxy_messager>    pointer_t;

                                snapdbproxy_messager(snapdbproxy * proxy, std::string const & addr, int port);

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message);
    virtual void                process_connection_failed(std::string const & error_message);
    virtual void                process_connected();

private:
    // this is owned by a snapdbproxy function so no need for a smart pointer
    // (and it would create a loop)
    snapdbproxy *               f_snapdbproxy;
};


class snapdbproxy_listener
        : public snap::snap_communicator::snap_tcp_server_connection
{
public:
    typedef std::shared_ptr<snapdbproxy_listener>    pointer_t;

                                snapdbproxy_listener(snapdbproxy * proxy, std::string const & addr, int port, int max_connections, bool reuse_addr, bool auto_close);

    // snap_communicator::snap_tcp_server_connection implementation
    virtual void                process_accept();

private:
    // this is owned by a snapdbproxy function so no need for a smart pointer
    // (and it would create a loop)
    snapdbproxy *            	f_snapdbproxy;
};



class snapdbproxy_connection
        : public snap::snap_thread::snap_runner
        , public QtCassandra::QCassandraProxyIO
{
public:
                                snapdbproxy_connection(QtCassandra::QCassandraSession::pointer_t session, int s);
    virtual                     ~snapdbproxy_connection();

    // implement snap_runner
    virtual void                run();

    // implement QCassandraProxyIO
    virtual ssize_t             read(void * buf, size_t count);
    virtual ssize_t             write(void const * buf, size_t count);

    void                        kill();

private:
    struct cursor_t
    {
        QtCassandra::QCassandraQuery::pointer_t f_query;
        int                                     f_column_count;
    };

    void                        send_order(QtCassandra::QCassandraQuery * q, QtCassandra::QCassandraOrder const & order);
    void                        declare_cursor(QtCassandra::QCassandraOrder const & order);
    void                        describe_cluster(QtCassandra::QCassandraOrder const & order);
    void                        fetch_cursor(QtCassandra::QCassandraOrder const & order);
    void                        close_cursor(QtCassandra::QCassandraOrder const & order);
    void                        read_data(QtCassandra::QCassandraOrder const & order);
    void                        execute_command(QtCassandra::QCassandraOrder const & order);

    QtCassandra::QCassandraProxy                f_proxy;
    QtCassandra::QCassandraSession::pointer_t   f_session;
    std::vector<cursor_t>                       f_cursors;
    int                                         f_socket;
    int                                         f_signal;
    struct pollfd                               f_poll_fds[2];
};


class snapdbproxy_thread
{
public:
    typedef std::shared_ptr<snapdbproxy_thread> pointer_t;

                            snapdbproxy_thread(QtCassandra::QCassandraSession::pointer_t session, int const s);
                            ~snapdbproxy_thread();

    bool                    is_running() const;

private:
    snapdbproxy_connection  f_connection;
    snap::snap_thread       f_thread;
    int                     f_socket;
};




class snapdbproxy
{
public:
    typedef std::shared_ptr<snapdbproxy>      pointer_t;

                                snapdbproxy( int argc, char * argv[] );
                                ~snapdbproxy();

    static pointer_t            instance( int argc, char * argv[] );

    void                        run();
    void                        process_message(snap::snap_communicator_message const & message);
    void                        process_connection(int const s);

    static void                 sighandler( int sig );

private:
                                snapdbproxy( snapdbproxy const & ) = delete;
    snapdbproxy &               operator = ( snapdbproxy const & ) = delete;

    void                        usage(advgetopt::getopt::status_t status);
    void                        setup_dbproxy();
    void                        next_wakeup();
    void                        stop(bool quitting);

    static pointer_t                            g_instance;

    advgetopt::getopt                           f_opt;
    snap::snap_config                           f_config;
    QString                                     f_log_conf = "/etc/snapwebsites/snapdbproxy.properties";
    QString                                     f_server_name;
    QString                                     f_communicator_addr;
    int                                         f_communicator_port;
    QString                                     f_snapdbproxy_addr;
    int                                         f_snapdbproxy_port;
    snap::snap_communicator::pointer_t          f_communicator;
    QString                                     f_host_list = "localhost";
    int                                         f_port = 9042;
    snapdbproxy_messager::pointer_t             f_messager;
    snapdbproxy_listener::pointer_t             f_listener;
    long                                        f_max_pending_connections = -1;
    bool                                        f_stop_received = false;
    bool                                        f_debug = false;
    QtCassandra::QCassandraSession::pointer_t   f_session;

    std::vector<snapdbproxy_thread::pointer_t>  f_connections;
};



// vim: ts=4 sw=4 et
