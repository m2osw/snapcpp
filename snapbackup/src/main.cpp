/*
 * Text:
 *      main.cpp
 *
 * Description:
 *      Reads and describes a Snap database. This ease checking out the
 *      current content of the database as the cassandra-cli tends to
 *      show everything in hexadecimal number which is quite unpractical.
 *      Now we do it that way for runtime speed which is much more important
 *      than readability by humans, but we still want to see the data in an
 *      easy practical way which this tool offers.
 *
 *      This contains the main() function.
 *
 * License:
 *      Copyright (c) 2012-2016 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "snapbackup.h"

#include <exception>

namespace
{
const std::vector<std::string> g_configuration_files; // Empty

const advgetopt::getopt::option g_snapbackup_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "Usage: %p [-<opt>] [table [row]]",
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
        '?',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "show this help output",
        advgetopt::getopt::no_argument
    },
    {
        'n',
        0,
        "context-name",
        "snap_websites",
        "name of the context (or keyspace) to dump/restore (defaults to 'snap_websites')",
        advgetopt::getopt::optional_argument
    },
    {
        'd',
        0,
        "dump-context",
        nullptr,
        "dump the snapwebsites context to SQLite database",
        advgetopt::getopt::required_argument
    },
    {
        'T',
        0,
        "tables",
        nullptr,
        "specify the list of tables to dump to SQLite database, or restore from SQLite to Cassandra",
        advgetopt::getopt::required_multiple_argument
    },
    {
        'r',
        0,
        "restore-context",
        nullptr,
        "restore the snapwebsites context from SQLite database (requires confirmation)",
        advgetopt::getopt::optional_argument
    },
    {
        '\0',
        0,
        "drop-context-first",
        nullptr,
        "before restoring, drop the snap_websites keyspace first",
        advgetopt::getopt::optional_argument
    },
    {
        'c',
        0,
        "count",
        nullptr,
        "specify the number of rows to display",
        advgetopt::getopt::optional_argument
    },
    {
        '\0',
        0,
        "yes-i-know-what-im-doing",
        nullptr,
        "Force the dropping of context and overwriting of database, without warning and stdin prompt. Only use this if you know what you're doing!",
        advgetopt::getopt::no_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "host",
        "localhost",
        "host IP address or name (defaults to localhost)",
        advgetopt::getopt::optional_argument
    },
    {
        'p',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "port",
        9042,
        "port on the host to connect to (defaults to 9042)",
        advgetopt::getopt::optional_argument
    },
    {
        'V',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "show the version of the snapdb executable",
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

bool confirm_drop_check()
{
    std::cout << "WARNING! This command is about to overwrite the Snap context on the " << std::endl
              << "         database server and is IRREVERSIBLE!" << std::endl
              << std::endl
              << "Make sure you know what you are doing and have appropriate backups" << std::endl
              << "before proceeding!" << std::endl
              << std::endl
              << "Are you really sure you want to do this?" << std::endl
              << "(type in \"Yes I know what I'm doing!\" and press ENTER): "
                 ;
    std::string input;
    std::getline( std::cin, input );
    bool const confirm( (input == "Yes I know what I'm doing!") );
    if( !confirm )
    {
        std::cerr << "warning: Not overwriting database, so exiting." << std::endl;
    }
    return confirm;
}

}
//namespace

int main(int argc, char *argv[])
{
    int retval = 0;

    try
    {
        getopt_ptr_t opt( new advgetopt::getopt( argc, argv, g_snapbackup_options, g_configuration_files, nullptr );

        snapbackup  s(opt);
        if( opt->is_defined("dump-context") )
        {
            s.dumpContext();
        }
        else if( opt->is_defined("restore-context") )
        {
            if( opt->is_defined("yes-i-know-what-im-doing")
                || confirm_drop_check() )
            {
                s.restoreContext();
            }
        }
        else if( opt->is_defined("help") )
        {
            opt->usage( advgetopt::getopt::error, "snapbackup" );
        }
        else
        {
            throw std::runtime_error("You must specify either --dump-context or --restore-context!");
            retval = 1;
        }
    }
    catch(std::exception const& e)
    {
        std::cerr << "snapbackup: exception: " << e.what() << std::endl;
        retval = 1;
    }

    return retval;
}

// vim: ts=4 sw=4 et
