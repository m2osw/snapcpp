//
// File:	iplock.cpp
// Object:	Allow users to easily add and remove IPs to/from a blacklist
//              of IP defined in an iptables firewall
//
// Copyright:	Copyright (c) 2007-2015 Made to Order Software Corp.
//		All Rights Reserved.
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

#include "iplock.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


class configuration
{
public:
	typedef std::vector<std::string>  ports_t;

	configuration();

	const std::string operator [] (const std::string& name) const
	{
		std::map<std::string, std::string>::const_iterator it = f_variables.find(name);
		if(it == f_variables.end()) {
			return "";
		}
		return it->second;
	}

	const ports_t ports();

private:
	std::map<std::string, std::string>	f_variables;
	bool					f_ports_defined;
	ports_t					f_ports;
};


configuration::configuration()
	//: f_variables -- auto-init
	: f_ports_defined(false)
	//, f_ports -- auto-init
{
	FILE * f = fopen("/etc/network/iplock.conf", "r");
	if(f == NULL)
	{
		// no configuration
		return;
	}

	char buf[256];
	int line = 0;
	while(fgets(buf, sizeof(buf) - 1, f) != NULL)
	{
		++line;
		buf[sizeof(buf) - 1] = '\0';
		char *s = buf;
		while(isspace(*s))
		{
			++s;
		}
		if(*s == '\0' || *s == '#')
		{
			// empty lines & comments
			continue;
		}
		const char *name = s;
		while(*s != '=' && *s != '\0')
		{
			if(isspace(*s))
			{
				*s = '\0';
				do
				{
					++s;
				}
				while(isspace(*s));
				break;
			}
			++s;
		}
		if(*s != '=')
		{
			fprintf(stderr, "iplock:error:configuration file variable name not followed by '=' (%s)\n", buf);
			exit(1);
		}
		*s++ = '\0';
		while(isspace(*s))
		{
			++s;
		}
		const char *value = s;
		while(*s != '\0')
		{
			++s;
		}
		while(s > value && isspace(s[-1]))
		{
			--s;
		}
		*s = '\0';
		if(s > value)
		{
			// remove quotes if any
			if((*value == '"'  && s[-1] == '"')
			|| (*value == '\'' && s[-1] == '\''))
			{
				s[-1] = '\0';
				++value;
			}
		}
		// save variable
		f_variables[name] = value;
	}

	fclose(f);
}


const configuration::ports_t configuration::ports()
{
	if(!f_ports_defined)
	{
		f_ports_defined = true;
		const std::string prts( operator [] ("ports") );
		const char *s = prts.c_str();
		while(*s != '\0')
		{
			while(*s == ',' || isspace(*s))
			{
				++s;
			}
			if(*s != '\0')
			{
				const char *p(s);
				while(*s != '\0' && *s != ',' && !isspace(*s))
				{
					++s;
				}
				f_ports.push_back(std::string(p, s - p));
			}
		}
	}
	return f_ports;
}



void usage()
{
	fprintf(stderr, "Usage: iplock [-opt] IP1 ...\n");
	fprintf(stderr, "  where -opt is one of:\n");
	fprintf(stderr, "    -h or --help     print out this help screen\n");
	fprintf(stderr, "    -b or --block    add a block (default)\n");
	fprintf(stderr, "    -r or --remove   remove the block\n");
	exit(1);
}


void block_ip(configuration& conf, const char *ip, bool quiet)
{
	const configuration::ports_t& ports(conf.ports());

	// repeat the block for each specified port
	for(configuration::ports_t::const_iterator p(ports.begin());
			p != ports.end(); ++p)
	{
		std::string cmd(conf["block"]);
		size_t port_position(cmd.find("[port]"));
		cmd.replace(port_position, 6, *p);
		size_t ip_position(cmd.find("[ip]"));
		cmd.replace(ip_position, 4, ip);
		if(quiet)
		{
			cmd += " 2>/dev/null";
		}
		if(system(cmd.c_str()) != 0)
		{
			perror("firewall command failed");
		}
	}
}


void unblock_ip(configuration& conf, const char *ip, bool quiet)
{
	const configuration::ports_t& ports = conf.ports();

	// repeat the unblock for each specified port
	for(configuration::ports_t::const_iterator p(ports.begin());
			p != ports.end(); ++p)
	{
		std::string cmd(conf["unblock"]);
		size_t port_position(cmd.find("[port]"));
		cmd.replace(port_position, 6, *p);
		size_t ip_position(cmd.find("[ip]"));
		cmd.replace(ip_position, 4, ip);
		if(quiet)
		{
			cmd += " 2>/dev/null";
		}
		if(system(cmd.c_str()) != 0)
		{
			perror("firewall command failed");
		}
	}
}


void verify_ip(const char *ip)
{
	int c(1);
	int n(-1);
	const char *s(ip);
	while(*s != '\0')
	{
		if(*s >= '0' && *s <= '9')
		{
			if(n == -1)
			{
				n = *s - '0';
			}
			else
			{
				n = n * 10 + *s - '0';
			}
		}
		else if(*s == '.')
		{
			if(n < 0 || n > 255)
			{
				fprintf(stderr, "iplock:error:IP numbers are limited to a value between 0 and 255. \"%s\" is invalid.\n", ip);
				exit(1);
			}
			// reset the number
			n = -1;
			++c;
		}
		else
		{
			fprintf(stderr, "iplock:error:IP addresses are currently limited to IPv4 syntax only (a.b.c.d) \"%s\" is invalid.\n", ip);
			exit(1);
		}
		++s;
	}
	if(c != 4 || n == -1)
	{
		fprintf(stderr, "iplock:error:IP addresses are currently limited to IPv4 syntax with exactly 4 numbers (a.b.c.d), %d found in \"%s\" is invalid.\n", c, ip);
		exit(1);
	}
}


int main(int argc, const char *argv[])
{
	configuration conf;

	bool block(true);
	bool quiet(false);
	for(int i = 1; i < argc; ++i)
	{
		if(strcmp(argv[i], "-h") == 0
		|| strcmp(argv[i], "--help") == 0)
		{
			usage();
			/*NOTREACHED*/
		}
		if(strcmp(argv[i], "--version") == 0)
		{
			std::cerr << IPLOCK_VERSION_STRING << std::endl;
			exit(0);
		}
		if(strcmp(argv[i], "-q") == 0
		|| strcmp(argv[i], "--quiet") == 0)
		{
			quiet = true;
		}
		else if(strcmp(argv[i], "-r") == 0
		|| strcmp(argv[i], "--remove") == 0)
		{
			block = false;
		}
		else if(strcmp(argv[i], "-b") == 0
		|| strcmp(argv[i], "--block") == 0)
		{
			block = true;
		}
		else if(argv[i][0] == '-')
		{
			std::cerr << "iplock:error:unknown command line flag \"" << argv[i] << "\"." << std::endl;
			exit(1);
		}
		else
		{
			verify_ip(argv[i]);
			if(block)
			{
				block_ip(conf, argv[i], quiet);
			}
			else
			{
				unblock_ip(conf, argv[i], quiet);
			}
		}
	}

	exit(0);
}

