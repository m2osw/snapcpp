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

namespace
{
    const std::vector<std::string> g_configuration_files =
    {
        "/etc/snapwebsites/snapmanagerdaemon.conf"
    };

    const advgetopt::getopt::option g_snapmanagerdaemon_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "Usage: %p [-<opt>]",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "where -<opt> is one or more of:",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "connect",
            nullptr,
            "Define the address and port of the snapcommunicator service (i.e. 127.0.0.1:4040).",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "log_config",
            "/etc/snapwebsites/snapmanagerdaemon.properties",
            "Full path of log configuration file",
            advgetopt::getopt::optional_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "Show this help screen.",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "Show the version of the snapcgi executable.",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            nullptr,
            advgetopt::getopt::end_of_options
        }
    };
}
// no name namespace



namespace snap_manager
{


/** \brief Initialize the manager.
 *
 * The manager gets initialized with the argc and argv in case it
 * gets started from the command line. That way one can use --version
 * and --help, especially.
 *
 * \param[in] argc  The number of argiments defined in argv.
 * \param[in] argv  The array of arguments.
 */
manager_daemon::manager_daemon( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapmanagerdaemon_options, g_configuration_files, "SNAPMANAGERDAEMON_OPTIONS")
    , f_communicator_port(4040)
    , f_communicator_address("127.0.0.1")
{
    if(f_opt.is_defined("version"))
    {
        std::cerr << SNAPMANAGERDAEMON_VERSION_MAJOR << "." << SNAPMANAGERDAEMON_VERSION_MINOR << "." << SNAPMANAGERDAEMON_VERSION_PATCH << std::endl;
        exit(1);
    }
    if(f_opt.is_defined("help"))
    {
        f_opt.usage(advgetopt::getopt::no_error, "Usage: %s -<arg> ...\n", argv[0]);
        exit(1);
    }

    // read log_config and setup the logger
    std::string logconfig(f_opt.get_string("log_config"));
    snap::logging::configure_conffile( logconfig.c_str() );
}


manager_daemon::~manager_daemon()
{
}


bool manager_daemon::init()
{
    // --server-name (mandatory)
    f_server_name = f_opt.get_string("server-name").c_str();

    // --connect (mandatory)
    tcp_client_server::get_addr_port(f_opt.get_string("connect").c_str(), f_communicator_address, f_communicator_port, "tcp");

    // TODO: make us snapwebsites by default and root only when required...
    //       (and use RAII to do the various switches)
    //
    if(setuid(0) != 0)
    {
        perror("manager_daemon:setuid(0)");
        return false;
    }
    if(setgid(0) != 0)
    {
        perror("manager_daemon:setuid(0)");
        return false;
    }

    return true;
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

    // initialize the communicator and its connections
    //
    f_communicator = snap::snap_communicator::instance();

    // create a messenger to communicate with the Snap Communicator process
    // and snapmanager.cgi as required
    //
    f_messenger.reset(new manager_messenger(this, f_communicator_address.toUtf8().data(), f_communicator_port));
    f_communicator->add_connection(f_messenger);

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
            reply.add_parameter("list", "HELP,LOG,QUITTING,READY,STOP,UNKNOWN");

            f_messenger->send_message(reply);
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
            return;
        }
        break;

    case 'S':
        if(command == "STOP")
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
    QString const & service(message.get_service());
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









}
// namespace snap_manager
// vim: ts=4 sw=4 et
