// Snap Websites Server -- to send UDP signals to backends
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


int main(int argc, char *argv[])
{
    try
    {
        // create a server object
        snap::server::pointer_t s( snap::server::instance() );
        s->setup_as_backend();

        // parse the command line arguments (this also brings in the .conf params)
        s->config(argc, argv);

        // Now create the qt application instance
        //
        s->prepare_qtapp( argc, argv );

        // get the proper message (Excuse the naming convension...)
        QString msg(s->get_parameter("__BACKEND_URI"));
        if(msg.isEmpty())
        {
            msg = "PING";
        }

        // determine UDP server name
        if(s->get_parameter("__BACKEND_ACTION") == "sendmail")
        {
            s->udp_ping("sendmail_udp_signal", msg.toUtf8().data());
        }
        else if(s->get_parameter("__BACKEND_ACTION") == "pagelist")
        {
            s->udp_ping("pagelist_udp_signal", msg.toUtf8().data());
        }
        else if(s->get_parameter("__BACKEND_ACTION") == "snapserver"
             || s->get_parameter("__BACKEND_ACTION") == "server")
        {
            s->udp_ping("snapserver_udp_signal", msg.toUtf8().data());
        }
        else if(s->get_parameter("__BACKEND_ACTION") == "images")
        {
            s->udp_ping("images_udp_signal", msg.toUtf8().data());
        }
        else if(s->get_parameter("__BACKEND_ACTION") == "snapwatchdog")
        {
            // here is why we probably want to have one file with all the UDP info
            snap::snap_config wc;
            // TODO: hard coded path is totally WRONG!
            wc.read_config_file( "/etc/snapwebsites/snapwatchdog.conf" );
            if(wc.contains("snapwatchdog_udp_signal"))
            {
                s->set_parameter("snapwatchdog_udp_signal", wc["snapwatchdog_udp_signal"]);
            }
            s->udp_ping("snapwatchdog_udp_signal", msg.toUtf8().data());
        }
        else if(s->get_parameter("__BACKEND_ACTION") == "snapinit")
        {
            // here is why we probably want to have one file with all the UDP info
            s->set_parameter("snapinit_udp_signal", "127.0.0.1:4039");
            s->udp_ping("snapinit_udp_signal", msg.toUtf8().data());
        }
        else if(s->get_parameter("__BACKEND_ACTION") == "snapcommunicator")
        {
            // here is why we probably want to have one file with all the UDP info
            s->set_parameter("snapcommunicator_udp_signal", "127.0.0.1:4041");
            s->udp_ping("snapcommunicator_udp_signal", msg.toUtf8().data());
        }
        else
        {
            std::cerr << "error: unknown/unsupported action \"" << s->get_parameter("__BACKEND_ACTION") << "\"." << std::endl;
            s->exit(1);
            snap::NOTREACHED();
        }

        // exit via the server so the server can clean itself up properly
        s->exit(0);
        snap::NOTREACHED();

        return 0;
    }
    catch(std::exception const& e)
    {
        // clean error on exception
        std::cerr << "snapsignal: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
