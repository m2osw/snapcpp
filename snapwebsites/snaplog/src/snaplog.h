/*
 * Text:
 *      snaplog.h
 *
 * Description:
 *      Logger for the Snap! system. This service uses snapcommunicator to listen
 *      to all "SNAPLOG" messages. It records each message into a MySQL database for
 *      later retrieval, making reporting a lot easier for the admin.
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

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snap_thread.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// QtCassandra lib
//
#include <QtCassandra/QCassandraQuery.h>
#include <QtCassandra/QCassandraSession.h>
#include <QtCassandra/QCassandraOrder.h>
#include <QtCassandra/QCassandraProxy.h>

// C++
//
#include <atomic>


class snaplog;


/** \brief Provide a tick in can we cannot immediately connect to Cassandra.
 *
 * The snaplog tries to connect to Cassandra on startup. It is part
 * of its initialization procedure.
 *
 * If that fails, it needs to try again later. This timer is used for
 * that purpose.
 */
class snaplog_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<snaplog_timer>    pointer_t;

    /** \brief The timer initialization.
     *
     * The timer ticks once per second to retrieve the current load of the
     * system and forward it to whichever computer that requested the
     * information.
     *
     * \param[in] cs  The snap communicator server we are listening for.
     *
     * \sa process_timeout()
     */
    snaplog_timer(snaplog * proxy)
        : snap_timer(0)  // run immediately
        , f_snaplog(proxy)
    {
    }

    // snap::snap_communicator::snap_timer implementation
    virtual void process_timeout() override;

private:
    // this is owned by a server function so no need for a smart pointer
    snaplog *               f_snaplog = nullptr;
};



class snaplog_messenger
        : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<snaplog_messenger>    pointer_t;

                                snaplog_messenger(snaplog * proxy, std::string const & addr, int port);

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message);
    virtual void                process_connection_failed(std::string const & error_message);
    virtual void                process_connected();

private:
    // this is owned by a snaplog function so no need for a smart pointer
    // (and it would create a loop)
    snaplog *               f_snaplog = nullptr;
};


class snaplog_listener
        : public snap::snap_communicator::snap_tcp_server_connection
{
public:
    typedef std::shared_ptr<snaplog_listener>    pointer_t;

                                snaplog_listener(snaplog * proxy, std::string const & addr, int port, int max_connections, bool reuse_addr);

    // snap_communicator::snap_tcp_server_connection implementation
    virtual void                process_accept();

private:
    // this is owned by a snaplog function so no need for a smart pointer
    // (and it would create a loop)
    snaplog *            	f_snaplog = nullptr;
};



class snaplog_connection
        : public snap::snap_thread::snap_runner
        , public QtCassandra::QCassandraProxyIO
{
public:
                                snaplog_connection(QtCassandra::QCassandraSession::pointer_t session, tcp_client_server::bio_client::pointer_t s, QString const & cassandra_host_list, int cassandra_port);
    virtual                     ~snaplog_connection();

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
        int                                     f_column_count = 0;
    };

    void                        send_order(QtCassandra::QCassandraQuery::pointer_t q, QtCassandra::QCassandraOrder const & order);
    void                        declare_cursor(QtCassandra::QCassandraOrder const & order);
    void                        describe_cluster(QtCassandra::QCassandraOrder const & order);
    void                        clear_cluster_description();
    void                        fetch_cursor(QtCassandra::QCassandraOrder const & order);
    void                        close_cursor(QtCassandra::QCassandraOrder const & order);
    void                        read_data(QtCassandra::QCassandraOrder const & order);
    void                        execute_command(QtCassandra::QCassandraOrder const & order);

    QtCassandra::QCassandraProxy                f_proxy;
    QtCassandra::QCassandraSession::pointer_t   f_session;
    std::vector<cursor_t>                       f_cursors;
    tcp_client_server::bio_client::pointer_t    f_client;
    std::atomic<int>                            f_socket /* = -1*/;
    QString                                     f_cassandra_host_list = "localhost";
    int                                         f_cassandra_port = 9042;
};


class snaplog_thread
{
public:
    typedef std::shared_ptr<snaplog_thread> pointer_t;

                            snaplog_thread(QtCassandra::QCassandraSession::pointer_t session, tcp_client_server::bio_client::pointer_t client, QString const & cassandra_host_list, int cassandra_port);
                            ~snaplog_thread();

    bool                    is_running() const;

private:
    tcp_client_server::bio_client::pointer_t
                        f_client;
    snaplog_connection  f_connection;
    snap::snap_thread   f_thread;
};




class snaplog
{
public:
    typedef std::shared_ptr<snaplog>      pointer_t;

                                snaplog( int argc, char * argv[] );
                                ~snaplog();

    std::string                 server_name() const;

    void                        run();
    void                        process_message(snap::snap_communicator_message const & message);
    void                        process_connection(tcp_client_server::bio_client::pointer_t client);
    void                        process_timeout();

    static void                 sighandler( int sig );

private:
                                snaplog( snaplog const & ) = delete;
    snaplog &                   operator = ( snaplog const & ) = delete;

    bool                        use_ssl() const;
    void                        usage(advgetopt::getopt::status_t status);
    void                        setup_dbproxy();
    void                        next_wakeup();
    void                        stop(bool quitting);
    void                        no_cassandra();
    void                        cassandra_ready();

    static pointer_t                          g_instance;

    advgetopt::getopt                         f_opt;
    snap::snap_config                         f_config;
    QString                                   f_log_conf                      = "/etc/snapwebsites/logger/snaplog.properties";
    std::string                               f_server_name;
    QString                                   f_communicator_addr             = "127.0.0.1";
    int                                       f_communicator_port             = 4040;
    QString                                   f_snaplog_addr                  = "127.0.0.1";
    int                                       f_snaplog_port                  = 4042;
    snap::snap_communicator::pointer_t        f_communicator;
    QString                                   f_cassandra_host_list           = "localhost";
    int                                       f_cassandra_port                = 9042;
    snaplog_messenger::pointer_t              f_messenger;
    snaplog_listener::pointer_t               f_listener;
    snaplog_timer::pointer_t                  f_timer;
    int                                       f_max_pending_connections       = -1;
    bool                                      f_ready                         = false;
    bool                                      f_force_restart                 = false;
    bool                                      f_stop_received                 = false;
    bool                                      f_debug                         = false;
    bool                                      f_no_cassandra_sent             = false;
    float                                     f_cassandra_connect_timer_index = 1.25f;
    QtCassandra::QCassandraSession::pointer_t f_session;

    std::vector<snaplog_thread::pointer_t>    f_connections;
};



// vim: ts=4 sw=4 et
