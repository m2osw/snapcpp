/*
 * Text:
 *      cxpath.cpp
 *
 * Description:
 *      Compile an XPath to binary byte code.
 *
 * License:
 *      Copyright (c) 2013 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <advgetopt.h>
#include <not_reached.h>
#include <qdomxpath.h>



const advgetopt::getopt::option cxpath_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "Usage: cxpath --<command> [--<opt>] ['<xpath>'] [<filename>.xml] ...",
        advgetopt::getopt::help_argument
    },
    // COMMANDS
    {
        '\0',
        0,
        NULL,
        NULL,
        "commands:",
        advgetopt::getopt::help_argument
    },
    {
        'c',
        0,
        "compile",
        NULL,
        "compile the specified XPath and save it to a .xpath file and optionally print out the compiled code",
        advgetopt::getopt::no_argument
    },
    {
        'd',
        0,
        "disassemble",
        NULL,
        "disassemble the specified .xpath file (if used with the -c, disassemble as we compile)",
        advgetopt::getopt::no_argument
    },
    {
        'h',
        0,
        "help",
        NULL,
        "display this help screen",
        advgetopt::getopt::no_argument
    },
    {
        'x',
        0,
        "execute",
        NULL,
        "execute an xpath (.xpath file or parsed on the fly XPath) against one or more .xml files",
        advgetopt::getopt::no_argument
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
        'o',
        0,
        "output",
        NULL,
        "name of the output file (the .xpath filename)",
        advgetopt::getopt::required_argument
    },
    {
        'p',
        0,
        "xpath",
        NULL,
        "an XPath",
        advgetopt::getopt::required_argument
    },
    {
        'v',
        0,
        "verbose",
        NULL,
        "make the process verbose",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "filename",
        NULL,
        NULL, // hidden argument in --help screen
        advgetopt::getopt::default_multiple_argument
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



advgetopt::getopt * g_opt;
bool                g_verbose;



void cxpath_compile()
{
    if(!g_opt->is_defined("xpath"))
    {
        fprintf(stderr, "error: --xpath not defined, nothing to copmile.\n");
        exit(1);
    }

    std::string xpath(g_opt->get_string("xpath"));
    if(g_verbose)
    {
        printf("compiling \"%s\" ... \n", xpath.c_str());
    }

    bool disassemble(g_opt->is_defined("disassemble"));

    QDomXPath dom_xpath;
    dom_xpath.setXPath(xpath.c_str(), disassemble);
}



int main(int argc, char *argv[])
{
    std::vector<std::string> empty_list;
    g_opt = new advgetopt::getopt(argc, argv, cxpath_options, empty_list, NULL);
    if(g_opt->is_defined("help"))
    {
        g_opt->usage(advgetopt::getopt::no_error, "Usage: cxpath --<command> [--<opt>] ['<xpath>'] [<filename>.xml] ...");
        snap::NOTREACHED();
    }

    if(g_opt->is_defined("compile"))
    {
        cxpath_compile();
    }

    return 0;
}

// vim: ts=4 sw=4 et
