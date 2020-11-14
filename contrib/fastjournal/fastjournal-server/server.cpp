/*
 * Copyright (c) 2020  Made to Order Software Corp.  All Rights Reserved
 *
 * https://snapwebsites.org/
 * contact@m2osw.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/** \file
 * \brief The fast journal backend server.
 *
 * This file starts the Fast Journal Backend service. This server accepts
 * connections from clients to receive requests for batch work.
 */

// self
//
#include    "server.h"


// fastjournal lib
//
#include    <fastjournal/version.h>


// snaplogger lib
//
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// C lib
//
#include    <unistd.h>


// snapdev lib
//
#include    <snapdev/poison.h>



namespace fastjournal
{


/** \brief Command line options.
 *
 * This table includes all the options supported by fastjournal-server
 * on the command line.
 */
advgetopt::option const g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("detach")
        , advgetopt::Flags(advgetopt::option_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_COMMAND_LINE
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("Whether to detach from the console.")
    ),
    advgetopt::define_option(
          advgetopt::Name("--")
        , advgetopt::Flags(advgetopt::command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_DEFAULT_OPTION
                    , advgetopt::GETOPT_FLAG_MULTIPLE>())
    ),
    advgetopt::end_options()
};


char const * const g_configuration_directories[] =
{
    "/etc/fastjournal",
    nullptr
};



// TODO: once we have stdc++20, remove all defaults
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "fastjournal-server",
    .f_group_name = nullptr,
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "FASTJOURNAL_SERVER_OPTIONS",
    .f_configuration_files = nullptr,
    .f_configuration_filename = "server.conf",
    .f_configuration_directories = g_configuration_directories,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_SYSTEM_PARAMETERS
                         | advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "This server is started on the backend where you want to persist the batch information.\n",
    .f_version = LIBFASTJOURNAL_VERSION_STRING,
    .f_license = "This software is licenced under the MIT",
    .f_copyright = "Copyright (c) 2020-" BOOST_PP_STRINGIZE(UTC_BUILD_YEAR) " by Made to Order Software Corporation",
};
#pragma GCC diagnostic pop



server::server(int argc, char * argv[])
    : f_opt(g_options_environment)
    , f_signal_handler(ed::signal_handler::get_instance())
{
    snaplogger::add_logger_options(f_opt);
    f_opt.finish_parsing(argc, argv);
    snaplogger::process_logger_options(f_opt, "/etc/fastjournal/logger");

    f_signal_handler->add_terminal_signals(ed::signal_handler::DEFAULT_SIGNAL_TERMINAL);
    f_signal_handler->add_ignore_signals(ed::signal_handler::DEFAULT_SIGNAL_IGNORE);

    // remove once we have the next version
    f_signal_handler->set_show_stack(ed::signal_handler::DEFAULT_SHOW_STACK);
}


int server::run()
{
    if(f_opt.is_defined("detach"))
    {
        // detach from console
        //
        pid_t const child(fork());
        if(child != 0)
        {
            // we're the parent
            if(child < 0)
            {
                SNAP_LOG_ERROR
                    << "fork() used for the --detach command failed."
                    << SNAP_LOG_SEND;
            }
            return 0;
        }

        // TODO: if we want to support a .pid file, here is where to
        //       implement that part
    }


    return 0;
}




} // fastjournal namespace
// vim: ts=4 sw=4 et
