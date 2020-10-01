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
 * \brief The Fast Journal CLI.
 *
 * This file handles CLI commands. You can simulate all the actions that
 * the other services run from a command line so that way you can see
 * the current status of the system.
 */

// self
//
#include    "cli.h"


// fastjournal lib
//
#include    <fastjournal/version.h>


// snaplogger lib
//
#include    <snaplogger/options.h>
#include    <snaplogger/message.h>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// snapdev lib
//
#include    <snapdev/poison.h>



namespace fastjournal
{


/** \brief Command line options.
 *
 * This table includes all the options supported by fastjournal-cli
 * on the command line.
 */
advgetopt::option const g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("verbose")
        , advgetopt::ShortName('v')
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
    .f_project_name = "fastjournal-client",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "FASTJOURNAL_CLIENT_OPTIONS",
    .f_configuration_files = nullptr,
    .f_configuration_filename = "client.conf",
    .f_configuration_directories = g_configuration_directories,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_SYSTEM_PARAMETERS
                         | advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = nullptr,
    .f_version = LIBFASTJOURNAL_VERSION_STRING,
    .f_license = "This software is licenced under the MIT",
    .f_copyright = "Copyright (c) 2020-" BOOST_PP_STRINGIZE(UTC_BUILD_YEAR) " by Made to Order Software Corporation",
};
#pragma GCC diagnostic pop



cli::cli(int argc, char * argv[])
    : f_opt(g_options_environment)
{
    snaplogger::add_logger_options(f_opt);
    f_opt.finish_parsing(argc, argv);
    snaplogger::process_logger_options(f_opt, "/etc/fastjournal/logger");
}


int cli::run()
{

    return 0;
}




} // fastjournal namespace
// vim: ts=4 sw=4 et
