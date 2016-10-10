//
// File:        src/iplock.cpp
// Object:      Allow users to easily add and remove IPs in an iptable
//              firewall; this is useful if you have a blacklist of IPs
//
// Copyright:   Copyright (c) 2007-2016 Made to Order Software Corp.
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


/** \file
 * \brief iplock tool.
 *
 * This implementation offers a way to easily and safely add and remove
 * IP address one wants to block temporarily.
 *
 * The tool makes use of the iptables tool to add and remove rules
 * to one specific table which is expected to be included in your
 * INPUT rules (with a -j \<table-name>).
 */


#include "iplock.h"
#include "version.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

#include <stdio.h>

#include "poison.h"




/** \brief List of configuration files.
 *
 * This variable is used as a list of configuration files. It is
 * empty here because the configuration file may include parameters
 * that are not otherwise defined as command line options.
 */
std::vector<std::string> const g_configuration_files; // Empty


/** \brief Command line options.
 *
 * This table includes all the options supported by iplock on the
 * command line.
 */
advgetopt::getopt::option const g_iplock_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "Usage: %p [-<opt>] [ip]",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "where -<opt> is one or more of:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        'b',
        0,
        "block",
        nullptr,
        "Block the speficied IP address. If already blocked, do nothing.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'n',
        0,
        "count",
        nullptr,
        "Return the number of times each IP address was blocked since the last counter reset. You may use the --reset along this command to atomically reset the counters as you retrieve them.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show usage and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'q',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "quiet",
        nullptr,
        "Prevent iptables from printing messages in stdour or stderr.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'r',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "reset",
        nullptr,
        "Use with the --count command to retrieve the counters and reset them atomically.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        's',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "scheme",
        nullptr,
        "Configuration file to define iptables commands. This is one name (no '/' or '.'). The default is \"http\".",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        't',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "total",
        nullptr,
        "Write the grand total only when --count is specified.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'u',
        0,
        "unblock",
        nullptr,
        "Unblock the specified IP address. If not already blocked, do nothing.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'v',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "verbose",
        nullptr,
        "Show commands being executed.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        "version",
        nullptr,
        "Show the version of iplock and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        "ip1 ip2 ip3 ... ipN",
        advgetopt::getopt::argument_mode_t::default_multiple_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};



/** \brief The list of files (one) to the iplock.conf configuration file.
 *
 * This vector includes the project name ("iplock") and the path
 * to the iplock configuration file.
 *
 * The project name is used so one can place another copy of the
 * iplock.conf file in a sub-directory named ".../iplock.d/..."
 *
 * Note that we do not give users a way to enter their own configuration
 * files. Those files can only be edited by root.
 */
std::vector<std::string> const g_iplock_configuration_files
{
    "@iplock@", // project name
    "/etc/iplock/iplock.conf"
};



/** \brief Scheme file options.
 *
 * This table includes all the variables supported by iplock in a
 * scheme file such as http.conf.
 */
advgetopt::getopt::option const g_iplock_configuration_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "chain",
        nullptr,
        "The name of the chain that iplock is expected to work with.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};








/** \brief Scheme file options.
 *
 * This table includes all the variables supported by iplock in a
 * scheme file such as http.conf.
 */
advgetopt::getopt::option const g_iplock_block_or_unblock_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "block",
        nullptr,
        "Block the speficied IP address. If already blocked, do nothing.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "check",
        nullptr,
        "Command to check whether a rule already exists or not.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "ports",
        nullptr,
        "Comma separated list of ports.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "unblock",
        nullptr,
        "Unblock the specified IP address. If not already blocked, do nothing.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};





/** \brief The configuration files for the --count command line option.
 *
 * This vector includes a set of parameters used to load the --count
 * options from a configuration file.
 */
std::vector<std::string> g_iplock_count_configuration_files
{
    "@iplock@",
    "/etc/iplock/count.conf"
};



/** \brief Scheme file options.
 *
 * This table includes all the variables supported by iplock in a
 * scheme file such as http.conf.
 */
advgetopt::getopt::option const g_iplock_count_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "acceptable_targets",
        nullptr,
        "The list of comma separated target names that will be counted.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "bytes_column",
        nullptr,
        "The column representing the number of bytes transferred.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "count",
        nullptr,
        "The command line to print out the counters from iptables.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "count_and_reset",
        nullptr,
        "The command line to print out and reset the counters from iptables.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "ignore_line_starting_with",
        nullptr,
        "Ignore any line starting with the specified value.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "ip_column",
        nullptr,
        "The column in which our IP is found (changes depending on whether you use an input or output IP--we are limited to the input a.k.a \"source\" IP address for now.).",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "lines_to_ignore",
        nullptr,
        "The number of lines to ignore at the start.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "packets_column",
        nullptr,
        "The column representing the number of packets received/sent.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "target_column",
        nullptr,
        "The column representing the number of packets received/sent.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};


/** \brief Free a FILE object.
 *
 * This deleter is used to make sure that FILE objects get freed
 * whenever the object holding it gets destroyed.
 *
 * \param[in] f  The FILE object to be freed.
 */
void file_deleter(FILE * f)
{
    fclose(f);
}


/** \brief Free a FILE object opened by popen().
 *
 * This deleter is used to make sure that FILE objects get freed
 * whenever the object holding it gets destroyed.
 *
 * \param[in] pipe  The FILE object to be freed.
 */
void pipe_deleter(FILE * pipe)
{
    pclose(pipe);
}









iplock::command::command(iplock * parent, char const * command_name, advgetopt::getopt::pointer_t opt)
    : f_iplock(parent)
    , f_opt(opt)
    , f_quiet(opt->is_defined("quiet"))
    , f_verbose(opt->is_defined("verbose"))
{
    // fake a pair of argc/argv which are empty
    //
    char const * argv[2]
    {
        command_name,
        nullptr
    };

    f_iplock_opt = std::make_shared<advgetopt::getopt>(
                1,
                const_cast<char **>(argv),
                g_iplock_configuration_options,
                g_iplock_configuration_files,
                "");

    if(!f_iplock_opt->is_defined("chain"))
    {
        std::cerr << "iplock:error: the \"chain\" parameter is required in \"iplock.conf\"." << std::endl;
        exit(1);
    }

    f_chain = f_iplock_opt->get_string("chain");
    if(f_chain.empty()
    || f_chain.size() > 30)
    {
        std::cerr << "iplock:error: the \"chain\" parameter cannot be more than 30 characters nor empty." << std::endl;
        exit(1);
    }

    std::for_each(
              f_chain.begin()
            , f_chain.end()
            , [&](auto const & c)
            {
                if((c < 'a' || c > 'z')
                && (c < 'A' || c > 'Z')
                && (c < '0' || c > '9')
                && c != '_')
                {
                    std::cerr << "error:iplock: invalid \"chain=...\" option \"" << f_chain << "\", only [a-zA-Z0-9_]+ are supported." << std::endl;
                    exit(1);
                }
            });
}


iplock::command::~command()
{
}


void iplock::command::verify_ip(std::string const & ip)
{
    // TODO: add support for IPv6 (we probably want our snap_addr
    //       class in a contrib "net" library first...
    //
    int c(1);
    int n(-1);
    char const * s(ip.c_str());
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
            if(n == -1)
            {
                std::cerr << "iplock:error: IPv4 addresses are currently limited to IPv4 syntax only (a.b.c.d) \"" << ip << "\" is invalid." << std::endl;
            }
            if(n < 0 || n > 255)
            {
                std::cerr << "iplock:error: IPv4 numbers are limited to a value between 0 and 255, \"" << ip << "\" is invalid." << std::endl;
                exit(1);
            }
            // reset the number
            n = -1;
            ++c;
        }
        else
        {
            std::cerr << "iplock:error: IPv4 addresses are currently limited to IPv4 syntax only (a.b.c.d) \"" << ip << "\" is invalid." << std::endl;
            exit(1);
        }
        ++s;
    }
    if(c != 4 || n == -1)
    {
        std::cerr << "iplock:error: IPv4 addresses are currently limited to IPv4 syntax with exactly 4 numbers (a.b.c.d), " << c << " found in \"" << ip << "\" is invalid." << std::endl;
        exit(1);
    }
}




iplock::block_or_unblock::block_or_unblock(iplock * parent, char const * command_name, advgetopt::getopt::pointer_t opt)
    : command(parent, command_name, opt)
    //, f_scheme("http") -- auto-init
{
    if(opt->is_defined("reset"))
    {
        std::cerr << "error:iplock: --reset is not supported by --block or --unblock." << std::endl;
        exit(1);
    }
    if(opt->is_defined("total"))
    {
        std::cerr << "error:iplock: --total is not supported by --block or --unblock." << std::endl;
        exit(1);
    }

    // the filename to define the ports, block, unblock commands
    //
    if(opt->is_defined("scheme"))
    {
        f_scheme = opt->get_string("scheme");

        // the scheme cannot be an empty string
        //
        if(f_scheme.empty())
        {
            std::cerr << "error:iplock: the name specified with --scheme cannot be empty." << std::endl;
            exit(1);
        }

        // make sure we accept that string as the name of a scheme
        //
        std::for_each(
                  f_scheme.begin()
                , f_scheme.end()
                , [&](auto const & c)
                {
                    if((c < 'a' || c > 'z')
                    && (c < 'A' || c > 'Z')
                    && (c < '0' || c > '9')
                    && c != '_')
                    {
                        std::cerr << "error:iplock: invalid --scheme option \"" << f_scheme << "\", only [a-zA-Z0-9_]+ are supported." << std::endl;
                        exit(1);
                    }
                });
    }

    // make sure there is at least one IP address
    //
    if(opt->size("--") == 0)
    {
        std::cerr << "error:iplock: --block and --unblock require at least one IP address." << std::endl;
        exit(1);
    }

    // read the scheme configuration file
    //
    // since the name of the file can change, we use a fully dynamically
    // allocated vector
    //
    std::vector<std::string> scheme_configuration_files
    {
        "@iplock@",
        "/etc/iplock/" + f_scheme + ".conf"
    };

    // fake a pair of argc/argv which are empty
    //
    char const * argv[2]
    {
        "iplock_block_or_unblock",
        nullptr
    };
    f_scheme_opt = std::make_shared<advgetopt::getopt>(
                1,
                const_cast<char **>(argv),
                g_iplock_block_or_unblock_options,
                scheme_configuration_files,
                "");

    // get the list of ports immediately
    //
    {
        std::string const ports(f_scheme_opt->get_string("ports"));
        char const * p(ports.c_str());
        while(*p != '\0')
        {
            if(std::isspace(*p) || *p == ',')
            {
                ++p;
                continue;
            }
            if(*p < '0' || *p > '9')
            {
                std::cerr << "iplock:error: invalid port specification in \"" << ports << "\", we only expect numbers separated by commas." << std::endl;
                exit(1);
            }

            // got a port
            //
            int port_number(*p - '0');
            for(++p; *p != '\0' && *p >= '0' && *p <= '9'; ++p)
            {
                port_number = port_number * 10 + *p - '0';
                if(port_number > 0xFFFF)
                {
                    std::cerr << "iplock:error: one of the port numbers in \"" << ports << "\" is too large." << std::endl;
                    exit(1);
                }
            }
            if(port_number == 0)
            {
                std::cerr << "iplock:error: you cannot (un)block port number 0." << std::endl;
                exit(1);
            }
            f_ports.push_back(static_cast<uint16_t>(port_number));
        }

        // TBD: we could look into supporting "all ports" if none are specified?
        //
        if(f_ports.empty())
        {
            std::cerr << "iplock:error: you must specify at least one port." << std::endl;
            exit(1);
        }
    }
}


iplock::block_or_unblock::~block_or_unblock()
{
}


void iplock::block_or_unblock::handle_ips(std::string const & cmdline, int run_on_result)
{
    // position where each rule gets insert (if the command is --block)
    //
    int num(1);

    std::string const check_cmdline(f_scheme_opt->get_string("check"));

    int const max(f_opt->size("--"));
    for(int idx(0); idx < max; ++idx)
    {
        std::string const ip(f_opt->get_string("--", idx));

        // TBD: should we verify all the IPs before starting to add/remove
        //      any one of them to the firewall? (i.e. be a little more
        //      atomic kind of a thing?)
        //
        verify_ip(ip);

        for(auto const port : f_ports)
        {
            // replace the variables in the command line
            //
            std::string check_cmd(boost::replace_all_copy(check_cmdline, "[chain]", f_chain));
            boost::replace_all(check_cmd, "[port]", std::to_string(static_cast<unsigned int>(port)));
            boost::replace_all(check_cmd, "[ip]", ip);
            boost::replace_all(check_cmd, "[num]", std::to_string(num));

            // although the -C does nothing, it will print a message
            // in stderr if the rule does not exist
            //
            check_cmd += " 1>/dev/null 2>&1";

            if(f_verbose)
            {
                std::cout << check_cmd << std::endl;
            }
            int const rc(system(check_cmd.c_str()));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            if(!WIFEXITED(rc))
            {
                if(!f_verbose)
                {
                    // if not verbose, make sure to show the command so the
                    // user knows what failed
                    //
                    int const save_errno(errno);
                    std::cerr << check_cmd << std::endl;
                    errno = save_errno;
                }
                perror("iplock: netfilter command failed");

                // TBD: we cannot continue without a valid answer on this
                //      one so we just try further...
                //
                continue;
            }
            int const exit_code(WEXITSTATUS(rc));
#pragma GCC diagnostic pop

            if(exit_code == run_on_result)
            {
                // replace the variables in the command line
                //
                std::string cmd(boost::replace_all_copy(cmdline, "[chain]", f_chain));
                boost::replace_all(cmd, "[port]", std::to_string(static_cast<unsigned int>(port)));
                boost::replace_all(cmd, "[ip]", ip);
                boost::replace_all(cmd, "[num]", std::to_string(num));

                // if user specified --quiet ignore all output
                //
                if(f_quiet)
                {
                    cmd += " 1>/dev/null 2>&1";
                }

                // if user specified --verbose show the command being run
                //
                if(f_verbose)
                {
                    std::cout << cmd << std::endl;
                }

                // run the command now
                //
                int const r(system(cmd.c_str()));
                if(r != 0)
                {
                    if(!f_verbose)
                    {
                        // if not verbose, make sure to show the command so the
                        // user knows what failed
                        //
                        int const save_errno(errno);
                        std::cerr << cmd << std::endl;
                        errno = save_errno;
                    }
                    perror("iplock: netfilter command failed");
                }

                // [num] is used by the -I command line option
                //
                // i.e. we insert at the beginning, but in the same order
                //      that the user defined his ports
                //
                ++num;
            }
        }
    }
}



/** \class iplock::block
 * \brief Block the specified IP addresses.
 *
 * This class goes through the list of IP addresses specified on the
 * command line and add them to the chain as defined in ipconfig.conf.
 *
 * By default, the scheme is set to "http". It can be changed with
 * the --scheme command line option.
 */

iplock::block::block(iplock * parent, advgetopt::getopt::pointer_t opt)
    : block_or_unblock(parent, "iplock --block", opt)
{
}


iplock::block::~block()
{
}


void iplock::block::run()
{
    handle_ips(f_scheme_opt->get_string("block"), 1);
}




/** \class iplock::unblock
 * \brief Unblock the specified IP addresses.
 *
 * This class goes through the list of IP addresses specified on the
 * command line and remove them from the chain as defined in ipconfig.conf.
 */

iplock::unblock::unblock(iplock * parent, advgetopt::getopt::pointer_t opt)
    : block_or_unblock(parent, "iplock --unblock", opt)
{
}


iplock::unblock::~unblock()
{
}


void iplock::unblock::run()
{
    handle_ips(f_scheme_opt->get_string("unblock"), 0);
}






/** \class iplock::count
 * \brief Generate a count of all the entries by IP address.
 *
 * This class goes through the list of rules we added so far in the
 * named chain and prints out the results to stdout.
 *
 * If multiple ports get blocked, then the total for all those ports
 * is reported.
 */


iplock::count::count(iplock * parent, advgetopt::getopt::pointer_t opt)
    : command(parent, "iplock --count", opt)
    , f_reset(opt->is_defined("reset"))
{
    if(opt->is_defined("scheme"))
    {
        std::cerr << "iplock:error: --scheme is not supported by --count." << std::endl;
        exit(1);
    }

    // read the count configuration file
    //
    // fake a pair of argc/argv which are empty
    //
    {
        char const * argv[2]
        {
            "iplock_count",
            nullptr
        };
        f_count_opt = std::make_shared<advgetopt::getopt>(
                    1,
                    const_cast<char **>(argv),
                    g_iplock_count_options,
                    g_iplock_count_configuration_files,
                    "");
    }

    // parse the list of targets immediately
    //
    {
        std::string const targets(f_count_opt->get_string("acceptable_targets"));
        char const * t(targets.c_str());
        while(*t != '\0')
        {
            if(std::isspace(*t) || *t == ',')
            {
                ++t;
                continue;
            }

            // got a target name
            //
            std::string target;
            for(; *t != '\0' && *t != ',' && !isspace(*t); ++t)
            {
                // verify that it is an acceptable character for a target name
                //
                if((*t < 'a' || *t > 'z')
                && (*t < 'A' || *t > 'Z')
                && (*t < '0' || *t > '0')
                && *t != '_')
                {
                    std::cerr << "iplock:error: a target name only supports [a-zA-Z0-9_]+ characters." << std::endl;
                    exit(1);
                }
                target += *t;
            }
            if(target.empty()
            || target.size() > 30)
            {
                std::cerr << "iplock:error: a target name cannot be empty or larger than 30 characters." << std::endl;
                exit(1);
            }
            f_targets.push_back(target);
        }
    }
}


iplock::count::~count()
{
}





void iplock::count::run()
{
    // the iptables -L command line option does not give you any formatting
    // or filtering power so we instead define many parameters in the
    // count.conf configuration file which we use here to parse the data
    // out
    //
    struct data_t
    {
        typedef std::map<std::string, data_t>   ip_map_t;

                        data_t(int64_t packets = 0, int64_t bytes = 0)
                            : f_packets(packets)
                            , f_bytes(bytes)
                        {
                        }

        data_t &        operator += (data_t const & rhs)
                        {
                            f_packets += rhs.f_packets;
                            f_bytes += rhs.f_bytes;

                            return *this;
                        }

        int64_t         f_packets = 0;
        int64_t         f_bytes = 0;
    };

    // run the command and retrieve its output
    //
    std::string cmd;
    if(f_opt->is_defined("reset"))
    {
        cmd = f_count_opt->get_string("count_and_reset");
    }
    else
    {
        cmd = f_count_opt->get_string("count");
    }
    boost::replace_all(cmd, "[chain]", f_chain);

    if(f_verbose)
    {
        std::cerr << "iplock:info: command to read counters: \"" << cmd << "\"." << std::endl;
    }

    std::shared_ptr<FILE> f(popen(cmd.c_str(), "r"), pipe_deleter);

    // we have a first very simple loop that allows us to read
    // lines to be ignored by not saving them anywhere
    //
    for(long lines_to_ignore(f_count_opt->get_long("lines_to_ignore")); lines_to_ignore > 0; --lines_to_ignore)
    {
        for(;;)
        {
            int const c(fgetc(f.get()));
            if(c == EOF)
            {
                std::cerr << "iplock:error: unexpected EOF while reading a line of output." << std::endl;
                exit(1);
            }
            if(c == '\n' || c == '\r')
            {
                break;
            }
        }
    }

    // the column we are currently interested in
    //
    // WARNING: in the configuration file, those column numbers are 1 based
    //          just like the rule number in iptables...
    //
    long const packets_column(f_count_opt->get_long("packets_column") - 1);
    long const bytes_column(f_count_opt->get_long("bytes_column") - 1);
    long const target_column(f_count_opt->get_long("target_column") - 1);
    long const ip_column(f_count_opt->get_long("ip_column") - 1);

    // make sure it is not completely out of range
    //
    if(packets_column < 0 || packets_column >= 100
    || bytes_column < 0   || bytes_column >= 100
    || target_column < 0  || target_column >= 100
    || ip_column < 0      || ip_column >= 100)
    {
        std::cerr << "iplock:error: unexpectendly small or large column number (number is expected to be between 1 and 99)." << std::endl;
        exit(1);
    }

    // make sure the user is not trying to get different values from
    // the exact same column (that is a configuration bug!)
    //
    if(packets_column == bytes_column
    || packets_column == target_column
    || packets_column == ip_column
    || bytes_column == target_column
    || bytes_column == ip_column
    || target_column == ip_column)
    {
        std::cerr << "iplock:error: all column numbers defined in count.conf must be different." << std::endl;
        exit(1);
    }

    // compute the minimum size that the `columns` vector must be to
    // be considered valid
    //
    size_t const min_column_count(std::max(packets_column, std::max(bytes_column, std::max(target_column, ip_column))) + 1);

    // get the starting column to be ignored (i.e. the -Z option adds
    // a line at the bottom which says "Zeroing chain `<chain-name>`"
    //
    std::string const ignore_line_starting_with(f_count_opt->get_string("ignore_line_starting_with"));

    // number of IP addresses allowed in the output or 0 for all
    //
    int const ip_max(f_opt->size("--"));

    // a map indexed by IP addresses with all the totals
    //
    data_t::ip_map_t totals;

    bool const merge_totals(f_opt->is_defined("total"));

    for(;;)
    {
        // read one line of output, immediately break it up in columns
        //
        std::vector<std::string> columns;
        std::string column;
        for(;;)
        {
            int const c(fgetc(f.get()));
            if(c == EOF)
            {
                if(!column.empty())
                {
                    std::cerr << "iplock:error: unexpected EOF while reading a line of output." << std::endl;
                    exit(1);
                }
                break;
            }
            if(c == '\n' || c == '\r')
            {
                break;
            }
            if(c == ' ')
            {
                // ignore empty columns (this happens because there are
                // many spaces between each column)
                //
                if(!column.empty()
                && (!columns.empty() || ignore_line_starting_with != column))
                {
                    columns.push_back(column);
                    column.clear();
                }
                continue;
            }
            column += c;

            // prevent columns that are too wide
            //
            if(column.length() > 256)
            {
                std::cerr << "iplock:error: unexpected long column, stopping process." << std::endl;
                exit(1);
            }
        }

        // are we done? (found EOF after the last line, thus no columns)
        //
        if(columns.empty())
        {
            break;
        }

        // make sure we have enough columns
        //
        if(columns.size() < min_column_count)
        {
            std::cerr << "iplock:error: not enough columns to satisfy the configuration column numbers." << std::endl;
            exit(1);
        }

        // filter by targets?
        //
        if(!f_targets.empty()
        && std::find(f_targets.begin(), f_targets.end(), columns[target_column]) == f_targets.end())
        {
            // target filtering missed
            //
            continue;
        }

        // get the source IP
        // make sure to remove the mask if present
        //
        std::string source_ip(columns[ip_column]);
        std::string::size_type pos(source_ip.find('/'));
        if(pos != std::string::npos)
        {
            source_ip = source_ip.substr(0, pos);
        }

        // filter by IP?
        //
        if(ip_max > 0)
        {
            bool found(false);
            for(int idx(0); idx < ip_max; ++idx)
            {
                std::string const ip(f_opt->get_string("--", idx));
                verify_ip(ip); // TODO: this should be done in a loop ahead of time instead of each time we loop here!

                if(source_ip == ip)
                {
                    found = true;
                    break;
                }
            }
            if(!found)
            {
                // ip filter missed
                //
                continue;
            }
        }

        // we got a valid set of columns, get the counters
        //
        int64_t const packets(boost::lexical_cast<int64_t>(columns[packets_column]));
        int64_t const bytes(boost::lexical_cast<int64_t>(columns[bytes_column]));

        // add this line's counters to the existing totals
        //
        data_t const line_counters(packets, bytes);
        if(merge_totals)
        {
            // user wants one grand total, ignore source_ip
            //
            totals["0.0.0.0"] += line_counters;
        }
        else
        {
            totals[source_ip] += line_counters;
        }
    }

    // done with the pipe
    //
    f.reset();

    // got the totals now!
    //
    for(auto const & t : totals)
    {
        std::cout << t.first << " " << t.second.f_packets << " " << t.second.f_bytes << std::endl;
    }
}



/** \brief Initialize the iplock object.
 *
 * This function parses the command line and  determines the command
 * that the end user selected (i.e. --block, --unblock, or --count.)
 *
 * If the user specified --help or --version, then this function
 * prints the help screen or version of iplock and exits the process
 * immediately.
 *
 * If no command was specified on the command line, then an error
 * is written to stderr and the process exits immediately.
 *
 * \param[in] argc  The number of arguments in argv.
 * \param[in] argv  The argument strings.
 */
iplock::iplock(int argc, char * argv[])
{
    advgetopt::getopt::pointer_t opt(std::make_shared<advgetopt::getopt>(argc, argv, g_iplock_options, g_configuration_files, "IPLOCK_OPTIONS"));

    // note: --help and --version are also commands (see below)
    //       but they have priority and do not generate an error
    //       if used along another command...

    if(opt->is_defined("help"))
    {
        opt->usage(advgetopt::getopt::status_t::no_error, "iplock v" IPLOCK_VERSION_STRING " -- to manage iptables automatically");
        exit(1);
    }

    if(opt->is_defined("version"))
    {
        std::cout << IPLOCK_VERSION_STRING << std::endl;
        exit(0);
    }

    // define the command
    //
    // since the user may specify any number of commands, we use
    // the set_command() function to make sure that only one
    // gets set...
    //
    if(opt->is_defined("block"))
    {
        set_command(std::make_shared<block>(this, opt));
    }
    if(opt->is_defined("unblock"))
    {
        set_command(std::make_shared<unblock>(this, opt));
    }
    if(opt->is_defined("count"))
    {
        set_command(std::make_shared<count>(this, opt));
    }

    // no command specified?
    //
    if(f_command == nullptr)
    {
        std::cerr << "iplock:error: you must specify one of: --block, --unblock, or --count." << std::endl;
        exit(1);
    }
}


/** \brief Save the command pointer in f_command.
 *
 * This function saves the specified \p c command pointer to the f_command
 * parameter.
 *
 * It is done that way so we can very easily detect whether more than one
 * command was specified on the command line.
 *
 * \param[in] c  The pointer to the command line to save in iplock.
 */
void iplock::set_command(command::pointer_t c)
{
    if(f_command != nullptr)
    {
        std::cerr << "iplock:error: you can only specify one command at a time, one of: --block, --unblock, or --count." << std::endl;
        exit(1);
    }
    f_command = c;
}


/** \brief Before running a command, make sure we are root.
 *
 * This function gets called by the run_command() function.
 *
 * The function exits the process with an error if becoming root is not
 * possible. This can happen if (1) the process is run by systemd and
 * systemd prevents such, (2) the binary is not marked with the 's'
 * bit.
 */
void iplock::make_root()
{
    if(setuid(0) != 0)
    {
        perror("iplock:error: setuid(0)");
        exit(1);
    }
    if(setgid(0) != 0)
    {
        perror("iplock:error: setgid(0)");
        exit(1);
    }
}


/** \brief Run the selected command.
 *
 * The constructor parses the command line options and from that
 * deterimes which command the user selected. This function runs
 * that command by calling its run() function.
 *
 * This function first makes sure the user is running as root.
 * This may change in the future if some of the commands may
 * otherwise be run as a regular user.
 */
void iplock::run_command()
{
    // all iptables commands require the user to be root.
    //
    make_root();

    f_command->run();
}



// vim: ts=4 sw=4 et
