// Snap Websites Server -- snap manager CGI, daemon, library, plugins
// Copyright (C) 2016  Made to Order Software Corp.
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

#include "manager.h"

#include <snapwebsites/log.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qstring_stream.h>

#include <glob.h>

#include <sstream>

#include "poison.h"


/** \file
 * \brief This file represents the Snap! Manager library.
 *
 * The snapmanagercgi, snapmanagerdaemon, and snapmanager-plugins are
 * all linked against this common library which adds some functionality
 * not otherwise available in the libsnapwebsites core library.
 */


/** \mainpage
 * \brief Snap! Manager Documentation
 *
 * \section introduction Introduction
 *
 * The Snap! Manager is a CGI, a daemon and a set of plugins that both
 * of these binaries use to allow for an infinite number of capabilities
 * in terms of managing a Snap! websites cluster.
 */



namespace snap_manager
{

namespace
{

manager::pointer_t g_instance;


void glob_deleter(glob_t * g)
{
    globfree(g);
}


/** \brief List of configuration files one can create to define parameters.
 *
 * This feature is not used because the getopt does not yet give us a way
 * to specify a configuration file (i.e. --config \<path>/\<file>.conf).
 *
 * At this point, we load the configuration file using the snapwebsites
 * library.
 */
std::vector<std::string> const g_configuration_files
{
    //"@snapwebsites@",  // project name
    //"/etc/snapwebsites/snapmanager.conf" -- we use the snap f_config variable instead
};

advgetopt::getopt::option const g_manager_options[] =
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
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "config",
        "/etc/snapwebsites/snapmanager.conf",
        "Path and filename of the snapmanager.cgi and snapmanagerdaemon configuration file.",
        advgetopt::getopt::required_argument
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
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "data-path",
        "/var/lib/snapwebsites/cluster-status",
        "Path to this process data directory to save the cluster status.",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "debug",
        nullptr,
        "Start in debug mode.",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "log-config",
        "/etc/snapwebsites/snapmanager.properties",
        "Full path of log configuration file.",
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
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "server-name",
        0,
        "Name of the server on which snapmanagerdaemon is running.",
        advgetopt::getopt::optional_argument // required for snapmanagerdaemon, ignored by snapmanager.cgi
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "snapdbproxy",
        0,
        "The IP address and port of the snapdbproxy service.",
        advgetopt::getopt::optional_argument // required for snapmanagerdaemon, ignored by snapmanager.cgi
    },
    {
        '\0',
        0,
        "stylesheet",
        "/etc/snapwebsites/snapmanagercgi-parser.xsl",
        "The stylesheet to use to transform the data before sending it to the client as HTML.",
        advgetopt::getopt::required_argument
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





} // no name namespace


/** \brief Get a fixed watchdog plugin name.
 *
 * The watchdog plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_MANAGER_STATUS_FILE_HEADER:
        return "header";

    default:
        // invalid index
        throw snap::snap_logic_exception("Invalid SNAP_NAME_MANAGER_...");

    }
    snap::NOTREACHED();
}




manager::manager(bool daemon)
    : snap_child(server_pointer_t())
    , f_daemon(daemon)
{
}



manager::~manager()
{
}



/** \brief Initialize the manager.
 *
 * The constructor parses the command ilne options in a symetrical way
 * for snapmanager.cgi and snapmanagerdaemon.
 *
 * \param[in] argc  The number of arguments in argv.
 * \param[in] argv  The list of command line arguments.
 * \param[in] daemon  Whether the daemon (true) or CGI (false) process starts. 
 */
void manager::init(int argc, char * argv[])
{
    g_instance = shared_from_this();

    // parse the arguments
    //
    f_opt.reset(new advgetopt::getopt(argc, argv, g_manager_options, g_configuration_files, "SNAPMANAGER_OPTIONS"));

    // --help
    //
    if(f_opt->is_defined("help"))
    {
        f_opt->usage(f_opt->no_error, "Usage: %s -<arg> ...\n", argv[0]);
        exit(1);
    }

    // --version
    //
    if(f_opt->is_defined("version"))
    {
        std::cout << SNAPMANAGERCGI_VERSION_STRING << std::endl;
        exit(0);
    }

    // read the configuration file
    //
    f_config.read_config_file( QString::fromUtf8( f_opt->get_string("config").c_str() ) );

    // --server-name (mandatory for snapmanagerdaemon, not expected for snapmanager.cgi)
    //
    if(f_daemon)
    {
        if(!f_opt->is_defined("server-name"))
        {
            throw std::runtime_error("fatal error: --server-name is a required argument for snapmanagerdaemon.");
        }
        f_server_name = f_opt->get_string("server-name").c_str();
    }
    else
    {
        if(f_opt->is_defined("server-name"))
        {
            throw std::runtime_error("fatal error: --server-name is not an authorized argument for snapmanager.cgi.");
        }
    }

    // --debug
    //
    f_debug = f_opt->is_defined("debug");

    // setup the logger
    // the definition in the configuration file has priority...
    //
    if(f_config.contains("log_server")
    && snap::logging::is_loggingserver_available(f_config["log_server"]))
    {
        f_log_conf = f_config["log_server"];
    }
    else
    {
        QString log_config_filename(QString("log_config_%1").arg(f_daemon ? "daemon" : "cgi"));
        if(f_config.contains(log_config_filename))
        {
            // use .conf definition when available
            f_log_conf = f_config[log_config_filename];
        }
        else
        {
            f_log_conf = QString::fromUtf8(f_opt->get_string("log-config").c_str());
        }
    }
    snap::logging::configure_conffile( f_log_conf );

    if(f_debug)
    {
        // Force the logger level to DEBUG
        // (unless already lower)
        //
        snap::logging::reduce_log_output_level( snap::logging::log_level_t::LOG_LEVEL_DEBUG );
    }

    // make sure there are no standalone parameters
    //
    if( f_opt->is_defined( "--" ) )
    {
        std::cerr << "fatal error: unexpected parameter found on daemon command line." << std::endl;
        f_opt->usage(f_opt->error, "Usage: %s -<arg> ...\n", argv[0]);
        snap::NOTREACHED();
    }

    // get the data path, we will be saving the status of each computer
    // in the cluster using this path
    //
    // Note: the user could change this path to use /run/snapwebsites/...
    //       instead so that way it saves the data to RAM instead of disk;
    //       however, by default we use the disk because it may end up being
    //       rather large and we do not want to swarm the memory of small
    //       VPSes; also that way snapmanager.cgi knows of all the statuses
    //       immediately after a reboot
    //
    f_data_path = "/var/lib/snapwebsites/cluster-status";
    if(f_config.contains("data_path"))
    {
        // use .conf definition when available
        f_data_path = f_config["data_path"];
    }
    // make sure directory exists
    if(snap::mkdir_p(f_data_path, false) != 0)
    {
        std::stringstream msg;
        msg << "manager::init(): mkdir_p(...): process could not create cluster-status directory \""
            << f_data_path
            << "\".";
        throw std::runtime_error(msg.str());
    }

    // get the user defined path to plugins if set
    //
    if(f_config.contains("plugins_path"))
    {
        f_plugins_path = f_config["plugins_path"];
    }
}


/** \brief Retrieve a pointer to the watchdog server.
 *
 * This function retrieve an instance pointer of the watchdog server.
 * If the instance does not exist yet, then it gets created. A
 * server is also a plugin which is named "server".
 *
 * \note
 * In the snapserver this function is static. Here it is useless...
 *
 * \return A manager pointer to the watchdog server.
 */
manager::pointer_t manager::instance()
{
    return g_instance;
}


QString manager::description() const
{
    return "Main manager plugin (\"server\")";
}


QString manager::dependencies() const
{
    return QString();
}


void manager::bootstrap(snap_child * snap)
{
    // virtual function stub
    NOTUSED(snap);
}


void manager::load_plugins()
{
    // we always want to load all the plugins
    //
    snap::snap_string_list all_plugins(snap::plugins::list_all(f_plugins_path));

    // the list_all() includes "server", but we cannot load the server
    // plugin
    //
    all_plugins.removeOne("server");

    if(!snap::plugins::load(f_plugins_path, this, std::static_pointer_cast<snap::plugins::plugin>(g_instance), all_plugins))
    {
        throw snapmanager_exception_cannot_load_plugins("the snapmanager library could not load its plugins");
    }
}


snap::snap_string_list manager::list_of_servers()
{
    snap::snap_string_list result;

    QString const pattern(QString("%1/*.db").arg(f_data_path));

    glob_t dir = glob_t();
    int const r(glob(
            pattern.toUtf8().data(),
            GLOB_NOESCAPE,
            [](const char * epath, int eerrno)
            {
                SNAP_LOG_ERROR("an error occurred while reading directory under \"")
                              (epath)
                              ("\". Got error: ")
                              (eerrno)
                              (", ")
                              (strerror(eerrno))
                              (".");

                // do not abort on a directory read error...
                return 0;
            },
            &dir));
    std::shared_ptr<glob_t> ai(&dir, glob_deleter);

    if(r != 0)
    {
        // do nothing when errors occur
        //
        switch(r)
        {
        case GLOB_NOSPACE:
            SNAP_LOG_ERROR("glob() did not have enough memory to alllocate its buffers.");
            break;

        case GLOB_ABORTED:
            SNAP_LOG_ERROR("glob() was aborted after a read error.");
            break;

        case GLOB_NOMATCH:
            SNAP_LOG_ERROR("glob() could not find any status information.");
            break;

        default:
            SNAP_LOG_ERROR("unknown glob() error code: ")(r)(".");
            break;

        }
    }
    else
    {
        // copy the list from the output of glob()
        //
        for(size_t idx(0); idx < dir.gl_pathc; ++idx)
        {
            result << QString::fromUtf8(dir.gl_pathv[idx]);
        }
    }

    return result;
}


QString manager::get_public_ip() const
{
    return f_public_ip;
}


snap::snap_string_list const & manager::get_snapmanager_frontend() const
{
    static snap::snap_string_list empty;
    return empty;
}


bool manager::stop_now_prima() const
{
    return false;
}


int manager::get_version_major()
{
    return SNAPMANAGERCGI_VERSION_MAJOR;
}


int manager::get_version_minor()
{
    return SNAPMANAGERCGI_VERSION_MINOR;
}


int manager::get_version_patch()
{
    return SNAPMANAGERCGI_VERSION_PATCH;
}


char const * manager::get_version_string()
{
    return SNAPMANAGERCGI_VERSION_STRING;
}



} // namespace snap_manager
// vim: ts=4 sw=4 et
