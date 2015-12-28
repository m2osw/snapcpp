// Snap Websites Server -- snap websites build server service support
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

// This is to have a build server we can start from a website. It is safe
// as it just tries to start the build.sh and does not offer any specific
// feature outside of that.

#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

void run(char const *cmd)
{
    int const r(system(cmd));
    if(r == -1)
    {
        std::cerr << "error: system(\"" << cmd << "\") failed.\n";
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    bool start(false);
    if(argc == 2)
    {
        if(strcmp(argv[1], "start") == 0)
        {
            start = true;
        }
        else if(strcmp(argv[1], "stop") == 0)
        {
            start = false;
        }
        else
        {
            std::cerr << "error: invalid command line option(s), expected 'start' or 'stop'." << std::endl;
            exit(1);
        }
    }
    else
    {
        std::cerr << "error: command line option missing expected 'start' or 'stop'." << std::endl;
        exit(1);
    }

    // become root
    if(setgid(0) == -1)
    {
        // if we cannot become group "root"
        std::cerr << "error: cannot become the \"root\" group on this computer.\n";
        exit(0);
    }
    if(setuid(0) == -1)
    {
        // if we cannot become user "root"
        std::cerr << "error: cannot become the \"root\" user on this computer.\n";
        exit(0);
    }

    // start or stop whatever services that are in the way of the build
    // system; at this time, we want to get rid of fisheye and jira
    // while running the build
    if(start)
    {
        run("service fisheye start");
        run("service jira start");
    }
    else
    {
        run("service fisheye stop");
        run("service jira stop");
    }

    return 0;
}

// vim: ts=4 sw=4 et
