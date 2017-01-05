/* as2js.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

/*

Copyright (c) 2005-2017 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include    "license.h"

// need to change to compiler.h once it compiles
#include    "as2js/parser.h"
#include    "as2js/as2js.h"

#include    <advgetopt/advgetopt.h>


/** \file
 * \brief This file is the actual as2js compiler.
 *
 * The project includes a library which does 99% of the work. This is
 * the implementation of the as2js command line tool that handles
 * command line options and initializes an Option object with those
 * before starting compiling various .js files.
 */




/** \brief Private implementations of the as2js compiler, the actual tool.
 *
 * This namespace is used to hide all the tool private functions to
 * avoid any conflicts.
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
     * This table includes all the options supported by the compiler.
     */
    advgetopt::getopt::option const g_snapserver_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: %p [-<opt>] <filename>.as ...",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Where -<opt> is one or more of:",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "licence",
            nullptr,
            nullptr, // hide from help output
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "license",
            nullptr,
            "Print out the license of this command line tool.",
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
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "Show version and exit.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "filename",
            nullptr,
            nullptr, // hidden argument in --help screen
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
}


class as2js_compiler
{
public:
    typedef std::shared_ptr<as2js_compiler>         pointer_t;
    typedef std::shared_ptr<advgetopt::getopt>      getopt_ptr_t;

    as2js_compiler(int argc, char *argv[]);

private:
    getopt_ptr_t        f_opt;
};


as2js_compiler::as2js_compiler(int argc, char *argv[])
{
    // The library takes care of these possibilities with the .rc file
    //if(g_configuration_files.empty())
    //{
    //    g_configuration_files.push_back("~/.config/as2js/as2js.rc");
    //    g_configuration_files.push_back("/etc/as2js/as2js.rc");
    //}
    f_opt.reset(
        new advgetopt::getopt( argc, argv, g_snapserver_options, g_configuration_files, "AS2JS_OPTIONS" )
    );

    if(f_opt->is_defined("help"))
    {
        f_opt->usage(advgetopt::getopt::status_t::no_error, "Usage: as2js [--opt] <source>.as");
        /*NOTREACHED*/
    }

    if(f_opt->is_defined("license")      // English
    || f_opt->is_defined("licence"))     // French
    {
        as2js_tools::license::license();
        exit(1);
    }

    if(f_opt->is_defined("version"))
    {
        std::cout << f_opt->get_program_name() << " v" << AS2JS_VERSION << std::endl
                << "libas2js v" << as2js::as2js_library_version() << std::endl;
        exit(1);
    }
}


int main(int argc, char *argv[])
{
    try
    {
        as2js_compiler::pointer_t c(new as2js_compiler(argc, argv));
    }
    catch(std::exception const& e)
    {
        std::cerr << "as2js: exception: " << e.what() << std::endl;
        exit(1);
    }

    return 0;
}


// vim: ts=4 sw=4 et
