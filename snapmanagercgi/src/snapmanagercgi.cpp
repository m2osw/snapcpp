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

namespace
{
    const std::vector<std::string> g_configuration_files =
    {
        "/etc/snapwebsites/snapmanagercgi.conf"//,
        //"~/.snapwebsites/snapmanagercgi.conf"    // TODO: tildes are not supported
    };

    const advgetopt::getopt::option g_snapmanagercgi_options[] =
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
        std::cerr << SNAPMANAGERCGI_VERSION_MAJOR << "." << SNAPMANAGERCGI_VERSION_MINOR << "." << SNAPMANAGERCGI_VERSION_PATCH << std::endl;
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
        // TODO: convert to use "snap_addr::addr"
        //
        std::string const snapcommunicator(f_opt.get_string("snapcommunicator"));
        std::string::size_type const pos(snapcommunicator.find_first_of(':'));
        if(pos == std::string::npos)
        {
            // only an address
            f_communicator_address = snapcommunicator;
        }
        else
        {
            // address first
            f_communicator_address = snapcommunicator.substr(0, pos);
            // port follows
            std::string const port(snapcommunicator.substr(pos + 1));
            f_communicator_port = 0;
            for(char const * p(port.c_str()); *p != '\0'; ++p)
            {
                char const c(*p);
                if(c < '0' || c > '9')
                {
                    SNAP_LOG_FATAL("Invalid port (found a character that is not a digit in \"")(snapcommunicator)("\".");
                    throw snap::snap_exception("the port in the snapcommunicator parameter is not valid: " + snapcommunicator + ".");
                }
                f_communicator_port = f_communicator_port * 10 + c - '0';
                if(f_communicator_port > 65535)
                {
                    SNAP_LOG_FATAL("Invalid port (Port number too large in \"")(snapcommunicator)("\".");
                    throw snap::snap_exception("the port in the snapcommunicator parameter is too large (we only support a number from 1 to 65535): " + snapcommunicator + ".");
                }
            }
            // forbid port zero
            if(f_communicator_port <= 0)
            {
                SNAP_LOG_FATAL("Invalid port (Port number too small in \"")(snapcommunicator)("\".");
                throw snap::snap_exception("the port in the snapcommunicator parameter is too small (we only support a number from 1 to 65535): " + snapcommunicator + ".");
            }
        }
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
            std::string body("<html><head><title>Method Not Allowed</title></head><body><p>Sorry. We only support GET and POST.</p></body></html>");
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

    std::string body("<html><head><title>Snap Manager</title></head><body><p>...TODO...</p></body></html>");
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




}
// namespace snap_manager



int main(int argc, char * argv[])
{
    try
    {
        snap_manager::manager cgi(argc, argv);
        try
        {
            if(!cgi.verify())
            {
                // not acceptable, the verify() function already sent a
                // response, just exit with 1
                return 1;
            }
            return cgi.process();
        }
        catch(std::runtime_error const & e)
        {
            // this should rarely happen!
            return cgi.error("503 Service Unavailable", nullptr, ("The Snap! C++ CGI script caught a runtime exception: " + std::string(e.what()) + ".").c_str());
        }
        catch(std::logic_error const & e)
        {
            // this should never happen!
            return cgi.error("503 Service Unavailable", nullptr, ("The Snap! C++ CGI script caught a logic exception: " + std::string(e.what()) + ".").c_str());
        }
        catch(...)
        {
            return cgi.error("503 Service Unavailable", nullptr, "The Snap! C++ CGI script caught an unknown exception.");
        }
    }
    catch(std::exception const & e)
    {
        // we are in trouble, we cannot even answer!
        std::cerr << "snapmanager: initialization exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
