// Snap Websites Server -- server to handle inter-process communication
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

#include "snap_communicator.h"

#include "addr.h"
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
 * |      v                 |     |      |              +----------------+  |
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


/** \brief The sequence number of a message being broadcast.
 *
 * Each instance of snapcommunicator may broadcast a message to other
 * snapcommunicators. When that happens, we want to ignore that
 * message in case it comes again to the same snapcommunicator. This
 * can be accomplished by saving which messages we received.
 *
 * We also control a number of hops and a timeout.
 *
 * This counter is added to the name of the computer running this
 * snapcommunicator (i.e. f_server_name) so for example it would
 * look as following if the computer name is "snap":
 *
 * \code
 *          snap-123
 * \endcode
 */
int64_t             g_broadcast_sequence = 0;


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
        QString address; // no default address for neighbors
        int port(4040);
        tcp_client_server::get_addr_port(neighbor, address, port, "tcp");

        // TODO: move canonicalization to tcp_client_server so other software
        //       can make use of it
        //
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






class snap_communicator_server;
typedef std::shared_ptr<snap_communicator_server>           snap_communicator_server_pointer_t;

class base_connection;
typedef std::shared_ptr<base_connection>                    base_connection_pointer_t;

class service_connection;
typedef std::shared_ptr<service_connection>                 service_connection_pointer_t;
typedef QMap<QString, service_connection_pointer_t>         service_connection_list_t;

class remote_snap_communicator;
typedef std::shared_ptr<remote_snap_communicator>           remote_snap_communicator_pointer_t;
typedef std::vector<remote_snap_communicator_pointer_t>     remote_snap_communicator_vector_t;
typedef QMap<QString, remote_snap_communicator_pointer_t>   remote_snap_communicator_list_t;

class gossip_to_remote_snap_communicator;
typedef std::shared_ptr<gossip_to_remote_snap_communicator> gossip_snap_communicator_pointer_t;
typedef QMap<QString, gossip_snap_communicator_pointer_t>   gossip_snap_communicator_list_t;



class remote_communicator_connections
        : public std::enable_shared_from_this<remote_communicator_connections>
{
public:
    typedef std::shared_ptr<remote_communicator_connections>    pointer_t;

                        remote_communicator_connections(snap_communicator_server_pointer_t communicator, snap_addr::addr const & my_addr);

    QString             get_my_address() const;
    void                add_remote_communicator(QString const & addr);
    void                stop_gossiping();
    void                too_busy(QString const & addr);
    void                start();
    void                gossip_received(QString const & addr);

private:
    snap_communicator_server_pointer_t  f_communicator_server;
    snap_addr::addr const &             f_my_address;
    bool                                f_started = false;
    QMap<QString, int>                  f_all_ips;
    remote_snap_communicator_list_t     f_smaller_ips;      // we connect to smaller IPs
    gossip_snap_communicator_list_t     f_gossip_ips;
    service_connection_list_t           f_larger_ips;       // larger IPs connect to us
};


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
    bool                        add_neighbors(QString const & new_neighbors);
    inline void                 verify_command(base_connection_pointer_t connection, snap::snap_communicator_message const & message);
    void                        process_connected(snap::snap_communicator::snap_connection::pointer_t connection);
    void                        broadcast_message(snap::snap_communicator_message const & message);

private:
    void                        refresh_heard_of();
    void                        shutdown(bool full);

    snap::server::pointer_t                             f_server;

    QString                                             f_server_name;
    snap::snap_communicator::pointer_t                  f_communicator;
    snap::snap_communicator::snap_connection::pointer_t f_local_listener;   // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_listener;         // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_ping;             // UDP/IP
    QString                                             f_server_types;
    snap_addr::addr                                     f_my_address;
    QString                                             f_local_services;
    sorted_list_of_strings_t                            f_local_services_list;
    QString                                             f_services_heard_of;
    sorted_list_of_strings_t                            f_services_heard_of_list;
    QString                                             f_explicit_neighbors;
    sorted_list_of_strings_t                            f_all_neighbors;
    remote_communicator_connections::pointer_t          f_remote_snapcommunicators;
    size_t                                              f_max_connections = SNAP_COMMUNICATOR_MAX_CONNECTIONS;
    bool                                                f_shutdown = false;
    snap::snap_communicator_message::vector_t           f_local_message_cache;
    std::map<QString, time_t>                           f_received_broadcast_messages;
};



class base_connection
{
public:
    typedef std::shared_ptr<base_connection>    pointer_t;


    /** \brief Initialize the base_connection() object.
     *
     * The constructor saves the communicator server pointer
     * so one can access it from any derived version.
     */
    base_connection(snap_communicator_server::pointer_t cs)
        : f_communicator_server(cs)
    {
    }


    /** \brief We have a destructor to make it virtual.
     *
     * Everything is otherwise automatically released.
     */
    virtual ~base_connection()
    {
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
     * or the destruction of the service_connection object occurred. It
     * represents the time when the specific service was shutdown.
     *
     * \return The approximative date when the connection ended in microseconds.
     */
    int64_t get_connection_ended() const
    {
        return f_ended_on;
    }


    /** \brief Save the name of the server.
     *
     * \param[in] server_name  The name of the server that is on the other
     *                         side of this connection.
     */
    void set_server_name(QString const & server_name)
    {
        f_server_name = server_name;
    }


    /** \brief Get the name of the server.
     *
     * \param[in] server_name  The name of the server that is on the other
     *                         side of this connection.
     */
    QString get_server_name() const
    {
        return f_server_name;
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
        for(auto const & s : list)
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


    /** \brief Check whether the service is known by that connection.
     *
     * This function returns true if the service was defined as one
     * this connection supports.
     *
     * \param[in] name  The name of the service to check for.
     *
     * \return true if the service is known.
     */
    bool has_service(QString const & name)
    {
        return f_services.contains(name);
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
        for(auto const & s : list)
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
        for(auto const & c : cmds)
        {
            QString const name(c.trimmed());
            if(!name.isEmpty())
            {
                f_understood_commands[name] = true;
            }
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


    /** \brief Check whether this connection received the COMMANDS message.
     *
     * This function returns true if the list of understood commands is
     * defined. This means we do know whether a verification (i.e. a call
     * to the understand_command() function) will return false because the
     * list of commands is empty or because a command is not understood.
     *
     * \return true if one or more commands are understood.
     */
    bool has_commands() const
    {
        return !f_understood_commands.empty();
    }


    /** \brief Remove a command.
     *
     * This function is used to make the system think that certain command
     * are actually not understood.
     *
     * At this time, it is only used when a connection goes away and we
     * want to send a STATUS message to various services interested in
     * such a message.
     *
     * \param[in] command  The command to remove.
     */
    void remove_command(QString const & command)
    {
        f_understood_commands.remove(command);
    }


    /** \brief Mark that connection as a remote connection.
     *
     * When we receive a connection from another snapconnector, we call
     * this function so later we can very quickly determine whether the
     * connection is a remote connection.
     */
    void mark_as_remote()
    {
        f_remote_connection = true;
    }


    /** \brief Check whether this connection is a remote connection.
     *
     * The function returns false by default. If the mark_as_remote()
     * was called, this function returns true.
     *
     * \return true if the connection was marked as a remote connection.
     */
    bool is_remote() const
    {
        return f_remote_connection;
    }


protected:
    snap_communicator_server::pointer_t     f_communicator_server;

private:
    sorted_list_of_strings_t                f_understood_commands;
    int64_t                                 f_started_on = -1;
    int64_t                                 f_ended_on = -1;
    QString                                 f_types;
    QString                                 f_server_name;
    sorted_list_of_strings_t                f_services;
    sorted_list_of_strings_t                f_services_heard_of;
    bool                                    f_remote_connection = false;
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
    : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
    , public base_connection
{
public:
                                    remote_snap_communicator(snap_communicator_server::pointer_t cs, QString const & addr, int port);
    virtual                         ~remote_snap_communicator();

    // snap_connection implementation
    virtual void                    process_message(snap::snap_communicator_message const & message);

    // snap_tcp_client_permanent_message_connection implementation
    virtual void                    process_connection_failed(std::string const & error_message);
    virtual void                    process_connected();

    snap_addr::addr const &         get_address() const;

private:
    snap_addr::addr                 f_address;
};



/** \brief To send a GOSSIP to a remote snapcommunicator.
 *
 * This class defines a connection used to send a GOSSIP message
 * to a remote communicator. Once the GOSSIP worked at least once,
 * this connection gets deleted.
 *
 * This connection is a timer, it manages an actual TCP/IP connection
 * which it attempts to create every now and then. This is because
 * we do not want to use too many resources to attempt to connect
 * to a computer which is down. (i.e. we use a thread to attempt
 * the connection since it can take forever if it does not work; i.e.
 * inter-computer socket connections may timeout after a minute or
 * two!)
 *
 * For the feat we use our 'permanent message connection.' This is
 * very well adapted. We just need to make sure to remove the
 * connection once we received confirmation the the GOSSIP message
 * was received by the remote host.
 */
class gossip_to_remote_snap_communicator
    : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    std::shared_ptr<gossip_to_remote_snap_communicator> pointer_t;

    static int64_t const        FIRST_TIMEOUT = 5LL * 1000000L;  // 5 seconds before first attempt

                                gossip_to_remote_snap_communicator(remote_communicator_connections::pointer_t rcs, QString const & addr, int port);

    // snap_connection implementation
    virtual void                process_timeout();

    // snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message);
    virtual void                process_connected();

    void                        kill();

private:
    QString const               f_addr;
    int const                   f_port = 0;
    int64_t                     f_wait = FIRST_TIMEOUT;
    remote_communicator_connections::pointer_t f_remote_communicators;
};


/** \brief Initialize the gossip remote communicator connection.
 *
 * This object is actually a timer. Each time we get a tick
 * (i.e. process_timeout() callback gets called), a connection
 * is attempted against the remote snapcommunicator daemon
 * specified by the addr and port parameters.
 *
 * The addr and port are both mandatory to this constructor.
 *
 * \param[in] cs  The snap communicator server object which we contact
 *                whenever the GOSSIP message was confirmed by the
 *                remote connection.
 * \param[in] addr  The IP address of the remote snap communicator.
 * \param[in] port  The port to connect to that snap communicator.
 */
gossip_to_remote_snap_communicator::gossip_to_remote_snap_communicator(remote_communicator_connections::pointer_t rcs, QString const & addr, int port)
    : snap_tcp_client_permanent_message_connection(
                addr.toUtf8().data(),
                port,
                tcp_client_server::bio_client::mode_t::MODE_PLAIN,
                -FIRST_TIMEOUT,  // must be negative so first timeout is active (otherwise we get an immediately attempt, which we do not want in this case)
                true)
    , f_addr(addr)
    , f_port(port)
    //, f_wait(FIRST_TIMEOUT) -- auto-init
    , f_remote_communicators(rcs)
{
}


/** \brief Process one timeout.
 *
 * We do not really have anything to do when a timeout happens. The
 * connection attempts are automatically done by the permanent
 * connection in the snap_communicator library.
 *
 * However, we want to increase the delay between attempts. For that,
 * we use this function and double the delay on each timeout until
 * it reaches about 1h. Then we stop doubling that delay. If the
 * remote snapcommunicator never makes it, we won't swamp the network
 * by false attempts to connect to a dead computer.
 *
 * \todo
 * We need to let the snapwatchdog know that such remote connections
 * fail for X amount of time. This is important to track what's
 * missing in the cluster (Even if we likely will have other means
 * to know of the problem.)
 */
void gossip_to_remote_snap_communicator::process_timeout()
{
    snap_tcp_client_permanent_message_connection::process_timeout();

    // increase the delay on each timeout until we reach 1h and then
    // repeat every 1h or so (i.e. if you change the FIRST_TIMEOUT
    // you may not reach exactly 1h here, also the time it takes
    // to try to connect is added to the delay each time.)
    //
    if(f_wait < 3600LL * 1000000LL)
    {
        f_wait *= 2;
        set_timeout_delay(f_wait);
    }
}


/** \brief Process the reply from our GOSSIP message.
 *
 * This function processes any messages received from the remote
 * system.
 *
 * We currently really only expect RECEIVED as a reply.
 *
 * \param[in] message  The message received from the remote snapcommunicator.
 */
void gossip_to_remote_snap_communicator::process_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("gossip connection received a message [")(message.to_message())("]");

    QString const & command(message.get_command());
    if(command == "RECEIVED")
    {
        // we got confirmation that the GOSSIP went across
        //
        f_remote_communicators->gossip_received(f_addr);
    }
}


/** \brief Once connected send the GOSSIP message.
 *
 * This function gets called whenever the connection is finally up.
 * This gives us the opportunity to send the GOSSIP message to the
 * remote host.
 *
 * Note that at this time this happens in the main thread. The
 * secondary thread was used to call the connect() function, but
 * it is not used to send or receive any messages.
 */
void gossip_to_remote_snap_communicator::process_connected()
{
    // TODO:
    // The default process_connected() function disables the timer
    // of the gossip connection. This means that we will not get
    // any further process_timeout() calls until we completely
    // lose the connection. This is possibly not what we want, or
    // at least we should let the snapwatchdog know that we were
    // connected to a snapcommunicator, yes, sent the GOSSIP,
    // all good up to here, but never got a reply! Not getting
    // a reply is likely to mean that the connection we establish
    // is somehow bogus even if it does not Hang Up on us.
    //
    // You may read the Byzantine fault tolerance in regard to
    // supporting a varied set of processes to detect the health
    // of many different nodes in a cluster.
    //
    // https://en.wikipedia.org/wiki/Byzantine_fault_tolerance
    //
    snap_tcp_client_permanent_message_connection::process_connected();

    // we are connected so we can send the GOSSIP message
    // (each time we reconnect!)
    //
    snap::snap_communicator_message gossip;
    gossip.set_command("GOSSIP");
    gossip.add_parameter("my_address", f_remote_communicators->get_my_address());
    send_message(gossip); // do not cache, if we lose the connection, we lose the message and that's fine in this case
}










remote_communicator_connections::remote_communicator_connections(snap_communicator_server::pointer_t communicator_server, snap_addr::addr const & my_addr)
    : f_communicator_server(communicator_server)
    , f_my_address(my_addr)
{
}


QString remote_communicator_connections::get_my_address() const
{
    return f_my_address.get_ipv4or6_string(true).c_str();
}


void remote_communicator_connections::add_remote_communicator(QString const & addr_port)
{
    SNAP_LOG_DEBUG("adding remote communicator at ")(addr_port);

    // no default address for neighbors
    snap_addr::addr remote_addr(addr_port.toUtf8().data(), "", 4040, "tcp");

    if(remote_addr == f_my_address)
    {
        // TBD: this may be normal (i.e. neighbors should send us our IP
        //      right back to us!)
        //
        SNAP_LOG_WARNING("address of remote snapcommunicator, \"")(addr_port)("\", is the same as my address, which means it is not remote.");
        return;
    }

    QString const addr(remote_addr.get_ipv4or6_string().c_str());
    int const port(remote_addr.get_port());

    // keep a copy of all addresses
    //
    if(f_all_ips.contains(addr))
    {
        if(f_started)
        {
            if(remote_addr < f_my_address)
            {
                // make sure it is defined!
                if(f_smaller_ips[addr])
                {
                    f_smaller_ips[addr]->set_timeout_date(snap::snap_communicator::get_current_date());
                }
                else
                {
                    SNAP_LOG_ERROR("smaller remote address is defined in f_all_ips but not in f_smaller_ips?");
                }
            }
            // else -- do we have to GOSSIP about this one? (see below)
            return;
        }

        // TBD: this may be normal (i.e. each neighbor should send us the
        //      same list of IP addresses.)
        //
        SNAP_LOG_ERROR("address of remote snapcommunicator, \"")(addr_port)("\", already exists.");
        return;
    }
    f_all_ips[addr] = port;

    // if this new IP is smaller than ours, then we start a connection
    //
    if(remote_addr < f_my_address)
    {
        // smaller connections are created as remote snap communicator
        // which are permanent message connections
        //
        f_smaller_ips[addr].reset(new remote_snap_communicator(f_communicator_server, addr, port));
        f_smaller_ips[addr]->set_name("remote communicator connection"); // we connect to remote host
        if(f_started)
        {
            // we already started (i.e. we got the complete list of all our
            // services up and going) so new remote communicator connections
            // are immediately started (see the start() function for more
            // details...)
            //
            // Note: the remote_snap_communicator() constructor calls
            //       set_enable(false)
            //
            f_smaller_ips[addr]->set_enable(true);
        }

        if(!snap::snap_communicator::instance()->add_connection(f_smaller_ips[addr]))
        {
            // this should never happens here since each new creates a
            // new pointer
            //
            // TBD: should we lose that connection from the f_smaller_ips
            //      map since it is not going to be used?
            //
            SNAP_LOG_ERROR("new remote connection could not be added to the snap_communicator list of connections");
        }
    }
    else //if(remote_addr != f_my_address) -- already tested at the beginning of the function
    {
        // in case the remote snapcommunicator has a larger address
        // it is expected to CONNECT to us; however, it may not yet
        // know about us so we want to send a GOSSIP message; this
        // means creating a special connection which attempts to
        // send the GOSSIP message up until it succeeds or the
        // application quits
        //
        f_gossip_ips[addr].reset(new gossip_to_remote_snap_communicator(shared_from_this(), addr, port));
        f_gossip_ips[addr]->set_name("gossip to remote snap communicator");

        if(!snap::snap_communicator::instance()->add_connection(f_gossip_ips[addr]))
        {
            // this should never happens here since each new creates a
            // new pointer
            //
            // TBD: should we lose that connection from the f_gossip_ips
            //      map since it is not going to be used?
            //
            SNAP_LOG_ERROR("new gossip connection could not be added to the snap_communicator list of connections");
        }
    }
}


/** \brief Stop all gossiping at once.
 *
 * This function can be called to remove all the gossip connections
 * at once.
 *
 * In most cases this function is called whenever the snapcommunicator
 * daemon receives a STOP or a SHUTDOWN.
 *
 * Also these connections do not support any other messages than the
 * GOSSIP and RECEIVED.
 */
void remote_communicator_connections::stop_gossiping()
{
    while(!f_gossip_ips.empty())
    {
        snap::snap_communicator::instance()->remove_connection(*f_gossip_ips.begin());
        f_gossip_ips.erase(f_gossip_ips.begin());
    }
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
void remote_communicator_connections::too_busy(QString const & addr)
{
    if(f_smaller_ips.contains(addr))
    {
        // wait for 1 day and try again (is 1 day too long?)
        f_smaller_ips[addr]->set_timeout_delay(24LL * 60LL * 60LL * 1000000LL);
    }
}


void remote_communicator_connections::start()
{
    // make sure we start only once
    //
    if(!f_started)
    {
        f_started = true;

        int64_t start_time(snap::snap_child::get_current_date());
        for(auto const & communicator : f_smaller_ips)
        {
            communicator->set_timeout_date(start_time);
            communicator->set_enable(true);

            // XXX: with 8,000 computers in a cluster, this represents
            //      a period of time of 2h 14m to get all the
            //      connections ready...
            //
            start_time += 1000000;
        }
    }
}


void remote_communicator_connections::gossip_received(QString const & addr)
{
    auto it(f_gossip_ips.find(addr));
    if(it != f_gossip_ips.end())
    {
        snap::snap_communicator::instance()->remove_connection(*it);
        f_gossip_ips.erase(it);
    }
}







/** \brief Listen for messages.
 *
 * The snapcommunicator TCP connection simply listen for process_message()
 * callbacks and processes those messages by calling the process_message()
 * of the connections class.
 *
 * It also listens for disconnections so it can send a new STATUS command
 * whenever the connection goes down.
 */
class service_connection
        : public snap::snap_communicator::snap_tcp_server_client_message_connection
        , public base_connection
{
public:
    typedef std::shared_ptr<service_connection>    pointer_t;


    /** \brief Create a service connection and assigns \p socket to it.
     *
     * The constructor of the service connection expects a socket that
     * was just accept()'ed.
     *
     * The snapcommunicator daemon listens on to two different ports
     * and two different addresses on those ports:
     *
     * \li TCP 127.0.0.1:4040 -- this address is expected to be used by all the
     * local services
     *
     * \li TCP 0.0.0.0:4040 -- this address is expected to be used by remote
     * snapcommunicators; it is often changed to a private network IP
     * address such as 192.168.0.1 to increase safety. However, if your
     * cluster spans multiple data centers, it will not be possible to
     * use a private network IP address.
     *
     * \li UDP 127.0.0.1:4041 -- this special port is used to accept UDP
     * signals sent to the snapcommunicator; UDP signals are most often
     * used to very quickly send signals without having to have a full
     * TCP connection to a daemon
     *
     * The connections happen on 127.0.0.1 are fully trusted. Connections
     * happening on 0.0.0.0 are generally viewed as tainted.
     *
     * \param[in] cs  The communicator server (i.e. parent)
     * \param[in] socket  The socket that was just created by the accept()
     *                    command.
     * \param[in] server_name  The name of the server we are running on
     *                         (i.e. generally your hostname.)
     */
    service_connection(snap_communicator_server::pointer_t cs, int socket, QString const & server_name)
        : snap_tcp_server_client_message_connection(socket)
        , base_connection(cs)
        , f_server_name(server_name)
        //, f_address(get_client_addr_port(), "tcp")   // TODO: see if we could instead have a get address which returns a sockaddr_in[6] or even have the snap_addr::addr in our tcp_client_server classes
        , f_address(get_remote_address().toUtf8().data(), "tcp")  // this is the address:port of the peer (the computer on the other side)
    {
    }


    /** \brief Connection lost.
     *
     * When a connection goes down it gets deleted. This is when we can
     * send a new STATUS event to all the other STATUS hungry connections.
     */
    ~service_connection()
    {
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
        remove_command("STATUS");

        // now ask the server to send a new STATUS to all connections
        // that understand that message; we pass our pointer since we
        // want to send the info about this connection in that STATUS
        // message
        //
        // TODO: we cannot use shared_from_this() in the destructor,
        //       it's too late since when we reach here the pointer
        //       was already destroyed so we get a bad_weak_ptr
        //       exception; we need to find a different way if we
        //       want this event to be noticed and a STATUS sent...
        //
        //f_communicator_server->send_status(shared_from_this());
    }


    // snap::snap_communicator::snap_tcp_server_client_message_connection implementation
    virtual void process_message(snap::snap_communicator_message const & message)
    {
        // make sure the destination knows who sent that message so it
        // is possible to directly reply to that specific instance of
        // a service
        //
        if(f_named)
        {
            snap::snap_communicator_message forward_message(message);
            forward_message.set_sent_from_server(f_server_name);
            forward_message.set_sent_from_service(get_name());
            f_communicator_server->process_message(shared_from_this(), forward_message, false);
        }
        else
        {
            f_communicator_server->process_message(shared_from_this(), message, false);
        }
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


    /** \brief Process a hang up.
     *
     * It is important for some processes to know when a remote connection
     * is lost (i.e. for dynamic QUORUM calculations in snaplock, for
     * example.) So we handle the process_hup() event and send a
     * DISCONNECTED if this connection is a remote connection.
     */
    virtual void process_hup()
    {
        snap_tcp_server_client_message_connection::process_hup();

        if(is_remote()
        && !get_server_name().isEmpty())
        {
            snap::snap_communicator_message disconnected;
            disconnected.set_command("DISCONNECTED");
            disconnected.set_service(".");
            disconnected.add_parameter("server_name", get_server_name());
            f_communicator_server->broadcast_message(disconnected);
        }
    }


    /** \brief Tell that the connection was given a real name.
     *
     * Whenever we receive an event through this connection,
     * we want to mark the message as received from the service.
     *
     * However, by default the name of the service is on purpose
     * set to an "invalid value" (i.e. a name with a space.) That
     * value is not expected to be used when forwarding the message
     * to another service.
     *
     * Once a system properly registers with the REGISTER message,
     * we receive a valid name then. That name is saved in the
     * connection and the connection is marked as having a valid
     * name.
     *
     * This very function must be called once the proper name was
     * set in this connection.
     */
    void properly_named()
    {
        f_named = true;
    }


    /** \brief Return the type of address this connection has.
     *
     * This function determines the type of address of the connection.
     *
     * \return A reference to the remote address of this connection.
     */
    snap_addr::addr const & get_address() const
    {
        return f_address;
    }


private:
    QString const               f_server_name;
    snap_addr::addr             f_address;
    bool                        f_named = false;
};







/** \brief Handle new connections from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */
class listener
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
     * \param[in] local  Whether this connection expects local services only.
     * \param[in] server_name  The name of the server running this instance.
     */
    listener(snap_communicator_server::pointer_t cs, std::string const & addr, int port, int max_connections, bool local, QString const & server_name)
        : snap_tcp_server_connection(addr, port, max_connections, true, false)
        , f_communicator_server(cs)
        , f_local(local)
        , f_server_name(server_name)
    {
    }

    // snap::snap_communicator::snap_server_connection implementation
    virtual void process_accept()
    {
        // a new client just connected, create a new service_connection
        // object and add it to the snap_communicator object.
        //
        int const new_socket(accept());
        if(new_socket < 0)
        {
            // an error occurred, report in the logs
            int const e(errno);
            SNAP_LOG_ERROR("somehow accept() failed with errno: ")(e)(" -- ")(strerror(e));
            return;
        }

        service_connection::pointer_t connection(new service_connection(f_communicator_server, new_socket, f_server_name));

        // TBD: is that a really weak test?
        //
        // XXX: add support for IPv6
        //
        std::string const addr(connection->get_client_addr());
        if(f_local)
        {
            if(addr != "127.0.0.1")
            {
                // TODO: find out why we do not get 127.0.0.1 when using such to connect...
                SNAP_LOG_WARNING("received what should be a local connection from \"")(addr)("\".");
                //return;
            }

            // set a default name in each new connection, this changes
            // whenever we receive a REGISTER message from that connection
            //
            connection->set_name("client connection");

            connection->set_server_name(f_server_name);
        }
        else
        {
            if(addr == "127.0.0.1")
            {
                SNAP_LOG_ERROR("received what should be a remote connection from 127.0.0.1");
                return;
            }

            // set a name for remote connections
            //
            // these names are not changed, if we want to do so, we could
            // whenever we receive the CONNECT message and use the name of
            // the server that connected
            //
            connection->set_name("remote connection"); // remote host connected to us
            connection->mark_as_remote();
        }

        if(!snap::snap_communicator::instance()->add_connection(connection))
        {
            // this should never happen here since each new creates a
            // new pointer
            //
            SNAP_LOG_ERROR("new client connection could not be added to the snap_communicator list of connections");
        }
    }

private:
    snap_communicator_server::pointer_t     f_communicator_server;
    bool const                              f_local = false;
    QString const                           f_server_name;
};








/** \brief Handle UDP messages from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */
class ping_impl
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
    ping_impl(snap_communicator_server::pointer_t cs, std::string const & addr, int port)
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
    // keep a copy of the server name handy
    f_server_name = f_server->get_parameter("server_name");

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

    int max_pending_connections(10);
    {
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
    }

    // create two listeners, for new arriving TCP/IP connections
    //
    // one listener is used to listen for local services which have to
    // connect using the 127.0.0.1 IP address
    //
    // the other listener listens to your local network and accepts
    // connections from other snapcommunicator servers
    //
    // local
    {
        // TODO: convert to use the 'addr' class instead
        QString addr("127.0.0.1");
        int port(4040);
        tcp_client_server::get_addr_port(f_server->get_parameter("local_listen"), addr, port, "tcp");
        if(addr != "127.0.0.1")
        {
            SNAP_LOG_FATAL("The local_listen parameter must have 127.0.0.1 as the IP address. %1 is not acceptable.")(addr);
            f_server->exit(1);
        }

        // make this listener the local listener
        //
        f_local_listener.reset(new listener(shared_from_this(), addr.toUtf8().data(), port, max_pending_connections, true, f_server_name));
        f_local_listener->set_name("snap communicator local listener");
        f_communicator->add_connection(f_local_listener);
    }
    // remote
    QString const listen_str(f_server->get_parameter("listen"));
    snap_addr::addr listen_addr(listen_str.toUtf8().data(), "0.0.0.0", 4040, "tcp");
    {
        // make this listener the remote listener, however, if the IP
        // address is 127.0.0.1 we skip on this one, we do not need
        // two listener on the local IP address
        //
        if(listen_addr.get_network_type() != snap_addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK)
        {
            f_listener.reset(new listener(shared_from_this(), listen_addr.get_ipv4or6_string(), listen_addr.get_port(), max_pending_connections, false, f_server_name));
            f_listener->set_name("snap communicator listener");
            f_communicator->add_connection(f_listener);
        }
        else
        {
            SNAP_LOG_WARNING("remote \"listen\" parameter is \"")(listen_str)("\" so it is ignored and no remote connections will be possible.");
        }
    }

    {
        QString addr("127.0.0.1"); // this default should work just fine
        int port(4041);
        tcp_client_server::get_addr_port(f_server->get_parameter("signal"), addr, port, "tcp");

        f_ping.reset(new ping_impl(shared_from_this(), addr.toUtf8().data(), port));
        f_ping->set_name("snap communicator messager (UDP)");
        f_communicator->add_connection(f_ping);
    }

    // transform the my_address to a snap_addr::addr object
    //
    f_my_address = snap_addr::addr(f_server->get_parameter("my_address").toUtf8().data(), "", listen_addr.get_port(), "tcp");
    snap_addr::addr::computer_interface_address_t cia(f_my_address.is_computer_interface_address());
    if(cia == snap_addr::addr::computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_ERROR)
    {
        int const e(errno);
        SNAP_LOG_ERROR("somehow getifaddrs() failed with errno: ")(e)(" -- ")(strerror(e));
        // we go on anyway...
    }
    else if(cia != snap_addr::addr::computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_TRUE)
    {
        std::string const addr(f_my_address.get_ipv6_string());
        SNAP_LOG_FATAL("my_address \"")(addr)("\" not found on this computer. Did a copy of the configuration file and forgot to change that entry?");
        throw snap::snap_exception(QString("my_address \"%1\" not found on this computer. Did a copy of the configuration file and forgot to change that entry?.").arg(QString::fromUtf8(addr.c_str())));
    }

    f_remote_snapcommunicators.reset(new remote_communicator_connections(shared_from_this(), f_my_address));

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
        f_remote_snapcommunicators->add_remote_communicator(it.key());
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


/** \brief Make sure that the connection understands a command.
 *
 * This function checks whether the specified connection (\p connection)
 * understands the command about to be sent to it (\p reply).
 *
 * \note
 * The test is done only when snapcommunicator is run in debug
 * mode to not waste time.
 *
 * \param[in,out] connection  The concerned connection that has to understand the command.
 * \param[in] message  The message about to be sent to \p connection.
 */
void snap_communicator_server::verify_command(base_connection::pointer_t connection, snap::snap_communicator_message const & message)
{
    // debug turned on?
    if(!f_server->is_debug())
    {
        // nope, do not waste any more time
        return;
    }

    if(!connection->has_commands())
    {
        // if we did not yet receive the COMMANDS message then we cannot
        // pretend that the understand_command() will return a sensible
        // result, so ignore that test...
        //
        return;
    }

    if(connection->understand_command(message.get_command()))
    {
        // all good, the command is implemented
        //
        return;
    }

    // if you get this message, it could be that you do implement
    // the command, but do not advertise it in your COMMANDS
    // reply to the HELP message sent by snapcommunicator
    //
    snap::snap_communicator::snap_connection::pointer_t c(std::dynamic_pointer_cast<snap::snap_communicator::snap_connection>(connection));
    if(c)
    {
        SNAP_LOG_FATAL("connection \"")(c->get_name())("\" does not understand ")(message.get_command())(".");
        throw snap::snap_exception(QString("Connection \"%1\" does not implement command \"%2\".").arg(c->get_name()).arg(message.get_command()));
    }

    SNAP_LOG_FATAL("connection does not understand ")(message.get_command())(".");
    throw snap::snap_exception(QString("Connection does not implement command \"%1\".").arg(message.get_command()));
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
    {
        QString const received_message(message.to_message());
        //if(received_message.indexOf(":snaplock ") == -1
        //&& received_message.indexOf(":lock_") == -1)
        {
            SNAP_LOG_TRACE("received a message [")(received_message)("]");
        }
    }

    QString const command(message.get_command());

    base_connection::pointer_t base(std::dynamic_pointer_cast<base_connection>(connection));
    remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(connection));
    service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));

    // TODO: move all the command bodies to sub-funtions.

    // check who this message is for
    QString const service(message.get_service());
    if(service.isEmpty()
    || service == "snapcommunicator")
    {
        if(f_shutdown)
        {
            // if the user sent us an UNREGISTER we should not generate a
            // QUITTING because the UNREGISTER is in reply to our STOP
            // TBD: we may want to implement the UNREGISTER in this
            //      situation?
            //
            if(!udp)
            {
                if(command != "UNREGISTER")
                {
                    // we are shutting down so just send a quick QUTTING reply
                    // letting the other process know about it
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("QUITTING");

                    verify_command(base, reply);
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception(QString("message \"%1\" sent on a \"weird\" connection.").arg(command));
                    }
                }

                // get rid of that connection now, we don't need any more
                // messages coming from it
                //
                f_communicator->remove_connection(connection);
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

                if(base)
                {
                    // the type is mandatory in an ACCEPT message
                    //
                    if(!message.has_parameter("types")
                    || !message.has_parameter("server_name"))
                    {
                        SNAP_LOG_ERROR("ACCEPT was received without a \"types\" or \"server_name\" parameter, which are mandatory.");
                        return;
                    }
                    base->set_connection_types(canonicalize_server_types(message.get_parameter("types")));
                    base->set_server_name(message.get_parameter("server_name"));

                    // reply to a CONNECT, this was to connect to another
                    // snapcommunicator on another computer, retrieve the
                    // data from that remote computer
                    //
                    base->connection_started();

                    if(message.has_parameter("services"))
                    {
                        base->set_services(message.get_parameter("services"));
                    }
                    if(message.has_parameter("heard_of"))
                    {
                        base->set_services_heard_of(message.get_parameter("heard_of"));
                    }
                    if(message.has_parameter("neighbors"))
                    {
                        add_neighbors(message.get_parameter("neighbors"));
                    }

                    // we just got some new services information,
                    // refresh our cache
                    //
                    refresh_heard_of();

                    // also request the COMMANDS of this connection
                    //
                    snap::snap_communicator_message help;
                    help.set_command("HELP");
                    //verify_command(base, help); -- precisely
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(help);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(help);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception(QString("message \"%1\" sent on a \"weird\" connection.").arg(command));
                    }
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

                if(base)
                {
                    if(message.has_parameter("list"))
                    {
                        base->set_commands(message.get_parameter("list"));

                        // here we verify that a few commands are properly
                        // defined, for some becausesince we already sent
                        // them to that connection and thus it should
                        // understand them; and a few more that are very
                        // possibly going to be sent
                        //
                        if(f_server->is_debug())
                        {
                            bool ok(true);
                            if(!base->understand_command("HELP"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand HELP.");
                                ok = false;
                            }
                            if(!base->understand_command("QUITTING"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand QUITTING.");
                                ok = false;
                            }
                            // on a remote we get ACCEPT instead of READY
                            if(remote_communicator
                            || base->is_remote())
                            {
                                if(!base->understand_command("ACCEPT"))
                                {
                                    SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand ACCEPT.");
                                    ok = false;
                                }
                            }
                            else
                            {
                                if(!base->understand_command("READY"))
                                {
                                    SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand READY.");
                                    ok = false;
                                }
                            }
                            if(!base->understand_command("STOP"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand STOP.");
                                ok = false;
                            }
                            if(!base->understand_command("UNKNOWN"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand UNKNOWN.");
                                ok = false;
                            }
                            if(!ok)
                            {
                                // end the process so developers can fix their
                                // problems (this is only if --debug was
                                // specified)
                                //
                                throw snap::snap_exception(QString("Connection %1 does not implement some required commands.").arg(connection->get_name()));
                            }
                        }
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

                if(base)
                {
                    // first we verify that we have a valid version to
                    // communicate between two snapcommunicators
                    //
                    if(!message.has_parameter("types")
                    || !message.has_parameter("version")
                    || !message.has_parameter("my_address")
                    || !message.has_parameter("server_name"))
                    {
                        SNAP_LOG_ERROR("CONNECT was sent without a \"types\", \"version\", or \"my_address\" parameter, both are mandatory.");
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

                    // always retrieve the connection types
                    //
                    base->set_connection_types(canonicalize_server_types(message.get_parameter("types")));
                    base->set_server_name(message.get_parameter("server_name"));

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
                        base->connection_started();

                        if(message.has_parameter("services"))
                        {
                            base->set_services(message.get_parameter("services"));
                        }
                        if(message.has_parameter("heard_of"))
                        {
                            base->set_services_heard_of(message.get_parameter("heard_of"));
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
                        reply.add_parameter("server_name", f_server_name);

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

                        QString const his_address(message.get_parameter("my_address"));

                        // he is a neighbor too, make sure to add it
                        // in our list of neighbors (useful on a restart
                        // to connect quickly)
                        //
                        add_neighbors(his_address);

                        // since we are accepting a CONNECT we have to make
                        // sure we cancel the GOSSIP events to that remote
                        // connection; it won't hurt, but it is a waste if
                        // we do not need it
                        //
                        // Note: the name of the function is "GOSSIP"
                        //       received because if the "RECEIVED"
                        //       message was sent back from that remote
                        //       snapcommunicator then it means that
                        //       remote daemon received our GOSSIP message
                        //       and receiving the "CONNECT" message is
                        //       very similar to receiving the "RECEIVED"
                        //       message after a "GOSSIP"
                        //
                        f_remote_snapcommunicators->gossip_received(his_address);
                    }

                    //verify_command(base, reply); -- we do not yet have a list of commands understood by the other snapcommunicator daemon

                    // also request the COMMANDS of this connection
                    //
                    snap::snap_communicator_message help;
                    help.set_command("HELP");
                    //verify_command(base, help); -- precisely
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                        remote_communicator->send_message(help);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                        service_conn->send_message(help);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("CONNECT sent on a \"weird\" connection.");
                    }

                    // status changed for this connection
                    //
                    send_status(connection);
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

                if(base)
                {
                    base->connection_ended();

                    // this has to be another snapcommunicator
                    // (i.e. an object that sent ACCEPT or CONNECT)
                    //
                    QString const types(base->get_connection_types());
                    if(!types.isEmpty()
                    && types != "client")
                    {
                        // we must ignore and we do ignore connections with a
                        // type of "" since they represent an uninitialized
                        // connection item (unconnected)
                        //
                        base->set_connection_types("");

                        if(!remote_communicator)
                        {
                            // disconnecting means it is gone so we can remove
                            // it from the communicator
                            //
                            // Note: this one happens when the computer that
                            //       sent us a CONNECT later sends us the
                            //       DISCONNECT
                            //
                            f_communicator->remove_connection(connection);
                        }
                        //else -- in this case we are in charge of attempting
                        //        to reconnect until it work...

                        // we just got some new services information,
                        // refresh our cache
                        //
                        refresh_heard_of();

                        if(!base->get_server_name().isEmpty())
                        {
                            snap::snap_communicator_message disconnected;
                            disconnected.set_command("DISCONNECTED");
                            disconnected.set_service(".");
                            disconnected.add_parameter("server_name", base->get_server_name());
                            broadcast_message(disconnected);
                        }
                    }
                    else
                    {
                        SNAP_LOG_ERROR("DISCONNECT was sent from a connection which is not of the right type (")(types)(").");
                    }
                    return;
                }
            }
            break;

        case 'G':
            if(command == "GOSSIP")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("GOSSIP is only accepted over a TCP connection.");
                    break;
                }

                if(base)
                {
                    // we got a GOSSIP message, this one will have addresses
                    // with various neighbors; we have two modes:
                    //
                    // 1) my_address=... is defined -- in this case the
                    //    remote host sent us his address because he was
                    //    not sure whether we knew about him; add that
                    //    address as a neighbor and go on as normal
                    //
                    // 2) heard_of=... is defined -- in this case, the
                    //    remote host received a GOSSIP from any one
                    //    snapcommunicator and it is propagating the
                    //    message; check all the IPs in that list and
                    //    if all are present in our list of neighbors,
                    //    do nothing; if all are not present, proceed
                    //    as normal in regard to attempt connections
                    //    and also forward our own GOSSIP to others
                    //    since we just heard of some new neighbors!
                    //
                    //    Note that at this point we use the Flooding
                    //    scheme and we implemented the Eventual
                    //    Consistency (because at some point in time
                    //    we eventually have an exact result.)
                    //
                    // When using (2) we are using what is called
                    // Gossiping in Computer Science. At thist time
                    // we use what is called the Flooding Algorithm.
                    //
                    // https://en.wikipedia.org/wiki/Flooding_(computer_networking)
                    //
                    // See also doc/focs2003-gossip.pdf
                    //
                    // We add two important features: (a) the list of
                    // nodes we already sent the message to, in
                    // order to avoid sending it to the same node
                    // over and over again; and (b) a serial number
                    // to be able to identify the message.
                    //
                    // Two other features that could be added are:
                    // (c) counting hops, after X hops were reached,
                    // stop forwarding the message because we should
                    // already have reached all nodes; (d) a specific
                    // date when the message times out.
                    //
                    // The serial number is used to know whether we
                    // already received a certain message. These can
                    // expire after a while (we may actually want to
                    // implement (d) from the get go so we know
                    // exactly when such expires).
                    //
                    // Our GOSSIP has one advantage, it is used to
                    // connect all the snapcommunicators together
                    // once. After that, the GOSSIP messages stop,
                    // no matter what (i.e. if a new snapcommunicator
                    // daemon is started, then the GOSSIP restart
                    // for that instance, but that's it.)
                    //
                    // However, we also offer a way to broadcast
                    // messages and these happen all the time
                    // (i.e. think of the snaplock broadcast messages).
                    // In those cases, we do not need to use the same
                    // algorithm because at that point we are expected
                    // to have a complete list of all the
                    // snapcommunicators available.
                    //
                    // (TODO: only we may not be connected to all of them,
                    // so we need to keep track of the snapcommunicators
                    // we are not connected to and ask others to do some
                    // forwarding!)
                    //
                    if(message.has_parameter("my_address"))
                    {
                        // this is a "simple" GOSSIP of a snapcommunicator
                        // telling us it exists and expects a connection
                        // from us
                        //
                        // in this case we just reply with RECEIVED to
                        // confirm that we get the GOSSIP message
                        //
                        QString const reply_to(message.get_parameter("my_address"));
                        add_neighbors(reply_to);
                        f_remote_snapcommunicators->add_remote_communicator(reply_to);

                        snap::snap_communicator_message reply;
                        reply.set_command("RECEIVED");
                        //verify_command(base, reply); -- in this case the remote snapcommunicator is not connected, so no HELP+COMMANDS and thus no verification possible
                        if(remote_communicator)
                        {
                            remote_communicator->send_message(reply);
                        }
                        else if(service_conn)
                        {
                            // Should this be an error instead since we only
                            // expect this message from remote snapcommunicators?
                            service_conn->send_message(reply);
                        }
                        else
                        {
                            // we have to have a remote or service connection here
                            //
                            throw snap::snap_exception("GOSSIP sent on a \"weird\" connection.");
                        }
                        return;
                    }
SNAP_LOG_ERROR("GOSSIP is not yet fully implemented.");
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

                if(base)
                {
                    // reply with COMMANDS
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("COMMANDS");

                    // list of commands understood by snapcommunicator
                    reply.add_parameter("list", "ACCEPT,COMMANDS,CONNECT,DISCONNECT,GOSSIP,HELP,LOG,QUITTING,REFUSE,REGISTER,SERVICES,SHUTDOWN,STOP,UNKNOWN,UNREGISTER");

                    //verify_command(base, reply); -- this verification does not work with remote snap communicator connections
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("HELP sent on a \"weird\" connection.");
                    }
                    return;
                }
            }
            break;

        case 'L':
            if(command == "LOG")
            {
                SNAP_LOG_INFO("Logging reconfiguration.");
                snap::logging::reconfigure();
                return;
            }
            break;

        case 'Q':
            if(command == "QUITTING")
            {
                // if this becomes problematic, we may need to serialize
                // our messages to know which was ignored...
                //
                SNAP_LOG_INFO("Received a QUITTING as a reply to a message.");
                return;
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

                // we were not connected so we do not have to
                // disconnect; mark that corresponding server
                // as too busy and try connecting again much
                // later...
                //
                QString addr;
                if(remote_communicator)
                {
                    addr = QString::fromUtf8(remote_communicator->get_client_addr().c_str());
                }
                else if(service_conn)
                {
                    addr = QString::fromUtf8(service_conn->get_client_addr().c_str());
                }
                else
                {
                    // we have to have a remote or service connection here
                    //
                    throw snap::snap_exception("REFUSE sent on a \"weird\" connection.");
                }
                f_remote_snapcommunicators->too_busy(addr);

                f_communicator->remove_connection(connection);
                return;
            }
            else if(command == "REGISTER")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("REGISTER is only accepted over a TCP connection.");
                    break;
                }

                if(base)
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
                    connection->set_name(service_name);
                    if(service_conn)
                    {
                        service_conn->properly_named();
                    }

                    base->set_connection_types("client");

                    // connection is up now
                    //
                    base->connection_started();

                    // tell the connect we are ready
                    // (the connection uses that as a trigger to start work)
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("READY");
                    //verify_command(base, reply); -- we cannot do that here since we did not yet get the COMMANDS reply
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);

                        // tell about the new service to those listening
                        //
                        snap::snap_communicator_message new_service;
                        new_service.set_service(".");
                        new_service.set_command("NEWSERVICE");
                        new_service.add_parameter("server", f_server_name);
                        new_service.add_parameter("service", service_name);
                        broadcast_message(new_service);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("REGISTER sent on a \"weird\" connection (1).");
                    }

                    // request the COMMANDS of this connection
                    //
                    snap::snap_communicator_message help;
                    help.set_command("HELP");
                    //verify_command(base, help); -- we cannot do that here since we did not yet get the COMMANDS reply
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(help);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(help);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("REGISTER sent on a \"weird\" connection (2).");
                    }

                    // status changed for this connection
                    //
                    send_status(connection);

                    // if we have local messages that were cached, then
                    // forward them now
                    //
                    // we use an index to make sure we can cleanly remove
                    // messages from the cache as we forward them to the
                    // new service
                    //
                    size_t max_messages(f_local_message_cache.size());
                    for(size_t idx(0); idx < max_messages; )
                    {
                        snap::snap_communicator_message const m(f_local_message_cache[idx]);
                        if(m.get_service() == service_name)
                        {
                            // whether it works, remove the message from
                            // the cache
                            //
                            f_local_message_cache.erase(f_local_message_cache.begin() + idx);
                            --max_messages;
                            // no ++idx since we removed the item at 'idx'

                            // TBD: should we remove the service name before forwarding?
                            //      (we have two instances)
                            //
                            //verify_command(base, m); -- we cannot do that here since we did not yet get the COMMANDS reply
                            if(remote_communicator)
                            {
                                remote_communicator->send_message(m);
                            }
                            else if(service_conn)
                            {
                                service_conn->send_message(m);
                            }
                            else
                            {
                                // we have to have a remote or service connection here
                                //
                                throw snap::snap_exception("REGISTER sent on a \"weird\" connection (3).");
                            }
                        }
                        else
                        {
                            ++idx;
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

                if(base)
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
                    if(f_local_services_list.isEmpty())
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
                    f_remote_snapcommunicators->start();

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
            if(command == "UNKNOWN")
            {
                SNAP_LOG_ERROR("we sent command \"")
                              (message.get_parameter("command"))
                              ("\" to \"")
                              (connection->get_name())
                              ("\" which told us it does not know that command so we probably did not get the expected result.");
                return;
            }
            else if(command == "UNREGISTER")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("UNREGISTER is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    if(!message.has_parameter("service"))
                    {
                        SNAP_LOG_ERROR("UNREGISTER was called without a \"service\" parameter, which is mandatory.");
                        return;
                    }
                    // also remove all the connection types
                    // an empty string represents an unconnected item
                    //
                    base->set_connection_types("");

                    // connection is down now
                    //
                    base->connection_ended();

                    // status changed for this connection
                    //
                    send_status(connection);

                    // now remove the service name
                    // (send_status() needs the name to still be in place!)
                    //
                    QString const save_name(connection->get_name());
                    connection->set_name("");

                    // get rid of that connection now (it is faster than
                    // waiting for the HUP because it will not be in the
                    // list of connections on the next loop.)
                    //
                    f_communicator->remove_connection(connection);

                    // if the unregistering service is snapinit, also
                    // proceed with a shutdown as if we received a STOP
                    // we have to do that because we cannot at the same
                    // time send an UNREGISTER and a STOP message from
                    // snapinit one after the other knowing that:
                    //
                    // 1) we have to send UNREGISTER first
                    // 2) if we UNREGISTER then we cannot safely use the
                    //    TCP connection anymore
                    // 3) so we could send the STOP using the UDP channel,
                    //    only there is no synchronization so we cannot
                    //    guarantee that UNREGISTER arrives before the
                    //    UNREGISTER...
                    // 4) when snapinit receives STOP, it initiates a
                    //    shutdown of all services on that computer;
                    //    it cannot distinguish from different types
                    //    of STOP signals (i.e. if we were to send a
                    //    STOP from snapinit to snapcommunicator without
                    //    first unregistering, we could not know what
                    //    STOP signal we are getting... the one to shutdown
                    //    evertything or to just send a STOP to the
                    //    snapcommunicator service.)
                    //
                    // So to break the loop we have to either UNREGISTER
                    // with a special case, or change the STOP and include
                    // a special case there. I choose the UNREGISTER because
                    // it is only understood by snapcommunicator whereas
                    // STOP is understood by all services so not having
                    // some special case is safer.
                    //
                    if(save_name == "snapinit")
                    {
                        // "false" like a STOP
                        shutdown(false);
                    }
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
            verify_command(base, reply);
            if(remote_communicator)
            {
                remote_communicator->send_message(reply);
            }
            else if(service_conn)
            {
                service_conn->send_message(reply);
            }
            else
            {
                // we have to have a remote or service connection here
                //
                throw snap::snap_exception("HELP sent on a \"weird\" connection.");
            }
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

    // broadcasting?
    if(service == "*"
    || service == "?"
    || service == ".")
    {
        broadcast_message(message);
        return;
    }

    QString const server_name(message.get_server());

    remote_snap_communicator_vector_t accepting_remote_connections;
    bool const all_servers(server_name.isEmpty() || server_name == "*");
    {
        // service is local, check whether the service is registered,
        // if registered, forward the message immediately
        //
        snap::snap_communicator::snap_connection::vector_t const & connections(f_communicator->get_connections());
        for(auto const & nc : connections)
        {
            base_connection::pointer_t base_conn(std::dynamic_pointer_cast<base_connection>(nc));
            if(base_conn)
            {
                // verify that there is a server name in all connections
                // (if not we have a bug somewhere else)
                //
                if(base_conn->get_server_name().isEmpty())
                {
                    if(!f_server->is_debug())
                    {
                        // ignore in non-debug versions because a throw
                        // completely breaks snapcommunicator... and it
                        // is not that important at this point without
                        // a programmer debugging this software
                        //
                        continue;
                    }
                    service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
                    if(conn)
                    {
                        throw std::runtime_error("server name missing in connection " + conn->get_name().toStdString() + "...");
                    }
                    if(base_conn->get_connection_types().isEmpty())
                    {
                        // not connected yet, forget about it
                        continue;
                    }
                    throw std::runtime_error("server name missing in connection " + base_conn->get_connection_types().toStdString() + "...");
                }

                if(all_servers
                || server_name == base_conn->get_server_name())
                {
                    service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
                    if(conn && conn->get_name() == service)
                    {
                        // we have such a service, just forward to it now
                        //
                        // TBD: should we remove the service name before forwarding?
                        //
                        try
                        {
                            verify_command(conn, message);
                            conn->send_message(message);
                        }
                        catch(std::runtime_error const & e)
                        {
                            // ignore the error because this can come from an
                            // external source (i.e. snapsignal) where an end
                            // user may try to break the whole system!
                            //
                            SNAP_LOG_DEBUG("snapcommunicator failed to send a message to connection \"")
                                          (conn->get_name())
                                          ("\" (error: ")
                                          (e.what())
                                          (")");
                        }
                        // we found a specific service to which we could
                        // forward the message so we can stop here
                        //
                        return;
                    }
                    else
                    {
                        remote_snap_communicator_pointer_t remote_connection(std::dynamic_pointer_cast<remote_snap_communicator>(nc));
                        // TODO: limit sending to remote only if they have that service?
                        //       (if we have the 'all_servers' set, otherwise it is not
                        //       required, for sure... also, if we have multiple remote
                        //       connections that support the same service we should
                        //       randomize which one is to receive that message--or
                        //       even better, check the current server load--but
                        //       seriously, if none of our direct connections know
                        //       of that service, we need to check for those that heard
                        //       of that service, and if that is also empty, send to
                        //       all... for now we send to all anyway)
                        if(remote_connection /*&& remote_connection->has_service(service)*/)
                        {
                            accepting_remote_connections.push_back(remote_connection);
                        }
                    }
                }
            }
        }

        if(all_servers && f_local_services_list.contains(service))
        {
            // its a service that is expected on this computer, but it is not
            // running right now... so cache the message
            //
            // TODO: we want to look into several things:
            //
            //   (1) limiting the cache size
            //   (2) not cache more than one signal message (i.e. PING, STOP, LOG...)
            //   (3) save the date when the message arrived and keep it in
            //       the cache only for a limited time (i.e. 5h)
            //
            f_local_message_cache.push_back(message);
            return;
        }

        // if attempting to send to self, we cannot go on from here
        //
        if(server_name == f_server_name)
        {
            SNAP_LOG_DEBUG("received event \"")(command)("\" for local service \"")(service)("\", which is not currently registered. Dropping message.");
            return;
        }
    }

    if(!accepting_remote_connections.empty())
    {
        // TODO: we probably need to change the message in a broadcast
        //       message in this case since we are in effect broadcasting
        //       it to all those remote servers!
        //
        for(auto const & r : accepting_remote_connections)
        {
            // we have such a server, just forward to it now
            //
            try
            {
                // This is being sent to a service on the remove connection
                // so we cannot verify that it is know (i.e. only the remote
                // connection has the list of commands of that service)
                //
                //verify_command(remote_connection, message);

                r->send_message(message);
            }
            catch(std::runtime_error const & e)
            {
                // ignore the error because this can come from an
                // external source (i.e. snapsignal) where an end
                // user may try to break the whole system!
                //
                SNAP_LOG_DEBUG("snapcommunicator failed to send a message to connection \"")
                              (r->get_name())
                              ("\" (error: ")
                              (e.what())
                              (")");
            }
            // send to all of them; if the server was named, the
            // vector will have a single entry anyway
            //return;
        }
    }
}


void snap_communicator_server::broadcast_message(snap::snap_communicator_message const & message)
{
    QString broadcast_msgid;
    QString informed_neighbors;
    int hops(0);
    time_t timeout(0);

    if(message.has_parameter("broadcast_msgid"))
    {
        // check whether the message already timed out
        //
        // this is a safety feature of our broadcasting capability
        // which should rarely be activated unless you have multiple
        // data center locations
        //
        timeout = message.get_integer_parameter("broadcast_timeout");
        time_t const now(time(nullptr));
        if(timeout < now)
        {
            return;
        }

        // check whether we already received that message, if so ignore
        // the second instance (it should not happen with the list of
        // neighbors included in the message, but just in case...)
        //
        broadcast_msgid = message.get_parameter("broadcast_msgid");
        auto const received_it(f_received_broadcast_messages.find(broadcast_msgid));
        if(received_it != f_received_broadcast_messages.cend())     // message arrived again?
        {
            // note that although we include neighbors it is normal that
            // this happens in a cluster where some computers are not
            // aware of certain nodes; for example, if A sends a
            // message to B and C, both B and C know of a node D
            // which is unknown to A, then both B and C will end
            // up forward that same message to D, so D will discard
            // the second instance it receives.
            //
            return;
        }

        // delete "received messages" that have now timed out (because
        // such are not going to be forwarded since we check the timeout
        // of a message early and prevent the broadcasting in that case)
        //
        for(auto it(f_received_broadcast_messages.cbegin()); it != f_received_broadcast_messages.cend(); )
        {
            if(it->second < now)
            {
                // this one timed out, we do not need to keep it in this
                // list, so erase (notice the it++ opposed to the else
                // that uses ++it)
                //
                f_received_broadcast_messages.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        // add the new message after we check for timed out entries
        // so that way we avoid going through this new entry within
        // the previous loop
        //
        f_received_broadcast_messages[broadcast_msgid] = timeout;

        // Note: we skip the canonicalization on this list of neighbors
        //       because we assume only us (snapcommunicator) handles
        //       that message and we know that it is already
        //       canonicalized here
        //
        informed_neighbors = message.get_parameter("broadcast_informed_neighbors");

        // get the number of hops this message already performed
        //
        hops = message.get_integer_parameter("broadcast_hops");
    }

    snap::snap_string_list informed_neighbors_list;
    if(!informed_neighbors.isEmpty())
    {
        informed_neighbors_list = informed_neighbors.split(',', QString::SkipEmptyParts);
    }

    QString const service(message.get_service());
    bool const all(hops < 5 && service == "*");
    bool const remote(hops < 5 && (all || service == "?"));

    // we always broadcast to all local services
    snap::snap_communicator::snap_connection::vector_t broadcast_connection;
    snap::snap_communicator::snap_connection::vector_t const & connections(f_communicator->get_connections());
    for(auto const & nc : connections)
    {
        // try for a service or snapcommunicator that connected to us
        //
        service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
        remote_snap_communicator_pointer_t remote_communicator;
        if(!conn)
        {
            remote_communicator = std::dynamic_pointer_cast<remote_snap_communicator>(nc);
        }
        bool broadcast(false);
        if(conn)
        {
            switch(conn->get_address().get_network_type())
            {
            case snap_addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK:
                // these are localhost services, avoid sending the
                // message is the destination does not know the
                // command
                //
                if(conn->understand_command(message.get_command())) // destination: "*" or "?" or "."
                {
                    //verify_command(conn, message); -- we reach this line only if the command is understood, it is therefore good
                    conn->send_message(message);
                }
                break;

            case snap_addr::addr::network_type_t::NETWORK_TYPE_PRIVATE:
                // these are computers within the same local network (LAN)
                // we forward messages if at least 'remote' is true
                //
                broadcast = remote; // destination: "*" or "?"
                break;

            case snap_addr::addr::network_type_t::NETWORK_TYPE_PUBLIC:
                // these are computers in another data center
                // we forward messages only when 'all' is true
                //
                broadcast = all; // destination: "*"
                break;

            default:
                // unknown/unexpected type of IP address, totally ignore
                break;

            }
        }
        else if(remote_communicator)
        {
            // another snapcommunicator that connected to us
            //
            switch(remote_communicator->get_address().get_network_type())
            {
            case snap_addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK:
                {
                    static bool warned(false);
                    if(!warned)
                    {
                        warned = true;
                        SNAP_LOG_WARNING("remote snap communicator was connected on a LOOPBACK IP address...");
                    }
                }
                break;

            case snap_addr::addr::network_type_t::NETWORK_TYPE_PRIVATE:
                // these are computers within the same local network (LAN)
                // we forward messages if at least 'remote' is true
                //
                broadcast = remote; // destination: "*" or "?"
                break;

            case snap_addr::addr::network_type_t::NETWORK_TYPE_PUBLIC:
                // these are computers in another data center
                // we forward messages only when 'all' is true
                //
                broadcast = all; // destination: "*"
                break;

            default:
                // unknown/unexpected type of IP address, totally ignore
                break;

            }
        }
        if(broadcast)
        {
            // get the IP address of the remote snapcommunicator
            //
            QString const address(QString::fromUtf8((conn ? conn->get_address() : remote_communicator->get_address()).get_ipv4or6_string(false, false).c_str()));
            if(!informed_neighbors_list.contains(address))
            {
                // not in the list of informed neighbors, add it and
                // keep nc in a list that we can use to actually send
                // the broadcast message
                //
                informed_neighbors_list << address;
                broadcast_connection.push_back(nc);
            }
        }
    }

    if(!broadcast_connection.empty())
    {
        // we are broadcasting now (Gossiping a regular message);
        // for the gossiping to work, we include additional
        // information in the message
        //
        QString const originator(QString::fromUtf8(f_my_address.get_ipv4or6_string().c_str()));
        if(!informed_neighbors_list.contains(originator))
        {
            // include self since we already know of the message too!
            // (no need for others to send it back to us)
            //
            informed_neighbors_list << originator;
        }

        // message is 'const', so we need to create a copy
        snap::snap_communicator_message broadcast_msg(message);

        // generate a new unique broadcast message identifier
        //
        ++g_broadcast_sequence;
        broadcast_msgid = QString("%1-%2").arg(f_server_name).arg(g_broadcast_sequence);
        broadcast_msg.add_parameter("broadcast_msgid", broadcast_msgid);

        // increase the number of hops; if we reach the limit, we still
        // want to forward the message, the destination will not forward
        // (broadcast) more, but it will possibly send that to its own
        // services
        //
        broadcast_msg.add_parameter("broadcast_hops", hops + 1);

        // mainly noise at this point, but I include the originator so
        // we can track that back if needed for debug purposes
        //
        broadcast_msg.add_parameter("broadcast_originator", originator);

        // define a timeout if this is the originator
        //
        if(timeout == 0)
        {
            // give message 10 seconds to arrive to any and all destinations
            timeout = time(nullptr) + 10;
        }
        broadcast_msg.add_parameter("broadcast_timeout", timeout);

        // note that we currently define the list of neighbors BEFORE
        // sending the message (anyway the send_message() just adds the
        // message to a memory cache at this point, so whether it will
        // be sent is not known until later.)
        //
        broadcast_msg.add_parameter("broadcast_informed_neighbors", informed_neighbors_list.join(","));

        for(auto const & bc : broadcast_connection)
        {
            service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(bc));
            if(conn)
            {
                conn->send_message(broadcast_msg);
            }
            else //if(remote_communicator) -- if 'broadcast' is true, we have either conn or remote_communicator defined
            {
                remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(bc));
                remote_communicator->send_message(broadcast_msg);
            }
        }
    }
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

    base_connection::pointer_t base_connection(std::dynamic_pointer_cast<base_connection>(connection));
    if(base_connection)
    {
        // check whether the connection is now up or down
        QString const types(base_connection->get_connection_types());
        reply.add_parameter("status", types.isEmpty() ? "down" : "up");

        // get the time when it was considered up
        int64_t const up_since(base_connection->get_connection_started());
        if(up_since != -1)
        {
            reply.add_parameter("up_since", up_since / 1000000); // send up time in seconds
        }

        // get the time when it was considered down (if not up yet, this will be skipped)
        int64_t const down_since(base_connection->get_connection_ended());
        if(down_since != -1)
        {
            reply.add_parameter("down_since", down_since / 1000000); // send up time in seconds
        }
    }

    // we have the message, now we need to find the list of connections
    // interested by the STATUS event
    // TODO: cache that list?
    // TODO: use the broadcast_message() function instead? (with service set to ".")
    //
    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
    for(auto const & conn : all_connections)
    {
        service_connection::pointer_t sc(std::dynamic_pointer_cast<service_connection>(conn));
        if(!sc)
        {
            // not a service_connection, ignore (i.e. servers)
            continue;
        }

        if(sc->understand_command("STATUS"))
        {
            // send that STATUS message
            //verify_command(sc, reply); -- we reach this line only if the command is understood
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
 *
 * \return true if any one of the \p new_neighbors were added.
 */
bool snap_communicator_server::add_neighbors(QString const & new_neighbors)
{
    // first time initialize and read the cache file
    //
    static QString cache_filename;
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
            char buf[1024];
            for(;;)
            {
                qint64 const r(cache.readLine(buf, sizeof(buf)));
                if(r < 0)
                {
                    break;
                }
                if(r > 0
                && buf[0] != '#')
                {
                    QString const line(QString::fromUtf8(buf, r).trimmed());
                    f_all_neighbors[line] = true;
                }
            }
        }
    }

    bool changed(false);
    if(!new_neighbors.isEmpty())
    {
        snap::snap_string_list list(new_neighbors.split(',', QString::SkipEmptyParts));
        for(auto const & s : list)
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

    return changed;
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
    for(auto const & connection : all_connections)
    {
        service_connection::pointer_t c(std::dynamic_pointer_cast<service_connection>(connection));
        if(!c)
        {
            // not a service_connection, ignore (i.e. servers)
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

    // all gossiping can stop at once, since we cannot recognize those
    // connections in the list returned by f_communicator, we better
    // do that cleanly ahead of time
    //
    if(f_remote_snapcommunicators)
    {
        f_remote_snapcommunicators->stop_gossiping();
    }

    // DO NOT USE THE REFERENCE -- we need a copy of the vector
    // because the loop below uses remove_connection() on the
    // original vector!
    //
    snap::snap_communicator::snap_connection::vector_t const all_connections(f_communicator->get_connections());
    for(auto const & connection : all_connections)
    {
        // a remote communicator for which we initiated a new connection?
        //
        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(connection));
        if(remote_communicator)
        {

// TODO: if the remote communicator IP address is the same as the
//       STOP, DISCONNECT, or SHUTDOWN message we just received,
//       then we have to just disconnect (HUP) instead of sending
//       a reply

            // remote communicators are just timers and can be removed
            // as is, no message are sent there (no interface to do so anyway)
            //
            snap::snap_communicator_message reply;

            // a remote snapcommunicator server needs to also
            // shutdown so duplicate that message there
            if(full)
            {
                // SHUTDOWN means we shutdown the entire cluster!!!
                reply.set_command("SHUTDOWN");

                verify_command(remote_communicator, reply);
                remote_communicator->send_message(reply);

                // this will prevent the SHUTDOWN from being sent
                // we need to have something that tells us that
                // the message was sent and at that time remove
                // the connection
                //
                f_communicator->remove_connection(remote_communicator);
            }
            else
            {
                // STOP means we do not shutdown the entire cluster
                reply.set_command("DISCONNECT");

                // in this case, the remote server closes the socket so
                // we will get a HUP and do not need to remove this
                // connection from here now
                //
                verify_command(remote_communicator, reply);
                remote_communicator->send_message(reply);
            }
        }
        else
        {
            // a standard service connection or a remote snapcommunicator server?
            //
            service_connection::pointer_t c(std::dynamic_pointer_cast<service_connection>(connection));
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

// TODO: if the remote communicator IP address is the same as the
//       STOP, DISCONNECT, or SHUTDOWN message we just received,
//       then we have to just disconnect (HUP) instead of sending
//       a reply

                        // a remote snapcommunicator server needs to also
                        // shutdown so duplicate that message there
                        if(full)
                        {
                            // SHUTDOWN means we shutdown the entire cluster!!!
                            reply.set_command("SHUTDOWN");
                        }
                        else
                        {
                            // DISCONNECT means only we are going down
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

                    verify_command(c, reply);
                    c->send_message(reply);

                    // we cannot yet remove the connection from the communicator
                    // or these messages will never be sent... the client is
                    // expected to reply with UNREGISTER which does the removal
                    // the remote connections are expected to disconnect when
                    // they receive a DISCONNECT
                }
            }
            // else -- ignore the main TCP and UDP servers which we
            //         handle below
        }
    }

    // remove the two main servers; we will not respond to any more
    // requests anyway
    //
    f_communicator->remove_connection(f_local_listener);    // TCP/IP
    f_communicator->remove_connection(f_listener);          // TCP/IP
    f_communicator->remove_connection(f_ping);              // UDP/IP
}


void snap_communicator_server::process_connected(snap::snap_communicator::snap_connection::pointer_t connection)
{
    snap::snap_communicator_message connect;
    connect.set_command("CONNECT");
    connect.add_parameter("version", snap::snap_communicator::VERSION);
    connect.add_parameter("types", f_server_types);
    connect.add_parameter("my_address", QString::fromUtf8(f_my_address.get_ipv4or6_string(true).c_str()));
    connect.add_parameter("server_name", f_server_name);
    if(!f_explicit_neighbors.isEmpty())
    {
        connect.add_parameter("neighbors", f_explicit_neighbors);
    }
    if(!f_local_services.isEmpty())
    {
        connect.add_parameter("services", f_local_services);
    }
    if(!f_services_heard_of.isEmpty())
    {
        connect.add_parameter("heard_of", f_services_heard_of);
    }
    service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));
    if(service_conn)
    {
        service_conn->send_message(connect);
    }
    else
    {
        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(connection));
        if(remote_communicator)
        {
            remote_communicator->send_message(connect);
        }
    }

    // status changed for this connection
    //
    send_status(connection);
}




/** \brief Setup a remote_snap_communicator object.
 *
 * This initialization function sets up the attached snap_timer
 * to 1 second delay before we try to connect to this remote
 * snapcommunicator. The timer is reused later when the connection
 * is lost, a snapcommunicator returns a REFUSE message to our
 * CONNECT message, and other similar errors.
 */
remote_snap_communicator::remote_snap_communicator(snap_communicator_server::pointer_t cs, QString const & addr, int port)
    : snap_tcp_client_permanent_message_connection(addr.toUtf8().data(), port, tcp_client_server::bio_client::mode_t::MODE_PLAIN, 5LL * 60LL * 1000000LL)
    , base_connection(cs)
    , f_address(addr.toUtf8().data(), "", 4040, "tcp")
{
    // prevent the timer from going until we get our list of
    // services from snapinit
    //
    set_enable(false);
}


remote_snap_communicator::~remote_snap_communicator()
{
}


void remote_snap_communicator::process_message(snap::snap_communicator_message const & message)
{
    f_communicator_server->process_message(shared_from_this(), message, false);
}


void remote_snap_communicator::process_connection_failed(std::string const & error_message)
{
    snap_tcp_client_permanent_message_connection::process_connection_failed(error_message);

    SNAP_LOG_ERROR("the connection to a remote communicator failed: \"")(error_message)("\".");
}


void remote_snap_communicator::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    f_communicator_server->process_connected(shared_from_this());
}


snap_addr::addr const & remote_snap_communicator::get_address() const
{
    return f_address;
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
        SNAP_LOG_INFO("--------------------------------- snapcommunicator started on ")(s->get_parameter("server_name"));

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
        SNAP_LOG_FATAL("snapcommunicator: snap exception caught: ")(except.what());
    }
    catch( std::exception const & std_except )
    {
        SNAP_LOG_FATAL("snapcommunicator: standard exception caught: ")(std_except.what());
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
