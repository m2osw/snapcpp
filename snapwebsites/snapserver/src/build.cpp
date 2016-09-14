// Snap Websites Server -- snap websites CGI function for build server
// Copyright (C) 2011-2016  Made to Order Software Corp.
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

#include <snapwebsites/not_used.h>

#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <sys/stat.h>

#include <iostream>

// We are using fprintf() on purpose... see comment below
//#include <snapwebsites/poison.h>


void output(int code
          , char const * page_title
          , char const * message)
{
    if(code != 200)
    {
        std::cout << "Status: " << code << "\n";
    }
    std::cout << "Server: Snap! C++\n"
              << "Expires: Sat,  1 Jan 2000 00:00:00 GMT\n"
              << "Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\n"
              << "Connection: close\n"
              << "X-Robots: noindex\n"
              << "\n"
              << "<html><head>"
              << "<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">"
              << "<title>"
              << page_title
              << "</title><head><body><h1>"
          ;
    if(code != 200)
    {
        std::cout << "HTTP " << code << " ";
    }
    std::cout << page_title
              << "</h1><p>"
              << message
              << "</p>"
        ;
    if(code == 200 || code == 503)
    {
        std::cout << "<p>Check <a href=\"/build-log.html\">Build Log</a></p>";
    }
    // no need for the Home Page link in an IFRAME
    //std::cout << "<p>Back to the <a href=\"/\">Home Page</a></p>"
    std::cout << "</body></html>"
              << "\n"
          ;
}

int main(int argc, char *argv[])
{
    snap::NOTUSED(argc);
    snap::NOTUSED(argv);

    // the process has to run as the user named "build" and group "build"
    // so first we want to do that, then we want to cd to the correct
    // place and start the process

    struct stat st;
    if(stat("/run/lock/snap-build.lock", &st) == 0)
    {
        output(503, "Build Error", "Build system lock is still in place. If you think this is an error, check that the script ended and delete the lock file: \"/run/lock/snap-build.lock\"");
        std::cerr << "error: the build system lock file is still in place.\n";
        exit(0);
    }

    // search for the "build" user
    struct passwd *p(getpwnam("build"));
    if(p == nullptr)
    {
        // if we cannot find a user named "build"
        output(500, "Build Error", "User \"build\" does not exist on this computer.");
        std::cerr << "error: cannot find user named \"build\" on this computer.\n";
        exit(0);
    }

    struct group *g(getgrnam("build"));
    if(g == nullptr)
    {
        // if we cannot find a group named "build"
        output(500, "Build Error", "Group \"build\" does not exist on this computer.");
        std::cerr << "error: cannot find group named \"build\" on this computer.\n";
        exit(0);
    }

    // make sure the current directory is valid when changing user/group
    if(chdir("/") == -1)
    {
        // if we cannot become group "build"
        output(500, "Build Error", "Could not cd to \"/\" directory.");
        std::cerr << "error: cannot become the \"build\" group on this computer.\n";
        exit(0);
    }

    // this doesn't work as expected, instead we use `su -l build ...`
    // which gives us everything needed (i.e. HOME, PATH, etc.)
    //
    // become build:build
    // (group first)
    //if(setegid(g->gr_gid) == -1)
    //{
    //    // if we cannot become group "build"
    //    output(500, "Build Error", "Could not become \"build\" group.");
    //    std::cerr << "error: cannot become the \"build\" group on this computer.\n";
    //    exit(0);
    //}
    //if(seteuid(p->pw_uid) == -1)
    //{
    //    // if we cannot become user "build"
    //    output(500, "Build Error", "Could not become \"build\" user.");
    //    std::cerr << "error: cannot become the \"build\" user on this computer.\n";
    //    exit(0);
    //}

    if(chdir("/home/build") == -1)
    {
        // if we cannot become group "build"
        output(500, "Build Error", "Could not cd to \"/home/build\" directory.");
        std::cerr << "error: cannot become the \"build\" group on this computer.\n";
        exit(0);
    }

    int const pid(fork());
    if(pid == 0)
    {
        // child process, just run the build script
        // become root:root so we can execute 'su'

        // the std::cerr are visible in the apache2 error.log if necessary
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

        // remove output with our HTML file, the user can be sent there
        // to see where the build process is at.
        //
        // the "</pre></body></html>" will always be missing...
        time_t today(time(NULL));
        struct tm const * now(localtime(&today));
        char date[256];
        strftime(date, sizeof(date), "%Y/%m/%d %T", now);
        snap::NOTUSED(freopen("/var/www/build/public_html/build-log.html", "w", stdout));
        // WARNING: using fprintf() to make sure we print in the new stdout
        //          and not the old one
        fprintf(stdout,
                     "<html><head>"
                       "<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">"
                       "<title>Build Log</title>"
                     "</head>"
                     "<body>"
                       "<h1>Build Log</h1>"
                       // no need for the Home Page link in an IFRAME
                       //"<p>Go back to the <a href=\"/\">Home Page</a></p>"
                       "<p>If not yet complete, click the \"Current/Last Build Status\" link to reload once in a while. We do not have an auto-refresh in this page.</p>"
                       "<p>Build started on: %s</p>"
                     "<pre>", date);
        fflush(stdout);

        // make sure we are detected from the Apache server
        snap::NOTUSED(freopen("/var/log/build-error.log", "a", stderr));
        snap::NOTUSED(freopen("/dev/null", "r", stdin));

        std::string command("bin/build.sh");
        std::string const qs(getenv("QUERY_STRING"));
        size_t pos(qs.find("finball")); // ameliorate at some point (i.e. projects=
        if(pos != std::string::npos)
        {
            // we have to use --noclean for a partial update
            command += " --noclean --projects finball";
        }

        // become the 'build' user with 'su' to make sure it works as expected
        execl("/bin/su"
            , "-l"
            , "build"
            , "-c"
            , command.c_str()
            , (char *) NULL);

        // it should never fail, unless the installation is "broken"
        // (not exactly as expected)
        int e(errno);
        std::cerr << "error: execl() failed (errno" << e << ".)\n";
        exit(1);
    }

    if(pid == -1)
    {
        output(500, "Build Error", "fork() failed.");
        std::cerr << "error: fork() failed.\n";
        exit(0);
    }

    // parent started the script, we can now returned
    // since we are a CGI, we probably want to output an error as HTML
    output(200, "Build Started", "The build process was started. Click on the link below to get the current status...");

    exit(0);
}

// vim: ts=4 sw=4 et
