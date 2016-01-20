// Snap Websites Server -- snap websites server
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

#include "snapwebsites.h"

#include "log.h"
#include "not_used.h"
#include "signal.h"
#include "snap_backend.h"
#include "snap_cassandra.h"
#include "tcp_client_server.h"

#include <sstream>

#include <QFile>
#include <QDirIterator>
#include <QHostAddress>
#include <QCoreApplication>
#include <QTextCodec>

#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

#include "poison.h"


/** \file
 * \brief This file represents the Snap! Server.
 *
 * The snapwebsites.cpp and corresponding header file represents the Snap!
 * Server. When you create a server object, its code is available here.
 * The server can listen for client connections or run backend processes.
 */


/** \mainpage
 * \brief Snap! C++ Documentation
 *
 * \section introduction Introduction
 *
 * The Snap! C++ environment includes a library, plugins, tools, and
 * the necessary executables to run the snap server: a fast C++
 * CMS (Content Management System).
 *
 * \section database The Database Environment in Snap! C++
 *
 * The database makes use of a Cassandra cluster. It is accessed using
 * the libQtCassandra class.
 *
 * \section todo_xxx_tbd Usage of TODO, XXX, and TBD
 *
 * The TODO mark within the code is used to talk about things that are
 * necessary but not yet implemented. The further we progress the less
 * of these we should see as we implement each one of them as required.
 *
 * The XXX mark within the code are things that should be done, although
 * it is most generally linked with a question: is it really necessary?
 * It can also be a question about the hard coded value (is 5 minutes
 * the right amount of time to wait between random session changes?)
 * In most cases these should disappear as we get the answer to the
 * questions. In effect they are between the TODO and the TBD.
 *
 * The TBD mark is a pure question: Is that code valid? A TBD does not
 * mean that the code needs change just that we cannot really decide,
 * at the time it get written, whether it is correct or not. With time
 * (especially in terms of usage) we should be able to answer the
 * question and transform the question in a comment explaining why
 * the code is one way or the other. Of course, if proven wrong, the
 * code is to be changed to better fit the needs.
 */


/** \brief The snap namespace.
 *
 * The snap namespace is used throughout all the snap objects: libraries,
 * plugins, tools.
 *
 * Plugins make use of a sub-namespace within the snap namespace.
 */
namespace snap
{


/** \brief Get a fixed name.
 *
 * The Snap! Server makes use of a certain number of fixed names
 * which instead of being defined in macros are defined here as
 * static strings. To retrieve one of the strings, call the function
 * with the appropriate index.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    // Names that are really considered low level
    case name_t::SNAP_NAME_SERVER:
        return "Snap! Server";

    case name_t::SNAP_NAME_CONTEXT:
        return "snap_websites";

    case name_t::SNAP_NAME_INDEX: // name used for the domains and websites indexes
        return "*index*"; // this is a row name inside the domains/websites tables

    case name_t::SNAP_NAME_DOMAINS: // domain/sub-domain canonicalization
        return "domains";

    case name_t::SNAP_NAME_WEBSITES: // remaining of URL canonicalization
        return "websites";

    case name_t::SNAP_NAME_SITES: // website global settings
        return "sites";

    case name_t::SNAP_NAME_BACKEND: // backend progress
        return "backend";

    // names used by CORE (server and snap_child)
    case name_t::SNAP_NAME_CORE_ADMINISTRATOR_EMAIL:
        return "core::administrator_email";

    case name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER:
        return "Content-Type";

    case name_t::SNAP_NAME_CORE_COOKIE_DOMAIN:
        return "core::cookie_domain";

    case name_t::SNAP_NAME_CORE_HTTP_ACCEPT_LANGUAGE:
        return "HTTP_ACCEPT_LANGUAGE";

    case name_t::SNAP_NAME_CORE_HTTP_USER_AGENT:
        return "HTTP_USER_AGENT";

    case name_t::SNAP_NAME_CORE_LAST_DYNAMIC_UPDATE:
        return "core::last_dynamic_update";

    case name_t::SNAP_NAME_CORE_LAST_UPDATED:
        return "core::last_updated";

    case name_t::SNAP_NAME_CORE_LOCATION_HEADER:
        return "Location";

    case name_t::SNAP_NAME_CORE_ORIGINAL_RULES:
        return "core::original_rules";

    case name_t::SNAP_NAME_CORE_PARAM_DEFAULT_PLUGINS:
        return "default_plugins";

    case name_t::SNAP_NAME_CORE_PARAM_PLUGINS:
        return "plugins";

    case name_t::SNAP_NAME_CORE_PARAM_PLUGINS_PATH:
        return "plugins_path";

    case name_t::SNAP_NAME_CORE_PLUGINS:
        return "core::plugins";

    case name_t::SNAP_NAME_CORE_PLUGIN_THRESHOLD:
        return "core::plugin_threshold";

    case name_t::SNAP_NAME_CORE_REDIRECT:
        return "core::redirect";

    case name_t::SNAP_NAME_CORE_REMOTE_ADDR:
        return "REMOTE_ADDR";

    case name_t::SNAP_NAME_CORE_REQUEST_METHOD:
        return "REQUEST_METHOD";

    case name_t::SNAP_NAME_CORE_REQUEST_URI:
        return "REQUEST_URI";

    case name_t::SNAP_NAME_CORE_RETRY_AFTER_HEADER:
        return "Retry-After";

    case name_t::SNAP_NAME_CORE_RULES:
        return "core::rules";

    case name_t::SNAP_NAME_CORE_SERVER_PROTOCOL:
        return "SERVER_PROTOCOL";

    case name_t::SNAP_NAME_CORE_SITE_LONG_NAME:
        return "core::site_long_name";

    case name_t::SNAP_NAME_CORE_SITE_NAME:
        return "core::site_name";

    case name_t::SNAP_NAME_CORE_SITE_READY:
        return "core::site_ready";

    case name_t::SNAP_NAME_CORE_SITE_SHORT_NAME:
        return "core::site_short_name";

    case name_t::SNAP_NAME_CORE_SNAPBACKEND:
        return "snapbackend";

    case name_t::SNAP_NAME_CORE_STATUS_HEADER:
        return "Status";

    case name_t::SNAP_NAME_CORE_USER_COOKIE_NAME:
        return "core::user_cookie_name";

    case name_t::SNAP_NAME_CORE_X_POWERED_BY_HEADER:
        return "X-Powered-By";

    default:
        // invalid index
        throw snap_logic_exception(QString("invalid name_t::SNAP_NAME_CORE_... (%1)").arg(static_cast<int>(name)));

    }
    NOTREACHED();
}


// definitions from the plugins so we can define the name and filename of
// the server plugin
namespace plugins
{
extern QString g_next_register_name;
extern QString g_next_register_filename;
}


/** \brief Hidden Snap! Server namespace.
 *
 * This namespace encompasses global variables only available to the
 * server code.
 */
namespace
{
    /** \brief List of configuration files.
     *
     * This variable is used as a list of configuration files. It may be
     * empty.
     */
    std::vector<std::string> const g_configuration_files;

    /** \brief Command line options.
     *
     * This table includes all the options supported by the server.
     */
    advgetopt::getopt::option const g_snapserver_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: %p [-<opt>]",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "where -<opt> is one or more of:",
            advgetopt::getopt::help_argument
        },
        {
            'a',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "action",
            nullptr,
            "Specify a server action.",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "add-host",
            nullptr,
            "Add a host to the lock table. Remember that you cannot safely do that while any one of the servers are running. This should not be used anymore since that feature was moved to the snapmanager instead.",
            advgetopt::getopt::optional_argument
        },
        {
            'b',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "background",
            nullptr,
            "Detaches the server to the background (default is stay in the foreground).",
            advgetopt::getopt::no_argument
        },
        {
            'c',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "config",
            nullptr,
            "Specify the configuration file to load at startup.",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "cron-action",
            nullptr,
            "Specify a server CRON action.",
            advgetopt::getopt::optional_argument
        },
        {
            'd',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "debug",
            nullptr,
            "Outputs debug logs. Perform additional checks in various places.",
            advgetopt::getopt::no_argument
        },
        {
            'f',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "logfile",
            nullptr,
            "Output log file to write to. Overrides the setting in the configuration file.",
            advgetopt::getopt::required_argument
        },
#ifdef SNAP_NO_FORK
        {
            'k',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "nofork",
            nullptr,
            "If set, this switch causes the server not to fork when a child is launched. This should never be use for a production server!",
            advgetopt::getopt::optional_argument
        },
#endif
        {
            'l',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "logconf",
            nullptr,
            "Log configuration file to read from. Overrides log_server / log_config in the configuration file.",
            advgetopt::getopt::required_argument
        },
        {
            'n',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "no-log",
            nullptr,
            "Don't create a logfile, just output to the console.",
            advgetopt::getopt::no_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "Show usage and exit.",
            advgetopt::getopt::no_argument
        },
        {
            'p',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "param",
            nullptr,
            "Define one or more server parameters on the command line (-p name=value).",
            advgetopt::getopt::required_multiple_argument
        },
        {
            '\0',
            0,
            "prepare-cassandra",
            nullptr,
            "Create the cassandra \"domans\" and \"websites\" tables and exit. This should not be used anymore since that feature was moved to the snapmanager instead.",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "version",
            nullptr,
            "Show the version of the server and exit.",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "filename",
            nullptr,
            nullptr, // hidden argument in --help screen
            advgetopt::getopt::default_multiple_argument
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

    struct connection_t
    {
        snap_communicator::pointer_t                    f_communicator;
        snap_communicator::snap_connection::pointer_t   f_listener;
        snap_communicator::snap_connection::pointer_t   f_child_death_listener;
        snap_communicator::snap_connection::pointer_t   f_messager;
    };

    /** \brief The pointers to communicator elements.
     *
     * The communicator we use to run the server events.
     *
     * \todo
     * At some point we need to look into whether it would be possible
     * for us to use a shared pointer. At this point the g_connection
     * gets allocated and never deleted (not a big deal since it is
     * ONE instance for the entire time the process is running.)
     */
    connection_t *          g_connection;
}
//namespace


//#pragma message "Why do we even have this? Adding a smart pointer causes a crash when the server detaches, so commented out."
// Note: We need the argc/argv when we create the application and those are
//       not available when we create the server (they are not passed along)
//       but I suppose the server could be ameliorated for that purpose...
QPointer<QCoreApplication> g_application;


/** \brief Server instance.
 *
 * The g_instance variable holds the current server instance.
 */
std::shared_ptr<server> server::g_instance;


/** \brief Return the server version.
 *
 * This function can be used to verify that the server version is
 * compatible with your plugin or to display the version.
 *
 * To compare versions, however, it is suggested that you make
 * use of the version_major(), version_minor(), and version_patch()
 * instead.
 *
 * \return A pointer to a constant string representing the server version.
 */
char const * server::version()
{
    return SNAPWEBSITES_VERSION_STRING;
}


/** \brief Return the server major version.
 *
 * This function returns the major version of the server. This can be used
 * to verify that you have the correct version of the server to run your
 * plugin.
 *
 * This is a positive number.
 *
 * \return The server major version as an integer.
 */
int server::version_major()
{
    return SNAPWEBSITES_VERSION_MAJOR;
}


/** \brief Return the server minor version.
 *
 * This function returns the minor version of the server. This can be used
 * to verify that you have the correct version of the server to run your
 * plugin.
 *
 * This is a positive number.
 *
 * \return The server minor version as an integer.
 */
int server::version_minor()
{
    return SNAPWEBSITES_VERSION_MINOR;
}


/** \brief Return the server patch version.
 *
 * This function returns the patch version of the server. This can be used
 * to verify that you have the correct version of the server to run your
 * plugin.
 *
 * This is a positive number.
 *
 * \return The server patch version as an integer.
 */
int server::version_patch()
{
    return SNAPWEBSITES_VERSION_PATCH;
}


/** \brief Get the server instance.
 *
 * The main central hub is the server object.
 *
 * Like all the plugins, there can be only one server instance.
 * Because of that, it is made a singleton which means whichever
 * plugin that first needs the server can get a pointer to it at
 * any time.
 *
 * \note
 * This function is not thread safe.
 *
 * \return A pointer to the server.
 */
server::pointer_t server::instance()
{
    if( !g_instance )
    {
        // plugins registration make use of those two variables
        plugins::g_next_register_name = "server";
        plugins::g_next_register_filename = __FILE__;

        g_instance.reset( new server );

        plugins::g_next_register_name.clear();
        plugins::g_next_register_filename.clear();
    }
    return g_instance;
}


/** \brief Return the current server pointer.
 *
 * When deriving from the snap server, you cannot put the pointer in
 * another variable than the g_instance pointer. However, you cannot
 * allocate the right type of server if you call the instance()
 * function because it does not use a factory model that allows you
 * to create any type of server.
 *
 * Instead, you call this get_instance() function and if it returns
 * a pointer, you create your own server and save its pointer in
 * the g_instance variable using the set_instance() function.
 *
 * \code
 *      pointer_t my_server(get_instance());
 *      if(!my_server)
 *      {
 *          ...
 *          set_instance(new my_server_class);
 *          ...
 *      }
 * \endcode
 *
 * \return The server instance if defined, may return a null pointer.
 */
server::pointer_t server::get_instance()
{
    return g_instance;
}


/** \brief When creating a server using a different factory.
 *
 * This function is used when one create a server using a different
 * factory than the main Snap Server factor (i.e. the
 * server::instance() function.) For example, the watchdog_server
 * uses this function to save a pointer of itself here.
 *
 * Note that the other server must be derived from the snap::server
 * class, obviously.
 *
 * See the get_instance() for more information about how to allocate
 * a new server. As an example, check out the lib/snapwatchdog.cpp file.
 *
 * \param[in] other_server  The other type of server to save in this instance.
 *
 * \return A pointer to the new instance of the server.
 */
server::pointer_t server::set_instance(pointer_t other_server)
{
    if(g_instance)
    {
        throw snap_logic_exception("server::set_instance() cannot be called more than once.");
    }

    return g_instance = other_server;
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString server::icon() const
{
    return "/images/snap/snap-logo-64x64.png";
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString server::description() const
{
    return "The server plugin is hard coded in the base of the system."
        " It handles the incoming and outgoing network connections."
        " The server handles a number of messages that are global.";
}


/** \brief Return our dependencies.
 *
 * The server has no dependencies so this function returns an empty string.
 *
 * \return An empty string.
 */
QString server::dependencies() const
{
    return QString();
}


/** \brief Required bootstrap definition.
 *
 * This function does nothing as the server object is already properly
 * initialized by the time this function gets called.
 *
 * However, since it is a pure virtual function, we suppose that it
 * is required.
 */
void server::bootstrap(snap_child * snap)
{
    // virtual function stub
    NOTUSED(snap);
}


/** \brief Update the server, the function is mandatory.
 *
 * This function is here because it is a pure virtual in the plug in. At this
 * time it does nothing and it probably will never have actual updates.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t server::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize the server.
 *
 * This function initializes the server.
 *
 * \note
 * The server is also a plugin. This is useful for having support for
 * signals in the server.
 */
server::server()
    : f_config_filename("/etc/snapwebsites/snapserver.conf")
{
    set_version(SNAPWEBSITES_VERSION_MAJOR, SNAPWEBSITES_VERSION_MINOR);

    // default parameters -- we may want to have a separate function and
    //                       maybe some clear separate variables?
    f_parameters["listen"]         = "0.0.0.0:4004";
    f_parameters["plugins_path"]   = "/usr/lib/snapwebsites/plugins";
    f_parameters["qs_action"]      = "a";
    f_parameters["qs_hit"]         = "hit";
    f_parameters["server_name"]    = "";
    f_parameters["wait_interval"]  = "5";  // default: 1 second
    f_parameters["wait_max_tries"] = "-1"; // default: indefinite wait
}


/** \brief Clean up the server.
 *
 * Since the server is a singleon, it never gets deleted while running.
 * Since we use a bare pointer, it should never go out of scope, thus
 * this function should never be called.
 */
server::~server()
{
    for( auto child : f_children_waiting )
    {
        delete child;
    }
    f_children_waiting.clear();
    //
    for( auto child : f_children_running )
    {
        if( child )
        {
            child->kill();
        }
        //
        delete child;
    }
    f_children_running.clear();

    // Destroy the QApplication instance.
    //
    g_application = nullptr;
}


/** \brief Exit the server.
 *
 * This function exists the program by calling the exit(3) function from
 * the C library. Before doing so, though, it will first make sure that
 * the server is cleaned up as required.
 *
 * \param[in] code  The exit code, generally 0 or 1.
 */
void server::exit( int const code )
{
    // Destroy the snapwebsites server instance.
    //
    g_instance.reset();
    g_application = nullptr;    // Make sure the QApplication instance is really deleted.

    // Call the C exit(3) function.
    //
    ::exit(code);

    // Sanity check!
    //
    NOTREACHED();
}


/** \brief Print out usage information to start the server.
 *
 * This function prints out a usage message that describes the arguments
 * that the server accepts on the command line.
 *
 * The function calls exit(1) and never returns.
 */
void server::usage()
{
    std::string server_name( "snapserver" );
    if( !f_servername.empty() )
    {
        server_name = f_servername;
    }

    std::cerr << "Configuration File: " << f_config_filename << std::endl << std::endl;

    f_opt->usage(advgetopt::getopt::no_error, "Usage: %s -<arg> ...\n", server_name.c_str());
    NOTREACHED();
    exit(1);
}


/** \brief Mark the server object as a backend tool instead.
 *
 * This function is called by the backend tool to mark the server
 * as a command line tool rather than a server. In general, this
 * is ignored, but there are a few cases where it is checked to
 * make sure that everything works as expected.
 *
 * The function can be called as many times as necessary.
 */
void server::setup_as_backend()
{
    f_backend = true;
}


/** \fn server::is_backend() const;
 * \brief Check whether the server is setup as a backend.
 *
 * This function returns false unless the setup_as_backend()
 * funciton was called.
 *
 * \return true if this is a server, false if this is used as a command line tool
 */


/** \brief Print the version string to stderr.
 *
 * This function prints out the version string of this server to the standard
 * error stream.
 *
 * This is a virtual function so that way servers and daemons that derive
 * from snap::server have a chance to show their own version.
 */
void server::show_version()
{
    std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
}


/** \brief Configure the server.
 *
 * This function parses the command line arguments and reads the
 * configuration file.
 *
 * By default, the configuration file is defined as:
 *
 * \code
 * /etc/snapwebsites/snapserver.conf
 * \endcode
 *
 * The user may use the --config argument to use a different file.
 *
 * The function does not return if any of the arguments generate an
 * error or if the configuration file has an invalid parameter.
 *
 * \note
 * In this function we still use syslog() to log errors because the
 * logger is initialized at the end of the function once we got
 * all the necessary information to initialize the logger. Later we
 * may want to record the configuration file errors and log them
 * if we can still properly initialize the logger.
 *
 * \param[in] argc  The number of arguments in argv.
 * \param[in] argv  The array of argument strings.
 */
void server::config(int argc, char * argv[])
{
    // Stop on these signals, log them, then terminate.
    //
    signal( SIGSEGV, sighandler );
    signal( SIGBUS,  sighandler );
    signal( SIGFPE,  sighandler );
    signal( SIGILL,  sighandler );
    signal( SIGTERM, sighandler );
    signal( SIGINT,  sighandler );

    // Force timezone to UTC/GMT so it does not vary between installations
    // (i.e. you could have Snap servers all over the world!)
    //
    setenv("TZ", "", 1);  // default is UTC
    tzset();

    // Force the locale to "C" so we do not get too many surprises.
    // Users may change their locale settings so a child may change
    // the locale for display formatting needs.
    //
    char const * default_locale(std::setlocale(LC_ALL, "C.UTF-8"));
    if(default_locale == nullptr)
    {
        std::locale const & loc(std::locale("C"));
        std::locale::global(loc);  // default depends on LC_... vars
        std::cin.imbue(loc);
        std::cout.imbue(loc);
        std::cerr.imbue(loc);
    }
    else
    {
        // if we can use UTF-8, do so rather than just plain C
        std::locale const & loc(std::locale("C.UTF-8"));
        std::locale::global(loc);  // default depends on LC_... vars
        std::cin.imbue(loc);
        std::cout.imbue(loc);
        std::cerr.imbue(loc);
    }
    // TBD: we initialize the Qt library later, I do not think it will
    //      change the locale on us, but this is a TBD until otherwise
    //      proven to be safe... (see QLocale) -- and that could change
    //      when we start using Qt 5.x

    // Parse command-line options...
    //
    f_opt.reset(
        new advgetopt::getopt( argc, argv, g_snapserver_options, g_configuration_files, "SNAPSERVER_OPTIONS" )
    );

    if(f_opt->is_defined("version"))
    {
        show_version();
        exit(1);
        NOTREACHED();
    }

    // We want the servername for later.
    //
    f_servername = f_opt->get_program_name();

    // Keep the server in the foreground?
    //
    f_foreground = !f_opt->is_defined( "background" );

    // Output log to stdout. Implies foreground mode.
    //
    f_debug = f_opt->is_defined( "debug" );

    // initialize the syslog() interface
    openlog(f_servername.c_str(), LOG_NDELAY | LOG_PID, LOG_DAEMON);

    bool help(false);

    snap_config::parameter_map_t cmd_line_params;
    if(f_opt->is_defined("param"))
    {
        int const max_params(f_opt->size("param"));
        for(int idx(0); idx < max_params; ++idx)
        {
            QString const param(QString::fromUtf8(f_opt->get_string("param", idx).c_str()));
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

    if( f_opt->is_defined( "filename" ) )
    {
        std::string const filename(f_opt->get_string("filename"));
        if( f_backend )
        {
            f_parameters["__BACKEND_URI"] = filename.c_str();
        }
        else
        {
            // If not backend, "--filename" is not currently useful.
            //
            SNAP_LOG_FATAL() << "fatal error: unexpected standalone parameter \"" << filename << "\", server not started. (in server::config())";
            syslog( LOG_CRIT, "unexpected standalone parameter \"%s\", server not started. (in server::config())", filename.c_str() );
            help = true;
        }
    }

    if( f_opt->is_defined( "action" ) )
    {
        std::string const action(f_opt->get_string("action"));
        if( f_backend )
        {
            f_parameters["__BACKEND_ACTION"] = action.c_str();
        }
        else
        {
            // If not backend, "--action" does not make sense.
            //
            SNAP_LOG_FATAL("unexpected command line option \"--action ")(action)("\", server not started as backend. (in server::config())");
            syslog( LOG_CRIT, "unexpected command line option \"--action %s\", server not started as backend. (in server::config())", action.c_str() );
            help = true;
        }
        if( f_opt->is_defined( "cron-action" ) )
        {
            // --action and --cron-action are mutually exclusive
            //
            SNAP_LOG_FATAL("command line options \"--action\" and \"--cron-action\" are mutually exclusive, server not started as backend. (in server::config())");
            syslog( LOG_CRIT, "command line options \"--action\" and \"--cron-action\" are mutually exclusive, server not started as backend. (in server::config())" );
            help = true;
        }
    }

    if( f_opt->is_defined( "cron-action" ) )
    {
        std::string const cron_action(f_opt->get_string("cron-action"));
        if( f_backend )
        {
            f_parameters["__BACKEND_CRON_ACTION"] = cron_action.c_str();
        }
        else
        {
            // If not backend, "--cron-action" does not make sense.
            //
            SNAP_LOG_FATAL("fatal error: unexpected command line option \"--cron-action ")(cron_action)("\", server not started as backend. (in server::config())");
            syslog( LOG_CRIT, "unexpected command line option \"--cron-action %s\", server not started as backend. (in server::config())", cron_action.c_str() );
            help = true;
        }
    }

    f_parameters.set_cmdline_params( cmd_line_params );

    // handle configuration file
    if( f_opt->is_defined( "config" ) )
    {
        f_config_filename = f_opt->get_string( "config" ).c_str();
    }
    f_parameters.read_config_file( f_config_filename );

    if( help || f_opt->is_defined( "help" ) )
    {
        usage();
        exit(1);
    }

    // the name of the server is mandatory, use hostname by default
    if(f_parameters["server_name"] == "")
    {
        char host[HOST_NAME_MAX + 1];
        if(gethostname(host, sizeof(host)) != 0)
        {
            std::stringstream ss;
            ss << "hostname is not available as the server name";
            SNAP_LOG_ERROR() << ss.str() << ".";
            syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
            exit(1);
        }
        f_parameters["server_name"] = host;
    }

    // Finally we can initialize the log system
    //
    if( f_opt->is_defined( "no-log" ) )
    {
        // Override log_config and output only to the console
        //
        logging::configure_console();
    }
    else if( f_opt->is_defined("logfile") )
    {
        // Override the output logfile specified in the configuration file.
        //
        logging::configure_logfile( f_opt->get_string( "logfile" ).c_str() );
    }
    else if( f_opt->is_defined("logconf") )
    {
        logging::configure_conffile( f_opt->get_string( "logconf" ).c_str() );
    }
    else
    {
        // Read the log configuration file and use it to specify the appenders
        // and log level. If a server version exists and the server is
        // available then use the loggging server.
        //
        QString const log_server( f_parameters["log_server"] );
        f_using_logging_server = logging::is_loggingserver_available( log_server );
        QString const log_config( f_using_logging_server
                                    ? log_server
                                    : f_parameters["log_config"] );
        if( log_config.isEmpty() )
        {
            // Fall back to output to the console
            //
            logging::configure_console();
        }
        else
        {
            // Configure the logging system according to the log configuration.
            //
            logging::configure_conffile( log_config );
        }
    }

#ifdef SNAP_NO_FORK
    SNAP_LOG_WARNING() << "SNAP_NO_FORK is defined! This is NOT a production-ready build!";
    if( f_opt->is_defined("nofork") )
    {
        SNAP_LOG_INFO() << "--nofork specified: snap_child will not fork and server will terminate.";
        f_nofork = true;
    }
#endif

    if( f_debug )
    {
        // Force the logger level to DEBUG or TRACE
        //
        logging::reduce_log_output_level( logging::log_level_t::LOG_LEVEL_DEBUG );
    }
}


/** \brief Retrieve the number of threads in this process.
 *
 * This function counts the total number of threads that this process is
 * currently running with.
 *
 * \todo
 * We should make sure that the count is 1 before any call to fork().
 *
 * \return The number of running threads in this process.
 */
size_t server::thread_count()
{
    struct stat task;

    if(stat("/proc/self/task", &task) != 0)
    {
        return -1;
    }

    return task.st_nlink - 2;
}


/** \brief Retrieve one of the configuration file parameters.
 *
 * This function returns the value of a named parameter. The
 * parameter is defined in the configuration file, it may also
 * be given a default value when the server is initialized.
 *
 * The following are the parameters currently supported by
 * the core system. Additional parameters may be defined by
 * plugins. Remember that parameters defined in the
 * configuration file are common to ALL the websites and at
 * this point plugins do not have direct access to the
 * get_parameter() function (look at the get_site_parameter()
 * function in the snap_child class as a better alternative
 * for plugins.)
 *
 * \li backend_nice -- the nice value to use with backends; if undefined, keep
 *     the default nice value (i.e. 0)
 * \li cassandra_host -- the IP address or server name to Cassandra; default is localhost
 * \li cassandra_port -- the port to use to connect to Cassandra; default is 9160
 * \li data_path -- path to the directory holding the system data (images, js, css, counters, etc.)
 * \li default_plugins -- list of default plugins to initialize a new website
 * \li listen -- address:port to listen to (default 0.0.0.0:4004)
 * \li plugins -- path to the list of plugins
 * \li qs_action -- the variable holding the action over this path ("view" if not specified)
 * \li max_pending_connections -- the number of connections that can wait in
 *     the server queue, there is Snap default (i.e. the Qt TCP server default
 *     is used if undefined, which in most cases means the system of 5.)
 * \li server_name -- the name of the server, defaults to gethostname()
 * \li timeout_wait_children -- the amount of time to wait before checking on
 *     the existing children; cannot be less than 100ms; defaults to 5,000ms
 *
 * \param[in] param_name  The name of the parameter to retrieve.
 *
 * \sa set_parameter()
 *
 * \return The value of the specified parameter.
 */
QString server::get_parameter(QString const & param_name) const
{
    if(f_parameters.contains(param_name))
    {
        return f_parameters[param_name];
    }
    return "";
}


/** \brief Set one of the configuration file parameters.
 *
 * \param[in] param_name  The name of the parameter to retrieve.
 * \param[in] value       The value to put into the parameter.
 *
 * \sa get_parameter()
 */
void server::set_parameter( const QString& param_name, const QString& value )
{
    f_parameters[param_name] = value;
}


/** \brief Set up the Qt4 application instance.
 *
 * This function creates the Qt4 application instance for application-wide use.
 *
 * \note This is code moved from config() above, since initializing and trying to delete
 * on detach caused a crash.
 */
void server::prepare_qtapp( int argc, char *argv[] )
{
    // make sure the Qt Locale is UTF-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    if(!g_application)
    {
        // We install a translator early, but language files are only
        // loaded if the user is logged in or a website specified a
        // locale which is not "en" or "en_US".
        //
        g_application = QPointer<QCoreApplication>( new QCoreApplication(argc, argv) );
        g_application->installTranslator(&f_translator);
    }
}


/** \brief Change the current translation.
 *
 * This function is called whenever a new translation becomes available.
 * In most cases this happens whenever a user is logged in the system.
 *
 * At some point we may want to provide a translation capability from
 * the server settings so one can have most error messages translated
 * in their main country language instead of the default English.
 *
 * The \p xml_data buffer is XML as output by Qt Linguist. We actually
 * make use of our own translation tool in Snap! and have a backend
 * process which gathers all the translations and generates one XML
 * file for each given language.
 *
 * \param[in] xml_data  The XML data representing the translation as
 *                      expected by QTranslator.
 */
void server::set_translation(QString const xml_data)
{
    // WARNING: the translation must not disappear when installed from
    //          using the load(char const *, int) overload function so
    //          we keep a copy in f_translation_xml
    //
    f_translation_xml = xml_data.toUtf8();

    // we use a cast because .data() returns a 'char *'
    f_translator.load(reinterpret_cast<uchar const *>(f_translation_xml.data()), f_translation_xml.size());
}


/** \brief Prepare the Cassandra database.
 *
 * This function ensures that the Cassandra database includes the default
 * context and tables (domain, website, contents.)
 *
 * This is called once each time the server is started. It does npt matter
 * too much as it is quite fast. Only the core tables are checked. We may
 * later provide a way for plugins to create different contexts but at
 * this point we expect all of them to only make use of the Core provided
 * context.
 *
 * Currently, plugins create new tables on the fly, although in a large
 * cluster that is not a valid strategy because the creation of a table
 * has to be synchronized throughout the entire cluster and that is very
 * slow on large cluster.
 *
 * \todo
 * If this function does not get called, the f_cassandra_host and
 * f_cassandra_port do not get defined. This is a problem that should
 * be addressed at some point.
 */
void server::prepare_cassandra()
{
    snap_cassandra cassandra( f_parameters );
    cassandra.connect();
    cassandra.init_context();
    QtCassandra::QCassandraContext::pointer_t context( cassandra.get_snap_context() );
    if( !context )
    {
        SNAP_LOG_FATAL("snap_websites context does not exist! Exiting.");
        exit(1);
    }
    //
    f_cassandra_host = cassandra.get_cassandra_host();
    f_cassandra_port = cassandra.get_cassandra_port();

    // setup the server name, this is important for locks
    context->setHostName(f_parameters["server_name"]);

    // create missing tables
    create_table(context, get_name(name_t::SNAP_NAME_DOMAINS),  "List of domain descriptions.");
    create_table(context, get_name(name_t::SNAP_NAME_WEBSITES), "List of website descriptions.");

    // --prepare-cassandra used?
    if(f_opt->is_defined("prepare-cassandra"))
    {
        exit(0);
    }

    // --add-host used?
    if(f_opt->is_defined("add-host"))
    {
        // The libQtCassandra library creates a lock table named
        // libQtCassandraLockTable. That table needs to include each host as
        // any one host may need to lock the system.
        QString host_name(f_opt->get_string("add-host").c_str());
        if(host_name.isEmpty())
        {
            host_name = f_parameters["server_name"];
        }
        context->addLockHost(host_name);
        exit(0);
    }
}


/** \brief Create a table in the specified context.
 *
 * The function checks whether the named table exists, if not it
 * creates it with default parameters. The result is a shared pointer
 * to the table in question.
 *
 * By default tables are just created in the Cassandra node you are
 * connected with. In order to use the table, it has to have been
 * propagated. This is done with a synchronization call. That call
 * is performed by this very function the first time a table is
 * queried if that table was created in an earlier call to this
 * function, then the synchronization function gets called and blocks
 * the process until the table was propagated. The current initialization
 * process expects the create_table() to be called a first time when
 * your plugin initial_update() is called, then called again once the
 * table is necessary. Therefore, this create_table() uses a 'call me
 * twice' scheme where the second call ensures the synchrony.
 *
 * \todo
 * Provide a structure that includes the different table parameters instead of
 * using hard coded defaults.
 *
 * \param[in] context  The context in which the table is to be created.
 * \param[in] table_name  The name of the new table, if it exists, nothing happens.
 * \param[in] comment  A comment about the new table.
 */
QtCassandra::QCassandraTable::pointer_t server::create_table(QtCassandra::QCassandraContext::pointer_t context, QString table_name, QString comment)
{
    // does table exist?
    QtCassandra::QCassandraTable::pointer_t table(context->findTable(table_name));
    if(!table)
    {
        // table is not there yet, create it
        table = context->table(table_name);
        table->setComment(comment);
        table->setColumnType("Standard"); // Standard or Super
        table->setKeyValidationClass("BytesType");
        table->setDefaultValidationClass("BytesType");
        table->setComparatorType("BytesType");
        table->setKeyCacheSavePeriodInSeconds(14400);
        table->setMemtableFlushAfterMins(60);
        //table->setMemtableThroughputInMb(247);
        //table->setMemtableOperationsInMillions(1.1578125);
        table->setGcGraceSeconds(864000);
        table->setMinCompactionThreshold(4);
        table->setMaxCompactionThreshold(22);
        table->setReplicateOnWrite(1);
        table->create();

        f_created_table[table_name] = true;
    }
    else if(f_created_table.contains(table_name))
    {
        // one single synchronization call for all the tables created
        // thus far is enough.
        f_created_table.clear();

        // table(s) were created, we must wait for them to be synchronized
        context->parentCassandra()->synchronizeSchemaVersions();
    }
    return table;
}


/** \brief Detach the server unless in foreground mode.
 *
 * This function detaches the server unless it is in foreground mode.
 */
void server::detach()
{
    if(f_foreground)
    {
        return;
    }

    // if the logger is using threads, it has to be shutdown (unconfigured)
    // before we call the fork(); this is a waste of time so we try not to
    // do it if we can
    if(is_logging_server())
    {
        logging::unconfigure();
    }

    // detaching using fork()
    pid_t const child_pid(fork());
    if(child_pid == 0)
    {
        // this is the child, make sure we keep the log alive
        logging::reconfigure();
        return;
    }

    // since we are quitting immediately we do not need to save the child_pid

    if(child_pid == -1)
    {
        logging::reconfigure();
        SNAP_LOG_FATAL("the server could not fork() a child process to detach itself from your console.");
        exit(1);
    }

    exit(0);
}


/** \brief Send a PING message to the specified UDP server.
 *
 * This function sends a PING message (4 bytes) to the specified
 * UDP server. This is used after you saved data in the Cassandra
 * cluster to wake up a background process which can then "slowly"
 * process the data further.
 *
 * Remember that UDP is not reliable so we do not in any way
 * guarantee that this goes anywhere. The function returns no
 * feedback at all. We do not wait for a reply since at the time
 * we send the message the listening server may be busy. The
 * idea of this ping is just to make sure that if the server is
 * sleeping at that time, it wakes up sooner rather than later
 * so it can immediately start processing the data we just added
 * to Cassandra.
 *
 * The \p message is expected to be a NUL terminated string. The
 * NUL is not sent across. At this point most of our servers
 * accept a PING message to wake up and start working on new
 * data.
 *
 * The \p upd_addr_port parameter is an IP address (IPv4 or IPv6)
 * which must be followed by a colon and a port number.
 *
 * \param[in] service  The name of the service to ping.
 * \param[in] uri  The website generating the ping.
 */
void server::udp_ping_server( QString const & service, QString const & uri )
{
    snap::snap_communicator_message ping;
    ping.set_command("PING");
    ping.set_service(service);
    ping.add_parameter("uri", uri);

    // TBD: we may want to cache that information in case we call
    //      this function more than once
    //
    QString addr("127.0.0.1");
    int port(4041);
    QString const communicator_addr_port( get_parameter("snapcommunicator_signal") );
    tcp_client_server::get_addr_port(communicator_addr_port, addr, port, "udp");

    snap_communicator::snap_udp_server_message_connection::send_message(addr.toUtf8().data(), port, ping);
}


/** \brief Block an IP address at the firewall level.
 *
 * This function sends a BLOCK message to the snapfirewall service in
 * order to have the specified \p ip blocked for the specified \p period.
 *
 * The \p period parameter is not required. If not specified, the default
 * will apply. At this time, the snapfirewall tool uses "day" as its default.
 * The supported periods are:
 *
 * \li "hour" -- block the IP address for one hour.
 * \li "day" -- block the IP address for 24h.
 * \li "week" -- block the IP address for 7 days.
 * \li "month" -- block the IP address for 31 days.
 * \li "year" -- block the IP address for 366 days.
 * \li "forever" -- block the IP address for 5 years.
 *
 * \param[in] ip  The IP address of to ban.
 * \param[in] period  The duration for which the ban applies.
 */
void server::block_ip( QString const & ip, QString const & period )
{
    // create a server object (we are a static function!)
    //
    snap::server::pointer_t s( snap::server::instance() );

    // retrieve the IP and port to the snapcommunicator
    //
    QString addr("127.0.0.1");
    int port(4041);
    tcp_client_server::get_addr_port(s->get_parameter("snapcommunicator_signal"), addr, port, "udp");

    // create a BLOCK message
    //
    snap::snap_communicator_message message;
    message.set_command("BLOCK");
    message.set_service("snapfirewall");
    message.add_parameter("ip", ip);
    if(!period.isEmpty())
    {
        message.add_parameter("period", period);
    }

    // send the message using a UDP signal
    //
    snap::snap_communicator::snap_udp_server_message_connection::send_message(addr.toUtf8().data(), port, message);
}


#ifdef SNAP_NO_FORK
/** \brief Don't fork the snap child if true.
 *
 * This is set via the command line. If set, the snap_child object will not fork.
 *
 * \note This is debug-only code, which should never be in production.
 */
bool server::nofork() const
{
    return f_nofork;
}
#endif



/** \brief Check which child died.
 *
 * This function is used to find children that died and remove them
 * from the list of zombies.
 *
 * \param[in] child_pid  The process identification of the child that died.
 */
void server::capture_zombies(pid_t child_pid)
{
    // capture zombies first
    snap_child_vector_t::size_type max_children(f_children_running.size());
    for(snap_child_vector_t::size_type idx(0); idx < max_children; ++idx)
    {
        if(f_children_running[idx]->get_child_pid() == child_pid)
        {
            if(f_children_running[idx]->check_status() != snap_child::status_t::SNAP_CHILD_STATUS_READY)
            {
                throw snapwebsites_exception_invalid_parameters("somehow capture_zombies() was called with a pid_t that did not represent a dead child.");
            }

            // it is ready, so it can be reused now
            f_children_waiting.push_back(f_children_running[idx]);
            f_children_running.erase(f_children_running.begin() + idx);
            return;
        }
        //if(f_children_running[idx]->check_status() == snap_child::status_t::SNAP_CHILD_STATUS_READY)
        //{
        //    // it is ready, so it can be reused now
        //    f_children_waiting.push_back(f_children_running[idx]);
        //    f_children_running.erase(f_children_running.begin() + idx);

        //    // removed one child so decrement index:
        //    --idx;
        //    --max_children;
        //}
    }
}


/** \brief Capture children death.
 *
 * This class used used to create a connection on started that allows
 * us to know when a child dies. Whenever that happens, we get a call
 * to the process_signal() callback.
 */
class signal_child_death
        : public snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<signal_child_death>     pointer_t;

                            signal_child_death(server * s);

    // snap_communicator::snap_signal implementation
    virtual void            process_signal();

private:
    // TBD: should this be a weak pointer?
    server *                f_server;
};


/** \brief Initialize the child death signal.
 *
 * The function initializes the snap_signal to listen on the SIGCHLD
 * Unix signal. It also saves the pointer \p s to the server so
 * it can be used to call various functions in the server whenever
 * the signal occurs.
 *
 * \param[in] s  The server pointer.
 */
signal_child_death::signal_child_death(server * s)
    : snap_signal(SIGCHLD)
    , f_server(s)
{
}


/** \brief Callback called each time the SIGCHLD signal occurs.
 *
 * This function gets called each time a child dies.
 *
 * The function checks all the children and removes zombies.
 */
void signal_child_death::process_signal()
{
    // check all our children and remove zombies
    //
    f_server->capture_zombies(get_child_pid());
}




/** \brief Listen and send messages with other services.
 *
 * This class is used to listen for incoming messages from
 * snapcommunicator and also to send messages
 *
 * \note
 * At this time we do not really send anything to anyone... but we may
 * start doing so to snapwatchdog to have an overall count of the child
 * processes that we create and other similar statistics. (i.e. we have
 * to think about the time when we create listening children and in
 * that case we do not want to count those children until they get
 * a new connection; before that they do not count.)
 */
class messager
        : public snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<messager>    pointer_t;

                        messager(server * s, std::string const & addr, int port);

    // snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void        process_message(snap_communicator_message const & message);
    virtual void        process_connected();

private:
    server *            f_server;
};


/** \brief Initialize the messager connection.
 *
 * This function initializes the messager connection. It saves
 * a pointer to the main Snap! server so it can react appropriately
 * whenever a message is received.
 *
 * \param[in] s  A pointer to the server so we can send messages there.
 * \param[in] addr  The address of the snapcommunicator server.
 * \param[in] port  The port of the snapcommunicator server.
 */
messager::messager(server * s, std::string const & addr, int port)
    : snap_tcp_client_permanent_message_connection(addr, port, tcp_client_server::bio_client::mode_t::MODE_PLAIN, snap_communicator::snap_tcp_client_permanent_message_connection::DEFAULT_PAUSE_BEFORE_RECONNECTING, false)
    , f_server(s)
{
}


/** \brief Process a message we just received.
 *
 * This function is called whenever the snapcommunicator received and
 * decided to forward a message to us.
 *
 * \param[in] message  The message we just recieved.
 */
void messager::process_message(snap_communicator_message const & message)
{
    f_server->process_message(message);
}


/** \brief Process was just connected.
 *
 * This callback happens whenever a new connection is established.
 * It sends a REGISTER command to the snapcommunicator. The READY
 * reply will be received when process_message() gets called. At
 * that point we are fully registered.
 *
 * This callback happens first so if we lose our connection to
 * the snapcommunicator server, it will re-register the snapserver
 * again as expected.
 */
void messager::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    snap::snap_communicator_message register_snapserver;
    register_snapserver.set_command("REGISTER");
    register_snapserver.add_parameter("service", "snapserver");
    register_snapserver.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_snapserver);
}


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever a message from snapcommunicator
 * is received.
 *
 * The function reacts according to the message command:
 *
 * \li HELP -- reply with the COMMANDS message and the few commands we
 *             understand
 * \li LOG -- reset the log
 * \li READY -- ignored, this means Snap Communicator acknowledge that we
 *              registered with it
 * \li STOP or QUITTING -- stop the server
 * \li UNKNOWN -- ignored command, we log the fact that we sent an unknown
 *                message to someone
 *
 * If another command is received, the function replies with the UNKNOWN
 * command to make sure the sender is aware that the command was ignored.
 *
 * \param[in] message  The message to process.
 */
void server::process_message(snap_communicator_message const & message)
{
    if(!g_connection || !g_connection->f_communicator)
    {
        SNAP_LOG_WARNING("received message after the g_connection or g_connection->f_communicator variables were cleared.");
        return;
    }

    SNAP_LOG_TRACE("received message [")(message.to_message())("]");

    QString const command(message.get_command());

    if(command == "STOP"
    || command == "QUITTING")  // QUITTING happens when we send a message to snapcommunicator after it received a STOP
    {
        SNAP_LOG_INFO("Stopping server.");

        if(g_connection->f_messager)
        {
            std::static_pointer_cast<messager>(g_connection->f_messager)->mark_done();

            if(command != "QUITTING")
            {
                snap::snap_communicator_message cmd;
                cmd.set_command("UNREGISTER");
                cmd.add_parameter("service", "snapserver");
                std::static_pointer_cast<messager>(g_connection->f_messager)->send_message(cmd);
            }
        }

        {
            g_connection->f_communicator->remove_connection(g_connection->f_listener);
            g_connection->f_communicator->remove_connection(g_connection->f_child_death_listener);
            //g_connection->f_communicator->remove_connection(g_connection->f_messager); -- will HUP once done
        }
        return;
    }

    if(command == "LOG")
    {
        SNAP_LOG_INFO("Logging reconfiguration.");
        logging::reconfigure();
        return;
    }

    if(command == "READY")
    {
        // TBD: should we start the listener here instead?
        //
        //      the fact is... if we lose the connection to
        //      snapcommunicator we would start the listener
        //      at another time anyway
        //
        return;
    }

    if(command == "HELP")
    {
        snap::snap_communicator_message reply;
        reply.set_command("COMMANDS");

        // list of commands understood by server
        reply.add_parameter("list", "HELP,LOG,QUITTING,READY,STOP,UNKNOWN");

        std::dynamic_pointer_cast<messager>(g_connection->f_messager)->send_message(reply);
        return;
    }

    if(command == "UNKNOWN")
    {
        SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
        return;
    }

    // unknown command is reported and process goes on
    //
    SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received on the TCP connection.");
    {
        snap::snap_communicator_message reply;
        reply.set_command("UNKNOWN");
        reply.add_parameter("command", command);
        std::dynamic_pointer_cast<messager>(g_connection->f_messager)->send_message(reply);
    }
    return;
}






/** \brief Handle new connections from clients.
 *
 * This function is an implementation of the snap server so we can
 * handle new connections from various clients.
 */
class listener_impl : public snap_communicator::snap_tcp_server_connection
{
public:
                                listener_impl(server * s, std::string const & addr, int port, int max_connections, bool reuse_addr, bool auto_close);

    // snap_communicator::snap_tcp_server_connection implementation
    virtual void                process_accept();

private:
    // this is owned by a server function so no need for a smart pointer
    server *            f_server;
};



/** \brief The listener initialization.
 *
 * The listener receives a pointer back to the snap::server object and
 * information on how to generate the new network connection to listen
 * on incoming connections from clients.
 *
 * The server listens to two types of messages:
 *
 * \li accept() -- a new connection is accepted from a client
 * \li recv() -- a UDP message was received
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
listener_impl::listener_impl(server * s, std::string const & addr, int port, int max_connections, bool reuse_addr, bool auto_close)
    : snap_tcp_server_connection(addr, port, max_connections, reuse_addr, auto_close)
    , f_server(s)
{
    non_blocking();
}


/** \brief This callback is called whenever a client tries to connect.
 *
 * This callback function is called whenever a new client tries to connect
 * to the server.
 *
 * The function retrieves the new connection socket, makes the socket
 * "keep alive" and then calls the process_connection() function of
 * the server.
 */
void listener_impl::process_accept()
{
    // a new client just connected
    //
    int const new_socket(accept());
    if(new_socket < 0)
    {
        // TBD: should we call process_error() instead? problem is this
        //      listener would be removed from the list of connections...
        //
        int const e(errno);
        SNAP_LOG_ERROR("accept() returned an error. (errno: ")(e)(" -- ")(strerror(e))("). No new connection will be created.");
        return;
    }

    // we just have a socket and the keepalive() function in the
    // snap_connection requires... a snap_connection object.
    //
    int optval(1);
    socklen_t const optlen(sizeof(optval));
    if(setsockopt(new_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) != 0)
    {
        SNAP_LOG_WARNING("listener_impl::process_accept(): an error occurred trying to mark socket with SO_KEEPALIVE.");
    }

    // process the new connection, which means create a child process
    // and run the necessary code to return an HTML page, a document,
    // robots.txt, etc.
    //
    f_server->process_connection(new_socket);
}





/** \brief Listen to incoming connections.
 *
 * This function initializes various connections which get added to
 * the snap_communicator object. These connections are:
 *
 * \li A listener, which opens a port to listen to new incoming connections.
 * \li A signal handler, also via a connection, which listens to the SIGCHLD
 *     Unix signal. This allows us to immediately manage zombie processes.
 * \li A messager, which is a permanent connection to the Snap Communicator
 *     server. Permanent because if the connection is lost, it will be
 *     reinstantiated as soon as possible.
 *
 * Our snap.cgi process is the one that connects to our listener, since at
 * this time we do not directly listen to port 80 or 443.
 *
 * The messager receives messages such as the STOP and LOG messages. The
 * STOP message actually requests that this very function returns as soon
 * as the server is done with anything it is currently doing.
 *
 * If the function finds an error in one of the parameters used from the
 * configuration file, then it logs an error and calls exit(1).
 *
 * Other errors may occur in which case it is likely that the process
 * will throw an error.
 */
void server::listen()
{
    // offer the user to setup the maximum number of pending connections
    long max_pending_connections(-1);
    bool ok;
    QString max_connections(f_parameters["max_pending_connections"]);
    if(!max_connections.isEmpty())
    {
        max_pending_connections = max_connections.toLong(&ok);
        if(!ok)
        {
            SNAP_LOG_FATAL("invalid max_pending_connections, a valid number was expected instead of \"")(max_connections)("\".");
            exit(1);
        }
        if(max_pending_connections < 1)
        {
            SNAP_LOG_FATAL("max_pending_connections must be positive, \"")(max_connections)("\" is not valid.");
            exit(1);
        }
    }

    // get the address/port info
    QString addr("0.0.0.0");
    int port(4004);
    tcp_client_server::get_addr_port(f_parameters["listen"], addr, port, "tcp");

    // convert the address information
    QHostAddress const a(addr);
    if(a.isNull())
    {
        SNAP_LOG_FATAL("invalid address specification in \"")(addr)(":")(port)("\".");
        exit(1);
    }

    // get timeout time for wait when children exist
    long timeout_wait_children(5000);
    QString timeout_wait_children_param(f_parameters["timeout_wait_children"]);
    if(!timeout_wait_children_param.isEmpty())
    {
        timeout_wait_children = timeout_wait_children_param.toLong(&ok);
        if(!ok)
        {
            SNAP_LOG_FATAL("invalid timeout_wait_children, a valid number was expected instead of \"")(timeout_wait_children_param)("\".");
            exit(1);
        }
        if(timeout_wait_children < 100)
        {
            SNAP_LOG_FATAL("timeout_wait_children must be at least 100, \"")(timeout_wait_children_param)("\" is not acceptable.");
            exit(1);
        }
    }

    // get the snapcommunicator IP and port
    QString communicator_addr("127.0.0.1");
    int communicator_port(4040);
    tcp_client_server::get_addr_port(get_parameter("snapcommunicator_listen"), communicator_addr, communicator_port, "tcp");

    // create a communicator
    //
    // only we use a bare pointer because otherwise the child processes
    // attempt to destroy these objects and that does not work right
    //
    g_connection = new connection_t;
    g_connection->f_communicator = snap_communicator::instance();

    // create a listener, for new arriving client connections
    //
    // auto-close is set to false because the accept() is not directly used
    // on the tcp_server object
    //
    g_connection->f_listener.reset(new listener_impl(this, addr.toUtf8().data(), port, max_pending_connections, true, false));
    g_connection->f_listener->set_name("server listener");
    g_connection->f_listener->set_priority(30);
    g_connection->f_communicator->add_connection(g_connection->f_listener);

    g_connection->f_child_death_listener.reset(new signal_child_death(this));
    g_connection->f_child_death_listener->set_name("child death listener");
    g_connection->f_child_death_listener->set_priority(75);
    g_connection->f_communicator->add_connection(g_connection->f_child_death_listener);

    g_connection->f_messager.reset(new messager(this, communicator_addr.toUtf8().data(), communicator_port));
    g_connection->f_messager->set_name("messager");
    g_connection->f_messager->set_priority(50);
    g_connection->f_communicator->add_connection(g_connection->f_messager);

    // the server was successfully started
    SNAP_LOG_INFO("Snap v" SNAPWEBSITES_VERSION_STRING " on \"" + f_parameters["server_name"] + "\" started.");

    // run until we get killed
    g_connection->f_communicator->run();

    // if we are returning that is because the signals were removed from
    // the communicator so we can now destroy the communicator
    g_connection->f_communicator.reset();
}


/** \brief Process an incoming connection.
 *
 * This function processes an incoming connection from a client.
 * This connection is from the snap.cgi to the snapserver.
 *
 * \param[in] socket  The socket which represents the new connection.
 */
void server::process_connection(int socket)
{
    snap_child * child;

    // we are handling one more connection, whether it works or
    // not we increase our internal counter
    ++f_connections_count;

    if(f_children_waiting.empty())
    {
        child = new snap_child(g_instance);
    }
    else
    {
        child = f_children_waiting.back();
        f_children_waiting.pop_back();
    }

    if(child->process(socket))
    {
        // this child is now busy
        f_children_running.push_back(child);
    }
    else
    {
        // it failed, we can keep that child as a waiting child
        f_children_waiting.push_back(child);

        // and tell the user about a problem without telling much...
        // (see the logs for more info.)
        // TBD Translation?
        std::string err("Status: HTTP/1.1 503 Service Unavailable\n"
                      "Expires: Sun, 19 Nov 1978 05:00:00 GMT\n"
                      "Content-type: text/html\n"
                      "\n"
                      "<h1>503 Service Unavailable</h1>\n"
                      "<p>Server cannot start child process.</p>\n");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        write(socket, err.c_str(), err.size());
#pragma GCC diagnostic pop
    }

    // since we do not create any object holding this socket, we have
    // to close it here...
    close(socket);
}


/** \brief Handle caught signals
 *
 * Catch the signal, then log the signal, then terminate with 1 status.
 */
void server::sighandler( int sig )
{
    QString signame;
    bool output_stack_trace(true);
    switch( sig )
    {
        case SIGSEGV : signame = "SIGSEGV"; break;
        case SIGBUS  : signame = "SIGBUS";  break;
        case SIGFPE  : signame = "SIGFPE";  break;
        case SIGILL  : signame = "SIGILL";  break;
        case SIGTERM : signame = "SIGTERM"; output_stack_trace = false; break;
        case SIGINT  : signame = "SIGINT";  output_stack_trace = false; break;
        default      : signame = "UNKNOWN"; break;
    }

    if( output_stack_trace )
    {
        snap_exception_base::output_stack_trace();
    }
    //
    SNAP_LOG_FATAL("signal caught: ")(signame);

    // is server available?
    if(g_instance)
    {
        g_instance->exit(1);
    }

    // server not available, exit directly
    ::exit(1);
}


/** \brief Run the backend process.
 *
 * This function creates a child and runs its backend function.
 *
 * The function may first initialize some more things in the server.
 *
 * When the backend process ends, the function returns. Assuming everything
 * works as expected, the function is exepcted to return cleanly.
 */
void server::backend()
{
    snap_backend the_backend(g_instance);
    the_backend.run_backend();
}


/** \brief Return the number of connections received by the server.
 *
 * This function returns the connections counter. Note that this
 * counter is just an in memory counter so once the server restarts
 * it is reset to zero.
 */
unsigned long server::connections_count()
{
    return f_connections_count;
}


/** \brief Servername, taken from argv[0].
 *
 * This method returns the server name, taken from the first argument on the command line.
 */
std::string server::servername() const
{
    return f_servername;
}


/** \fn void server::init()
 * \brief Initialize the Snap Websites server.
 *
 * This function readies the Init signal.
 *
 * At this time, it does nothing.
 */


/** \fn void server::update(int64_t last_updated)
 * \brief Update the Snap Websites server.
 *
 * This signal ensures that the data managed by this plugin is up to date.
 *
 * \param[in] last_updated  The date and time when the website was last updated.
 */


/** \fn void server::process_cookies()
 * \brief Process a cookies on an HTTP request.
 *
 * This signal is used to 
 *
 * At this time, it does nothing.
 */


/** \fn void server::attach_to_session()
 * \brief Process the attach to session event.
 *
 * This signal gives plugins a chance to attach data to the session right
 * before the process ends.
 */


/** \fn void server::detach_from_session()
 * \brief Process the detach from session event.
 *
 * This signal gives plugins a chance to detach data that they attached to
 * the sessions earlier. This signal happens early on after the process
 * initialized the plugins.
 */


/** \fn void server::define_locales(QString & locales)
 * \brief Give plugins a chance to define the acceptable page locales.
 *
 * This signal is used whenever the user tries to access a page and
 * the language and/or country was not already defined as a sub-domain,
 * a path segment, or an option (a GET variable, also called a query string
 * variable.)
 *
 * This function requests the plugins to define a list of
 * \<language>_\<country> pairs separated by commas.
 *
 * \param[in,out] locales  The variable receiving the locales.
 */


/** \fn void server::process_post(QString const & url)
 * \brief Process a POST request at the specified URL.
 *
 * This signal is sent when the server is called with a POST instead of a GET.
 *
 * \param[in] url  The URL to process a POST from.
 */


/** \fn void server::execute(QString const & url)
 * \brief Execute the URL.
 *
 * This signal is called once the plugins were fully initialized. At this point
 * only one plugin is expected to implement that signal: path.
 *
 * \param[in] url  The URL to execute.
 */


/** \fn void server::register_backend_cron(backend_action_set & actions)
 * \brief Execute the specified backend action every 5 minutes.
 *
 * This signal is called when the backend server is asked to run a backend
 * service as a CRON server. By default the service runs once every five
 * minutes. It can also be awaken with a PING message and ended with a STOP
 * message.
 *
 * Plugins that handle backend work that happens on a regular schedule and
 * has to be available quickly through a PING are registered using this
 * signal.
 *
 * If you are writing a one time signal process, use the
 * register_backend_action() instead. And if you are writing a backend
 * process that does not need to be quickly awaken via a PING, use the
 * regular process_backend() signal.
 *
 * The actions are run through the on_backend_action() virtual function
 * of the backend_action structure.
 *
 * \param[in,out] actions  A map where plugins can register the actions they support.
 */


/** \fn void server::register_backend_action(backend_action_set & actions)
 * \brief Execute the specified backend action.
 *
 * This signal is called when the server is run as a backend service.
 * Plugins that handle backend work can register when this signal is
 * triggered.
 *
 * If you want to register a backend that executes every five minutes
 * and can quickly be awaken using a PING event, then use the
 * register_backend_cron() function instead. For actions that are to
 * run over and over again instead of once in a while on an explicit
 * call, implement the backend_process() instead.
 *
 * The actions are run through the on_backend_action() virtual function
 * of the backend_action structure.
 *
 * \param[in,out] actions  A map where plugins can register the actions they support.
 */


/** \fn void server::backend_process()
 * \brief Execute the backend processes.
 *
 * This signal runs backend processes that do not need to be run immediately
 * when something changes in the database. For example, the RSS feeds are
 * updated when this signal calls the feed on_backend_process()
 * implementation.
 *
 * The signal is sent to all plugins that registered with the standard
 * SNAP_LISTEN() macro. The function is expected to do some work and then
 * return. The function should not take too long or it has to verify
 * whether the STOP event was sent to the backend.
 *
 * This implementation is different from the backend actions which are
 * either permanent (register_backend_cron) or a one time call
 * (register_backend_action). The CRON actions stay and run forever and
 * can be awaken by a PING event.
 */


/** \fn void server::save_content()
 * \brief Request new content to be saved.
 *
 * This signal is sent after the update signal returns. This gives a chance
 * to the content plugin to save the data. It is somewhat specialized at this
 * point, unfortunately, but it has the advantage of working properly.
 */


/** \fn void server::xss_filter(QDomNode & node, QString const & acceptable_tags, QString const & acceptable_attributes)
 * \brief Implementation of the XSS filter signal.
 *
 * This signal is used to clean any possible XSS potential problems in the specified
 * \p node.
 *
 * \param[in,out] node  The HTML node to check with XSS filters.
 * \param[in] acceptable_tags  The tags kept in the specified HTML.
 *                             (i.e. "p a ul li")
 * \param[in] acceptable_attributes  The list of (not) acceptable attributes
 *                                   (i.e. "!styles")
 *
 * \return true if the signal has to be sent to other plugins.
 */


/** \fn permission_error_callback::on_error(snap_child::http_code_t const err_code, QString const& err_name, QString const& err_description, QString const& err_details, bool const err_by_mime_type)
 * \brief Generate an error.
 *
 * This function is called if an error is generated. If so then the function
 * should mark the permission as not available for that user.
 *
 * This function accepts the same parameters as the snap_child::die()
 * function.
 *
 * This implementation of the function does not returned. However, it cannot
 * expect that all implementations would not return (to the contrary!)
 *
 * \param[in] err_code  The error code such as 501 or 503.
 * \param[in] err_name  The name of the error such as "Service Not Available".
 * \param[in] err_description  HTML message about the problem.
 * \param[in] err_details  Server side text message with details that are logged only.
 * \param[in] err_by_mime_type  If returning an error, do not return HTML when this element MIME type is something else, instead send a file of that type, but still with the HTTP error code supplied.
 */


/** \fn permission_error_callback::on_redirect(QString const& err_name, QString const& err_description, QString const& err_details, bool err_security, QString const& path, snap_child::http_code_t const http_code)
 * \brief Generate a message and redirect the user.
 *
 * This function is called if an error is generated, but an error that can
 * be "fixed" (in most cases by having the user log in or enter his
 * credentials for a higher level of security on the website.)
 *
 * This function accepts the same parameters as the message::set_error()
 * function followed by the same parameters as the snap_child::redirect()
 * function.
 *
 * This implementation of the function does not returned. However, it cannot
 * expect that all implementations would not return (to the contrary!)
 *
 * \param[in] err_name  The name of the error such as "Value Too Small".
 * \param[in] err_description  HTML message about the problem.
 * \param[in] err_details  Server side text message with details that are logged only.
 * \param[in] err_security  Whether this message is considered a security related message.
 * \param[in] path  The path where the user is being redirected.
 * \param[in] http_code  The code to use while redirecting the user.
 */


/** \fn void server::improve_signature(QString const& path, QString& signature)
 * \brief Improve the die() signature to add at the bottom of pages.
 *
 * This function calls all the plugins that define the signature signal so
 * they can append their own link or other information to the signature.
 * The signature is a simple mechanism to get several plugins to link to
 * a page where the user can go to continue his browsing on that website.
 * In most cases it is never called because the site will have a
 * page_not_found() call which is answered properly and thus a regular
 * error page is shown.
 *
 * The signature parameter is passed to the plugins as an in/out parameter.
 * The plugins are free to do whatever they want, including completely
 * overwrite the content although keep in mind that you cannot ensure
 * that a plugin is last in the last. In most cases you should limit
 * yourself to doing something like this:
 *
 * \code
 *   signature += " <a href=\"/search\">Search This Website</a>";
 * \endcode
 *
 * This very function does nothing, just returthisns true.
 *
 * \param[in] path  The path that generated the error
 * \param[in,out] signature  The signature as inline HTML code (i.e. no blocks!)
 *
 * \return true if the signal has to be sent to other plugins.
 */


/** \brief Load a file.
 *
 * This function is used to load a file. As additional plugins are added
 * additional protocols can be supported.
 *
 * The file information defaults are kept as is as much as possible. If
 * a plugin returns a file, though, it is advised that any information
 * available to the plugin be set in the file object.
 *
 * The base load_file() function (i.e. this very function) supports the
 * file system protocol (file:) and the Qt resources protocol (qrc:).
 * Including the "file:" protocol is not required. Also, the Qt resources
 * can be indicated simply by adding a colon at the beginning of the
 * filename (":/such/as/this/name").
 *
 * \param[in,out] file  The file name and content.
 * \param[in,out] found  Whether the file was found.
 *
 * \return true if the signal is to be propagated to all the plugins.
 */
bool server::load_file_impl(snap_child::post_file_t & file, bool & found)
{
    QString filename(file.get_filename());

    found = false;

    int const colon_pos(filename.indexOf(':'));
    int const slash_pos(filename.indexOf('/'));
    if(colon_pos <= 0                    // no protocol
    || colon_pos > slash_pos            // no protocol
    || filename.startsWith("file:")     // file protocol
    || filename.startsWith("qrc:"))     // Qt resource protocol
    {
        if(filename.startsWith("file:"))
        {
            // remove the protocol
            filename = filename.mid(5);
        }
        else if(filename.startsWith("qrc:"))
        {
            // remove the protocol, but keep the colon
            filename = filename.mid(3);
        }
        QFile f(filename);
        if(!f.open(QIODevice::ReadOnly))
        {
            // file not found...
            SNAP_LOG_ERROR("error trying to read file \"")(filename)("\", system error: ")(f.errorString());
            return false;
        }
        file.set_filename(filename);
        file.set_data(f.readAll());
        found = true;
        // return false since we already "found" the file
        return false;
    }

    return true;
}



/** \fn void server::table_is_accessible(QString const & table_name, accessible_flag_t & accessible)
 * \brief Check whether a table can securily be used in a script.
 *
 * This signal is sent by the cell() function of snap_expr objects.
 * The plugins receiving the signal can check the table name
 * and mark it as accessible or secure.
 *
 * A table only marked as accessible can be accessed safely.
 *
 * \code
 *   void my_plugin::on_table_isaccessible(QString const & table_name, accessible_flag_t & accessible)
 *   {
 *      if(table_name == get_name(name_t::SNAP_NAME_MYPLUGIN_TABLE_NAME))
 *      {
 *          accessible.mark_as_accessible();
 *      }
 *   }
 * \endcode
 *
 * It is possible for a plugin to mark certain tables as not accessible,
 * whether or not a plugin mark them as accessible. For example:
 *
 * \code
 *   void content::on_table_isaccessible(QString const & table_name, accessible_flag_t & accessible)
 *   {
 *      if(table_name == get_name(name_t::SNAP_NAME_CONTENT_SECRET_TABLE))
 *      {
 *          // explicitly mark this table as a secure table
 *          accessible.mark_as_secure();
 *      }
 *   }
 * \endcode
 *
 * This is used, for example, to protect the users and secret tables.
 * Even though passwords are encrypted, allowing an end user to get a copy
 * of the encrypted password would dearly simplify the work of a hacker in
 * finding the unencrypted password.
 *
 * The \p secure flag is used to mark the cell as secure. Simply call
 * the mark_as_secure() function to do so. This means the table cannot
 * be accessed and the cell() function fails.
 *
 * \param[in] table  The table being accessed.
 * \param[in] accessible  Whether the cell is secure.
 */


/** \fn void server::add_snap_expr_functions(snap_expr::functions_t& functions)
 * \brief Give a change to different plugins to add functions for snap_expr.
 *
 * This function gives a chance to any plugin listening to this signal
 * to add functions that the snap_expr can then make use of.
 *
 * \return true in case the signal is to be broadcast.
 */


/** \fn void server::output_result(QString const& uri_path, QByteArray& result)
 * \brief Implementation of the output_result signal.
 *
 * The output_result() signal offers the result buffer to all the plugins
 * to look at. Since the buffer is passed as a reference, a plugin can
 * modify it as required although it is not generally expected to happen.
 *
 * It may also be used to process the result and exit if a plugin thinks
 * that the default processing is not going to be capable of handling
 * the data appropriately. For example, the server_access plugin
 * intercepts all results and transforms them to an AJAX response in
 * case the request was an AJAX request.
 *
 * \param[in,out] result  The result buffer.
 *
 * \return true if the signal has to be sent to other plugins.
 */



/** \brief Initializes a quiet error callback object.
 *
 * This function initializes an error callback object. It expects a pointer
 * to the running snap_child.
 *
 * The \p log parameter is used to know whether the errors and redirects
 * should be logged or not. In most cases it probably will be set to
 * false to avoid large amounts of logs.
 *
 * \param[in,out] snap  The snap pointer.
 * \param[in] log  The log flag, if true send all errors to the loggers.
 */
quiet_error_callback::quiet_error_callback(snap_child * snap, bool log)
    : f_snap(snap)
    , f_log(log)
    //, f_error(false) -- auto-init
{
}


/** \brief Generate an error.
 *
 * This function is called when the user is trying to view something that
 * is not accessible. The system already checked to know whether the user
 * could upgrade to a higher level of control and failed, so the user
 * simply cannot access this page. Hence we do not try to redirect him to
 * a log in screen, and instead generate an error.
 *
 * In this default implementation, we simply log the information (assuming
 * the object was created with the log flag set to true) and mark the
 * object as erroneous.
 *
 * \param[in] err_code  The HTTP code to be returned to the user.
 * \param[in] err_name  The name of the error being generated.
 * \param[in] err_description  A more complete description of the error.
 * \param[in] err_details  The internal details about the error (for system administrators only).
 * \param[in] err_by_mime_type  If returning an error, do not return HTML when this element MIME type is something else, instead send a file of that type, but still with the HTTP error code supplied.
 */
void quiet_error_callback::on_error(snap_child::http_code_t const err_code, QString const & err_name, QString const & err_description, QString const & err_details, bool const err_by_mime_type)
{
    // since we ignore the error here anyway we can ignore this flag...
    NOTUSED(err_by_mime_type);

    f_error = true;

    if(f_log)
    {
        // log the error so administrators know something happened
        SNAP_LOG_ERROR("error #")(static_cast<int>(err_code))(":")(err_name)(": ")(err_description)(" -- ")(err_details);
    }
}


/** \brief Redirect the user so he can log in.
 *
 * In most cases this function is used to redirect the user to a log in page.
 * It may be a log in screen to escalate the user to a new level so he can
 * authorize changes requiring a higher level of control.
 *
 * In the base implementation, the error is logged (assuming the object was
 * created with the log flag set to true) and the object is marked as
 * erroneous, meaning that the object being checked will remain hidden.
 * However, the user does not get redirected.
 *
 * \param[in] err_name  The name of the error being generated.
 * \param[in] err_description  A more complete description of the error.
 * \param[in] err_details  The internal details about the error (for system administrators only).
 * \param[in] err_security  Whether the error is a security error (cannot be displayed to the end user).
 * \param[in] path  The path that generated the error.
 * \param[in] http_code  The HTTP code to be returned to the user.
 */
void quiet_error_callback::on_redirect(
        /* message::set_error() */ QString const & err_name, QString const & err_description, QString const & err_details, bool err_security,
        /* snap_child::page_redirect() */ QString const & path, snap_child::http_code_t const http_code)
{
    NOTUSED(err_security);

    f_error = true;
    if(f_log)
    {
        // log the feat so administrators know something happened
        SNAP_LOG_ERROR("error #")(static_cast<int>(http_code))(":")(err_name)(": ")(err_description)(" -- ")(err_details)(" (would redirect to: \"")(path)("\")");
    }
}


/** \brief Clear the error.
 *
 * This function clear the error flag.
 *
 * This class is often used in a loop such as the one used to generate all
 * the boxes on a page. The same object can be reused to check
 * wehther a box is accessible or not, however, the object needs to clear
 * its state before you test another box or all the boxes after the first
 * that's currently forbidden would get hidden.
 */
void quiet_error_callback::clear_error()
{
    f_error = false;
}


/** \brief Check whether an error occurred.
 *
 * This function returns true if one of the on_redirect() or on_error()
 * function were called during the process. If so, then the page is
 * protected.
 *
 * In most cases the redirect is used to send the user to the log in screen.
 * If the user is on a page that proves he cannot have an account or is
 * already logged in and he cannot increase his rights, then the on_error()
 * function is used. So in effect, either function represents the same
 * thing: the user cannot access the specified page.
 *
 * \return true if an error occurred and thus the page is not accessible.
 */
bool quiet_error_callback::has_error() const
{
    return f_error;
}


/** \brief Add an action to the specified action set.
 *
 * This function adds an action to this action set.
 *
 * The action name must be unique within a plugin. The function
 * forces the name of the plugin as a namespace so the name ends
 * up looking something like this (for the "reset" action of
 * the "list" plugin):
 *
 * \code
 *      list::reset
 * \endcode
 *
 * \exception snapwebsites_exception_invalid_parameters
 * If the plugin does not implement the backend_action, then this
 * exception is raised. It should happen rarely since without
 * implementing that interface you end up never receiving the
 * event. That being said, if you implement a function and forget
 * to add the derivation, it will compile and raise this exception.
 *
 * \param[in] action  The action to be added.
 * \param[in] p  The plugin that is registering this \p action.
 */
void server::backend_action_set::add_action(QString const & action, plugin * p)
{
    // make sure that this plugin implements the backend action
    //
    backend_action * ba(dynamic_cast<backend_action *>(p));
    if(ba == nullptr)
    {
        throw snapwebsites_exception_invalid_parameters("snapwebsites.cpp: server::backend_action_set::add_action() was called with \"%1\" twice.");
    }

    // calculate the full name of this action
    //
    QString const name(QString("%1::%2").arg(p->get_plugin_name()).arg(action));

    // make sure we do not get duplicates
    //
    if(f_actions.contains(name))
    {
        throw snapwebsites_exception_invalid_parameters("snapwebsites.cpp: server::backend_action_set::add_action() was called with \"%1\" twice.");
    }

    f_actions[name] = ba;
}


/** \brief Check whether a named action is defined in this set.
 *
 * Note that various websites may have various actions registered
 * depending on which plugin is installed. This function is used
 * to know whether an action is defined for that website.
 *
 * \note
 * The backend processing function exits with an error when an
 * action is not defined. This does not prevent the process from
 * moving forward (since the same action is generally run against
 * all the installed websites.)
 *
 * \param[in] action  The action to check the existance of.
 *
 * \return true if a plugin defines that specific action.
 */
bool server::backend_action_set::has_action(QString const & action) const
{
    return f_actions.contains(action);
}


/** \brief Actually call the backend action function.
 *
 * This function calls the plugin implementation of the on_backend_action()
 * function.
 *
 * The function is passed the \p action parameter since the same function
 * may get called for any number of actions (depending on how many where
 * recorded.)
 *
 * \warning
 * Note that CRON and non-CRON actions are both executed the same way.
 * The plugin is aware of which action was registered as a CRON action
 * and which was registered as a non-CRON action.
 *
 * \param[in] action  The action being executed.
 */
void server::backend_action_set::execute_action(QString const & action)
{
    if(has_action(action))
    {
        // the plugin itself expects the action name without the namespace
        // so we remove it here before we run the callback
        //
        plugins::plugin * p(dynamic_cast<plugins::plugin *>(f_actions[action]));
        if(p) // always expected to be defined
        {
            QString const namespace_name(p->get_plugin_name());
            // the +2 is to skip the '::'
            f_actions[action]->on_backend_action(action.mid(namespace_name.length() + 2));
        }
    }
}


/** \brief Retrieve the name of the plugin of a given action.
 *
 * This function retrieves the name of the plugin linked to a certain
 * action.
 *
 * \param[in] action  The action of which we are interested by the plugin.
 *
 * \return The name of the plugin, if the action is defined, otherwise "".
 */
QString server::backend_action_set::get_plugin_name(QString const & action)
{
    if(has_action(action))
    {
        plugins::plugin * p(dynamic_cast<plugins::plugin *>(f_actions[action]));
        if(p) // always expected to be defined
        {
            return p->get_plugin_name();
        }
    }

    return QString();
}


/** \brief Display the list of actions.
 *
 * This function can be used to display a list of actions.
 *
 * \warning
 * This function is definitely not re-entrant (although it can
 * be called any number of times by the same thread with the
 * same result.)
 */
void server::backend_action_set::display()
{
    f_actions["list"] = nullptr;
    for(actions_map_t::const_iterator it(f_actions.begin()); it != f_actions.end(); ++it)
    {
        std::cout << "  " << it.key() << std::endl;
    }
    f_actions.remove("list");
}


} // namespace snap
// vim: ts=4 sw=4 et
