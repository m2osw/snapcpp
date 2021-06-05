// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snap-builder
// contact@m2osw.com
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// self
//
#include    "project.h"

#include    "snap_builder.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <algorithm>


// C lib
//
#include    <sys/stat.h>



namespace builder
{




project::project(
          snap_builder * parent
        , std::string const & name
        , advgetopt::string_list_t const & deps)
    : f_snap_builder(parent)
    , f_name(name)
{
    if(f_name == "snapbuilder")
    {
        return;
    }

    for(auto & d : deps)
    {
        add_dependency(d);
    }

    if(find_project())
    {
        //SNAP_LOG_INFO
        //    << "find project in: \""
        //    << f_project_path
        //    << "\""
        //    << SNAP_LOG_SEND;

        load_project();
    }
}


bool project::find_project()
{
    struct stat s;

    // top folder?
    //
    f_project_path = f_snap_builder->get_root_path() + "/" + f_name;
    if(stat(f_project_path.c_str(), &s) == 0)
    {
        return true;
    }

    // contrib?
    //
    f_project_path = f_snap_builder->get_root_path() + "/contrib/" + f_name;
    if(stat(f_project_path.c_str(), &s) == 0)
    {
        return true;
    }

    // not found
    //
    return false;
}


void project::load_project()
{
    if(!retrieve_version())
    {
        return;
    }

    if(!check_state())
    {
        return;
    }

    if(!get_last_commit_timestamp())
    {
        return;
    }

    f_valid = true;

    load_remote_data();
}


bool project::retrieve_version()
{
    std::string cmd("cd ");
    cmd += f_project_path;
    cmd += "; dpkg-parsechangelog --show-field Version";

    //SNAP_LOG_INFO
    //    << "retrieve version with: "
    //    << cmd
    //    << SNAP_LOG_SEND;

    FILE * p(popen(cmd.c_str(), "r"));
    char buf[256];
    fgets(buf, sizeof(buf) - 1, p);
    buf[sizeof(buf) - 1] = '\0';
    pclose(p);
    f_version = buf;

    std::string::size_type tilde(f_version.find('~'));
    if(tilde != std::string::npos)
    {
        f_version = f_version.substr(0, tilde);
    }

    //SNAP_LOG_INFO
    //    << "local version: "
    //    << f_version
    //    << SNAP_LOG_SEND;

    return !f_version.empty();
}


bool project::check_state()
{
    // verify that we committed
    //
    {
        std::string cmd("cd ");
        cmd += f_project_path;
        cmd += "; git diff-index --quiet HEAD --";

        //SNAP_LOG_INFO
        //    << "verify committed with: "
        //    << cmd
        //    << SNAP_LOG_SEND;

        int const r(system(cmd.c_str()));
        if(r != 0)
        {
            f_state = "not committed";
            return true;
        }
    }

    // verify that we pushed
    //
    {
        std::string cmd("cd ");
        cmd += f_project_path;
        cmd += "; test \"`git rev-parse @{u}`\" = \"`git rev-parse HEAD`\"";

        //SNAP_LOG_INFO
        //    << "verify pushed with: "
        //    << cmd
        //    << SNAP_LOG_SEND;

        int const r(system(cmd.c_str()));
        if(r != 0)
        {
            f_state = "not pushed";
            return true;
        }
    }

    // state looks good so far
    //
    f_state = "ready";
    return true;
}


bool project::get_last_commit_timestamp()
{
    std::string cmd("cd ");
    cmd += f_project_path;
    cmd += "; git log -1 --format=%ct";

    //SNAP_LOG_INFO
    //    << "last commit timestamp with: "
    //    << cmd
    //    << SNAP_LOG_SEND;

    FILE * p(popen(cmd.c_str(), "r"));
    char buf[256];
    fgets(buf, sizeof(buf) - 1, p);
    buf[sizeof(buf) - 1] = '\0';
    pclose(p);

    f_last_commit = atol(buf);

    //SNAP_LOG_INFO
    //    << "last commit timestamp: "
    //    << f_last_commit
    //    << SNAP_LOG_SEND;

    return f_last_commit > 0;
}


bool project::is_valid() const
{
    return f_valid;
}


std::string const & project::get_name() const
{
    return f_name;
}


std::string const & project::get_version() const
{
    return f_version;
}


std::string const & project::get_state() const
{
    return f_state;
}


time_t project::get_last_commit() const
{
    return f_last_commit;
}


std::string project::get_last_commit_as_string() const
{
    char buf[256];
    tm t;
    localtime_r(&f_last_commit, &t);
    strftime(buf, sizeof(buf), "%D %T", &t);
    buf[sizeof(buf) - 1];
    return buf;
}


void project::load_remote_data()
{
    // a build is complete only once all the releases are built (or failed to)
    //
    // we have one Packages.gz per release which lists the latest version
    // available
    //
    //    dists/<release>/main/binary-amd64/Packages.gz
    //
    // note that all the releases are present, but we only support a few
    //
    // once we have the package built, we can check the date with a HEAD
    // request of the .deb (the .deb itself doesn't actually have a Date:
    // field, somehow?!). The Packages.gz file includes the path to the
    // file which starts from the same top launchpad URL:
    //
    //    pool/main/a/as2js/as2js_0.1.32.0~<release>_amd64.deb
    //
    // just like the release, multiple architectures means we also have one
    // package per architecture
    //
    // Example:
    //    curl -I http://ppa.launchpad.net/snapcpp/ppa/ubuntu/pool/main/a/as2js/as2js_0.1.32.0~hirsute_amd64.deb

    std::string url(f_snap_builder->get_launchpad_url());
}


project::dependencies_t project::get_dependencies() const
{
    return f_dependencies;
}


bool project::operator < (project const & rhs) const
{
    // B E A/dependencies => A > B
    //
    // (i.e. if B is a dependency of A then B appears before A)
    //
    auto it(std::find(f_dependencies.begin(), f_dependencies.end(), rhs.f_name));
    if(it != f_dependencies.end())
    {
        return false;
    }

    // A E B/dependencies => A < B
    //
    // (i.e. if A is a dependency of B then A appears before B)
    //
    auto jt(std::find(rhs.f_dependencies.begin(), rhs.f_dependencies.end(), f_name));
    if(jt != rhs.f_dependencies.end())
    {
        return true;
    }

    // A and B do not depend on each other, sort by name
    //
    return f_name < rhs.f_name;
}


void project::sort(std::vector<pointer_t> & v)
{
    std::sort(v.begin(), v.end(), compare);
}


void project::add_dependency(std::string const & name)
{
    f_dependencies.insert(name);
}


bool project::compare(pointer_t a, pointer_t b)
{
    return *a < *b;
}


void project::simplify(vector_t & v)
{
    // add all the project to a map so we can search them painlessly
    //
    map_t m;
    for(auto & p : v)
    {
        m[p->get_name()] = p;
    }

    // first we make sure that we have all the dependencies in our
    // f_dependencies list; as far as I know that's how I generate
    // it so it should always be the case
    //
    for(auto & p : v)
    {
        p->add_missing_dependencies(p, m);

        // reset the recursed flag to make sure we capture all dependencies
        // (this should not be needed as far as I know; because the
        // dependencies form a valid tree which doesn't cycle...)
        //
        for(auto & r : v)
        {
            r->f_recursed_add_dependencies = false;
        }
    }

    // now we want to create a trimmed version of the list of dependencies
    // which is a list with the minimum number of dependencies so that all
    // the projects will still be built
    //
}


/** \brief Function used to add all the missing dependencies to all the projects.
 *
 * This recursive function adds all the dependencies of the dependencies of
 * \p p to \p p. So if \p p depends on a project m, and project m has
 * dependency q which is not yet defined in \p p, then q gets added to
 * \p p.
 *
 * This is recursive meaning that if p depends on m which depends on q, all
 * will be checked.
 */
void project::add_missing_dependencies(pointer_t p, map_t & m)
{
    if(p->f_recursed_add_dependencies)
    {
        return;
    }
    p->f_recursed_add_dependencies = true;

    for(;;)
    {
        dependencies_t const dependencies(p->get_dependencies());
        for(auto const & dependency_name : dependencies)
        {
            auto it(m.find(dependency_name));
            if(it == m.end())
            {
                SNAP_LOG_ERROR
                    << "Project \""
                    << p->get_name()
                    << "\" has dependency \""
                    << dependency_name
                    << "\" which did not match any project name."
                    << SNAP_LOG_SEND;
                continue;
            }
            add_missing_dependencies(it->second, m);
            dependencies_t const sub_dependencies(it->second->get_dependencies());
            for(auto const & sub_dependency_name : sub_dependencies)
            {
                p->add_dependency(sub_dependency_name);
            }
        }

        if(dependencies.size() == p->get_dependencies().size())
        {
            break;
        }
// this happens, I'd need to look closer for why & how does it work...
//std::cerr << "--------------- found missing dependencies in \"" << p->get_name() << "\"!?\n";
    }
}




} // builder namespace
// vim: ts=4 sw=4 et
