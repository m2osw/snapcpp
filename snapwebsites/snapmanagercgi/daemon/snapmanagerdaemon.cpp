//
// File:        snapmanagerdaemon.cpp
// Object:      Allow for applying functions on any computer.
//
// Copyright:   Copyright (c) 2016 Made to Order Software Corp.
//              All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "snapmanagerdaemon.h"

#include "mkdir_p.h"

#include <sstream>




namespace snap_manager
{


/** \brief Initialize the manager daemon.
 *
 * Initialize the various variable members that need a dynamic
 * initialization.
 */
manager_daemon::manager_daemon()
    : manager(true)
    , f_status_connection(new status_connection(this))
    , f_status_runner(this, f_status_connection)
    , f_status_thread("status", &f_status_runner)
{
}


void manager_daemon::init(int argc, char * argv[])
{
    manager::init(argc, argv);

    f_status_connection->set_server_name(f_server_name);

    // --connect <communicator IP:port> (mandatory)
    //
    tcp_client_server::get_addr_port(f_opt->get_string("connect").c_str(), f_communicator_address, f_communicator_port, "tcp");

    // TODO: make us snapwebsites by default and root only when required...
    //       (and use RAII to do the various switches)
    //
    if(setuid(0) != 0)
    {
        perror("manager_daemon:setuid(0)");
        throw std::runtime_error("fatal error: could not setup executable to run as user root.");
    }
    if(setgid(0) != 0)
    {
        perror("manager_daemon:setgid(0)");
        throw std::runtime_error("fatal error: could not setup executable to run as group root.");
    }

    // get the list of front end servers (i.e. list of computer(s)
    // accepting snapmanager.cgi requests)
    //
    if(f_config.contains("snapmanager_frontend"))
    {
        f_status_runner.set_snapmanager_frontend(f_config["snapmanager_frontend"]);
    }

    // now try to load all the plugins
    //
    load_plugins();
}


manager_daemon::~manager_daemon()
{
}


int manager_daemon::run()
{
    // Stop on these signals, log them, then terminate.
    //
    // Note: the handler uses the logger which the create_instance()
    //       initializes
    //
    signal( SIGSEGV, manager_daemon::sighandler );
    signal( SIGBUS,  manager_daemon::sighandler );
    signal( SIGFPE,  manager_daemon::sighandler );
    signal( SIGILL,  manager_daemon::sighandler );

    SNAP_LOG_INFO("--------------------------------- snapmanagerdaemon started on ")(f_server_name);

    // initialize the communicator and its connections
    //
    f_communicator = snap::snap_communicator::instance();

    // create a messenger to communicate with the Snap Communicator process
    // and snapmanager.cgi as required
    //
    f_messenger.reset(new manager_messenger(this, f_communicator_address.toUtf8().data(), f_communicator_port));
    f_communicator->add_connection(f_messenger);

    // also add the status connection created in the constructor
    //
    f_communicator->add_connection(f_status_connection);

    // now run our listening loop
    //
    f_communicator->run();

    return 0;
}


/** \brief A static function to capture various signals.
 *
 * This function captures unwanted signals like SIGSEGV and SIGILL.
 *
 * The handler logs the information and then the service exists.
 * This is done mainly so we have a chance to debug problems even
 * when it crashes on a remote server.
 *
 * \warning
 * The signals are setup after the construction of the manager_daemon
 * object because that is where we initialize the logger.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void manager_daemon::sighandler( int sig )
{
    QString signame;
    switch( sig )
    {
    case SIGSEGV:
        signame = "SIGSEGV";
        break;

    case SIGBUS:
        signame = "SIGBUS";
        break;

    case SIGFPE:
        signame = "SIGFPE";
        break;

    case SIGILL:
        signame = "SIGILL";
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    {
        snap::snap_exception_base::output_stack_trace();
        SNAP_LOG_FATAL("Fatal signal caught: ")(signame);
    }

    // Exit with error status
    //
    ::exit( 1 );
    snap::NOTREACHED();
}


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message. This includes the basic READY, HELP, and STOP commands.
 *
 * \param[in] message  The message we just received.
 */
void manager_daemon::process_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_server_name);

    QString const command(message.get_command());

    switch(command[0].unicode())
    {
    case 'H':
        if(command == "HELP")
        {
            // Snap! Communicator is asking us about the commands that we support
            //
            snap::snap_communicator_message reply;
            reply.set_command("COMMANDS");

            // list of commands understood by service
            // (many are considered to be internal commands... users
            // should look at the LOCK and UNLOCK messages only)
            //
            reply.add_parameter("list", "HELP,LOG,MANAGE,MANAGERRESEND,MANAGERSTATUS,QUITTING,READY,SERVER_PUBLIC_IP,STOP,UNKNOWN,UNREACHABLE");

            f_messenger->send_message(reply);

            // if we are a front end computer, we want to be kept informed
            // of the status of all the other computers in the cluster...
            // so ask all the other snapmanagerdaemon to broadcast their
            // status again
            //
            if(f_status_runner.is_snapmanager_frontend(f_server_name))
            {
                snap::snap_communicator_message resend;
                resend.set_service("*");
                resend.set_command("MANAGERRESEND");
                f_messenger->send_message(resend);
            }
            return;
        }
        break;

    case 'L':
        if(command == "LOG")
        {
            // logrotate just rotated the logs, we have to reconfigure
            //
            SNAP_LOG_INFO("Logging reconfiguration.");
            snap::logging::reconfigure();
            return;
        }
        break;

    case 'M':
        if(command == "MANAGE")
        {
            // run the RPC call
            //
            manage(message);
            return;
        }
        else if(command == "MANAGERRESEND")
        {
            f_status_runner.resend_status();
            return;
        }
        else if(command == "MANAGERSTATUS")
        {
            // record the status of this and other managers
            //
            set_manager_status(message);
            return;
        }
        break;

    case 'Q':
        if(command == "QUITTING")
        {
            // If we received the QUITTING command, then somehow we sent
            // a message to Snap! Communicator, which is already in the
            // process of quitting... we should get a STOP too, but we
            // can just quit ASAP too
            //
            stop(true);
            return;
        }
        break;

    case 'R':
        if(command == "READY")
        {
            // we now are connected to the snapcommunicator
            //

            // request a copy of our public IP address
            {
                snap::snap_communicator_message public_ip;
                public_ip.set_command("PUBLIC_IP");
                f_messenger->send_message(public_ip);
            }
            return;
        }
        break;

    case 'S':
        if(command == "SERVER_PUBLIC_IP")
        {
            // snapcommunicator replied
            //
            f_public_ip = message.get_parameter("public_ip");

            // start the status thread, used to gather this computer's status
            //
            if(!f_status_thread.start())
            {
                SNAP_LOG_ERROR("snapmanagerdaemon could not start its helper thread. Quitting immediately.");
                stop(false);
            }
            return;
        }
        else if(command == "STOP")
        {
            // Someone is asking us to leave (probably snapinit)
            //
            stop(false);
            return;
        }
        break;

    case 'U':
        if(command == "UNKNOWN")
        {
            // we sent a command that Snap! Communicator did not understand
            //
            SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            return;
        }
        else if(command == "UNREACHABLE")
        {
            unreachable_message(message);
            return;
        }
        break;

    }

    // unknown commands get reported and process goes on
    //
    SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received on the connection with Snap! Communicator.");
    {
        snap::snap_communicator_message reply;
        reply.set_command("UNKNOWN");
        reply.add_parameter("command", command);
        f_messenger->send_message(reply);
    }

    return;
}




/** \brief Called whenever we receive the STOP command or equivalent.
 *
 * This function makes sure the manager_daemon exits as quickly as
 * possible.
 *
 * \li Marks the messenger as done.
 * \li UNREGISTER from snapcommunicator.
 *
 * \note
 * If the f_messenger is still in place, then just sending the
 * UNREGISTER is enough to quit normally. The socket of the
 * f_messenger will be closed by the snapcommunicator server
 * and we will get a HUP signal. However, we get the HUP only
 * because we first mark the messenger as done.
 *
 * \param[in] quitting  Set to true if we received a QUITTING message.
 */
void manager_daemon::stop(bool quitting)
{
    if(f_messenger)
    {
        f_messenger->mark_done();

        // unregister if we are still connected to the messenger
        // and Snap! Communicator is not already quitting
        //
        if(!quitting)
        {
            snap::snap_communicator_message cmd;
            cmd.set_command("UNREGISTER");
            cmd.add_parameter("service", "snapmanagerdaemon");
            f_messenger->send_message(cmd);
        }
    }

    if(f_status_connection)
    {
        // WARNING: we cannot send a message to the status thread
        //          if it was not started
        //
        if(f_status_thread.is_running())
        {
            snap::snap_communicator_message cmd;
            cmd.set_command("STOP");
            f_status_connection->send_message(cmd);
        }

        // WARNING: currently, the send_message() of an inter-process
        //          connection immediately writes the message in the
        //          destination thread FIFO and immediately sends a
        //          signal; as a side effect we can immediatly forget
        //          about the status connection
        //
        f_communicator->remove_connection(f_status_connection);
        f_status_connection.reset();
    }
}


/** \brief Manage this computer.
 *
 * This function processes a MANAGE command received by this daemon.
 *
 * This command is the one that allows us to fully manage a remote
 * computer from snapmanager.cgi.
 *
 * We decided that we would use ONE global message which supports
 * many functions rather than defining many messages and possibly
 * have problems later because of some clashes.
 *
 * \param[in] message  The message being worked on.
 */
void manager_daemon::manage(snap::snap_communicator_message const & message)
{
    // check that the service sending a MANAGE command is the one we
    // expect (note that's not a very powerful security check, but overall
    // it allows us to make sure that snap_child() and other such services
    // do not contact us with a MANAGE command.)
    //
    QString const & service(message.get_sent_from_service());
    if(service != "snapmanagercgi")
    {
        snap::snap_communicator_message reply;
        reply.set_command("INVALID");
        reply.add_parameter("what", "command MANAGE cannot be sent from service " + service);
        f_messenger->send_message(reply);
        return;
    }

    // check the command requested by the sender, this is found in
    // the "function" parameter; functions must be specified in
    // uppercase just like commands
    //
    QString const function(message.get_parameter("function"));
    if(function.isEmpty())
    {
        snap::snap_communicator_message reply;
        reply.set_command("INVALID");
        reply.add_parameter("what", "command MANAGE must have a \"function\" parameter");
        f_messenger->send_message(reply);
        return;
    }

    switch(function[0].unicode())
    {
    case 'I':
        if(function == "INSTALL")
        {
            installer(message);
            return;
        }
        break;

    }

    {
        snap::snap_communicator_message reply;
        reply.set_command("INVALID");
        reply.add_parameter("what", "command MANAGE did not understand function \"" + function + "\"");
        f_messenger->send_message(reply);
    }
}




/** \brief Forword message to snapcommunicator.
 *
 * When we receive a message from our thread, and that message is not
 * directory to us (i.e. service name is the empty string of
 * snapmanagerdaemon) then the message needs to be sent to the
 * snapcommunicator.
 *
 * This function makes sure to send the message to the specified services
 * or even computers.
 *
 * At this time this is used to send the MANAGERSTATUS to all the
 * computer currently registered.
 *
 * \param[in] message  The message to forward to snapcommunicator.
 */
void manager_daemon::forward_message(snap::snap_communicator_message const & message)
{
    // make sure the messenger is still alive
    //
    if(f_messenger)
    {
        f_messenger->send_message(message);
    }
}


/** \brief Check whether the configuration file defined any front ends.
 *
 * Whenever a new status is found, it is sent to the front end computer.
 * This is generally done by the manager_status thread.
 *
 * This function allows the status check in the self plugin to know whether
 * this parameter is defined without having to reload the file.
 *
 * \return true if there are snapmanager frontends defined.
 */
bool manager_daemon::has_snapmanager_frontend() const
{
    return f_status_runner.has_snapmanager_frontend();
}


/** \brief Check whether the status runner thread is asking to stop ASAP.
 *
 * This function is expected to be called by plugins whenever their
 * retrieve_status() signal implementation function gets called. It
 * makes sure that the thread can quit quickly if asked to do so.
 *
 * This is important especially if some of your status gathering
 * functions are slow (i.e. run a command such as dpkg-query)
 *
 * \return true if the thread was asked to quit as soon as possible.
 */
bool manager_daemon::stop_now_prima() const
{
    return f_status_runner.stop_now_prima();
}


/** \brief A computer was unreachable, make sure to take note.
 *
 * The snapcommunicator will attempt to connect to remote computers
 * that are expected to run snapcommunicator, either with a direct
 * connection or to send it a GOSSIP message.
 *
 * If these connections fail, the snapcommunicator system sends an
 * UNREACHABLE message to all the local services currently
 * registered.
 *
 * Here we acknowledge the fact and make sure the mark that computer
 * as Down (it could still be marked as Up from previous runs.)
 *
 * \param[in] message  The message being worked on.
 */
void manager_daemon::unreachable_message(snap::snap_communicator_message const & message)
{
    // the parameter "who" must exist and define the IP address of the
    // computer that could not connect
    //
    QString const & addr(message.get_parameter("who"));

}





}
// namespace snap_manager
// vim: ts=4 sw=4 et
