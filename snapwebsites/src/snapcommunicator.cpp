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


/** \brief Set of connections in the snapcommunicator tool.
 *
 * All the connections and sockets in general will all appear
 * in this class.
 */
class connections
{
public:
    typedef std::shared_ptr<snap::snap_communicator>     pointer_t;

                                connections(snap::server::pointer_t s);

                                connections(connections const & src) = delete;
    snap::snap_communicator &   operator = (connections const & rhs) = delete;

    void                        init();
    void                        run();

    void                        add_connection(snap::snap_communicator::snap_connection::pointer_t client);
    void                        remove_connection(snap::snap_communicator::snap_connection::pointer_t client);
    void                        process_connection(snap::snap_communicator::snap_connection::pointer_t client);
    void                        process_message(snap::snap_communicator_message const & message);

private:
    snap::server::pointer_t                             f_server;

    snap::snap_communicator::pointer_t                  f_communicator;
    snap::snap_communicator::snap_connection::pointer_t f_listener;
    snap::snap_communicator::snap_connection::pointer_t f_messager;
};



class snap_communicator_connection
{
public:
    QString const &         get_service_name() const;
    void                    set_service_name(QString const & name);

private:
    QString                 f_service_name;
};


QString const & snap_communicator_connection::get_service_name() const
{
    return f_service_name;
}


void snap_communicator_connection::set_service_name(QString const & name)
{
    f_service_name = name;
}







/** \brief Our version of snap_tcp_server_client_connection object.
 *
 * The snap_tcp_server_client_connection class has a pure virtual function
 * and thus it cannot be instantiated. In order to have a way to
 * instantiate such an object, we create our own class
 * and implement the process_signal() function.
 */

class connection_impl
        : public snap::snap_communicator::snap_tcp_server_client_message_connection
{
public:
                                connection_impl(int socket);

    virtual void                process_message(snap::snap_communicator_message const & message);
};




connection_impl::connection_impl(int socket)
    : snap_tcp_server_client_message_connection(socket)
{
}


void connection_impl::process_message(snap::snap_communicator_message const & message)
{
    if(message.get_command() == "STOP")
    {
        return;
    }
}





/** \brief Handle new connections from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */
class listener_impl
        : public snap::snap_communicator::snap_tcp_server_connection
        , public snap_communicator_connection
{
public:
                                        listener_impl(connections * s, std::string const & addr, int port, int max_connections, bool reuse_addr, bool auto_close);

    // snap::snap_communicator::snap_server_connection implementation
    virtual void                        process_accept();

private:
    // this is owned by a server function so no need for a smart pointer
    connections *                       f_connections;
};



/** \brief The listener initialization.
 *
 * The listener receives a pointer back to the snap::server object and
 * information on how to generate the new network connection to listen
 * on incoming connections from clients.
 *
 * \param[in] s  The server we are listening for.
 * \param[in] addr  The address to listen on. Most often it is 0.0.0.0.
 * \param[in] port  The port to listen on.
 * \param[in] max_connections  The maximum number of connections to keep
 *            waiting; if more arrive, refuse them until we are done with
 *            some existing connections.
 * \param[in] reuse_addr  Whether to let the OS reuse that socket immediately.
 * \param[in] auto_close  Whether to automatically close the socket once more
 *            needed anymore.
 */
listener_impl::listener_impl(connections * s, std::string const & addr, int port, int max_connections, bool reuse_addr, bool auto_close)
    : snap_tcp_server_connection(addr, port, max_connections, reuse_addr, auto_close)
    , f_connections(s)
{
    //non_blocking();
}


/** \brief This callback is called whenever something happens on the server.
 *
 * This is called whenever something happens to the server, which may be
 * a new connection from a client or receiving a UDP signal (most often
 * a STOP command.)
 *
 * The function processes the events by calling functions on the server.
 *
 * \param[in] we  The event that just triggered this call.
 */
void listener_impl::process_accept()
{
    // a new client just connected, save that connection in
    // our connections handler;
    //
    int new_socket(accept());

    snap::snap_communicator::snap_tcp_server_client_connection::pointer_t connection(new connection_impl(new_socket));
    connection->set_name("client connection");
    connection->keep_alive();

    snap::snap_communicator::instance()->add_connection(connection);
}






/** \brief Handle new connections from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */
class messager_impl
        : public snap::snap_communicator::snap_udp_server_connection
        , public snap_communicator_connection
{
public:
                                        messager_impl(connections * c, std::string const & addr, int port);

    // snap::snap_communicator::snap_server_connection implementation
    virtual void                        process_read();

private:
    // this is owned by a server function so no need for a smart pointer
    connections *                       f_connections;
};



/** \brief The listener initialization.
 *
 * The listener receives a pointer back to the snap::server object and
 * information on how to generate the new network connection to listen
 * on incoming connections from clients.
 *
 * \param[in] s  The server we are listening for.
 * \param[in] addr  The address to listen on. Most often it is 0.0.0.0.
 * \param[in] port  The port to listen on.
 * \param[in] max_connections  The maximum number of connections to keep
 *            waiting; if more arrive, refuse them until we are done with
 *            some existing connections.
 * \param[in] reuse_addr  Whether to let the OS reuse that socket immediately.
 * \param[in] auto_close  Whether to automatically close the socket once more
 *            needed anymore.
 */
messager_impl::messager_impl(connections * c, std::string const & addr, int port)
    : snap_udp_server_connection(addr, port)
    , f_connections(c)
{
    non_blocking();
}


/** \brief This callback is called whenever something happens on the server.
 *
 * This is called whenever something happens to the server, which may be
 * a new connection from a client or receiving a UDP signal (most often
 * a STOP command.)
 *
 * The function processes the events by calling functions on the server.
 */
void messager_impl::process_read()
{
    // retrieve message from UDP socket
    //
    // Are these really always packets or can we receive UDP data
    // pieces by pieces?
    //
    char buf[256];
    ssize_t r(recv(buf, sizeof(buf) / sizeof(buf[0]) - 1));
    if(r > 0)
    {
        buf[r] = '\0';
        QString udp_message(QString::fromUtf8(buf));
        snap::snap_communicator_message message;
        if(message.from_message(udp_message))
        {
            // we just received a signal (UDP message)
            // we have to forward that to the right system
            //
            f_connections->process_message(message);
        }
    }
}






/** \brief Construct the connections object.
 *
 * This function saves the server pointer in the connections object.
 * It is used later to gather various information and call helper
 * functions.
 */
connections::connections(snap::server::pointer_t s)
    : f_server(s)
{
}


/** \brief Initialize the connections.
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
void connections::init()
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

        f_listener.reset(new listener_impl(this, addr.toUtf8().data(), port, max_pending_connections, true, false));
        f_listener->set_name("snap communicator listener");
        add_connection(f_listener);
    }

    {
        QString addr("127.0.0.1"); // this default should work just fine
        int port(4041);
        QString const signal_info(f_server->get_parameter("signal"));
        if(!signal_info.isEmpty())
        {
            f_server->get_addr_port(signal_info, addr, port, 4041);
        }

        f_messager.reset(new messager_impl(this, addr.toUtf8().data(), port));
        f_messager->set_name("snap communicator messager (UDP)");
        add_connection(f_messager);
    }
}


/** \brief The execution loop.
 *
 * This function runs the execution loop until the snapcommunicator
 * system receives a QUIT message.
 */
void connections::run()
{
    // run "forever" (until we receive a QUIT message)
    f_communicator->run();

    // we are done, cleanly get rid of the communicator
    f_communicator.reset();
}


/** \brief Handle new TCP/IP clients.
 *
 * This function adds all TCP/IP clients to our list of signals we want
 * to listen to.
 *
 * Snap! Communicator then waits for the client to disclose its name
 * and other various parameters.
 *
 * \param[in] client  The client that we want to save as a new connection.
 */
void connections::add_connection(snap::snap_communicator::snap_connection::pointer_t client)
{
    // TODO: how are we to remove clients that die on us? (i.e. leaving
    //       without sending a proper QUIT message)
    //
    f_communicator->add_connection(client);
}


/** \brief Handle new TCP/IP clients.
 *
 * This function adds all TCP/IP clients to our list of signals we want
 * to listen to.
 *
 * Snap! Communicator then waits for the client to disclose its name
 * and other various parameters.
 *
 * \param[in] client  The client that we want to save as a new connection.
 */
void connections::remove_connection(snap::snap_communicator::snap_connection::pointer_t client)
{
    f_communicator->remove_connection(client);
}


/** \brief Process the connection message.
 *
 * Just like a UDP message, we can process a TCP message. The main
 * difference here is that TCP messages are addressed to us and
 * thus we need to run this function to process this message
 * specifically. We may end up replying to the called hence
 * we have a pointer to that specific client.
 *
 * \param[in] client  The client concerned about something being messaged about.
 */
void connections::process_connection(snap::snap_communicator::snap_connection::pointer_t client)
{
    NOTUSED(client);
}


/** \brief Process a message we just received.
 *
 * This function is called whenever a UDP message is recieved.
 *
 * \note
 * We do not need to pass a connection as parameter since we know the
 * message came through f_messager.
 *
 * \param[in] client  The connection that just sent us that message.
 * \param[in] message  The message were were just sent.
 */
void connections::process_message(snap::snap_communicator_message const & message)
{
    // split the message from 'server name' and 'command word'
    QString const service(message.get_service());
    if(service.isEmpty())
    {
        // in this case we want to broadcast the message to all the other
        // sub-systems (i.e. a "massive" QUIT message...)
    }
    else
    {
        // the user specified a name so we want to send the message to
        // that specific service only
        snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
        for(auto c : all_connections)
        {
            if(c->get_name() == service)
            {
                // we found it!
                break;
            }
        }
    }
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
        connections communicator( s );
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
