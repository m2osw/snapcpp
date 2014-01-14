// Snap Websites Server -- snap websites server
// Copyright (C) 2011-2014  Made to Order Software Corp.
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
#include "plugins.h"
#include "signal.h"
#include "log.h"
#include "not_reached.h"
#include "tcp_client_server.h"

#include <iostream>
#include <memory>
#include <sstream>

#include <QStringList>
#include <QFile>
#include <QDirIterator>
#include <QHostAddress>
#include <QCoreApplication>
#include <QTextCodec>

#include <syslog.h>
#include <errno.h>
#include <signal.h>

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
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_SERVER:
        return "Snap! Server";

    case SNAP_NAME_CONTEXT:
        return "snap_websites";

    case SNAP_NAME_INDEX: // name used for the domains and websites indexes
        return "*index*"; // this is a row name inside the domains/websites tables

    case SNAP_NAME_DOMAINS: // domain/sub-domain canonalization
        return "domains";

    case SNAP_NAME_WEBSITES: // remaining of URL canonalization
        return "websites";

    case SNAP_NAME_SITES: // website global settings
        return "sites";

    case SNAP_NAME_CORE_ADMINISTRATOR_EMAIL:
        return "core::administrator_email";

    case SNAP_NAME_CORE_HTTP_USER_AGENT:
        return "HTTP_USER_AGENT";

    case SNAP_NAME_CORE_LAST_UPDATED:
        return "core::last_updated";

    case SNAP_NAME_CORE_SITE_NAME:
        return "core::site_name";

    case SNAP_NAME_CORE_SITE_SHORT_NAME:
        return "core::site_short_name";

    case SNAP_NAME_CORE_SITE_LONG_NAME:
        return "core::site_long_name";

    case SNAP_NAME_CORE_PLUGINS:
        return "core::plugins";

    case SNAP_NAME_CORE_REDIRECT:
        return "core::redirect";

    case SNAP_NAME_CORE_RULES:
        return "core::rules";

    case SNAP_NAME_CORE_ORIGINAL_RULES:
        return "core::original_rules";

    case SNAP_NAME_CORE_PLUGIN_THRESHOLD:
        return "core::plugin_threshold";

    case SNAP_NAME_CORE_COOKIE_DOMAIN:
        return "core::cookie_domain";

    case SNAP_NAME_CORE_USER_COOKIE_NAME:
        return "core::user_cookie_name";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_CORE_...");

    }
    NOTREACHED();
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
    const std::vector<std::string> g_configuration_files;

    /** \brief Command line options.
     *
     * This table includes all the options supported by the server.
     */
    const advgetopt::getopt::option g_snapserver_options[] =
    {
        {
            'a',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "action",
            NULL,
            "Specify a server action.",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "add-host",
            NULL,
            "Add a host to the lock table. Remember that you cannot safely do that while any one of the servers are running.",
            advgetopt::getopt::optional_argument
        },
        {
            'c',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "config",
            "/etc/snapwebsites/snapserver.conf",
            "Specify the configuration file to load at startup.",
            advgetopt::getopt::optional_argument
        },
        {
            'd',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "debug",
            NULL,
            "Keeps the server in the foreground (default is to detact and background), and display the log to the stdout.",
            advgetopt::getopt::no_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            NULL,
            "Show usage and exit.",
            advgetopt::getopt::no_argument
        },
        {
            'p',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "param",
            NULL,
            "Define one or more server parameters on the command line (-p name=value).",
            advgetopt::getopt::required_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::end_of_options
        }
    };
}
//namespace


//#pragma message "Why do we even have this? Adding a smart pointer causes a crash when the server detaches, so commented out."
// Note: We need the argc/argv when we create the application and those are
//       not available when we create the server (they are not passed along)
//       but I suppose the server could be ameliorated for that purpose...
std::shared_ptr<QCoreApplication> g_application;


/** \brief Server instance.
 *
 * The f_instance variable holds the current server instance.
 */
std::shared_ptr<server> server::f_instance;


/** \brief Set the permission and reason for refusal.
 *
 * This function marks the permission flag as not permitted (i.e. it
 * sets it to false.) The default value of the permission flag is
 * true. Note that once this function was called once it is not possible
 * to set the flag back to true.
 *
 * \param[in] new_reason  The reason for the refusal, can be set to "".
 */
void server::permission_flag::not_permitted(QString const& new_reason)
{
    f_allowed = false;

    if(!new_reason.isEmpty())
    {
        if(!f_reason.isEmpty())
        {
            f_reason += "\n";
        }
        // TBD: should we prevent "\n" in "new_reason"?
        f_reason += new_reason;
    }
}


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
const char *server::version()
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
    if( !f_instance )
    {
        f_instance.reset( new server );
    }
    return f_instance;
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


/** \brief Update the server, the function is mandatory.
 *
 * This function is here because it is a pure virtual in the plug in. At this
 * time it does nothing and it probably will never have actual updates.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t server::do_update(int64_t /*last_updated*/)
{
    SNAP_PLUGIN_UPDATE_INIT();
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize the server.
 *
 * This function initializes the server.
 */
server::server()
{
    // default parameters -- we may want to have a separate function and
    //                       maybe some clear separate variables?
    f_parameters["listen"] = "0.0.0.0:4004";
    f_parameters["plugins"] = "/usr/lib/snapwebsites/plugins";
    f_parameters["qs_path"] = "q";
    f_parameters["qs_action"] = "a";
    f_parameters["server_name"] = "";
}


/** \brief Clean up the server.
 *
 * Since the server is a singleon, it never gets deleted while running.
 * Since we use a bare pointer, it should never go out of scope, thus
 * this function should never be called.
 */
server::~server()
{
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

    f_opt->usage(advgetopt::getopt::no_error, "Usage: %s -<arg> ...\n", server_name.c_str());

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
void server::config(int argc, char *argv[])
{
    f_opt.reset(
        new advgetopt::getopt( argc, argv, g_snapserver_options, g_configuration_files, "SNAPSERVER_OPTIONS" )
    );

    // We want the servername for later.
    //
    f_servername = argv[0];

    // initialize the syslog() interface
    openlog("snapserver", LOG_NDELAY | LOG_PID, LOG_DAEMON);

    bool help(false);

    parameter_map_t cmd_line_params;
    if(f_opt->is_defined("param"))
    {
        int const max(f_opt->size("param"));
        for(int idx(0); idx < max; ++idx)
        {
            QString param(QString::fromUtf8(f_opt->get_string("param", idx).c_str()));
            int const p(param.indexOf('='));
            if(p == -1)
            {
                if(f_debug)
                {
                    std::cerr << "fatal error: unexpected parameter \"--param " << f_opt->get_string("param", idx) << "\". No '=' found in the parameter definition. (in server::config())" << std::endl;
                }
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

    if( f_opt->is_defined( "action" ) )
    {
        const std::string action( f_opt->get_string("action" ) );
        if( f_backend )
        {
            if(f_parameters.find("__BACKEND_ACTION") == f_parameters.end())
            {
                f_parameters["__BACKEND_ACTION"] = action.c_str();
            }
            else
            {
                // with the advgetopt this should never occur
                if(f_debug)
                {
                    std::cerr << "fatal error: unexpected parameter \"--action "<< action << "\", at most one action can be specified, backend not started. (in server::config())" << std::endl;
                }
                syslog( LOG_CRIT, "unexpected parameter \"--action %s\", at most one action can be specified, backend not started. (in server::config())", action.c_str() );
                help = true;
            }
        }
        else
        {
            // If not backend, "--action" doesn't make sense.
            //
            if(f_debug)
            {
                std::cerr << "fatal error: unexpected command line option \"--action " << action << "\", server not started. (in server::config())" << std::endl;
            }
            syslog( LOG_CRIT, "unexpected command line option \"--action %s\", server not started. (in server::config())", action.c_str() );
            help = true;
        }
    }
    //
    f_config = f_opt->get_string( "config" ).c_str();
    //
    f_debug = f_opt->is_defined( "debug" );

    if( help || f_opt->is_defined( "help" ) )
    {
        usage();
        exit(1);
    }

    // read the configuration file now
    QFile c;
    c.setFileName(f_config);
    c.open(QIODevice::ReadOnly);
    if(!c.isOpen())
    {
        // if for nothing else we need to have the list of plugins so we always
        // expect to have a configuration file... if we're here we could not
        // read it, unfortunately
        std::stringstream ss;
        ss << "cannot read configuration file \"" << f_config.toUtf8().data() << "\"" << std::endl;
        if(f_debug)
        {
            std::cerr << ss.str() << "." << std::endl;
        }
        syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
        exit(1);
    }

    // read the configuration file variables as parameters
    char buf[256];
    for(int line(1); c.readLine(buf, sizeof(buf)) > 0; ++line)
    {
        // make sure the last byte is '\0'
        buf[sizeof(buf) - 1] = '\0';
        int len = strlen(buf);
        if(len == 0 || (buf[len - 1] != '\n' && buf[len - 1] != '\r'))
        {
            std::stringstream ss;
            ss << "line " << line << " in \"" << f_config.toUtf8().data() << "\" is too long" << std::endl;
            if(f_debug)
            {
                std::cerr << ss.str() << "." << std::endl;
            }
            syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
            exit(1);
        }
        buf[len - 1] = '\0';
        --len;
        while(len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        {
            --len;
            buf[len] = '\0';
        }
        if(len == 0)
        {
            // comment or empty line
            continue;
        }
        char *n(buf);
        while(isspace(*n))
        {
            ++n;
        }
        if(*n == '#' || *n == '\0')
        {
            // comment or empty line
            continue;
        }
        char *v(n);
        while(*v != '=' && *v != '\0')
        {
            // TODO verify that the name is only ASCII? (probably not too
            //      important because if not it will be ignored anyway)
            ++v;
        }
        if(*v != '=')
        {
            std::stringstream ss;
            ss << "invalid variable on line " << line << " in \"" << f_config.toUtf8().data() << "\", no equal sign found" << std::endl;
            if(f_debug)
            {
                std::cerr << ss.str() << "." << std::endl;
            }
            syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
            exit(1);
        }
        char *e;
        for(e = v; e > n && isspace(e[-1]); --e);
        *e = '\0';
        do
        {
            ++v;
        }
        while(isspace(*v));
        for(e = v + strlen(v); e > v && isspace(e[-1]); --e);
        *e = '\0';
        if(v != e && ((v[0] == '\'' && e[-1] == '\'') || (v[0] == '"' && e[-1] == '"')))
        {
            // remove single or double quotes
            v++;
            e[-1] = '\0';
        }
        // keep the command line defined parameters
        if(!cmd_line_params.contains(n))
        {
            f_parameters[n] = QString::fromUtf8(v);
        }
        else if(f_debug)
        {
            std::cerr << "warning: parameter \"" << n << "\" from the configuration file ("
                      << v << ") ignored as it was specified on the command line ("
                      << f_parameters[n].toStdString() << ")." << std::endl;
        }
    }

    // the name of the server is mandatory, use hostname by default
    if(f_parameters["server_name"] == "")
    {
        char host[HOST_NAME_MAX + 1];
        if(gethostname(host, sizeof(host)) != 0)
        {
            std::stringstream ss;
            ss << "hostname is not available as the server name";
            if(f_debug)
            {
                std::cerr << ss.str() << "." << std::endl;
            }
            syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
            exit(1);
        }
        f_parameters["server_name"] = host;
    }

    // finally we can initialize the log system
    logging::configure(f_parameters["log_config"]);

    // I really prevent to have an immediate message to let users know that
    // the server is starting; however, until we detach() ourselves the
    // process identifier is going to be wrong.
    SNAP_LOG_INFO("Snap v" SNAPWEBSITES_VERSION_STRING " on \"" + f_parameters["server_name"] + "\" started.");
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
 * \li cassandra_host -- the IP address or server name to Cassandra; default is localhost
 * \li cassandra_port -- the port to use to connect to Cassandra; default is 9160
 * \li data_path -- path to the directory holding the system data (images, js, css, counters, etc.)
 * \li default_plugins -- list of default plugins to initialize a new website
 * \li listen -- address:port to listen to (default 0.0.0.0:4004)
 * \li plugins -- path to the list of plugins
 * \li qs_path -- the variable holding the path in the URL; defaults to "q"
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
 * \return The value of the specified parameter.
 */
QString server::get_parameter(const QString& param_name) const
{
    if(f_parameters.contains(param_name))
    {
        return f_parameters[param_name];
    }
    return "";
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
        g_application.reset( new QCoreApplication(argc, argv) );
    }
}


/** \brief Before quitting your application, call this to delete the application.
 *
 * The application pointer is kept in memory until this function gets called.
 * It is not generally necessary to call this function, however, some
 * processes in Qt may still need proper cleanup. This will generally take
 * care of those before quitting.
 */
void server::close_qtapp()
{
    g_application.reset();
}


/** \brief Exit the server.
 *
 * This function exists the program by calling the exit(3) function from
 * the C library. Before doing so, though, it will first make sure that
 * the server is cleaned up as required.
 *
 * \param[in] code  The exit code, generally 0 or 1.
 */
void server::exit(int code)
{
    close_qtapp();
    // call the C exit(3) function
    ::exit(code);
    NOTREACHED();
}


/** \brief Prepare the Cassandra database.
 *
 * This function ensures that the Cassandra database includes the default
 * context and tables (domain, website, contents.)
 *
 * This is called once each time the server is started. It doesn't matter
 * too much as it is quite fast. Only the core tables are checked. Plug-ins
 * can create new tables on the fly so it doesn't matter as much. We may
 * later provide a way for plugins to create different contexts but at
 * this point we expect all of them to only make use of the Core provided
 * context.
 */
void server::prepare_cassandra()
{
    // This function connects to the Cassandra database, but it doesn't
    // keep the connection. We are the server and the connection would
    // not be shared properly between all the children.
    f_cassandra_host = get_parameter("cassandra_host");
    if(f_cassandra_host.isEmpty())
    {
        f_cassandra_host = "localhost";
    }
    QString port_str(get_parameter("cassandra_port"));
    if(port_str.isEmpty())
    {
        port_str = "9160";
    }
    bool ok;
    f_cassandra_port = port_str.toLong(&ok);
    if(!ok)
    {
        SNAP_LOG_FATAL("invalid cassandra_port, a valid number was expected instead of \"")(port_str)("\".");
        exit(1);
    }
    if(f_cassandra_port < 1 || f_cassandra_port > 65535)
    {
        SNAP_LOG_FATAL("invalid cassandra_port, a port must be between 1 and 65535, ")(f_cassandra_port)(" is not.");
        exit(1);
    }
    QtCassandra::QCassandra cassandra;
    if(!cassandra.connect(f_cassandra_host, f_cassandra_port))
    {
        SNAP_LOG_FATAL("the connection to the Cassandra server failed (")(f_cassandra_host)(":")(f_cassandra_port)(").");
        exit(1);
    }
    // we need to read all the contexts in order to make sure the
    // findContext() works
    cassandra.contexts();
    QString context_name(snap::get_name(snap::SNAP_NAME_CONTEXT));
    QSharedPointer<QtCassandra::QCassandraContext> context(cassandra.findContext(context_name));
    if(context.isNull())
    {
        // create the context since it doesn't exist yet
        context = cassandra.context(context_name);
        context->setStrategyClass("org.apache.cassandra.locator.SimpleStrategy");
        context->setReplicationFactor(1);
        context->create();
        // we don't put the tables in here so we can call the create_table()
        // and have the tables created as required (i.e. as we add new ones
        // they get added as expected, no need for special handling.)
    }
    context->setHostName(f_parameters["server_name"]);

    // create missing tables
    create_table(context, get_name(SNAP_NAME_DOMAINS),  "List of domain descriptions.");
    create_table(context, get_name(SNAP_NAME_WEBSITES), "List of website descriptions.");

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


// XXX -- would this ever be necessary here?
/** \brief Get the QCassandra pointer.
 *
 * This function retrieves the QCassandra pointer from the server.
 */
//QSharedPointer<QtCassandra::QCassandra> server::get_cassandra()
//{
//    if(f_cassandra.isNull())
//    {
//        f_cassandra = QSharedPointer<QtCassandra::QCassandra>(new QtCassandra::QCassandra);
//        if(!f_cassandra->connect(f_cassandra_host, f_cassandra_port))
//        {
//            SNAP_LOG_FATAL("the connection to the Cassandra server failed (")(f_cassandra_host)(":")(f_cassandra_port)(")");
//            exit(1);
//        }
//    }
//
//    return f_cassandra;
//}


/** \brief Create a table in the specified context.
 *
 * The function checks whether the named table exists, if not it creates it with
 * default parameters. The result is a shared pointer to the table in question.
 *
 * \todo
 * Provide a structure that includes the different table parameters instead of
 * using hard coded defaults.
 *
 * \param[in] context  The context in which the table is to be created.
 * \param[in] table_name  The name of the new table, if it exists, nothing happens.
 * \param[in] comment  A comment about the new table.
 */
QSharedPointer<QtCassandra::QCassandraTable> server::create_table(QSharedPointer<QtCassandra::QCassandraContext> context, QString table_name, QString comment)
{
    // does table exist?
    QSharedPointer<QtCassandra::QCassandraTable> table(context->findTable(table_name));
    if(table.isNull())
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
    }
    return table;
}

/** \brief Detach the server unless in debug mode.
 *
 * This function detaches the server unless it is in debug mode.
 */
void server::detach()
{
    if(f_debug)
    {
        return;
    }

    // detaching using fork()
    pid_t child_pid = fork();
    if(child_pid == 0)
    {
        // this is the child, make sure we keep the log alive
        logging::reconfigure();
        return;
    }

    // since we're quitting immediately we do not need to save the child_pid

    if(child_pid == -1)
    {
        SNAP_LOG_FATAL("the server could not fork() a child process to detach itself from your console.");
        exit(1);
    }

    exit(0);
}

/** \brief Listen to incoming connections.
 *
 * This function loops over a listen waiting for connections to this
 * server. The listen is made blocking since there is nothing else
 * we have to do than wait for events from the Apache server and
 * our snap.cgi tool.
 *
 * This function never returns.
 *
 * If the function finds an error in one of the parameters used from the
 * configuration file, then it prints an error and calls exit(1).
 *
 * If the server cannot start listening, then it simply prints an error
 * and calls exit(1).
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
    QString listen_info(f_parameters["listen"]);
    if(listen_info.isEmpty())
    {
        listen_info = "0.0.0.0:4004";
    }
    QStringList host(listen_info.split(":"));
    if(host.count() == 1)
    {
        host[1] = "4004";
    }

    // convert the address information
    QHostAddress a(host[0]);
    if(a.isNull())
    {
        SNAP_LOG_FATAL("invalid address specification in \"")(host[0])(":")(host[1])("\".");
        exit(1);
    }

    // convert the port information
    long p = host[1].toLong(&ok);
    if(!ok || p < 0 || p > 65535)
    {
        SNAP_LOG_FATAL("invalid port specification in \"")(host[0])(":")(host[1])("\".");
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

    // initialize the server
    tcp_client_server::tcp_server s(host[0].toUtf8().data(), p, max_pending_connections, true, true);

    // wait until we get killed
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGCHLD);
        sigprocmask(SIG_BLOCK, &set, NULL);
    }
    for(;;)
    {
        // capture zombies first
        snap_child_vector_t::size_type max_children(f_children_running.size());
        for(snap_child_vector_t::size_type idx(0); idx < max_children; ++idx)
        {
            if(f_children_running[idx]->check_status() == snap_child::SNAP_CHILD_STATUS_READY)
            {
                // it's ready, so it can be reused now
                f_children_waiting.push_back(f_children_running[idx]);
                f_children_running.erase(f_children_running.begin() + idx);

                // removed one child so decrement index:
                --idx;
                --max_children;
            }
        }

        // retrieve all the connections and process them
        int socket(s.accept());
        // callee becomes the owner of socket
        if(socket != -1)
        {
            process_connection(socket);
        }
    }
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
    snap_child* child;

    // we're handling one more connection, whether it works or
    // not we increase our internal counter
    ++f_connections_count;

    if(f_children_waiting.empty())
    {
        child = new snap_child(f_instance);
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
        // socket will be closed by the next accept() call
    }
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
    snap_child child(f_instance);
    child.backend();
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


/** \brief Implementation of the bootstrap signal.
 *
 * This function readies the bootstrap signal.
 *
 * At this time, it does nothing.
 *
 * \param[in,out] snap  The snap child process.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::bootstrap_impl(snap_child * /*snap*/)
{
    return true;
}

/** \brief Initialize the Snap Websites server.
 *
 * This function readies the Init signal.
 *
 * At this time, it does nothing.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::init_impl()
{
    return true;
}

/** \brief Update the Snap Websites server.
 *
 * This function ensure that the data managed by this plugin is up to
 * date.
 *
 * This function does nothing at this point.
 *
 * \param[in] last_updated  The date and time when the website was last updated.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::update_impl(int64_t /*last_updated*/)
{
    return true;
}


/** \brief Process a cookies on an HTTP request.
 *
 * This function readies the process_cookies signal.
 *
 * At this time, it does nothing.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::process_cookies_impl()
{
    return true;
}


/** \brief Process the attach to session event.
 *
 * This function readies the attach to session event.
 *
 * At this time, it does nothing.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::attach_to_session_impl()
{
    return true;
}


/** \brief Process the detach from session event.
 *
 * This function readies the detach from session event.
 *
 * At this time, it does nothing.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::detach_from_session_impl()
{
    return true;
}


/** \brief Process a POST request at the specified URL.
 *
 * This function readies the process_post signal.
 *
 * At this time, it does nothing.
 *
 * \param[in] url  The URL to process a POST from.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::process_post_impl(const QString& /*url*/)
{
    return true;
}


/** \brief Execute the URL.
 *
 * This function readies the Execute signal.
 *
 * At this time, it does nothing.
 *
 * \param[in] url  The URL to execute.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::execute_impl(const QString& /*url*/)
{
    return true;
}


/** \brief Execute the specified backend action.
 *
 * This function readies the register_backend_action signal.
 *
 * At this time, it does nothing.
 *
 * \param[in,out] actions  A map where plugins can register the actions they support.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::register_backend_action_impl(backend_action_map_t& /*actions*/)
{
    return true;
}

/** \brief Execute the backend processes.
 *
 * This function readies the backend_process signal.
 *
 * At this time, it does nothing.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::backend_process_impl()
{
    return true;
}


/** \brief Request new content to be saved.
 *
 * This function readies the Save Content signal.
 *
 * At this time, it does nothing.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::save_content_impl()
{
    return true;
}


/** \brief Implementation of the XSS filter signal.
 *
 * This function readies the XSS filter signal.
 *
 * At this time, it does nothing.
 *
 * \param[in] node  The HTML node to check with XSS filters.
 * \param[in] acceptable_tags  The tags kept in the specified HTML. (i.e. "p a ul li")
 * \param[in] acceptable_attributes  The list of (not) acceptable attributes (i.e. "!styles")
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool server::xss_filter_impl(QDomNode& /*node*/,
                             const QString& /*acceptable_tags*/,
                             const QString& /*accepted_attributes*/)
{
    return true;
}


/** \brief Validate the user action.
 *
 * This function validates the user action. If invalid or if that means
 * the user does not have enough rights to access the specified path,
 * then the event calls die() at some point and returns.
 *
 * \param[in] path  The path being validated.
 * \param[in] action  The action being performed against \p path.
 * \param[in] callback  Call functions on errors.
 *
 * \return true if the event has to carry on.
 */
bool server::validate_action_impl(QString const& path, QString const& action, permission_error_callback& callback)
{
    (void) path;
    (void) action;
    (void) callback;
    return true;
}

/** \fn permission_error_callback::on_error(snap_child::http_code_t err_code, QString const& err_name, QString const& err_description, QString const& err_details)
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
 */


/** \fn permission_error_callback::on_redirect(QString const& err_name, QString const& err_description, QString const& err_details, bool err_security, QString const& path, snap_child::http_code_t http_code)
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


/** \brief Check whether a user has permission to access a page.
 *
 * This event is sent to all plugins that want to check for permissions.
 * In general, just the permissions plugin does that work, but other
 * plugins can also check. The result is true by default and if any
 * plugin decides that the page is not accessible, the result is set
 * to false. A plugin is not allowed to set the flag back to false.
 *
 * \param[in] user_path  The path to the user being checked.
 * \param[in] path  The path being checked.
 * \param[in] action  The action being checked.
 * \param[in] login_status  The status the user is in.
 * \param[in,out] result  The returned result.
 *
 * \return true if the signal should be propagated.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool server::access_allowed_impl(QString const& user_path, QString const& path, QString const& action, QString const& login_status, permission_flag& result)
{
    return result.allowed();
}
#pragma GCC diagnostic pop


/** \brief Improve the die() signature to add at the bottom of pages.
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool server::improve_signature_impl(const QString& path, QString& signature)
{
    return true;
}
#pragma GCC diagnostic pop


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
 * The \p name parameter is the name of a variable in the server
 * configuration file.
 *
 * \param[in] name  The name of the configuration variable used to read the IP and port
 * \param[in] message  The message to send, "PING" by default.
 */
void server::udp_ping(const char *name, const char *message)
{
    // TODO: we should have a common function to read and transform the
    //       parameter to a valid IP/Port pair (see below)
    QString udp_addr_port(get_parameter(name));
    QString addr, port;
    int bracket(udp_addr_port.lastIndexOf("]"));
    int p(udp_addr_port.lastIndexOf(":"));
    if(bracket != -1 && p != -1)
    {
        if(p > bracket)
        {
            // IPv6 port specification
            addr = udp_addr_port.mid(0, bracket + 1); // include the ']'
            port = udp_addr_port.mid(p + 1); // ignore the ':'
        }
        else
        {
            throw snapwebsites_exception_invalid_parameters("invalid [IPv6]:port specification, port missing for UDP ping");
        }
    }
    else if(p != -1)
    {
        // IPv4 port specification
        addr = udp_addr_port.mid(0, p); // ignore the ':'
        port = udp_addr_port.mid(p + 1); // ignore the ':'
    }
    else
    {
        throw snapwebsites_exception_invalid_parameters("invalid IPv4:port specification, port missing for UDP ping");
    }
    udp_client_server::udp_client client(addr.toUtf8().data(), port.toInt());
    client.send(message, strlen(message)); // we do not send the '\0'
}




} // namespace snap

// vim: ts=4 sw=4 et
