// Snap! Websites -- Test Suite main()
// Copyright (C) 2015-2016  Made to Order Software Corp.
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

/** \file
 * \brief Snap! Websites main unit test.
 *
 * This file includes code common to all our tests. At this time it is
 * mainly the main() function that checks the command line arguments.
 *
 * This test suite uses catch.hpp, for details see:
 *
 *   https://github.com/philsquared/Catch/blob/master/docs/tutorial.md
 */

// Tell catch we want it to add the runner code in this file.
#define CATCH_CONFIG_RUNNER

#include "catch_tests.h"

#include "qstring_stream.h"

#include <cstring>

#include <stdlib.h>

namespace snap_test
{

char * g_progname = nullptr;
std::string g_progdir;
bool g_verbose = false;

} // snap_test namespace

int main(int argc, char * argv[])
{
    // define program name
    snap_test::g_progname = argv[0];
    char * e1(strrchr(snap_test::g_progname, '/' ));
    char * e2(strrchr(snap_test::g_progname, '\\'));
    if(e1 && e1 > e2)
    {
        snap_test::g_progname = e1 + 1; // LCOV_EXCL_LINE
        snap_test::g_progdir = std::string(argv[0], e1);
    }
    else if(e2 && e2 > e1)
    {
        snap_test::g_progname = e2 + 1; // LCOV_EXCL_LINE
        snap_test::g_progdir = std::string(argv[0], e2);
    }
    if(snap_test::g_progdir.empty())
    {
        // no directory in argv[0], use "." as the default
        snap_test::g_progdir = ".";
    }

    unsigned int seed(static_cast<unsigned int>(time(nullptr)));
    bool help(false);
    for(int i(1); i < argc;)
    {
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            help = true; // LCOV_EXCL_LINE
            ++i;
        }
        else if(strcmp(argv[i], "--seed") == 0)
        {
            if(i + 1 >= argc) // LCOV_EXCL_LINE
            {
                std::cerr << "error: --seed need to be followed by the actual seed." << std::endl; // LCOV_EXCL_LINE
                exit(1); // LCOV_EXCL_LINE
            }
            seed = atoll(argv[i + 1]); // LCOV_EXCL_LINE
            // remove the --seed and <value>
            argc -= 2; // LCOV_EXCL_LINE
            for(int j(i); j < argc; ++j) // LCOV_EXCL_LINE
            {
                argv[j] = argv[j + 2]; // LCOV_EXCL_LINE
            }
        }
        else if(strcmp(argv[i], "--verbose") == 0)
        {
            snap_test::g_verbose = true;
            // remove the --verbose
            argc -= 1; // LCOV_EXCL_LINE
            for(int j(i); j < argc; ++j) // LCOV_EXCL_LINE
            {
                argv[j] = argv[j + 1]; // LCOV_EXCL_LINE
            }
        }
        else if(strcmp(argv[i], "--version") == 0)
        {
            std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
            exit(0);
        }
        else
        {
            // no error here, catch.hpp may generate an error, though
            ++i;
        }
    }
    srand(seed);
    std::cout << snap_test::g_progname << "[" << getpid() << "]" << ": version " << SNAPWEBSITES_VERSION_STRING << ", seed is " << seed << std::endl;

    if(help)
    {
        std::cout << std::endl // LCOV_EXCL_LINE
                  << "WARNING: at this point we hack the main() to add the following options:" << std::endl // LCOV_EXCL_LINE
                  << "  --seed <seed>             to force the seed at the start of the process to a specific value (i.e. to reproduce the exact same test over and over again)" << std::endl // LCOV_EXCL_LINE
                  << "  --verbose                 request for the errors to always be printed in std::cerr" << std::endl // LCOV_EXCL_LINE
                  << "  --version                 print out the version of this test and exit with 0" << std::endl // LCOV_EXCL_LINE
                  << std::endl; // LCOV_EXCL_LINE
    }

    return Catch::Session().run(argc, argv);
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
