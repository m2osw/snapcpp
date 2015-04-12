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

#include "snapwebsites.h"
#include "snap_config.h"
#include "not_reached.h"


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
 * |  |  Communicator  |    |     |  |  Communicator  |        | images     |
 * |  +----------------+    |     |  +----------------+        |  PING      |
 * |      ^                 |     |      ^                     |            |
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

class snap_communicator_server
{
public:
    typedef std::shared_ptr<snap_communicator_server>     pointer_t;

                                snap_communicator_server(int argc, char *argv[]);
                                snap_communicator_server(snap_communicator_server const & src) = delete;
    snap_communicator_server &  operator = (snap_communicator_server const & rhs) = delete;
                                ~snap_communicator_server();

    void                        run();

private:
    getopt_ptr_t                f_opt;
    tcp_server::pointer_t       f_tcp_server;
    udp_server::pointer_t       f_udp_server;
    std::string                 f_servername;
    std::string                 f_config_filename;
    controlled_vars::fboot_t    f_foreground;
    snap_config                 f_parameters;
};


snap_communicator_server::snap_communicator_server(int argc, char *argv[])
    : f_config_filename("/etc/snapwebsites/snapserver.conf")
{
    f_opt.reset(new advgetopt::getopt(argc, argv, g_snapserver_options, g_configuration_files, "SNAPCOMMUNICATOR_OPTIONS"));

    if(f_opt->is_defined("version"))
    {
        std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
        NOTREACHED();
    }

    f_servername = f_opt->get_program_name();
    f_foreground = !f_opt->is_defined("background");
    f_debug = f_opt->is_defined("debug");

    openlog("snapcommunicator", LOG_NDELAY | LOG_PID, LOG_DAEMON);

    bool help(false);

    snap_config::parameter_map_t cmd_line_params;
    if(f_opt->is_defined("param"))
    {
        int const max_params(f_opt->size("param"));
        for(int idx(0); idx < max_params; ++idx)
        {
            QString param(QString::fromUtf8(f_opt->get_string("param", idx).c_str()));
            int const p(param.indexOf('='));
            if(p == -1)
            {
                SNAP_LOG_FATAL() << "fatal error: unexpected parameter \"--param " << f_opt->get_string("param", idx) << "\". No '=' found in the parameter definition. (in server::config())";
                syslog(LOG_CRIT, "unexpected parameter \"--param %s\". No '=' found in the parameter definition. (in server::config())", f_opt->get_string("param", idx).c_str());
                help = true;
            }
            else
            {
                // got a user defined parameter
                QString const name(param.left(p));
                f_parameters[name] = param.mid(p + 1);
                cmd_line_params[name] = ""; // the value is not important here
            }
        }
    }

    f_parameters.set_cmdline_params(cmd_line_params);

    if(f_opt->is_defined("config"))
    {
        f_config_filename = f_opt->get_string("config").c_str();
    }
    f_parameters.read_config_file(f_config_filename);

    if(f_parameters["tcp_addr"] == ""
    || f_parameters["tcp_port"] == ""
    || f_parameters["udp_addr"] == ""
    || f_parameters["udp_port"] == "")
    {
        SNAP_LOG_FATAL() << "fatal error: unexpected parameter \"--param " << f_opt->get_string("param", idx) << "\". No '=' found in the parameter definition. (in server::config())";
        syslog(LOG_CRIT, "unexpected parameter \"--param %s\". No '=' found in the parameter definition. (in server::config())", f_opt->get_string("param", idx).c_str());
        help = true;
    }

    // any errors and the help flag is set to true
    if(help || f_opt->is_defined("help"))
    {
        usage();
        exit(1);
    }

    if(f_debug)
    {
        // Override output level and force it to be debug
        logging::setLogOutputLevel(logging::LOG_LEVEL_DEBUG);
    }

    f_tcp_server.reset(new tcp_server(f_parameters["tcp_addr"], f_parameters["tcp_port"].toInt(), 100, true, false));
    f_udp_server.reset(new udp_server(udp_addr, udp_port.toInt()));
}


snap_communicator_server::~snap_communicator_server()
{
}


void snap_communicator::usage()
{
    std::string const server_name(f_servername.empty() ? "snapcommunicator" : f_servername);

    std::cerr << "Configuration File: " << f_config_filename << std::endl << std::endl;

    f_opt->usage(advgetopt::getopt::no_error, "Usage: %s -<arg> ...\n", server_name.c_str());
    NOTREACHED();
    exit(1);
}


snap_communicator_server::run()
{
}




int main(int argc, char *argv[])
{
    try
    {
        // create a server object
        snap::server::pointer_t s( new snap_communicator_server );
        s->setup_as_backend();

        // parse the command line arguments (this also brings in the .conf params)
        s->config(argc, argv);

        // Now create the qt application instance
        //
        s->prepare_qtapp( argc, argv );

// TODO: actually implement that thing (most of the code will be in the
//       library, though)

        // // get the proper message (Excuse the naming convension...)
        // QString msg(s->get_parameter("__BACKEND_URI"));
        // if(msg.isEmpty())
        // {
        //     msg = "PING";
        // }

        // // determine UDP server name
        // if(s->get_parameter("__BACKEND_ACTION") == "sendmail")
        // {
        //     s->udp_ping("sendmail_udp_signal", msg.toUtf8().data());
        // }
        // else if(s->get_parameter("__BACKEND_ACTION") == "pagelist")
        // {
        //     s->udp_ping("pagelist_udp_signal", msg.toUtf8().data());
        // }
        // else if(s->get_parameter("__BACKEND_ACTION") == "snapserver"
        //      || s->get_parameter("__BACKEND_ACTION") == "server")
        // {
        //     s->udp_ping("snapserver_udp_signal", msg.toUtf8().data());
        // }
        // else if(s->get_parameter("__BACKEND_ACTION") == "images")
        // {
        //     s->udp_ping("images_udp_signal", msg.toUtf8().data());
        // }
        // else if(s->get_parameter("__BACKEND_ACTION") == "snapwatchdog")
        // {
        //     // here is why we probably want to have one file with all the UDP info
        //     snap::snap_config wc;
        //     // TODO: hard coded path is totally WRONG!
        //     wc.read_config_file( "/etc/snapwebsites/snapwatchdog.conf" );
        //     if(wc.contains("snapwatchdog_udp_signal"))
        //     {
        //         s->set_parameter("snapwatchdog_udp_signal", wc["snapwatchdog_udp_signal"]);
        //     }
        //     s->udp_ping("snapwatchdog_udp_signal", msg.toUtf8().data());
        // }
        // else
        // {
        //     std::cerr << "error: unknown/unsupported action \"" << s->get_parameter("__BACKEND_ACTION") << "\"." << std::endl;
        //     s->exit(1);
        //     snap::NOTREACHED();
        // }

        // exit via the server so the server can clean itself up properly
        s->exit(0);
        snap::NOTREACHED();

        return 0;
    }
    catch(std::exception const& e)
    {
        // clean error on exception
        std::cerr << "snapcommunicator: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
