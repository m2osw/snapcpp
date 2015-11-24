// Snap Websites Server -- server to handle inter-process communication
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

#include "snap_communicator.h"

#include "log.h"
#include "mkdir_p.h"
#include "not_used.h"
#include "snapwebsites.h"

#include <QFile>

#include <sys/resource.h>

#include <atomic>

/** \file
 * \brief Implementation of the snap inter-process communication.
 *
 * This file is the binary we start to allow inter-process communication
 * between front and back end processes on all computers within a Snap
 * cluster.
 *
 * The idea is to have ONE inter-process communicator server running
 * PER computer. These then communicate between each others and are
 * used to send messages between each process that registered with
 * them.
 *
 * This means if you want to send a signal (i.e. PING) to the "images"
 * backend, you connect with this inter-process communicator on your
 * computer, and send the PING command to that process. The communicator
 * then takes care of finding the "images" backend on any one of your
 * Snap servers, and send the PING there.
 *
 * The following shows a simple setup with two computers. Each have a
 * Snap Communicator server running. Both of these servers are connected
 * to each others. When the Snap! Server spawns a child process (because
 * a client connected) and that child process wants to send a PING to the
 * Image Backend it sends it using a UDP signal to the Snap Communicator
 * on Computer 2. That then gets transmitted to Computer 1 Snap Communitor
 * and finally to the Image Backend.
 *
 * \code
 * +------------------------+     +-----------------------------------------+
 * |  Computer 1            |     |  Computer 2                             |
 * |                        |     |                                         |
 * |  +----------------+  Connect |  +----------------+                     |
 * |  |     Snap       |<----------->|     Snap       |<-------+            |
 * |  |  Communicator  |  (TCP/IP)|  |  Communicator  |        | images     |
 * |  +----------------+    |     |  +----------------+        |  PING      |
 * |      ^                 |     |      ^                     |  (UDP)     |
 * |      | Connect         |     |      | Connect      +----------------+  |
 * |      | (TCP/IP)        |     |      | (TCP/IP)     |   Snap Child   |  |
 * |      |                 |     |      |              |    Process     |  |
 * |      |                 |     |      |              +----------------+  |
 * |  +----------------+    |     |  +----------------+        ^            |
 * |  |     Images     |    |     |  |     Snap!      |        |            |
 * |  |    Backend     |    |     |  |    Server      |--------+            |
 * |  +----------------+    |     |  +----------------+  fork()             |
 * |                        |     |                                         |
 * +------------------------+     +-----------------------------------------+
 * \endcode
 *
 * The connection between Snap Communicator servers may happen in any
 * direction. In general, it will happen from the last communicator started
 * to the first running (since the first will fail to connect to the last
 * since the last is still not listening.) That connection makes use of
 * TCP/IP and has a protocol similar to the communication between various
 * parts and the communicator. That is, it sends commands written on one
 * line. The commands may be followed by parameters separated by spaces.
 *
 * Replies are also commands. For example, the HELP command is a way to
 * request a system to send us the COMMANDS and SIGNALS commands to tell
 * us about its capabilities.
 *
 * See also:
 * http://snapwebsites.org/implementation/feature-requirements/inter-process-signalling-core
 */



namespace
{


typedef QMap<QString, bool>                 sorted_list_of_strings_t;


sorted_list_of_strings_t canonicalize_services(QString const & services)
{
    snap::snap_string_list list(services.split(',', QString::SkipEmptyParts));

    // use a map to remove duplicates
    //
    sorted_list_of_strings_t result;

    int const max_services(list.size());
    for(int idx(0); idx < max_services; ++idx)
    {
        QString const service(list[idx].trimmed());
        if(service.isEmpty())
        {
            // this can happen because of the trimmed() call
            continue;
        }

        // TBD: add a check on the name? (i.e. "[A-Za-z_][A-Za-z0-9_]*")
        //

        result[service] = true;
    }

    return result;
}


QString canonicalize_server_types(QString const & server_types)
{
    snap::snap_string_list list(server_types.split(',', QString::SkipEmptyParts));

    // use a map to remove duplicates
    //
    QMap<QString, bool> result;

    int const max_types(list.size());
    for(int idx(0); idx < max_types; ++idx)
    {
        QString const type(list[idx].trimmed());
        if(type.isEmpty())
        {
            // this can happen, especially because of the trimmed() call
            //
            continue;
        }
        if(type != "apache"
        && type != "frontend"
        && type != "backend"
        && type != "cassandra")
        {
            // ignore unknown/unwanted types
            // (i.e. we cannot have "client" here since that is reserved
            // for processes that use REGISTER)
            //
            SNAP_LOG_WARNING("received an invalid server type \"")(type)("\", ignoring.");
        }
        else
        {
            result[type] = true;
        }
    }

    return static_cast<snap::snap_string_list const>(result.keys()).join(",");
}


QString canonicalize_neighbors(QString const & neighbors)
{
    snap::snap_string_list list(neighbors.split(','));

    int const max_addr_port(list.size());
    for(int idx(0); idx < max_addr_port; ++idx)
    {
        QString const neighbor(list[idx].trimmed());
        if(neighbor.isEmpty())
        {
            // this can happen, especially because of the trimmed() call
            //
            continue;
        }
        // TODO: move canonicalization to tcp_client_server so other software
        //       can make use of it
        //
        QString address;
        int port;
        tcp_client_server::get_addr_port(neighbor, address, port, 4040);

        if(tcp_client_server::is_ipv4(address.toUtf8().data()))
        {
            // TODO: the inet_pton() does not support all possible IPv4
            //       notations that is_ipv4() "accepts".
            //
            struct in_addr addr;
            if(inet_pton(AF_INET, address.toUtf8().data(), &addr) != 1)
            {
                SNAP_LOG_ERROR("invalid neighbor address \"")(list[idx])("\", we could not convert it to a valid IPv4 address.");
                continue;
            }
            char buf[64];
            inet_ntop(AF_INET, &addr, buf, sizeof(buf));
            // removing leading zero, making sure we have the dotted notation
            list[idx] = QString("%1:%2").arg(buf).arg(port);
        }
        else if(tcp_client_server::is_ipv6(address.toUtf8().data()))
        {
            struct in6_addr addr;
            if(inet_pton(AF_INET6, address.toUtf8().data(), &addr) != 1)
            {
                SNAP_LOG_ERROR("invalid neighbor address \"")(list[idx])("\", we could not convert it to a valid IPv6 address.");
                continue;
            }
            char buf[64];
            inet_ntop(AF_INET6, &addr, buf, sizeof(buf));
            // removing leading zero, making sure the '::' is used at the
            // right place, etc.
            list[idx] = QString("[%1]:%2").arg(buf).arg(port);
        }
        else
        {
            SNAP_LOG_ERROR("invalid neighbor address \"")(list[idx])("\", it was not recognized as an IPv4 or an IPv6 address.");
            continue;
        }
    }

    return list.join(",");
}



} // no name namespace


class connection_impl;
typedef std::shared_ptr<connection_impl>                    connection_impl_pointer_t;

class remote_snap_communicator;
typedef std::shared_ptr<remote_snap_communicator>           remote_snap_communicator_pointer_t;
typedef QMap<QString, remote_snap_communicator_pointer_t>   remote_snap_communicator_list_t;


/** \brief Set of connections in the snapcommunicator tool.
 *
 * All the connections and sockets in general will all appear
 * in this class.
 */
class snap_communicator_server
        : public std::enable_shared_from_this<snap_communicator_server>
{
public:
    typedef std::shared_ptr<snap_communicator_server>     pointer_t;

    static size_t const         SNAP_COMMUNICATOR_MAX_CONNECTIONS = 100;

                                snap_communicator_server(snap::server::pointer_t s);
                                snap_communicator_server(snap_communicator_server const & src) = delete;
    snap_communicator_server &  operator = (snap_communicator_server const & rhs) = delete;

    void                        init();
    void                        run();

    // one place where all messages get processed
    void                        process_message(snap::snap_communicator::snap_connection::pointer_t connection, snap::snap_communicator_message const & message, bool udp);

    void                        send_status(snap::snap_communicator::snap_connection::pointer_t connection);
    QString                     get_server_types() const;
    QString                     get_local_services() const;
    QString                     get_services_heard_of() const;
    void                        add_neighbors(QString const & new_neighbors);
    void                        connection_lost(QString const & addr);

private:
    void                        refresh_heard_of();
    void                        shutdown(bool full);

    snap::server::pointer_t                             f_server;

    snap::snap_communicator::pointer_t                  f_communicator;
    snap::snap_communicator::snap_connection::pointer_t f_listener;     // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_messager;     // UDP/IP
    QString                                             f_server_types;
    QString                                             f_local_services;
    sorted_list_of_strings_t                            f_local_services_list;
    QString                                             f_services_heard_of;
    sorted_list_of_strings_t                            f_services_heard_of_list;
    QString                                             f_explicit_neighbors;
    sorted_list_of_strings_t                            f_all_neighbors;
    remote_snap_communicator_list_t                     f_remote_snapcommunicators;
    size_t                                              f_max_connections = SNAP_COMMUNICATOR_MAX_CONNECTIONS;
    bool                                                f_shutdown = false;
    snap::snap_communicator_message::vector_t           f_local_message_cache;
};





/** \brief Describe a remove snapcommunicator by IP address, etc.
 *
 * This class defines a snapcommunicator server. Mainly we include
 * the IP address of the server to connect to.
 *
 * The object also maintains the status of that server. Whether we
 * can connect to it (because if not the connection stays in limbo
 * and we should not try again and again forever. Instead we can
 * just go to sleep and try again "much" later saving many CPU
 * cycles.)
 *
 * It also gives us a way to quickly track snapcommunicator objects
 * that REFUSE our connection.
 */
class remote_snap_communicator
    : public snap::snap_communicator::snap_timer
{
public:
    class thread_done_impl
        : public snap::snap_communicator::snap_thread_done_signal
    {
    public:
        typedef std::shared_ptr<thread_done_impl>   pointer_t;

                            thread_done_impl(remote_snap_communicator * sc)
                                : f_remote_snap_communicator(sc)
                            {
                            }

    virtual void            process_read()
                            {
                                f_remote_snap_communicator->thread_done();
                            }

    private:
        remote_snap_communicator *  f_remote_snap_communicator;
    };

    class remote_connect
        : public snap::snap_thread::snap_runner
    {
    public:
                            remote_connect(thread_done_impl::pointer_t thread_done, QString const & addr, int port)
                                : snap::snap_thread::snap_runner("background remote snapconnector")
                                , f_thread_done(thread_done)
                                , f_address(addr)
                                , f_port(port)
                                , f_socket(-1)
                            {
                            }

        virtual void        run();

        int                 get_socket() const
                            {
                                return f_socket;
                            }

    private:
        thread_done_impl::pointer_t f_thread_done;
        QString                     f_address;
        int                         f_port = -1;
        std::atomic<int>            f_socket;
    };

    /** \brief Setup a remote_snap_communicator object.
     *
     * This initialization function sets up the attached snap_timer
     * to 1 second delay before we try to connect to this remote
     * snapcommunicator. The timer is reused later when the connection
     * is lost, a snapcommunicator returns a REFUSE message to our
     * CONNECT message, and other similar errors.
     */
    remote_snap_communicator(snap_communicator_server::pointer_t cs, QString const & addr, int port)
        : snap_timer(1000000)
        , f_communicator_server(cs)
        //, f_next_attempt(0) -- auto-init
        //, f_connection(nullptr) -- auto-init
        , f_thread_done(new thread_done_impl(this))
        , f_remote_connect(f_thread_done, addr, port)
        , f_thread("remote snapconnector thread", &f_remote_connect)
    {
        // prevent the timer from going until we get our list of
        // services from snapinit
        //
        set_enable(false);
    }

    // snap::snap_communicator::snap_connection implementation
    virtual void                process_timeout();

    void                        too_busy();
    void                        connection_lost();
    void                        thread_done();

private:
    enum class thread_state_t
    {
        IDLE,
        RUNNING
    };

    snap_communicator_server::pointer_t         f_communicator_server;
    int64_t                                     f_next_attempt = 0;
    std::weak_ptr<connection_impl>              f_connection;
    thread_state_t                              f_state = thread_state_t::IDLE;
    thread_done_impl::pointer_t                 f_thread_done;
    remote_connect                              f_remote_connect;
    snap::snap_thread                           f_thread;
};






/** \brief Listen for messages.
 *
 * The snapcommunicator TCP connection simply listen for process_message()
 * callbacks and processes those messages by calling the process_message()
 * of the connections class.
 *
 * It also listens for disconnections so it can send a new STATUS command
 * whenever the connection goes down.
 */
class connection_impl
        : public snap::snap_communicator::snap_tcp_server_client_message_connection
{
public:
    typedef std::shared_ptr<connection_impl>    pointer_t;

    connection_impl(snap_communicator_server::pointer_t cs, int socket)
        : snap_tcp_server_client_message_connection(socket)
        , f_communicator_server(cs)
    {
    }

    /** \brief Connection lost.
     *
     * When a connection goes down it gets deleted. This is when we can
     * send a new STATUS event to all the other STATUS hungry connections.
     */
    ~connection_impl()
    {
        f_communicator_server->connection_lost(QString::fromUtf8(get_addr().c_str()));

        // save when it is ending in case we did not get a DISCONNECT
        // or an UNREGISTER event
        //
        connection_ended();

        // clearly mark this connection as "invalid"
        //
        set_connection_types("");

        // make sure that if we were a connection understanding STATUS
        // we do not send that status
        //
        f_understood_commands.remove("STATUS");

        // now ask the server to send a new STATUS to all connections
        // that understand that message; we pass our pointer since we
        // want to send the info about this connection in that STATUS
        // message
        //
        f_communicator_server->send_status(shared_from_this());
    }

    // snap::snap_communicator::snap_tcp_server_client_message_connection implementation
    virtual void process_message(snap::snap_communicator_message const & message)
    {
        f_communicator_server->process_message(shared_from_this(), message, false);
    }

    /** \brief Remove ourselves when we receive a timeout.
     *
     * Whenever we receive a shutdown, we have to remove everything but
     * we still want to send some message and to do so we need to use
     * the timeout which happens after we finalize all read and write
     * callbacks.
     */
    virtual void process_timeout()
    {
        remove_from_communicator();
    }

    /** \brief Save when the connection started.
     *
     * This function is called whenever a CONNECT or REGISTER message
     * is received since those mark the time when a connection starts.
     *
     * You can later retrieve when the connection started with the
     * get_connection_started() function.
     *
     * This call also resets the f_ended_on in case we were able to
     * reuse the same connection multiple times (reconnecting means
     * a new socket and thus a brand new connection object...)
     */
    void connection_started()
    {
        f_started_on = snap::snap_communicator::get_current_date();
        f_ended_on = -1;
    }

    /** \brief Return information on when the connection started.
     *
     * This function gives you the date and time when the connection
     * started, meaning when the connection received a CONNECT or
     * REGISTER event.
     *
     * If the events have no yet occured, then the connection returns
     * -1 instead.
     *
     * \return The date and time when the connection started in microseconds.
     */
    int64_t get_connection_started() const
    {
        return f_started_on;
    }

    /** \brief Connection ended, save the date and time of the event.
     *
     * Whenever we receive a DISCONNECT or UNREGISTER we call this
     * function. It also gets called in the event a connection
     * is deleted without first receiving a graceful DISCONNECT
     * or UNREGISTER event.
     */
    void connection_ended()
    {
        // save the current only if the connection really started
        // before and also only once (do not update the time later)
        //
        if(f_started_on != -1
        && f_ended_on == -1)
        {
            f_ended_on = snap::snap_communicator::get_current_date();
        }
    }

    /** \brief Timestamp when the connection was ended.
     *
     * This value represents the time when the UNREGISTER, DISCONNECT,
     * or the destruction of the connection_impl object occurred. It
     * represents the time when the specific service was shutdown.
     *
     * \return The approximative date when the connection ended in microseconds.
     */
    int64_t get_connection_ended() const
    {
        return f_ended_on;
    }

    /** \brief Define the type of snapcommunicator server.
     *
     * This function is called whenever a CONNECT or an ACCEPT is received.
     * It saves the type=... parameter. By default the type is empty meaning
     * that the connection was not yet fully initialized.
     *
     * When a REGISTER is received instead of a CONNECT or an ACCEPT, then
     * the type is set to "client".
     *
     * \param[in] types  List of connection types of this snapcommunicator.
     */
    void set_connection_types(QString const & types)
    {
        f_types = types;
    }

    /** \brief Retrieve the current type of this connection.
     *
     * By default a connection is given the special type "" which means
     * that it is not currently properly initialized yet. To properly
     * initialize a connection one has to either CONNECT (between
     * snapcommunicator servers) or REGISTER (a snapbackend, snapserver,
     * snapwatchdog, and other local services.)
     *
     * The type is set to "client" for local services and another
     * word, such as "frontend", when representing another snapserver.
     *
     * \return The type of server this connection represents.
     */
    QString const & get_connection_types() const
    {
        return f_types;
    }

    /** \brief Define the list of services supported by the snapcommunicator.
     *
     * Whenever a snapcommunicator connects to another one, either by
     * doing a CONNECT or replying to a CONNECT by an ACCEPT, it is
     * expected to list services that it supports (the list could be
     * empty as it usually is on a Cassandra node.) This function
     * saves that list.
     *
     * This defines the name of services and thus where to send various
     * messages such as a PING to request a service to start doing work.
     *
     * \param[in] services  The list of services this server handles.
     */
    void set_services(QString const & services)
    {
        snap::snap_string_list list(services.split(','));
        for(auto s : list)
        {
            f_services[s] = true;
        }
    }

    /** \brief Retrieve the list of services offered by other snapcommunicators.
     *
     * This function saves in the input parameter \p services the list of
     * services that this very snapcommunicator offers.
     *
     * \param[in,out] services  The map where all the services are defined.
     */
    void get_services(sorted_list_of_strings_t & services)
    {
        services.unite(f_services);
    }

    /** \brief Define the list of services we heard of.
     *
     * This function saves the list of services that were heard of by
     * another snapcommunicator server. This list may be updated later
     * with an ACCEPT event.
     *
     * This list is used to know where to forward a message if we do
     * not have a more direct link to those services (i.e. the same
     * service defined in our own list or in a snapcommunicator
     * we are directly connected to.)
     *
     * \param[in] services  The list of services heard of.
     */
    void set_services_heard_of(QString const & services)
    {
        snap::snap_string_list list(services.split(','));
        for(auto s : list)
        {
            f_services_heard_of[s] = true;
        }
    }

    /** \brief Retrieve the list of services heard of by another server.
     *
     * This function saves in the input parameter \p services the list of
     * services that this snapcommunicator heard of.
     *
     * \param[in,out] services  The map where all the services are defined.
     */
    void get_services_heard_of(sorted_list_of_strings_t & services)
    {
        services.unite(f_services_heard_of);
    }

    /** \brief List of defined commands.
     *
     * This function saves the list of commands known by another process.
     * The \p commands parameter is broken up at each comma and the
     * resulting list saved in the f_understood_commands map for fast
     * retrieval.
     *
     * In general a process receives the COMMANDS event whenever it
     * sent the HELP event to request for this list.
     *
     * \param[in] commands  The list of understood commands.
     */
    void set_commands(QString const & commands)
    {
        snap::snap_string_list cmds(commands.split(','));
        for(auto c : cmds)
        {
            f_understood_commands[c] = true;
        }
    }

    /** \brief Check whether a certain command is understood by this connection.
     *
     * This function checks whether this connection understands \p command.
     *
     * \param[in] command  The command to check for.
     *
     * \return true if the command is supported, false otherwise.
     */
    bool understand_command(QString const & command)
    {
        return f_understood_commands.contains(command);
    }

private:
    snap_communicator_server::pointer_t     f_communicator_server;
    sorted_list_of_strings_t                f_understood_commands;
    int64_t                                 f_started_on = -1;
    int64_t                                 f_ended_on = -1;
    QString                                 f_types;
    sorted_list_of_strings_t                f_services;
    sorted_list_of_strings_t                f_services_heard_of;
};







/** \brief Handle new connections from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */
class listener_impl
        : public snap::snap_communicator::snap_tcp_server_connection
{
public:
    /** \brief The listener initialization.
     *
     * The listener creates a new TCP server to listen for incoming
     * TCP connection.
     *
     * \param[in] addr  The address to listen on. Most often it is 0.0.0.0.
     * \param[in] port  The port to listen on.
     * \param[in] max_connections  The maximum number of connections to keep
     *            waiting; if more arrive, refuse them until we are done with
     *            some existing connections.
     * \param[in] reuse_addr  Whether to let the OS reuse that socket immediately.
     * \param[in] auto_close  Whether to automatically close the socket once more
     *            needed anymore.
     */
    listener_impl(snap_communicator_server::pointer_t cs, std::string const & addr, int port, int max_connections, bool reuse_addr, bool auto_close)
        : snap_tcp_server_connection(addr, port, max_connections, reuse_addr, auto_close)
        , f_communicator_server(cs)
    {
    }

    // snap::snap_communicator::snap_server_connection implementation
    virtual void process_accept()
    {
        // a new client just connected, create a new connection_impl
        // object and add it to the snap_communicator object.
        //
        int const new_socket(accept());
        if(new_socket < 0)
        {
            // an error occurred, report in the logs
            int const e(errno);
            SNAP_LOG_ERROR("somehow accept() failed with errno: ")(e);
            return;
        }

        connection_impl::pointer_t connection(new connection_impl(f_communicator_server, new_socket));

        // set a default name in each new connection, this changes
        // whenever we receive a REGISTER event from that connection
        //
        connection->set_name("client connection");

        snap::snap_communicator::instance()->add_connection(connection);
    }

private:
    snap_communicator_server::pointer_t     f_communicator_server;
};








/** \brief Handle new connections from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */
class messager_impl
        : public snap::snap_communicator::snap_udp_server_message_connection
{
public:
    /** \brief The messager initialization.
     *
     * The messager receives UDP messages from various sources (mainly
     * backends at this point.)
     *
     * \param[in] cs  The snap communicator server we are listening for.
     * \param[in] addr  The address to listen on. Most often it is 127.0.0.1.
     *                  for the UDP because we currently only allow for
     *                  local messages.
     * \param[in] port  The port to listen on.
     */
    messager_impl(snap_communicator_server::pointer_t cs, std::string const & addr, int port)
        : snap_udp_server_message_connection(addr, port)
        , f_communicator_server(cs)
    {
    }

    // snap::snap_communicator::snap_server_connection implementation
    virtual void process_message(snap::snap_communicator_message const & message)
    {
        f_communicator_server->process_message(shared_from_this(), message, true);
    }

private:
    // this is owned by a server function so no need for a smart pointer
    snap_communicator_server::pointer_t f_communicator_server;
};









/** \brief Construct the snap_communicator_server object.
 *
 * This function saves the server pointer in the snap_communicator_server
 * object. It is used later to gather various information and call helper
 * functions.
 */
snap_communicator_server::snap_communicator_server(snap::server::pointer_t s)
    : f_server(s)
{
}


/** \brief Initialize the snap_communicator_server.
 *
 * This function is used to initialize the connetions object. This means
 * setting up a few parameters such as the nice level of the application
 * and priority scheme for listening to events.
 *
 * Then it creates two sockets: one listening on TCP/IP and the other
 * listening on UDP/IP. The TCP/IP is for other servers to connect to
 * and listen communicate various status between various servers. The
 * UDP/IP is used to very quickly send messages between servers. The
 * UDP/IP messages are viewed as signals to wake up a server so it
 * starts working on new data (in most cases, at least.)
 */
void snap_communicator_server::init()
{
    // change nice value of the Snap! Communicator process
    {
        QString const nice_str(f_server->get_parameter("nice"));
        bool ok(false);
        int const nice(nice_str.toInt(&ok));
        if(!ok
        || nice < 0
        || nice > 19)
        {
            SNAP_LOG_FATAL("the nice parameter from the configuration file must be a valid number between 0 and 19. %1 is not valid.")(nice_str);
            f_server->exit(1);
        }
        // process 0 represents 'self'
        setpriority(PRIO_PROCESS, 0, nice);
    }

    {
        f_server_types = canonicalize_server_types(f_server->get_parameter("server_types"));

        f_explicit_neighbors = canonicalize_neighbors(f_server->get_parameter("neighbors"));
        add_neighbors(f_explicit_neighbors);

        // check a user defined maximum number of connections
        // by default this is set to SNAP_COMMUNICATOR_MAX_CONNECTIONS,
        // which at this time is 100
        //
        QString const max_connections(f_server->get_parameter("max_connections"));
        if(!max_connections.isEmpty())
        {
            bool ok(false);
            f_max_connections = max_connections.toLongLong(&ok, 10);
            if(!ok
            || f_max_connections < 10)
            {
                SNAP_LOG_FATAL("the max_connections parameter is not a valid decimal number or is smaller than 10 (")(max_connections)(").");
                f_server->exit(1);
            }
        }
    }

    f_communicator = snap::snap_communicator::instance();

    // create a listener, for new arriving TCP/IP connections
    //
    // auto-close is set to false because the accept() is not directly used
    // on the tcp_server object
    //
    {
        QString addr("127.0.0.1"); // this default is most certainly wrong
        int port(4040);
        QString const listen_info(f_server->get_parameter("listen"));
        if(!listen_info.isEmpty())
        {
            tcp_client_server::get_addr_port(listen_info, addr, port, 4040);
        }

        int max_pending_connections(10);
        QString const max_pending_connections_str(f_server->get_parameter("max_pending_connections"));
        if(!max_pending_connections_str.isEmpty())
        {
            bool ok(false);
            max_pending_connections = max_pending_connections_str.toInt(&ok);
            if(!ok
            || max_pending_connections < 5
            || max_pending_connections > 1000)
            {
                SNAP_LOG_FATAL("the max_pending_connections parameter from the configuration file must be a valid number between 5 and 1000. %1 is not valid.")(max_pending_connections_str);
                f_server->exit(1);
            }
        }

        f_listener.reset(new listener_impl(shared_from_this(), addr.toUtf8().data(), port, max_pending_connections, true, false));
        f_listener->set_name("snap communicator listener");
        f_communicator->add_connection(f_listener);
    }

    {
        QString addr("127.0.0.1"); // this default should work just fine
        int port(4041);
        QString const signal_info(f_server->get_parameter("signal"));
        if(!signal_info.isEmpty())
        {
            tcp_client_server::get_addr_port(signal_info, addr, port, 4041);
        }

        f_messager.reset(new messager_impl(shared_from_this(), addr.toUtf8().data(), port));
        f_messager->set_name("snap communicator messager (UDP)");
        f_communicator->add_connection(f_messager);
    }

    // we also want to create timer for each neighbor
    //
    // right now we only have explicit neights until we support the
    // reading of saved gossiped neighbors which means that we can
    // as well implement the full set right now
    //
    for(sorted_list_of_strings_t::const_iterator it(f_all_neighbors.begin());
                                                 it != f_all_neighbors.end();
                                                 ++it)
    {
        QString addr;
        int port(4040);
        QString const ip(it.key());
        tcp_client_server::get_addr_port(ip, addr, port, 4040);
        // TODO: we should never have two entries with the same IP address
        //       (even if the port is different, because you can only have
        //       one snapcommunicator per computer)
        f_remote_snapcommunicators[addr].reset(new remote_snap_communicator(shared_from_this(), addr, port));
    }
}


/** \brief The execution loop.
 *
 * This function runs the execution loop until the snapcommunicator
 * system receives a QUIT message.
 */
void snap_communicator_server::run()
{
    // run "forever" (until we receive a QUIT message)
    f_communicator->run();

    // we are done, cleanly get rid of the communicator
    f_communicator.reset();
}


/** \brief Process a message we just received.
 *
 * This function is called whenever a TCP or UDP message is received.
 * The function accepts all TCP messages, however, UDP messages are
 * limited to a very few such as STOP and SHUTDOWN. You will want to
 * check the documentation of each message to know whether it can
 * be sent over UDP or not.
 *
 * Note that the main reason why the UDP port is not allowed for most
 * messages is to send a reply you have to have TCP. This means responses
 * to those messages also need to be sent over TCP (because we could
 * not have sent an ACCEPT as a response to a CONNECT over a UDP
 * connection.)
 *
 * \param[in] connection  The connection that just sent us that message.
 * \param[in] message  The message were were just sent.
 * \param[in] udp  If true, this was a UDP message.
 */
void snap_communicator_server::process_message(snap::snap_communicator::snap_connection::pointer_t connection, snap::snap_communicator_message const & message, bool udp)
{
    QString const command(message.get_command());

    connection_impl::pointer_t c(std::dynamic_pointer_cast<connection_impl>(connection));

    // TODO: move all the command bodies to sub-funtions.

    // check who this message is for
    QString const service(message.get_service());
//std::cerr << "SNAP COMMUNICATOR: received a message [" << message.to_message() << "]\n";
    if(service.isEmpty()
    || service == "snapcommunicator")
    {
        if(f_shutdown)
        {
            if(!udp)
            {
                // we are shutting down so just send a quick QUTTING reply
                // letting the other process know about it
                //
                snap::snap_communicator_message reply;
                reply.set_command("QUITTING");
                c->send_message(reply);
            }
            //else -- UDP message arriving after f_shutdown are ignored
            return;
        }

        // this one is for us!
        switch(command[0].unicode())
        {
        case 'A':
            if(command == "ACCEPT")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("ACCEPT is only accepted over a TCP connection.");
                    return;
                }

                if(c)
                {
                    // the type is mandatory in an ACCEPT message
                    //
                    if(!message.has_parameter("types"))
                    {
                        SNAP_LOG_ERROR("ACCEPT was received without a \"type\" parameter, which is mandatory.");
                        return;
                    }
                    c->set_connection_types(canonicalize_server_types(message.get_parameter("types")));

                    // reply to a CONNECT, this was to connect to another
                    // snapcommunicator on another computer
                    //
                    c->connection_started();

                    if(message.has_parameter("services"))
                    {
                        c->set_services(message.get_parameter("services"));
                    }
                    if(message.has_parameter("heard_of"))
                    {
                        c->set_services_heard_of(message.get_parameter("heard_of"));
                    }
                    if(message.has_parameter("neighbors"))
                    {
                        add_neighbors(message.get_parameter("neighbors"));
                    }

                    // we just got some new services information,
                    // refresh our cache
                    //
                    refresh_heard_of();
                    return;
                }
            }
            break;

        case 'C':
            if(command == "COMMANDS")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("COMMANDS is only accepted over a TCP connection.");
                    return;
                }

                if(c)
                {
                    if(message.has_parameter("list"))
                    {
                        c->set_commands(message.get_parameter("list"));
                    }
                    else
                    {
                        SNAP_LOG_ERROR("COMMANDS was sent without a \"list\" parameter.");
                    }
                    return;
                }
            }
            else if(command == "CONNECT")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("CONNECT is only accepted over a TCP connection.");
                    return;
                }

                if(c)
                {
                    // first we verify that we have a valid version to
                    // communicate between two snapcommunicators
                    //
                    if(!message.has_parameter("type")
                    && !message.has_parameter("version"))
                    {
                        SNAP_LOG_ERROR("CONNECT was sent without a \"type\" and/or a \"version\" parameter, both are mandatory.");
                        return;
                    }
                    if(message.get_integer_parameter("version") != snap::snap_communicator::VERSION)
                    {
                        SNAP_LOG_ERROR("CONNECT was sent with an incompatible version. Expected ")(snap::snap_communicator::VERSION)(", received ")(message.get_integer_parameter("version"));
                        return;
                    }

                    // TODO: add necessary test to know whether we are
                    //       interconnecting simultaneously (i.e. A
                    //       sent a CONNECT to B at the same time as
                    //       B sent a CONNECT to A.)
                    //
                    //       I think we can just compare both IP addresses
                    //       and decide that the smaller one is the server
                    //       and the larger one the client and adjust our
                    //       behavior accordingly. Both IP addresses have
                    //       to be different and on the same network.

                    // always retrieve the connection type
                    //
                    c->set_connection_types(canonicalize_server_types(message.get_parameter("types")));

                    snap::snap_communicator_message reply;

                    // add neighbors with which the guys asking to
                    // connect can attempt to connect with...
                    //
                    if(!f_explicit_neighbors.isEmpty())
                    {
                        reply.add_parameter("neighbors", f_explicit_neighbors);
                    }

                    // always send the server type, whether we accept or
                    // refuse this connection
                    //
                    reply.add_parameter("types", f_server_types);

                    // cool, a remote snapcommunicator wants to connect
                    // with us, make sure we did not reach the maximum
                    // number of connection though...
                    //
                    if(f_communicator->get_connections().size() >= f_max_connections)
                    {
                        // too many connections already, refuse this new
                        // one from a remove system
                        //
                        reply.set_command("REFUSE");
                    }
                    else
                    {
                        // same as ACCEPT (see above) -- maybe we could have
                        // a sub-function...
                        //
                        c->connection_started();

                        if(message.has_parameter("services"))
                        {
                            c->set_services(message.get_parameter("services"));
                        }
                        if(message.has_parameter("heard_of"))
                        {
                            c->set_services_heard_of(message.get_parameter("heard_of"));
                        }
                        if(message.has_parameter("neighbors"))
                        {
                            add_neighbors(message.get_parameter("neighbors"));
                        }

                        // we just got some new services information,
                        // refresh our cache
                        //
                        refresh_heard_of();

                        // the message expects the ACCEPT reply
                        //
                        reply.set_command("ACCEPT");

                        // services
                        if(!f_local_services.isEmpty())
                        {
                            reply.add_parameter("services", f_local_services);
                        }

                        // heard of
                        if(!f_services_heard_of.isEmpty())
                        {
                            reply.add_parameter("heard_of", f_services_heard_of);
                        }
                    }

                    c->send_message(reply);
                    return;
                }
            }
            break;

        case 'D':
            if(command == "DISCONNECT")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("DISCONNECT is only accepted over a TCP connection.");
                    return;
                }

                if(c)
                {
                    c->connection_ended();

                    // this has to be another snapcommunicator
                    // (i.e. an object that sent ACCEPT or CONNECT)
                    //
                    QString const types(c->get_connection_types());
                    if(!types.isEmpty()
                    && types != "client")
                    {
                        // we must ignore and we do ignore connections with a
                        // type of "" since they represent an uninitialized
                        // connection item (unconnected)
                        //
                        c->set_connection_types("");

                        // disconnecting means it is gone so we can remove
                        // it from the communicator; note that won't remove
                        // it from the list being processed in the run()
                        // function so its functions may still be called
                        // once after this removal
                        //
                        f_communicator->remove_connection(connection);

                        // we just got some new services information,
                        // refresh our cache
                        //
                        refresh_heard_of();
                    }
                    else
                    {
                        SNAP_LOG_ERROR("DISCONNECT was sent from a connection which is not of the right type (")(types)(").");
                    }
                    return;
                }
            }
            break;

        case 'H':
            if(command == "HELP")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("HELP is only accepted over a TCP connection.");
                    break;
                }

                if(c)
                {
                    // reply with COMMANDS
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("COMMANDS");

                    // list of commands understood by snapcommunicator
                    reply.add_parameter("list", "ACCEPT,CONNECT,COMMANDS,DISCONNECT,HELP,REFUSE,REGISTER,SHUTDOWN,STOP,UNREGISTER");

                    c->send_message(reply);
                    return;
                }
            }
            break;

        case 'R':
            if(command == "REFUSE")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("REFUSE is only accepted over a TCP connection.");
                    break;
                }

                if(c)
                {
                    // we were not connected so we do not have to
                    // disconnect; mark that corresponding server
                    // as too busy and try connecting again much
                    // later...
                    //
                    QString const addr(QString::fromUtf8(c->get_addr().c_str()));
                    if(f_remote_snapcommunicators.contains(addr))
                    {
                        f_remote_snapcommunicators[addr]->too_busy();
                    }

                    f_communicator->remove_connection(connection);
                    return;
                }
            }
            else if(command == "REGISTER")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("REGISTER is only accepted over a TCP connection.");
                    break;
                }

                if(c)
                {
                    if(!message.has_parameter("service")
                    || !message.has_parameter("version"))
                    {
                        SNAP_LOG_ERROR("REGISTER was called without a \"service\" and/or a \"version\" parameter, both are mandatory.");
                        return;
                    }
                    if(message.get_integer_parameter("version") != snap::snap_communicator::VERSION)
                    {
                        SNAP_LOG_ERROR("REGISTER was called with an incompatible version. Expected ")(snap::snap_communicator::VERSION)(", received ")(message.get_integer_parameter("version"));
                        return;
                    }
                    // the "service" parameter is the name of the service,
                    // now we can process messages for this service
                    //
                    QString const service_name(message.get_parameter("service"));
                    c->set_name(service_name);

                    c->set_connection_types("client");

                    // connection is up now
                    //
                    c->connection_started();

                    // tell the connect we are ready
                    // (the connection uses that as a trigger to start work)
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("READY");
                    c->send_message(reply);

                    // request the COMMANDS of this connection
                    //
                    snap::snap_communicator_message help;
                    help.set_command("HELP");
                    c->send_message(help);

                    // status changed for this connection
                    //
                    send_status(c);

                    // if we have local messages that were cached, then
                    // forward them now
                    //
                    if(!f_local_message_cache.empty())
                    {
                        for(auto m : f_local_message_cache)
                        {
                            if(m.get_service() == service_name)
                            {
                                // TBD: should we remove the service name before forwarding?
                                //      (we have to instances)
                                //
                                c->send_message(m);
                            }
                        }
                    }
                    return;
                }
            }
            break;

        case 'S':
            if(command == "SERVICES")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("SERVICES is only accepted over a TCP connection.");
                    return;
                }

                if(c)
                {
                    if(!message.has_parameter("list"))
                    {
                        SNAP_LOG_ERROR("SERVICES was called without a \"list\" parameter, it is mandatory.");
                        return;
                    }
                    // the "service" parameter is the name of the service,
                    // now we can process messages for this service
                    //
                    f_local_services_list = canonicalize_services(message.get_parameter("list"));

                    // Since snapinit started us, this list cannot ever be empty!
                    //
                    if(!f_local_services_list.isEmpty())
                    {
                        SNAP_LOG_ERROR("SERVICES was called with an empty \"list\", there should at least be snapcommunicator (and snapwatchdog).");
                        return;
                    }

                    // create a string so we can send the list of services
                    // at once instead of recreating the string each time
                    //
                    f_local_services = static_cast<snap::snap_string_list>(f_local_services_list.keys()).join(",");

                    // now we can get the connections to other communicators
                    // started
                    //
                    int64_t start_time(snap::snap_child::get_current_date());
                    for(auto communicator : f_remote_snapcommunicators)
                    {
                        communicator->set_timeout_date(start_time);
                        communicator->set_enable(true);

                        start_time += 1000000;
                    }
                    return;
                }
            }
            else if(command == "SHUTDOWN")
            {
                shutdown(true);
                return;
            }
            else if(command == "STOP")
            {
                shutdown(false);
                return;
            }
            break;

        case 'U':
            if(command == "UNREGISTER")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("UNREGISTER is only accepted over a TCP connection.");
                    return;
                }

                if(c)
                {
                    if(!message.has_parameter("service"))
                    {
                        SNAP_LOG_ERROR("UNREGISTER was called without a \"service\" parameter, which is mandatory.");
                        return;
                    }
                    // remove the service name immediately
                    //
                    c->set_name("");

                    // also remove the connection type, an empty type
                    // represents an unconnected item
                    //
                    c->set_connection_types("");

                    // connection is down now
                    //
                    c->connection_ended();

                    // status changed for this connection
                    //
                    send_status(c);

                    // get rid of that connection now (it is faster than
                    // waiting for the HUP because it will not be in the
                    // list of connections on the next loop.)
                    //
                    f_communicator->remove_connection(c);
                    return;
                }
            }
            break;

        }

        // if they used a TCP connection to send this message, let the caller
        // know that we do not understand his message
        //
        if(!udp)
        {
            snap::snap_communicator_message reply;
            reply.set_command("UNKNOWN");
            reply.add_parameter("command", command);
            c->send_message(reply);
        }

        // done
        SNAP_LOG_ERROR("unknown command \"")(command)("\" or not sent from what is considered the correct connection for that message.");
        return;
    }

    //
    // the message includes a service name, so we want to forward that
    // message to that service
    //
    // for that purpose we consider the following three lists:
    //
    // 1. we have the service in our local services, we must forward it
    //    to that connection; if the connection is not up and running yet,
    //    cache the information
    //
    // 2. the service is not one of ours, but we found a remote
    //    snapcommunicator server that says it is his, forward the
    //    message to that snapcommunicator instead
    //
    // 3. the service is in the "heard of" list of services, send that
    //    message to that snapcommunicator, it will then forward it
    //    to the correct server (or another proxy...)
    //
    // 4. the service cannot be found anywhere, we save it in our remote
    //    cache (i.e. because it will only be possible to send that message
    //    to a remote snapcommunicator and not to a service on this system)
    //

    if(f_local_services_list.contains(service))
    {
        // service is local, check whether the service is registered,
        // if registered, forward the message immediately
        //
        snap::snap_communicator::snap_connection::vector_t const & connections(f_communicator->get_connections());
        for(auto nc : connections)
        {
            if(nc->get_name() == service)
            {
                // we have such a service, just forward to it now
                //
                // TBD: should we remove the service name before forwarding?
                //
                std::dynamic_pointer_cast<connection_impl>(nc)->send_message(message);
                return;
            }
        }

        // its a service that is expected on this computer, but it is not
        // running right now...
        //
        f_local_message_cache.push_back(message);
        return;
    }

std::cerr<< "received event for remote service *** not yet implemented! ***\n";

}


/** \brief Send the current status of a client to connections.
 *
 * Some connections (at this time only the snapwatchdog) may be interested
 * by the STATUS event. Any connection that understands the STATUS
 * event will be sent that event whenever the status of a connection
 * changes (specifically, on a REGISTER and on an UNREGISTER or equivalent.)
 *
 * \param[in] client  The client that just had its status changed.
 */
void snap_communicator_server::send_status(snap::snap_communicator::snap_connection::pointer_t connection)
{
    snap::snap_communicator_message reply;
    reply.set_command("STATUS");

    // the name of the service is the name of the connection
    reply.add_parameter("service", connection->get_name());

    // check whether the connection is now up or down
    connection_impl::pointer_t c(std::dynamic_pointer_cast<connection_impl>(connection));
    QString const types(c->get_connection_types());
    reply.add_parameter("status", types.isEmpty() ? "down" : "up");

    // get the time when it was considered up
    int64_t const up_since(c->get_connection_started());
    if(up_since != -1)
    {
        reply.add_parameter("up_since", up_since / 1000000); // send up time in seconds
    }

    // get the time when it was considered down (if not up yet, this will be skipped)
    int64_t const down_since(c->get_connection_ended());
    if(down_since != -1)
    {
        reply.add_parameter("down_since", down_since / 1000000); // send up time in seconds
    }

    // we have the message, now we need to find the list of connections
    // interested by the STATUS event
    // TODO: cache that list?
    //
    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
    for(auto conn : all_connections)
    {
        connection_impl::pointer_t sc(std::dynamic_pointer_cast<connection_impl>(conn));
        if(!sc)
        {
            // not a connection_impl, ignore (i.e. servers)
            continue;
        }

        if(sc->understand_command("STATUS"))
        {
            // send that STATUS message
            sc->send_message(reply);
        }
    }
}


/** \brief Return the server type of this snap communicator server.
 */
QString snap_communicator_server::get_server_types() const
{
    return f_server_types;
}


/** \brief Return the list of services offered on this computer.
 */
QString snap_communicator_server::get_local_services() const
{
    return f_local_services;
}


/** \brief Return the list of services we heard of.
 */
QString snap_communicator_server::get_services_heard_of() const
{
    return f_services_heard_of;
}


/** \brief Add neighbors to this communicator server.
 *
 * Whenever a snap communicator connects to another snap communicator
 * server, it is given a list of neighbors. These are added using
 * this function. In the end, all servers are expected to have a
 * complete list of all the neighbors.
 *
 * \todo
 * Make this list survive restarts of the snap communicator server.
 *
 * \param[in] new_neighbors  The list of new neighbors
 */
void snap_communicator_server::add_neighbors(QString const & new_neighbors)
{
    if(!new_neighbors.isEmpty())
    {
        static QString cache_filename;

        // first time initialize and read the cache file
        //
        if(cache_filename.isEmpty())
        {
            // get the path to the cache, create if necessary
            //
            cache_filename = f_server->get_parameter("cache_path");
            if(cache_filename.isEmpty())
            {
                cache_filename = "/var/cache/snapwebsites";
            }
            snap::mkdir_p(cache_filename);
            cache_filename += "/neighbors.txt";

            QFile cache(cache_filename);
            if(cache.open(QIODevice::ReadOnly))
            {
                QByteArray buffer(cache.readLine(1024));
                QString line(QString::fromUtf8(buffer).trimmed());
                if(!line.isEmpty()
                && line[0] != '#')
                {
                    f_all_neighbors[line] = true;
                }
            }
        }

        bool changed(false);
        snap::snap_string_list list(new_neighbors.split(',', QString::SkipEmptyParts));
        for(auto s : list)
        {
            if(!f_all_neighbors.contains(s))
            {
                changed = true;
                f_all_neighbors[s] = true;
            }
        }

        // if the map changed, then save the change in the cache
        //
        // TODO: we may be able to optimize this by not saving on each and
        //       every call; although since it should remain relatively
        //       small, we should be fine (yes, 8,000 computers is still
        //       a small file in this cache.)
        //
        if(changed)
        {
            QFile cache(cache_filename);
            if(cache.open(QIODevice::WriteOnly))
            {
                for(sorted_list_of_strings_t::const_iterator n(f_all_neighbors.begin());
                                                             n != f_all_neighbors.end();
                                                             ++n)
                {
                    cache.write(n.key().toUtf8());
                    cache.putChar('\n');
                }
            }
            else
            {
                SNAP_LOG_ERROR("could not open cache file \"")(cache_filename)("\" for writing.");
            }
        }
    }
}


/** \brief A connection being destroyed first calls this function...
 *
 * This function checks whether the connection we are currently
 * losing is a snap communicator server. If so we have to re-enable
 * and reset the corresponding timer.
 */
void snap_communicator_server::connection_lost(QString const & addr)
{
    if(f_remote_snapcommunicators.contains(addr))
    {
        f_remote_snapcommunicators[addr]->connection_lost();
    }
}


/** \brief The list of services we know about from other snapcommunicators.
 *
 * This function gathers the list of services that this snapcommunicator
 * heard of. This means, the list of all the services offered by other
 * snapcommunicators, heard of or not, minus our own services (because
 * these other servers will return our own services as heard of!)
 */
void snap_communicator_server::refresh_heard_of()
{
    // reset the list
    f_services_heard_of_list.clear();

    // first gather all the services we have access to
    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
    for(auto connection : all_connections)
    {
        connection_impl::pointer_t c(std::dynamic_pointer_cast<connection_impl>(connection));
        if(!c)
        {
            // not a connection_impl, ignore (i.e. servers)
            continue;
        }

        // get list of services and heard of services
        c->get_services(f_services_heard_of_list);
        c->get_services_heard_of(f_services_heard_of_list);
    }

    // now remove services we are in control of
    for(sorted_list_of_strings_t::const_iterator it(f_local_services_list.begin());
                                                 it != f_local_services_list.end();
                                                 ++it)
    {
        f_services_heard_of_list.remove(it.key());
    }

    // generate a string we can send in a CONNECT or an ACCEPT
    f_services_heard_of.clear();
    for(sorted_list_of_strings_t::const_iterator it(f_services_heard_of_list.begin());
                                                 it != f_services_heard_of_list.end();
                                                 ++it)
    {
        f_services_heard_of += it.key() + ",";
    }
    if(!f_services_heard_of.isEmpty())
    {
        // remove the ending ","
        f_services_heard_of.remove(f_services_heard_of.length() - 1, 1);
    }

    // done
}


/** \brief This snapcommunicator received the SHUTDOWN or a STOP command.
 *
 * This function processes the SHUTDOWN or STOP commands. It is a bit of
 * work since we have to send a message to all connections and the message
 * vary depending on the type of connection.
 *
 * \param[in] full  Do a full shutdown (true) or just a stop (false).
 */
void snap_communicator_server::shutdown(bool full)
{
    // from now on, we are shutting down; use this flag to make sure we
    // do not accept any more REGISTER, CONNECT and other similar
    // messages
    //
    f_shutdown = true;

    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
    for(auto connection : all_connections)
    {
        // a remote communicator timer?
        //
        remote_snap_communicator::pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(connection));
        if(remote_communicator)
        {
            // remote communicators are just timers and can be removed
            // as is, no message are sent there (no interface to do so anyway)
            //
            f_communicator->remove_connection(remote_communicator);
        }
        else
        {
            // a standard service connection or a remote snapcommunicator server?
            //
            connection_impl::pointer_t c(std::dynamic_pointer_cast<connection_impl>(connection));
            if(c)
            {
                QString const types(c->get_connection_types());
                if(types.isEmpty())
                {
                    // not initialized, just get rid of that one
                    f_communicator->remove_connection(c);
                }
                else
                {
                    snap::snap_communicator_message reply;
                    if(types != "client")
                    {
                        // a remote snapcommunicator server needs to also
                        // shutdown so duplicate that message there
                        if(full)
                        {
                            // SHUTDOWN means we shutdown the entire cluster!!!
                            reply.set_command("SHUTDOWN");
                        }
                        else
                        {
                            // STOP means we do not shutdown the entire cluster
                            reply.set_command("DISCONNECT");
                        }
                    }
                    else
                    {
                        // a standard client (i.e. pagelist, images, etc.)
                        // needs to stop so send that message instead
                        //
                        reply.set_command("STOP");
                    }

                    c->send_message(reply);

                    // we cannot yet remove the connection from the communicator
                    // or these messages will never be sent... instead we can
                    // setup a timeout which will be processed immediately if
                    // we put a very small delay
                    //
                    c->set_timeout_delay(1);
                }
            }
            // else ignore the main TCP and UDP servers which we
            // handle below (i.e. we avoid possibly hundred of dynamic_cast
            // that way.)
        }
    }

    // remove the two main servers; we will not respond to any more
    // requests anyway
    //
    f_communicator->remove_connection(f_listener);
    f_communicator->remove_connection(f_messager);
}




/** \brief Attempt a connection to a remote snapcommunicator.
 *
 * This function is run in a separate thread so that way we can
 * take as long as we want to connect to that other
 * snapcommunicator.
 *
 * If the connection fails, the TCP library throws an exception.
 * Here we catch this exception and return with no TCP connection
 * object allocated.
 */
void remote_snap_communicator::remote_connect::run()
{
    f_socket = -1;
    try
    {
        // connect to the specified address and port using a TCP socket
        //
        QByteArray const utf8(f_address.toUtf8());
        tcp_client_server::tcp_client::pointer_t tcp_connection(new tcp_client_server::tcp_client(std::string(utf8.data(), utf8.size()), f_port));

        // save the new socket (we have to use a duplicate since the
        // tcp_connection object is going to close its copy.)
        //
        f_socket = dup(tcp_connection->get_socket());

        // Tell the main thread that this thread is done, this call
        // immediately wakes up the poll() command
        //
        f_thread_done->thread_done();

        // close the new tcp_connection object immediately
    }
    catch(tcp_client_server::tcp_client_server_runtime_error const & e)
    {
        // connection failed... we will have to try again later
        SNAP_LOG_ERROR("connection to snapcommunicator at ")(f_address)(":")(f_port)(" failed with: ")(e.what());
    }
}


/** \brief Remote snapcommunicator connection attempt.
 *
 * On a timeout of this connection we attempt to connect to this
 * remote snapcommunicator service.
 *
 * If the conenction attempt was already sent, then the timeout
 * represents an opportunity to check whether the connection
 * succeeded.
 *
 */
void remote_snap_communicator::process_timeout()
{
    if(f_state == thread_state_t::IDLE)
    {
        // the pointer should always be a nullptr when we reach this
        // line, but in case we did not properly disable the connect
        // we do it here now; if the pointer is not nullptr then we
        // will get a destruction at some point leading to a
        // re-enabling of the timer and back here with a nullptr
        //
        connection_impl::pointer_t connection(f_connection.lock());
        if(connection)
        {
            // stop this timeout since we are properly connected
            set_enable(false);
            return;
        }

        if(!f_thread_done)
        {
            f_thread_done.reset(new thread_done_impl(this));
        }
        snap::snap_communicator::instance()->add_connection(f_thread_done);

        f_state = thread_state_t::RUNNING;
        f_thread.start();

        // stop the timer and wait for the thread signal
        //
        set_enable(false);
    }
}


/** \brief Callback once the thread done signal gets called.
 *
 */
void remote_snap_communicator::thread_done()
{
    f_state = thread_state_t::IDLE;

    // The f_socket parameter is marked atomic so it can be used
    // between the secondary and main threads without mutexes
    //
    int const socket(f_remote_connect.get_socket());
    if(socket == -1)
    {
        // wait for 5 minutes before we attempt this connection again
        //
        set_timeout_delay(5LL * 60LL * 1000000LL);
        set_enable(true);
        return;
    }

    // create the TCP connection to communicate with that neighbor
    //
    connection_impl::pointer_t connection(new connection_impl(f_communicator_server, socket));

    // keep a copy here, but we only keep a weak pointer to
    // avoid locking the object when we somehow lose connection
    //
    f_connection = connection;

    // set the name for this type of connections; this name does
    // not change later
    //
    connection->set_name("snapcommunicator connection");

    snap::snap_communicator::instance()->add_connection(connection);

    // we initiated the connection so we have to send a CONNECT
    //
    snap::snap_communicator_message reply;
    //reply.set_service("" or "snapcommunicator") -- "" is the default
    reply.set_command("CONNECT");

    // server version
    reply.add_parameter("version", snap::snap_communicator::VERSION);

    // server type
    reply.add_parameter("types", f_communicator_server->get_server_types());

    // services
    QString const services(f_communicator_server->get_local_services());
    if(!services.isEmpty())
    {
        reply.add_parameter("services", f_communicator_server->get_local_services());
    }

    // heard of
    QString const services_heard_of(f_communicator_server->get_services_heard_of());
    if(!services_heard_of.isEmpty())
    {
        reply.add_parameter("heard_of", services_heard_of);
    }

    connection->send_message(reply);
}


/** \brief A remote communicator refused our connection.
 *
 * When a remote snap communicator server already manages too many
 * connections, it may end up refusing our additional connection.
 * When this happens, we have to avoid trying to connect again
 * and again.
 *
 * Here we use a very large delay of 24h before trying to connect
 * again later. I do not really think this is necessary because
 * if we have too many connections we anyway always have too many
 * connections. That being said, once in a while a computer dies
 * and thus the number of connections may drop to a level where
 * we will be accepted.
 *
 * At some point we may want to look into having seeds instead
 * of allowing connections to all the nodes.
 */
void remote_snap_communicator::too_busy()
{
    // wait for 1 day and try again (is 1 day too long?)
    set_timeout_delay(24LL * 60LL * 60LL * 1000000LL);
}


/** \brief Call whenever a connection is lost.
 *
 * Whenever we lose a connection to a snap communicator, this
 * function gets called. This happens in the connection_impl
 * destructor.
 *
 * The function makes sure the timer connection is re-enabled and
 * sets the timeout to 5 minutes so we do not try to reconnect
 * over and over again wasting time (i.e. the other server may
 * be rebooted which may take a good 2 minutes with the network
 * and startup of Snap...) Assuming we have a large network,
 * 5 minutes will amount to nothing. On a very small network,
 * it could prevent some messages from going through. We will
 * have to test and get a better feel for the right value and
 * possibly know to make some smart choice on the value based
 * on various parameters such as the number of neighbors we
 * know of.
 */
void remote_snap_communicator::connection_lost()
{
    set_enable(true);
    set_timeout_delay(5LL * 60LL * 1000000LL);
}







int main(int argc, char * argv[])
{
    int exitval( 1 );
    try
    {
        // create a server object
        snap::server::pointer_t s( snap::server::instance() );
        //s->setup_as_backend();

        // parse the command line arguments (this also brings in the .conf params)
        //
        s->set_default_config_filename( "/etc/snapwebsites/snapcommunicator.conf" );
        s->config( argc, argv );

        // if possible, detach the server
        s->detach();
        // Only the child (backend) process returns here

        // Now create the qt application instance
        //
        s->prepare_qtapp( argc, argv );

        // show when we started in the log
        SNAP_LOG_INFO("--------------------------------- snapcommunicator started.");

        // Run the snap communicator server; note that the snapcommunicator
        // server is snap_communicator and not snap::server
        //
        {
            snap_communicator_server::pointer_t communicator( new snap_communicator_server( s ) );
            communicator->init();
            communicator->run();
        }

        exitval = 0;
    }
    catch( snap::snap_exception const & except )
    {
        SNAP_LOG_FATAL("snapcommunicator: exception caught: ")(except.what());
    }
    catch( std::exception const & std_except )
    {
        SNAP_LOG_FATAL("snapcommunicator: exception caught: ")(std_except.what())(" (there are mainly two kinds of exceptions happening here: Snap logic errors and Cassandra exceptions that are thrown by thrift)");
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snapcommunicator: unknown exception caught!");
    }

    // exit via the server so the server can clean itself up properly
    snap::server::exit( exitval );

    snap::NOTREACHED();
    return 0;
}

// vim: ts=4 sw=4 et
