//
// File:        snapmanagercgi.cpp
// Object:      Allow for managing a Snap! Cluster.
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

#include "snapmanagercgi.h"

// C lib
//
#include <fcntl.h>
#include <glob.h>
#include <sys/file.h>

namespace
{

/** \brief The magic at expected on the first line of a status file.
 *
 * This string defines the magic string expected on the first line of
 * the file.
 *
 * \note
 * Note that our reader ignores \r characters so this is not currently
 * a 100% exact match, but since only our application is expected to
 * create / read these files, we are not too concerned.
 */
char const status_file_magic[] = "Snap! Status v1";

std::vector<std::string> const g_configuration_files
{
    "@snapwebsites@",  // project name
    "/etc/snapwebsites/snapmanagercgi.conf"
};

advgetopt::getopt::option const g_snapmanagercgi_options[] =
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
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "clients",
        nullptr,
        "Define the address of computers that are authorized to connect to this snapmanager.cgi instance.",
        advgetopt::getopt::optional_argument
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
        "data_path",
        "/var/lib/snapwebsites/cluster-status",
        "Path to this process data directory to save the cluster status.",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "log_config",
        "/etc/snapwebsites/snapmanagercgi.properties",
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
manager::manager( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapmanagercgi_options, g_configuration_files, "SNAPMANAGERCGI_OPTIONS")
    , f_communicator_port(4040)
    , f_communicator_address("127.0.0.1")
{
    if(f_opt.is_defined("version"))
    {
        std::cerr << SNAPMANAGERCGI_VERSION_STRING << std::endl;
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


manager::~manager()
{
}


int manager::error(char const * code, char const * msg, char const * details)
{
    if(details == nullptr)
    {
        details = "No details.";
    }

    SNAP_LOG_FATAL("error(\"")(code)("\", \"")(msg)("\", \"")(details)("\")");

    std::string body("<h1>");
    body += code;
    body += "</h1><p>";
    body += (msg == nullptr ? "Sorry! We found an invalid server configuration or some other error occurred." : msg);
    body += "</p>";

    std::cout   << "Status: " << code                       << std::endl
                << "Expires: Sun, 19 Nov 1978 05:00:00 GMT" << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << body.length()      << std::endl
                << "X-Powered-By: snapmanager.cgi"          << std::endl
                << std::endl
                << body
                ;

    return 1;
}


/** \brief Verify that the request is acceptable.
 *
 * This function makes sure that the request corresponds to what we
 * generally expect.
 *
 * \return true if the request is accepted, false otherwise.
 */
bool manager::verify()
{
    // If not defined, keep the default of localhost:4040
    if(f_opt.is_defined("snapcommunicator"))
    {
        snap_addr::addr const a(f_opt.get_string("snapcommunicator"), "127.0.0.1", 4040, "tcp");
        f_communicator_address = a.get_ipv4or6_string(false, false);
        f_communicator_port = a.get_port();
    }

    // catch "invalid" methods early so we do not waste
    // any time with methods we do not support at all
    //
    // later we want to add support for PUT, PATCH and DELETE though
    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * request_method(getenv("REQUEST_METHOD"));
        if(request_method == nullptr)
        {
            SNAP_LOG_FATAL("Request method is not defined.");
            std::string body("<html><head><title>Method Not Defined</title></head><body><p>Sorry. We only support GET and POST.</p></body></html>");
            std::cout   << "Status: 405 Method Not Defined"         << std::endl
                        << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, POST"                       << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snapmanager.cgi"          << std::endl
                        << std::endl
                        << body;
            return false;
        }
        if(strcmp(request_method, "GET") != 0
        && strcmp(request_method, "POST") != 0)
        {
            SNAP_LOG_FATAL("Request method is \"")(request_method)("\", which we currently refuse.");
            if(strcmp(request_method, "BREW") == 0)
            {
                // see http://tools.ietf.org/html/rfc2324
                std::cout << "Status: 418 I'm a teapot" << std::endl;
            }
            else
            {
                std::cout << "Status: 405 Method Not Allowed" << std::endl;
            }
            //
            std::string const body("<html><head><title>Method Not Allowed</title></head><body><p>Sorry. We only support GET and POST.</p></body></html>");
            std::cout   << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, POST"                       << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snapamanager.cgi"         << std::endl
                        << std::endl
                        << body;
            return false;
        }
    }

    // get the client IP address
    //
    char const * remote_addr(getenv("REMOTE_ADDR"));
    if(remote_addr == nullptr)
    {
        error("400 Bad Request", nullptr, "The REMOTE_ADDR parameter is not available.");
        return false;
    }

    // verify that this is a client we allow to use snapmanager.cgi
    //
    if(!f_opt.is_defined("clients"))
    {
        error("403 Forbidden", "You are not allowed on this server.", "The clients=... parameter is undefined.");
        return false;
    }

    {
        snap_addr::addr const remote_address(std::string(remote_addr) + ":80", "tcp");
        std::string const client(f_opt.get_string("clients"));

        snap::snap_string_list const client_list(QString::fromUtf8(client.c_str()).split(',', QString::SkipEmptyParts));
        bool found(false);
        for(auto const & c : client_list)
        {
            snap_addr::addr const client_address((c + ":80").toUtf8().data(), "tcp");
            if(client_address == remote_address)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            error("403 Forbidden", "You are not allowed on this server.", ("Your remote address is " + remote_address.get_ipv4or6_string()).c_str());
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * http_host(getenv("HTTP_HOST"));
        if(http_host == nullptr)
        {
            error("400 Bad Request", "The host you want to connect to must be specified.", nullptr);
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("HTTP_HOST=")(http_host);
#endif
        if(tcp_client_server::is_ipv4(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv4 address.");
            std::cout   << "Status: 444 No Response"        << std::endl
                        << "Connection: close"              << std::endl
                        << "X-Powered-By: snapmanager.cgi"  << std::endl
                        << std::endl
                        ;
            snap::server::block_ip(remote_addr, "week");
            return false;
        }
        if(tcp_client_server::is_ipv6(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv6 address.");
            std::cout   << "Status: 444 No Response"        << std::endl
                        << "Connection: close"              << std::endl
                        << "X-Powered-By: snapmanager.cgi"  << std::endl
                        << std::endl
                        ;
            snap::server::block_ip(remote_addr, "week");
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * request_uri(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REQUEST_URI)));
        if(request_uri == nullptr)
        {
            // this should NEVER happen because without a path after the method
            // we probably do not have our snapmanager.cgi run anyway...
            //
            error("400 Bad Request", "The path to the page you want to read must be specified.", nullptr);
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("REQUEST_URI=")(request_uri);
#endif

        // if we do not receive this, somehow someone was able to access
        // snapmanager.cgi without specifying /cgi-bin/... which is not
        // correct
        //
        if(strncasecmp(request_uri, "/cgi-bin/", 9) != 0)
        {
            error("404 Page Not Found", "We could not find the page you were looking for.", "The REQUEST_URI cannot start with \"/cgi-bin/\".");
            snap::server::block_ip(remote_addr);
            return false;
        }

        // TBD: we could test <protocol>:// instead of specifically http
        //
        if(strncasecmp(request_uri, "http://", 7) == 0
        || strncasecmp(request_uri, "https://", 8) == 0)
        {
            // avoid proxy accesses
            error("404 Page Not Found", nullptr, "The REQUEST_URI cannot start with \"http[s]://\".");
            snap::server::block_ip(remote_addr);
            return false;
        }

		// TODO: move to snapserver because this could be the name of a legal page...
        if(strcasestr(request_uri, "phpmyadmin") != nullptr)
        {
            // block myPhpAdmin accessors
            error("410 Gone", "MySQL left.", nullptr);
            snap::server::block_ip(remote_addr, "year");
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * user_agent(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
        if(user_agent == nullptr)
        {
            // we request an agent specification
            //
            error("400 Bad Request", "The accessing agent must be specified.", nullptr);
            snap::server::block_ip(remote_addr, "month");
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("HTTP_USER_AGENT=")(request_uri);
#endif

        // left trim
        while(isspace(*user_agent))
        {
            ++user_agent;
        }

        // if we receive this, someone tried to directly access our
        // snapmanager.cgi, which will not work right so better
        // err immediately
        //
        if(*user_agent == '\0'
        || (*user_agent == '-' && user_agent[1] == '\0')
        || strcasestr(user_agent, "ZmEu") != nullptr)
        {
            // note that we consider "-" as empty for this test
            error("400 Bad Request", nullptr, "The agent string cannot be empty.");
            snap::server::block_ip(remote_addr, "month");
            return false;
        }
    }

    // success
    return true;
}


/** \brief Process one hit to snapmanager.cgi.
 *
 * This is the function that generates the HTML or AJAX reply to
 * the client.
 *
 * \return 0 if the process worked as expected, 1 otherwise.
 */
int manager::process()
{
    // WARNING: do not use std::string because nullptr will crash
    char const * request_method( getenv("REQUEST_METHOD") );
    if(request_method == nullptr)
    {
        // the method was already checked in verify(), before this
        // call so it should always be defined here...
        //
        SNAP_LOG_FATAL("Method not defined in REQUEST_METHOD.");
        std::string body("<html><head><title>Method Not Defined</title></head><body><p>Sorry. We only support GET and POST.</p></body></html>");
        std::cout   << "Status: 405 Method Not Defined"         << std::endl
                    << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                    << "Connection: close"                      << std::endl
                    << "Allow: GET, POST"                       << std::endl
                    << "Content-Type: text/html; charset=utf-8" << std::endl
                    << "Content-Length: " << body.length()      << std::endl
                    << "X-Powered-By: snap.cgi"                 << std::endl
                    << std::endl
                    << body;
        return false;
    }
#ifdef _DEBUG
    SNAP_LOG_DEBUG("processing request_method=")(request_method);
#endif

    // retrieve the query string, that's all we use in this one (i.e.
    // at this point we ignore the path)
    //
    // TODO: add support to make sure the administrator uses HTTPS?
    //       (this can be done in Apache2)
    //
    char const * query_string(getenv("QUERY_STRING"));
    if(query_string != nullptr)
    {
        f_uri.set_query_string(QString::fromUtf8(query_string));
    }

    QDomDocument doc;
    QDomElement root(doc.createElement("manager"));
    doc.appendChild(root);

    generate_content(doc, root);

    std::string const xsl_filename(f_opt.get_string("stylesheet"));
//SNAP_LOG_WARNING("Doc = [")(doc.toString())("]");

    snap::xslt x;
    x.set_xsl_from_file(QString::fromUtf8(xsl_filename.c_str()));
    x.set_document(doc);
    QString const body(x.evaluate_to_string());

    //std::string body("<html><head><title>Snap Manager</title></head><body><p>...TODO...</p></body></html>");
    std::cout   //<< "Status: 200 OK"                         << std::endl
                << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << body.length()      << std::endl
                << "X-Powered-By: snap.cgi"                 << std::endl
                << std::endl
                << body;

    return 0;
}


/** \brief Generate the body of the page.
 *
 * This function checks the various query strings passed to the manager
 * and depending on those, generates a page.
 */
void manager::generate_content(QDomDocument doc, QDomElement root)
{
    QDomElement output(doc.createElement("output"));
    root.appendChild(output);

    QString const function(f_uri.query_option("function"));

    // is a host name specified?
    // if so then the function / page has to be applied to that specific host
    //
    if(f_uri.has_query_option("host"))
    {
        // the function is to be applied to that specific host
        //
        if(!function.isEmpty())
        {
            // apply a function on that specific host
            //
        }
        else
        {
            // no function + specific host, show a complete status from
            // that host
            //
        }
    }
    else
    {
        // no host specified, if there is a function it has to be applied
        // to all computers, otherwise show the list of computers and their
        // basic status
        //
        if(!function.isEmpty())
        {
            // execute function on all computers
            //
        }
        else
        {
            // "just" a cluster status...
            //
            get_cluster_status(doc, output);
        }
    }

}


void manager::get_cluster_status(QDomDocument doc, QDomElement output)
{
    struct err_callback
    {
        static int func(const char * epath, int eerrno)
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
        }
    };

    class safe_status_file
    {
    public:
        safe_status_file(char const * filename)
            : f_filename(filename)
            //, f_fd(-1)
        {
        }

        ~safe_status_file()
        {
            close();
        }

        void close()
        {
            if(f_fd != -1)
            {
                // Note: there is no need for an explicit unlock, the close()
                //       has the same effect on that file
                //::flock(f_fd, LOCK_UN);
                ::close(f_fd);
            }
        }

        bool open()
        {
            close();

            // open the file
            //
            f_fd = ::open(f_filename.c_str(), O_RDONLY | O_CLOEXEC, 0);
            if(f_fd < 0)
            {
                SNAP_LOG_ERROR("could not open file \"")
                              (f_filename)
                              ("\" to save snapmanagerdamon status.");
                return false;
            }

            // make sure we are the only one on the case
            //
            if(::flock(f_fd, LOCK_SH) != 0)
            {
                SNAP_LOG_ERROR("could not lock file \"")
                              (f_filename)
                              ("\" to read snapmanagerdamon status.");
                return false;
            }

            // transform to a FILE * so that way we benefit from the
            // caching mechanism without us having to re-implement such
            //
            f_file = fdopen(f_fd, "rb");
            if(f_file == nullptr)
            {
                SNAP_LOG_ERROR("could not allocate a FILE* for file \"")
                              (f_filename)
                              ("\" to read snapmanagerdamon status.");
                return false;
            }

            return true;
        }

        bool readline(QString & result)
        {
            std::string line;
            for(;;)
            {
                char buf[2];
                int const r(::fread(buf, sizeof(buf[0]), 1, f_file));
                if(r != 1)
                {
                    // reached end of file?
                    if(feof(f_file))
                    {
                        // we reached the EOF
                        result = QString::fromUtf8(line.c_str());
                        return false;
                    }
                    // there was an error
                    int const e(errno);
                    SNAP_LOG_ERROR("an error occurred while reading status file. Error: ")
                                  (e)
                                  (", ")
                                  (strerror(e));
                    result = QString();
                    return false; // simulate an EOF so we stop the reading loop
                }
                if(buf[0] == '\n')
                {
                    result = QString::fromUtf8(line.c_str());
                    return true;
                }
                // ignore any '\r'
                if(buf[0] != '\r')
                {
                    buf[1] = '\0';
                    line += buf;
                }
            }
            snap::NOTREACHED();
        }

    private:
        std::string f_filename;
        int         f_fd = -1;
        FILE *      f_file = nullptr;
    };

    std::string pattern;
    if(f_opt.get_string("data_path").empty())
    {
        pattern = "*.db";
    }
    else
    {
        pattern = f_opt.get_string("data_path") + "/*.db";
    }
    glob_t dir = glob_t();
    int const r(glob(pattern.c_str(), GLOB_NOESCAPE, err_callback::func, &dir));
    if(r != 0)
    {
        //globfree(&dir); -- needed in this case?

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
        QDomText text(doc.createTextNode("An error occurred while reading status data. Please check your snapmanagercgi.log file for more information."));
        output.appendChild(text);
        return;
    }

    // output/table
    QDomElement table(doc.createElement("table"));
    output.appendChild(table);

    table.setAttribute("class", "cluster-status");

    // output/table/tr
    QDomElement tr(doc.createElement("tr"));
    table.appendChild(tr);

    // output/table/th[1]
    QDomElement th(doc.createElement("th"));
    tr.appendChild(th);

        QDomText text(doc.createTextNode("Host"));
        th.appendChild(text);

    // output/table/th[2]
    th = doc.createElement("th");
    tr.appendChild(th);

        text = doc.createTextNode("Status");
        th.appendChild(text);

    bool has_error(false);
    for(size_t idx(0); idx < dir.gl_pathc; ++idx)
    {
        safe_status_file file(dir.gl_pathv[idx]);
        if(file.open())
        {
            QString line;
            if(!file.readline(line))
            {
                SNAP_LOG_ERROR("an error occurred while trying to read the first line of status file \"")
                              (dir.gl_pathv[idx])
                              ("\".");
                has_error = true;
            }
            else if(line == status_file_magic)
            {
                // we got what looks like a valid status file
                //
                while(file.readline(line))
                {
                    int const pos(line.indexOf('='));
                    if(pos < 1)
                    {
                        SNAP_LOG_ERROR("invalid line in \"")
                                      (dir.gl_pathv[idx])
                                      ("\", it has no \"name=...\".");
                    }
                    else
                    {
                        QString const path(dir.gl_pathv[idx]);
                        int basename_pos(path.lastIndexOf('/'));
                        if(basename_pos < 0)
                        {
                            // this should not happen, although it is perfectly
                            // possible that the administrator used "" as the
                            // path where statuses should be saved.
                            //
                            basename_pos = 0;
                        }
                        QString const host(path.mid(basename_pos + 1, path.length() - basename_pos - 1 - 3));

                        QString const name(line.mid(0, pos));
                        QString const value(line.mid(pos + 1));

                        if(name == "status")
                        {
                            // output/table/tr
                            tr = doc.createElement("tr");
                            table.appendChild(tr);

                            // output/table/td[1]
                            QDomElement td(doc.createElement("td"));
                            tr.appendChild(td);

                                // output/table/td[1]/a
                                QDomElement anchor(doc.createElement("a"));
                                td.appendChild(anchor);

                                anchor.setAttribute("href", QString("?host=%1").arg(host));

                                    text = doc.createTextNode(host);
                                    anchor.appendChild(text);

                            // output/table/td[2]
                            td = doc.createElement("td");
                            tr.appendChild(td);

                                text = doc.createTextNode(value);
                                td.appendChild(text);
                        }
                    }
                }
            }
            else
            {
                SNAP_LOG_ERROR("status file \"")
                              (dir.gl_pathv[idx])
                              ("\" does not start with the expected magic. Found: \"")
                              (line)
                              ("\", expected: \"")
                              (status_file_magic)
                              ("\".");
                has_error = true;
            }
        }
        else
        {
            SNAP_LOG_ERROR("could not open file \"")(dir.gl_pathv[idx])("\", skipping.");
            has_error = true;
        }
    }

    if(has_error)
    {
        // output/p
        QDomElement p(doc.createElement("p"));
        output.appendChild(p);

        p.setAttribute("class", "error");

            text = doc.createTextNode("Errors occurred while reading the status. Please check your snapmanagercgi.log file for details.");
            p.appendChild(text);
    }

    // free that memory (not useful in a CGI script though)
    globfree(&dir);
}



}
// namespace snap_manager
// vim: ts=4 sw=4 et
