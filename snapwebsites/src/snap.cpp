// Snap Websites Server -- snap websites CGI function
// Copyright (C) 2011-2013  Made to Order Software Corp.
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
// UNIQUE_ID=TtISeX8AAAEAAHhHi7kAAAAB
// HTTP_HOST=alexis.m2osw.com
// HTTP_USER_AGENT=Mozilla/5.0 (X11; Linux i686 on x86_64; rv:8.0.1) Gecko/20111121 Firefox/8.0.1 SeaMonkey/2.5
// HTTP_ACCEPT=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
// HTTP_ACCEPT_LANGUAGE=en-us,en;q=0.8,fr-fr;q=0.5,fr;q=0.3
// HTTP_ACCEPT_ENCODING=gzip, deflate
// HTTP_ACCEPT_CHARSET=ISO-8859-1,utf-8;q=0.7,*;q=0.7
// HTTP_CONNECTION=keep-alive
// HTTP_COOKIE=SESS8b653582e586f876284c0be25de5ac73=d32eb1fccf3f3f3beb5bc2b9439dd160; DRUPAL_UID=1
// HTTP_CACHE_CONTROL=max-age=0
// HTTP_REFERER=http://snapwebsites.com/
// PATH=/usr/local/bin:/usr/bin:/bin
// SERVER_SIGNATURE=
// SERVER_SOFTWARE=Apache
// SERVER_NAME=alexis.m2osw.com
// SERVER_ADDR=192.168.1.1
// SERVER_PORT=80
// REMOTE_HOST=adsl-64-166-38-38.dsl.scrm01.pacbell.net
// REMOTE_ADDR=64.166.38.38
// DOCUMENT_ROOT=/usr/clients/www/alexis.m2osw.com/public_html/
// SERVER_ADMIN=alexis@m2osw.com
// SCRIPT_FILENAME=/usr/clients/www/alexis.m2osw.com/cgi-bin/env_n_args.cgi
// REMOTE_PORT=37722
// GATEWAY_INTERFACE=CGI/1.1
// SERVER_PROTOCOL=HTTP/1.1
// REQUEST_METHOD=GET
// QUERY_STRING=testing=environment
// REQUEST_URI=/cgi-bin/env_n_args.cgi?testing=environment
// SCRIPT_NAME=/cgi-bin/env_n_args.cgi
//

#include "tcp_client_server.h"
#include "snapwebsites.h"
#include <advgetopt/advgetopt.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>

namespace
{
    const std::vector<std::string> g_configuration_files  =
    {
        "/etc/snapwebsites/snapcgi.conf"//,
        //"~/.snapwebsites/snapcgi.conf"    // TODO: tildes are not supported
    };

    const advgetopt::getopt::option g_snapcgi_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: snap.cgi [-<opt>]",
            advgetopt::getopt::help_argument
        },
        // OPTIONS
        {
            '\0',
            0,
            NULL,
            NULL,
            "options:",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
            "snapserver",
            NULL,
            "IP address on which the snapserver is running, it may include a port (i.e. 192.168.0.1:4004)",
            advgetopt::getopt::required_argument
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


class snap_cgi
{
public:
    snap_cgi( int argc, char *argv[] );
    ~snap_cgi();

    int error(const char *code, const char *msg);
    bool verify();
    int process();

private:
    advgetopt::getopt   f_opt;
    int                 f_port;     // snap server port
    std::string         f_address;  // snap server address
};

snap_cgi::snap_cgi( int argc, char *argv[] )
    : f_opt(argc, argv, g_snapcgi_options, g_configuration_files, "SNAPCGI_OPTIONS")
    , f_port(4004)
    , f_address("0.0.0.0")
{
    openlog("snap.cgi", LOG_NDELAY | LOG_PID, LOG_DAEMON);
}

snap_cgi::~snap_cgi()
{
}

int snap_cgi::error(const char *code, const char *msg)
{
    // XXX
    // We should look into having that using the main Snap log settings.
    syslog(LOG_CRIT, "%s", msg);

    std::cout   << "HTTP/1.1 " << code << std::endl
                << "Expires: Sun, 19 Nov 1978 05:00:00 GMT" << std::endl
                << "Content-type: text/html" << std::endl
                << std::endl
                << "<h1>Internal Server Error</h1>" << std::endl
                << "<p>Sorry! We found an invalid server configuration or some other error occured.</p>"
                << std::endl
                ;

    return 1;
}

bool snap_cgi::verify()
{
    // The default is localhost:4004
    //if( !f_opt.is_defined("snapserver") )
    //{
    //    throw tcp_client_server::tcp_client_server_parameter_error("the snapserver parameter is not defined.");
    //}

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
            const std::string port(snapserver.substr(pos + 1));
            f_port = 0;
            for(const char *p(port.c_str()); *p != '\0'; ++p)
            {
                char c(*p);
                if(c < '0' || c > '9')
                {
                    throw tcp_client_server::tcp_client_server_parameter_error("the port in the snapserver parameter is not valid: " + snapserver + ".");
                }
                f_port = f_port * 10 + c - '0';
                if(f_port > 65535)
                {
                    throw tcp_client_server::tcp_client_server_parameter_error("the port in the snapserver parameter is too large (we only support a number from 0 to 65535): " + snapserver + ".");
                }
            }
            // XXX forbid port zero?
        }
    }

    // catch "invalid" methods early so we don't waste
    // any time with methods we don't support
    // later we may add support for PUT and DELETE though
    const char *request_method(getenv("REQUEST_METHOD"));
    if(request_method == NULL)
    {
        std::cout   << "Status: 405 Method Not Defined"         << std::endl
                    << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                    << "Allow: GET, HEAD, POST"                 << std::endl
                    << std::endl;
        return false;
    }
    if(strcmp(request_method, "GET") != 0
    && strcmp(request_method, "HEAD") != 0
    && strcmp(request_method, "POST") != 0)
    {
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
        std::cout   << "Expires: Sat, 1 Jan 2000 00:00:00 GMT" << std::endl
                    << "Allow: GET, HEAD, POST"                << std::endl
                    << std::endl;
        return false;
    }

    // success
    return true;
}

int snap_cgi::process()
{
    tcp_client_server::tcp_client socket(f_address, f_port);

#define START_COMMAND "#START=" SNAPWEBSITES_VERSION_STRING
    if(socket.write(START_COMMAND "\n", sizeof(START_COMMAND)) != sizeof(START_COMMAND))
#undef START_COMMAND
    {
        return error("504 Gateway Timeout", "error while writing to the child process (1).");
    }
    for(char **e(environ); *e; ++e)
    {
        int len = strlen(*e);
        if(socket.write(*e, len) != len)
        {
            return error("504 Gateway Timeout", "error while writing to the child process (2).");
        }
        if(socket.write("\n", 1) != 1)
        {
            return error("504 Gateway Timeout", "error while writing to the child process (3).");
        }
    }
    if(strcmp(getenv("REQUEST_METHOD"), "POST") == 0)
    {
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
                if(!is_multipart || c != EOF)
                {
                    var += "\n";
                }
                if(socket.write(var.c_str(), var.length()) != static_cast<int>(var.length()))
                {
                    return error("504 Gateway Timeout", ("error while writing POST variable \"" + var + "\" to the child process.").c_str());
                }
                if(c == EOF)
                {
                    // this was the last variable
                    break;
                }
                var.clear();
            }
            else
            {
                var += c;
            }
        }
    }
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
    int again(1);
    bool more(false);
    for(;;)
    {
        char buf[64 * 1024];
        int r(socket.read(buf, sizeof(buf)));
        if(r > 0)
        {
            if(fwrite(buf, r, 1, stdout) != 1)
            {
                // there is not point in calling error() from here because
                // the connection is probably broken anyway, just report
                // the problem in syslog()
                syslog(LOG_CRIT, "an I/O error occurred while sending the response to the client");
                return 1;
            }

// to get a "verbose" CGI (For debug purposes)
#if 0
{
FILE *f = fopen("/tmp/snap.output", "a");
if(f != NULL)
{
fwrite(buf, r, 1, f);
fclose(f);
}
}
#endif
        }
        else if(r == -1)
        {
            break;
        }
        else if(r == 0)
        {
            // wait 1 second
            --again;
            if(again == 0)
            {
                break;
            }
            more = true;
            sleep(1);
        }
    }
    // TODO: handle potential read problems...
    return 0;
}

int main(int argc, char *argv[])
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
    catch(const std::runtime_error& e)
    {
        // this should never happen!
        cgi.error("503 Service Unavailable", ("The Snap! C++ CGI script caught a runtime exception: " + std::string(e.what()) + ".").c_str());
        return 1;
    }
    catch(const std::logic_error& e)
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

// vim: ts=4 sw=4 et
