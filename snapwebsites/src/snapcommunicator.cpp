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
#include "not_reached.h"
#include "not_used.h"
#include "snap_config.h"
#include "snapwebsites.h"

#include <sys/time.h>
#include <sys/resource.h>

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


} // no name namespace




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

                                snap_communicator_server(snap::server::pointer_t s);
                                snap_communicator_server(snap_communicator_server const & src) = delete;
    snap_communicator_server &  operator = (snap_communicator_server const & rhs) = delete;

    void                        init();
    void                        run();

    // one place where all messages get processed
    void                        process_message(snap::snap_communicator::snap_connection::pointer_t connection, snap::snap_communicator_message const & message);

    void                        send_status(snap::snap_communicator::snap_connection::pointer_t connection);

private:
    void                        refresh_heard_of();

    snap::server::pointer_t                             f_server;

    snap::snap_communicator::pointer_t                  f_communicator;
    snap::snap_communicator::snap_connection::pointer_t f_listener;     // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_messager;     // UDP/IP
    QString                                             f_server_type;
    QString                                             f_local_services;
    snap::snap_string_list                              f_local_services_list;
    QString                                             f_services_heard_of;
    sorted_list_of_strings_t                            f_services_heard_of_list;
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
        // save when it is ending in case we did not get a DISCONNECT
        // or an UNREGISTER event
        //
        connection_ended();

        // clearly mark this connection as "invalid"
        //
        set_connection_type("");

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
        f_communicator_server->process_message(shared_from_this(), message);
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

    /** \brief Define the type of snapcommunicator server.
     *
     * This function is called whenever an ACCEPT is received. It saves
     * the type=... parameter. By default the type is empty meaning that
     * the connection was not yet fully initialised.
     *
     * When a REGISTER is received instead of an ACCEPT, then the type
     * is set to "client".
     */
    void set_connection_type(QString const & type)
    {
        f_type = type;
    }

    /** \brief Retrieve the current type of this connection.
     *
     * By default a connection is given the special type "" which means
     * that it is not currently properly initialized yet. To properly
     * initialize a connection one has to either CONNECT (between
     * snapcommunicator servers) or REGISTER (a backend, snapserver,
     * snapmonitor, etc.)
     *
     * The type is set to "client" for local services and another
     * word, such as "frontend", when representing another snapserver.
     *
     * \return The type of server this connection represents.
     */
    QString const & get_connection_type() const
    {
        return f_type;
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

private:
    snap_communicator_server::pointer_t     f_communicator_server;
    sorted_list_of_strings_t                f_understood_commands;
    int64_t                                 f_started_on = -1;
    int64_t                                 f_ended_on = -1;
    QString                                 f_type;
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

        snap::snap_communicator::snap_tcp_server_client_connection::pointer_t connection(new connection_impl(f_communicator_server, new_socket));

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
        f_communicator_server->process_message(shared_from_this(), message);
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
        f_server_type = f_server->get_parameter("server_type");
        if(f_server_type != "apache"
        && f_server_type != "frontend"
        && f_server_type != "backend"
        && f_server_type != "cassandra")
        {
            f_server_type = "frontend";
        }

        // TODO: apply some verification to that list?
        //       (i.e. at this point we send it as it to the other
        //       snapservers as the "services=..." parameter)
        //
        f_local_services = f_server->get_parameter("local_services");
        f_local_services_list = f_local_services.split(',');
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
            f_server->get_addr_port(listen_info, addr, port, 4040);
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
            f_server->get_addr_port(signal_info, addr, port, 4041);
        }

        f_messager.reset(new messager_impl(shared_from_this(), addr.toUtf8().data(), port));
        f_messager->set_name("snap communicator messager (UDP)");
        f_communicator->add_connection(f_messager);
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
 * This function is called whenever a TCP or UDP message is recieved.
 * (whether it is received through TCP or UDP we view all messages
 * exactly the same way.)
 *
 * \param[in] client  The connection that just sent us that message.
 * \param[in] message  The message were were just sent.
 */
void snap_communicator_server::process_message(snap::snap_communicator::snap_connection::pointer_t connection, snap::snap_communicator_message const & message)
{
    QString const command(message.get_command());

    connection_impl::pointer_t c(std::dynamic_pointer_cast<connection_impl>(connection));

    // check who this message is for
    QString const service(message.get_service());
    if(service.isEmpty() || service == "snapcommunicator")
    {
        // this one is for us!
        switch(command[0].unicode())
        {
        case 'A':
            if(command == "ACCEPT")
            {
                if(c)
                {
                    // reply to a CONNECT, this was to connect to another
                    // snapcommunicator on another computer
                    //
                    c->connection_started();

                    if(message.has_parameter("type"))
                    {
                        c->set_connection_type(message.get_parameter("type"));
                    }
                    if(message.has_parameter("services"))
                    {
                        c->set_services(message.get_parameter("services"));
                    }
                    if(message.has_parameter("heard_of"))
                    {
                        c->set_services_heard_of(message.get_parameter("heard_of"));
                    }

                    // we just got some new services information,
                    // refresh our cache
                    //
                    refresh_heard_of();
                }
            }
            break;

        case 'C':
            if(command == "COMMANDS")
            {
                if(c && message.has_parameter("list"))
                {
                    c->set_commands(message.get_parameter("list"));
                }
            }
            else if(command == "CONNECT")
            {
                if(c)
                {
                    // same as ACCEPT (see above) -- maybe we could have
                    // a sub-function...
                    //
                    c->connection_started();

                    if(message.has_parameter("type"))
                    {
                        c->set_connection_type(message.get_parameter("type"));
                    }
                    if(message.has_parameter("services"))
                    {
                        c->set_services(message.get_parameter("services"));
                    }
                    if(message.has_parameter("heard_of"))
                    {
                        c->set_services_heard_of(message.get_parameter("heard_of"));
                    }

                    // we just got some new services information,
                    // refresh our cache
                    //
                    refresh_heard_of();

                    // the message expects the ACCEPT reply
                    //
                    snap::snap_communicator_message reply;
                    //reply.set_service("" or "snapcommunicator") -- "" is the default
                    reply.set_command("ACCEPT");

                    // server type
                    reply.add_parameter("type", f_server_type);

                    // services
                    reply.add_parameter("services", f_local_services);

                    // heard of
                    reply.add_parameter("heard_of", f_services_heard_of);

                    c->send_message(reply);
                }
            }
            break;

        case 'D':
            if(command == "DISCONNECT")
            {
                if(c)
                {
                    c->connection_ended();

                    // this has to be another snapcommunicator
                    // (i.e. an object that sent ACCEPT or CONNECT)
                    //
                    QString const type(c->get_connection_type());
                    if(!type.isEmpty()
                    && type != "client")
                    {
                        // we must ignore, and we ignore connections with a
                        // type of "" as they are expected to be "uninitialized"
                        //
                        c->set_connection_type("");

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
                }
            }
            break;

        }

        // done
        return;
    }





    //{
    //    // the user specified a name so we want to send the message to
    //    // that specific service only
    //    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
    //    for(auto c : all_connections)
    //    {
    //        if(c->get_name() == service)
    //        {
    //            // we found it!
    //            break;
    //        }
    //    }
    //}
}


/** \brief Send the current status of a client to connections.
 *
 * Some connections (at this time only the snapmonitor) may be interested
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

    connection_impl::pointer_t c(std::dynamic_pointer_cast<connection_impl>(connection));
    QString const type(c->get_connection_type());
    reply.add_parameter("status", type.isEmpty() ? "down" : "up");
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
    for(auto s : f_local_services_list)
    {
        f_services_heard_of_list.remove(s);
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

        // Run the snap communicator server; note that the snapcommunicator
        // server is snap_communicator and not snap::server
        //
        snap_communicator_server communicator( s );
        communicator.init();
        communicator.run();

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
