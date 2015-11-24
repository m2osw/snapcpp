// Snap Websites Server -- snap websites CGI function
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

// at this point this is just a passwthrough process, at some point we may
// want to implement a (complex) cache system that works here

//
// The following is a sample environment from Apache2.
//
// # arguments
// argv[0] = "/usr/clients/www/alexis.m2osw.com/cgi-bin/env_n_args.cgi"
//
// # See also: http://www.cgi101.com/book/ch3/text.html
//
// # environment
// UNIQUE_ID=VjAW4H8AAAEAAC7d0YIAAAAE
// SCRIPT_URL=/images/finball/20130711-lightning-by-Karl-Gehring.png
// SCRIPT_URI=http://csnap.m2osw.com/images/finball/20130711-lightning-by-Karl-Gehring.png
// CLEAN_SNAP_URL=1
// HTTP_HOST=csnap.m2osw.com
// HTTP_USER_AGENT=Mozilla/5.0 (X11; Linux i686 on x86_64; rv:41.0) Gecko/20100101 Firefox/41.0 SeaMonkey/2.38
// HTTP_ACCEPT=image/png,image/*;q=0.8,*/*;q=0.5
// HTTP_ACCEPT_LANGUAGE=en-US,en;q=0.8,fr-FR;q=0.5,fr;q=0.3
// HTTP_ACCEPT_ENCODING=gzip, deflate
// HTTP_REFERER=http://csnap.m2osw.com/css/finball/finball_0.0.127.min.css
// HTTP_COOKIE=cookieconsent_dismissed=yes; xUVt9AD6G4xKO_AU=036d371e8c10f340/2034695214
// HTTP_CONNECTION=keep-alive
// HTTP_CACHE_CONTROL=max-age=0
// PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
// SERVER_SIGNATURE=
// SERVER_SOFTWARE=Apache
// SERVER_NAME=csnap.m2osw.com
// SERVER_ADDR=162.226.130.121
// SERVER_PORT=80
// REMOTE_HOST=halk.m2osw.com
// REMOTE_ADDR=162.226.130.121
// DOCUMENT_ROOT=/usr/clients/www/csnap.m2osw.com/public_html/
// REQUEST_SCHEME=http
// CONTEXT_PREFIX=/cgi-bin/
// CONTEXT_DOCUMENT_ROOT=/usr/clients/www/csnap.m2osw.com/cgi-bin/
// SERVER_ADMIN=webmaster@m2osw.com
// SCRIPT_FILENAME=/usr/clients/www/csnap.m2osw.com/cgi-bin/snap.cgi
// REMOTE_PORT=51596
// GATEWAY_INTERFACE=CGI/1.1
// SERVER_PROTOCOL=HTTP/1.1
// REQUEST_METHOD=GET
// QUERY_STRING=
// REQUEST_URI=/images/finball/20130711-lightning-by-Karl-Gehring.png
// SCRIPT_NAME=/cgi-bin/snap.cgi
//

#include "log.h"
#include "not_reached.h"
#include "snapwebsites.h"
#include "tcp_client_server.h"

#include <advgetopt/advgetopt.h>


namespace
{
    const std::vector<std::string> g_configuration_files =
    {
        "/etc/snapwebsites/snapcgi.conf"//,
        //"~/.snapwebsites/snapcgi.conf"    // TODO: tildes are not supported
    };

    const advgetopt::getopt::option g_snapcgi_options[] =
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
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "snapserver",
            nullptr,
            "IP address on which the snapserver is running, it may include a port (i.e. 192.168.0.1:4004)",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "log_config",
            "/etc/snapwebsites/snapcgilog.properties",
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
//namespace


class snap_cgi
{
public:
    snap_cgi( int argc, char * argv[] );
    ~snap_cgi();

    int error(char const * code, char const * msg);
    bool verify();
    int process();

private:
    advgetopt::getopt   f_opt;
    int                 f_port;     // snap server port
    std::string         f_address;  // snap server address
};


snap_cgi::snap_cgi( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapcgi_options, g_configuration_files, "SNAPCGI_OPTIONS")
    , f_port(4004)
    , f_address("0.0.0.0")
{
    if(f_opt.is_defined("version"))
    {
        std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
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


snap_cgi::~snap_cgi()
{
}


int snap_cgi::error(char const * code, char const * msg)
{
    SNAP_LOG_ERROR(msg);

    std::string body;
    if(strncmp(code, "404 ", 4) == 0)
    {
        body = "<h1>Page Not Found</h1>"
               "<p>The requested page was not found on this server.</p>";
    }
    else
    {
        body = "<h1>Internal Server Error</h1>"
               "<p>Sorry! We found an invalid server configuration or some other error occured.</p>";
    }

    std::cout   << "Status: " << code                       << std::endl
                << "Expires: Sun, 19 Nov 1978 05:00:00 GMT" << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << body.length()      << std::endl
                << "X-Powered-By: snap.cgi"                 << std::endl
                << std::endl
                << body
                ;

    return 1;
}


bool snap_cgi::verify()
{
    // If not defined, keep the default of localhost:4004
    if(f_opt.is_defined("snapserver"))
    {
        std::string snapserver(f_opt.get_string("snapserver"));
        std::string::size_type pos(snapserver.find_first_of(':'));
        if(pos == std::string::npos)
        {
            // only an address
            f_address = snapserver;
        }
        else
        {
            // address first
            f_address = snapserver.substr(0, pos);
            // port follows
            std::string const port(snapserver.substr(pos + 1));
            f_port = 0;
            for(char const * p(port.c_str()); *p != '\0'; ++p)
            {
                char const c(*p);
                if(c < '0' || c > '9')
                {
                    SNAP_LOG_FATAL("Invalid port (found a character that is not a digit in \"")(snapserver)("\".");
                    throw tcp_client_server::tcp_client_server_parameter_error("the port in the snapserver parameter is not valid: " + snapserver + ".");
                }
                f_port = f_port * 10 + c - '0';
                if(f_port > 65535)
                {
                    SNAP_LOG_FATAL("Invalid port (Port number too large in \"")(snapserver)("\".");
                    throw tcp_client_server::tcp_client_server_parameter_error("the port in the snapserver parameter is too large (we only support a number from 1 to 65535): " + snapserver + ".");
                }
            }
            // forbid port zero
            if(f_port <= 0)
            {
                SNAP_LOG_FATAL("Invalid port (Port number too small in \"")(snapserver)("\".");
                throw tcp_client_server::tcp_client_server_parameter_error("the port in the snapserver parameter is too small (we only support a number from 1 to 65535): " + snapserver + ".");
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
            std::string body("<html><head><title>Method Not Defined</title></head><body><p>Sorry. We only support GET, HEAD, and POST.</p></body></html>");
            std::cout   << "Status: 405 Method Not Defined"         << std::endl
                        << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, HEAD, POST"                 << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snap.cgi"                 << std::endl
                        << std::endl
                        << body;
            return false;
        }
        if(strcmp(request_method, "GET") != 0
        && strcmp(request_method, "HEAD") != 0
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
            std::string body("<html><head><title>Method Not Allowed</title></head><body><p>Sorry. We only support GET, HEAD, and POST.</p></body></html>");
            std::cout   << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, HEAD, POST"                 << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snap.cgi"                 << std::endl
                        << std::endl
                        << body;
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * http_host(getenv("HTTP_HOST"));
        if(http_host == nullptr)
        {
            error("400 Bad Request", "The host you want to connect to must be specified.");
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("HTTP_HOST=")(http_host);
#endif
        if(tcp_client_server::is_ipv4(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv4 address.");
            std::cout   << "Status: 444 No Response" << std::endl
                        << "Connection: close" << std::endl
                        << "X-Powered-By: snap.cgi" << std::endl
                        << std::endl
                        ;
            // TODO: send IP to firewall
            return false;
        }
        if(tcp_client_server::is_ipv6(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv6 address.");
            std::cout   << "Status: 444 No Response" << std::endl
                        << "Connection: close" << std::endl
                        << "X-Powered-By: snap.cgi" << std::endl
                        << std::endl
                        ;
            // TODO: send IP to firewall
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
            // we probably do not have our snap.cgi run anyway...
            //
            error("400 Bad Request", "The path to the page you want to read must be specified.");
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("REQUEST_URI=")(request_uri);
#endif

        // if we receive this, someone tried to directly access our snap.cgi
        // which will not work right so better err immediately
        //
        if(strncmp(request_uri, "/cgi-bin/", 9) == 0)
        {
            error("404 Page Not Found", "The REQUEST_URI cannot start with \"/cgi-bin/\".");
            // TODO: send IP to firewall?
            return false;
        }

        // TBD: we could test <protocol>:// instead of specifically http
        //
        if(strncmp(request_uri, "http://", 7) == 0
        || strncmp(request_uri, "https://", 8) == 0)
        {
            // avoid proxy accesses
            error("404 Page Not Found", "The REQUEST_URI cannot start with \"http[s]://\".");
            // TODO: send IP to firewall?
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * user_agent(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
        if(user_agent == nullptr)
        {
            // this should NEVER happen because without a path after the method
            // we probably do not have our snap.cgi run anyway...
            //
            error("400 Bad Request", "The path to the page you want to read must be specified.");
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

        // if we receive this, someone tried to directly access our snap.cgi
        // which will not work right so better err immediately
        //
        if(*user_agent == 0)
        {
            error("400 Bad Request", "The HTTP_USER_AGENT cannot be empty.");
            // TODO: send IP to firewall service?
            return false;
        }
    }

    // success
    return true;
}


int snap_cgi::process()
{
    // WARNING: do not use std::string because nullptr will crash
    char const * request_method( getenv("REQUEST_METHOD") );
    if(request_method == nullptr)
    {
        // the method was already checked in the validate, before this
        // call so it should always be defined...
        //
        SNAP_LOG_FATAL("Method not defined in REQUEST_METHOD.");
        std::string body("<html><head><title>Method Not Defined</title></head><body><p>Sorry. We only support GET, HEAD, and POST.</p></body></html>");
        std::cout   << "Status: 405 Method Not Defined"         << std::endl
                    << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                    << "Connection: close"                      << std::endl
                    << "Allow: GET, HEAD, POST"                 << std::endl
                    << "Content-Type: text/html; charset=utf-8" << std::endl
                    << "Content-Length: " << body.length()      << std::endl
                    << "X-Powered-By: snap.cgi"                 << std::endl
                    << std::endl
                    << body;
        return false;
    }
#ifdef _DEBUG
    SNAP_LOG_DEBUG("processing request_method=")(request_method);

    SNAP_LOG_DEBUG("f_address=")(f_address.c_str())(", f_port=")(f_port);
#endif
    tcp_client_server::tcp_client socket(f_address, f_port);

#define START_COMMAND "#START=" SNAPWEBSITES_VERSION_STRING
    if(socket.write(START_COMMAND "\n", sizeof(START_COMMAND)) != sizeof(START_COMMAND))
#undef START_COMMAND
    {
        return error("504 Gateway Timeout", "error while writing to the child process (1).");
    }
    for(char ** e(environ); *e; ++e)
    {
        std::string env(*e);
        int const len(static_cast<int>(env.size()));
        //
        // Replacing all '\n' in the env variables to '|'
        // to prevent snap_child from complaining and die.
        //
        std::replace( env.begin(), env.end(), '\n', '|' );
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("Writing environment '")(env.c_str())("'");
#endif
        if(socket.write(env.c_str(), len) != len)
        {
            return error("504 Gateway Timeout", "error while writing to the child process (2).");
        }
        if(socket.write("\n", 1) != 1)
        {
            return error("504 Gateway Timeout", "error while writing to the child process (3).");
        }
    }
    if( strcmp(request_method, "POST") == 0 )
    {
#ifdef _DEBUG
        SNAP_LOG_DEBUG("writing #POST");
#endif
        if(socket.write("#POST\n", 6) != 6)
        {
            return error("504 Gateway Timeout", "error while writing to the child process (4).");
        }
        // we also want to send the POST variables
        // http://httpd.apache.org/docs/2.4/howto/cgi.html
        // note that in case of a non-multipart post variables are separated
        // by & and the variable names and content cannot include the & since
        // that would break the whole scheme so we can safely break (add \n)
        // at that location
        bool const is_multipart(QString(getenv("CONTENT_TYPE")).startsWith("multipart/form-data"));
        int break_char(is_multipart ? '\n' : '&');
        std::string var;
        for(;;)
        {
            int c(getchar());
            if(c == break_char || c == EOF)
            {
                // a POST without variables most often ends up with
                // one empty line which we ignore
                if(!var.empty())
                {
                    if(!is_multipart || c != EOF)
                    {
                        var += "\n";
                    }
                    if(socket.write(var.c_str(), var.length()) != static_cast<int>(var.length()))
                    {
                        return error("504 Gateway Timeout", ("error while writing POST variable \"" + var + "\" to the child process.").c_str());
                    }
#ifdef _DEBUG
                    SNAP_LOG_DEBUG("wrote var=")(var.c_str());
#endif
                    var.clear();
                }
                if(c == EOF)
                {
                    // this was the last variable
                    break;
                }
            }
            else
            {
                var += c;
            }
        }
    }
#ifdef _DEBUG
    SNAP_LOG_DEBUG("writing #END");
#endif
    if(socket.write("#END\n", 5) != 5)
    {
        return error("504 Gateway Timeout", "error while writing to the child process (4).");
    }

    // if we get here then we can just copy the output of the child to Apache2
    // the wait will flush all the writes as necessary
    //
    // XXX   buffer the entire data? it is definitively faster to pass it
    //       through as it comes in, but in order to be able to return an
    //       error instead of a broken page, we may want to consider
    //       buffering first?
    for(;;)
    {
        char buf[64 * 1024];
        int const r(socket.read(buf, sizeof(buf)));
        if(r > 0)
        {
//#ifdef _DEBUG
//            SNAP_LOG_DEBUG("writing buf=")(buf);
//#endif
            if(fwrite(buf, r, 1, stdout) != 1)
            {
                // there is not point in calling error() from here because
                // the connection is probably broken anyway, just report
                // the problem in to the logger
                SNAP_LOG_FATAL("an I/O error occurred while sending the response to the client");
                return 1;
            }

// to get a "verbose" CGI (For debug purposes)
#if 0
            {
                FILE *f = fopen("/tmp/snap.output", "a");
                if(f != nullptr)
                {
                    fwrite(buf, r, 1, f);
                    fclose(f);
                }
            }
#endif
        }
        else if(r == -1)
        {
            SNAP_LOG_FATAL("an I/O error occurred while reading the response from the server");
            break;
        }
        else if(r == 0)
        {
			// normal exit
            break;
        }
    }
    // TODO: handle potential read problems...
#ifdef _DEBUG
    SNAP_LOG_DEBUG("Closing connection...");
#endif
    return 0;
}


int main(int argc, char * argv[])
{
    try
    {
        snap_cgi cgi( argc, argv );
        try
        {
            if(!cgi.verify())
            {
                return 1;
            }
            return cgi.process();
        }
        catch(std::runtime_error const & e)
        {
            // this should never happen!
            cgi.error("503 Service Unavailable", ("The Snap! C++ CGI script caught a runtime exception: " + std::string(e.what()) + ".").c_str());
            return 1;
        }
        catch(std::logic_error const & e)
        {
            // this should never happen!
            cgi.error("503 Service Unavailable", ("The Snap! C++ CGI script caught a logic exception: " + std::string(e.what()) + ".").c_str());
            return 1;
        }
        catch(...)
        {
            // this should never happen!
            cgi.error("503 Service Unavailable", "The Snap! C++ CGI script caught an unknown exception.");
            return 1;
        }
    }
    catch(std::exception const & e)
    {
        // we are in trouble, we cannot even answer
        std::cerr << "snap: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
