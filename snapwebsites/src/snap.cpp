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
#include <advgetopt/advgetopt.h>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <syslog.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

namespace
{
	const std::vector<std::string> g_configuration_files  =
	{
		"/etc/snapwebsites/snapcgi.conf"//,
		//"~/.snapwebsites/snapcgi.conf"	// TODO: tildes are not supported
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
			"serveraddr",
			NULL,
			"IP address on which the snapserver is running",
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
    advgetopt::getopt	f_opt;
};

snap_cgi::snap_cgi( int argc, char *argv[] )
    : f_opt(argc, argv, g_snapcgi_options, g_configuration_files, "SNAPCGI_OPTIONS")
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

	std::cout	<< "HTTP/1.1 " << code << std::endl
				<< "Expires: Sun, 19 Nov 1978 05:00:00 GMT" << std::endl
				<< "Content-type: text/html" << std::endl
				<< std::endl
				<< "<h1>Internal Error</h1>" << std::endl
				<< "<p>Sorry! We found an invalid server configuration or some other error occured.</p>"
				<< std::endl
				;

	return 1;
}

bool snap_cgi::verify()
{
	if( !f_opt.is_defined("serveraddr") )
	{
		throw tcp_client_server::tcp_client_server_parameter_error("serveraddr is not defined!");
	}

	// catch "invalid" methods early so we don't waste
	// any time with methods we don't support
	// later we may add support for PUT and DELETE though
	const char *request_method(getenv("REQUEST_METHOD"));
	if(request_method == NULL)
	{
		std::cout	<< "Status: 405 Method Not Defined"			<< std::endl
					<< "Expires: Sat, 1 Jan 2000 00:00:00 GMT"	<< std::endl
					<< "Allow: GET, HEAD, POST"					<< std::endl
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
		std::cout	<< "Expires: Sat, 1 Jan 2000 00:00:00 GMT" << std::endl
					<< "Allow: GET, HEAD, POST"                << std::endl
					<< std::endl;
		return false;
	}

	// success
	return true;
}

int snap_cgi::process()
{
	// TODO: we need to get the host address & port from the configuration file
	//
	int port = 4004;
	try
	{
		if( !f_opt.is_defined("serveraddr") )
		{
			throw tcp_client_server::tcp_client_server_parameter_error("serveraddr is not defined!");
		}

		tcp_client_server::tcp_client socket( f_opt.get_string("serveraddr"), port);

		if(socket.write("#START\n", 7) != 7)
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
			std::string var;
			for(;;)
			{
				int c(getchar());
				if(c == '&' || c == EOF)
				{
					var += "\n";
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
		// TODO: buffer the entire data? it is definitively faster to pass it
		//       through as it comes in, but in order to be able to return an
		//       error instead of a broken page, we may want to consider
		//       buffering first?
		int again(1);
		bool more(false);
		for(;;)
		{
			char buf[64 * 1024];
			qint64 r(socket.read(buf, sizeof(buf)));
			if(r > 0)
			{
				fwrite(buf, r, 1, stdout);
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
	catch(tcp_client_server::tcp_client_server_runtime_error&)
	{
		std::stringstream err;
		err << "CGI client could not connect to server (socket error: ";
		//
		// TODO: add some error string in our socket
		//
		// please use strstream, since sprintf() is unsafe...
		//
		//char buf[16];
		//sprintf(buf, "%d", socket.error()); <--- EEEEWWWWW
		//err += buf;
#if 0
		err << socket.error();
#else
		err << "...";
#endif
		err << ").";
		return error( "503 Service Unavailable", err.str().c_str() );
	}
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
	catch(...)
	{
		// this should never happen!
		cgi.error("503 Service Unavailable", "the script caught an exception.");
		return 1;
	}
}

// vim: ts=4 sw=4
