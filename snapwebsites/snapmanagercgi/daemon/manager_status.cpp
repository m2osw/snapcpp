/*
 * Text:
 *      manager_status.cpp
 *
 * Description:
 *      The implementation of the status gathering thread.
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


// ourselves
//
#include "snapmanagerdaemon.h"

// our lib
//
#include "log.h"
//#include "process.h"

// C++ lib
//
//#include <fstream>

// C lib
//
//#include <fcntl.h>
//#include <sys/file.h>


namespace snap_manager
{


namespace
{

manager_status::status_function_t const g_status_functions[] =
{
    &manager_status::status_check_running_services,
    &manager_status::status_has_list_of_frontend_computers
};

}
// no name namespace



/** \brief Initialize the manager status.
 *
 * This constructor names the runner object "manager_status". It also
 * saves a reference to the status connection object which is used
 * to (1) send new MANAGERSTATUS and (2) receive STOP when we are done
 * and the thread needs to quit.
 */
manager_status::manager_status(status_connection::pointer_t sc)
    : snap_runner("manager_status")
    , f_status_connection(sc)
    //, f_running(true)
{
    f_status_connection->set_thread_b(this);
}


/** \brief Save the list of front end snapmanager.cgi computers.
 *
 * We really only need to forward the current status of the
 * cluster computer to a few front end computers accepting
 * requests from snapmanager.cgi (these should be 100% private
 * computers if you have an in house stack of computers.)
 *
 * The list includes hosts name. The same name you define in
 * the snapinit.conf file. If undefined there, then that name
 * would be your hostname.
 *
 * If the list is undefined (remains empty) then the messages
 * are broadcast to all computers.
 *
 * \param[in] snapmanager_frontend  The comma separated list of host names.
 */
void manager_status::set_snapmanager_frontend(QString const & snapmanager_frontend)
{
    f_snapmanager_frontend = snapmanager_frontend.split(",", QString::SkipEmptyParts);

    for(auto & f : f_snapmanager_frontend)
    {
        f = f.trimmed();
    }
}


/** \brief Thread used to permanently gather this server status.
 *
 * Each computer in the Snap! cluster should be running an instance
 * of the snapmanagerdaemon system. This will gather basic information
 * about the state of the each system and send the information to
 * all the computers who have snapmanager.cgi active.
 */
void manager_status::run()
{
    // run as long as the parent thread did not ask us to quit
    //
    QString status;

    for(;;)
    {
        // first gather a new set of statuses
        //
        f_server_status.clear();

        size_t const max_status(sizeof(g_status_functions) / sizeof(g_status_functions[0]));
        for(size_t idx(0); idx < max_status; ++idx)
        {
            // we may be asked to wake up immediately and at that point
            // we may notice that we are not expected to continue working
            //
            if(!continue_running() || !f_running)
            {
                return;
            }

            // get one status
            //
            (this->*g_status_functions[idx])();
        }

        // now convert the resulting f_server_status to a string,
        // make sure to place the "status" first since we load just
        // that when we show the entire cluster information
        //
        QString const previous_status(status);
        status.clear();
        QString status_string;
        for(auto const & ss : f_server_status)
        {
            if(ss.first == "status")
            {
                status_string = ss.second;
            }
            else
            {
                // sanity check to make sure user does not use '='
                // in the name (otherwise we will have a bug when
                // parsing that back to name / value later.)
                //
                if(ss.first.indexOf('=') != -1)
                {
                    throw std::runtime_error("the name of a status variable cannot include an '=' character.");
                }

                status += QString("%1=%2\n").arg(ss.first).arg(ss.second);
            }
        }

        // always prepend the status variable
        status = QString("status=%1\n%2").arg(status_string).arg(status);

        // generate a message to send the snapmanagerdaemon
        // (but only if the status changed, otherwise it would be a waste)
        //
        if(status != previous_status)
        {
            // TODO: designate a few computers that are to be used as
            //       front ends with snapmanager.cgi and only send the
            //       status information to those computers
            //
            if(f_snapmanager_frontend.isEmpty())
            {
                // user did not specify a list of front end hosts for
                // snapmanager.cgi so we instead broadcast the message
                // to all computers in the cluster (with a large cluster
                // this is not a good idea...)
                //
                snap::snap_communicator_message status_message;
                status_message.set_command("MANAGERSTATUS");
                status_message.set_service("*");
                status_message.add_parameter("status", status);
                f_status_connection->send_message(status_message);
            }
            else
            {
                // send the message only to the few specified frontends
                // so that way we can be sure to avoid send a huge pile
                // of messages throughout the entire cluster
                //
                for(auto const & f : f_snapmanager_frontend)
                {
                    snap::snap_communicator_message status_message;
                    status_message.set_command("MANAGERSTATUS");
                    status_message.set_server(f);
                    status_message.set_service("snapmanagerdaemon");
                    status_message.add_parameter("status", status);
                    f_status_connection->send_message(status_message);
                }
            }
        }

        // wait for messages or 1 minute
        //
        f_status_connection->poll(60 * 1000000LL);
    }
}


/** \brief Process a message sent to us by our "parent".
 *
 * This function gets called whenever the manager_daemon object
 * sends us a message.
 *
 * \param[in] message  The message to process.
 */
void manager_status::process_message(snap::snap_communicator_message const & message)
{
    QString const & command(message.get_command());

    switch(command[0].unicode())
    {
    case 'S':
        if(command == "STOP")
        {
            // this will stop the manager_status thread as soon as possible
            //
            f_running = false;
        }
        break;

    }
}


void manager_status::status_check_running_services()
{
    f_server_status["status"] = "Up";
}


void manager_status::status_has_list_of_frontend_computers()
{
    if(f_snapmanager_frontend.isEmpty())
    {
        f_server_status["warning:snapmanager_no_frontend"] = "The snapmanager_frontend variable is empty. This is most likely not what you want.";
    }
}






} // namespace snap_manager
// vim: ts=4 sw=4 et
