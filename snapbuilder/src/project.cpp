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


// cppprocess lib
//
#include    <cppprocess/io_capture_pipe.h>
#include    <cppprocess/io_data_pipe.h>


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <algorithm>
#include    <fstream>


// C lib
//
#include    <sys/stat.h>



namespace builder
{


namespace
{


// the dot process needs to survive the function so we have a global
// we don't need to run it more than once anyway (but we need to see how
// to avoid destroying an old version too soon)
//
cppprocess::process::pointer_t  g_dot_process = cppprocess::process::pointer_t();


} // no name namespace




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
    if(fgets(buf, sizeof(buf) - 1, p) == nullptr)
    {
        buf[0] = '\0';
    }
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
    if(fgets(buf, sizeof(buf) - 1, p) == nullptr)
    {
        buf[0] = '\0';
    }
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


/** \brief Get all the dependencies of this project.
 *
 * Each project may depend on one or more other project. This list includes
 * all the dependencies, whatever the depth.
 *
 * This parameter is what we read from the source `deps.make` file, although
 * in many cases some dependencies are missing so we use our
 * add_missing_dependencies() function to complement the list.
 *
 * \note
 * The class uses a set of strings since there is no point in duplicating
 * names. It is likely sorted in alphabetic order.
 *
 * \return The set of dependencies.
 */
project::dependencies_t project::get_dependencies() const
{
    return f_dependencies;
}


/** \brief The trimmed list of dependencies.
 *
 * This list, contrary to the previous one, is going to be as small as
 * possible (trimmed). It only includes direct dependencies of this
 * project.
 *
 * Say that you have the following definitions in the `deps.make`:
 *
 *     a: b c d
 *     b: c d
 *     ...
 *
 * The `a` only needs to depend on `b` because `b` already depends on `c`
 * and `d` so in effect when `a` depends on `b` it also depends on what
 * `b` depends on, which are `c` and `d`.
 *
 * This is true in this case as well:
 *
 *      a: b c d
 *      b: c
 *      c: d
 *      ...
 *
 * `a` also depends on `c` and `d` through `b`, even if the dependency on
 * `d` is through a dependency of `b` and not a direct dependency of `b`.
 *
 * Note that since we have a list of all the dependencies of each project
 * (a.k.a. `f_dependencies`), this is a rather easy to compute list. We
 * just have to do:
 *
 *     a.f_trimmed_dependencies = a.f_dependencies
 *                                      - b.f_dependencies
 *                                      - c.f_dependencies
 *                                      - d.f_dependencies;
 *
 * \return The list of trimmed dependencies.
 */
project::dependencies_t project::get_trimmed_dependencies() const
{
    return f_trimmed_dependencies;
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
    for(auto & p : v)
    {
        if(p->f_dependencies.size() > 1)
        {
            auto q(p->f_dependencies.begin());
            auto it(m.find(*q));
            if(it == m.end())
            {
                SNAP_LOG_ERROR
                    << "Project \""
                    << p->get_name()
                    << "\" has dependency \""
                    << *q
                    << "\" which did not match any project name."
                    << SNAP_LOG_SEND;
                continue;
            }
            std::set_difference(
                      p->f_dependencies.begin()
                    , p->f_dependencies.end()
                    , it->second->f_dependencies.begin()
                    , it->second->f_dependencies.end()
                    , std::inserter(p->f_trimmed_dependencies, p->f_trimmed_dependencies.begin()));
            for(++q; q != p->f_dependencies.end(); ++q)
            {
                it = m.find(*q);
                if(it == m.end())
                {
                    SNAP_LOG_ERROR
                        << "Project \""
                        << p->get_name()
                        << "\" has dependency \""
                        << *q
                        << "\" which did not match any project name."
                        << SNAP_LOG_SEND;
                    continue;
                }
                project::dependencies_t trimmed;
                std::set_difference(
                          p->f_trimmed_dependencies.begin()
                        , p->f_trimmed_dependencies.end()
                        , it->second->f_dependencies.begin()
                        , it->second->f_dependencies.end()
                        , std::inserter(trimmed, trimmed.begin()));
                p->f_trimmed_dependencies = trimmed;
            }
        }
        else
        {
            // when we have one or zero dependencies, there is no need
            // to subtract anything, just copy
            //
            p->f_trimmed_dependencies = p->f_dependencies;
        }
    }
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
// this happens, I'd need to look closer for why & how...
//std::cerr << "--------------- found missing dependencies in \"" << p->get_name() << "\"!?\n";
    }
}


void project::generate_svg(
      vector_t & v
    , cppprocess::io::process_io_done_t output_captured)
{
    std::stringstream dot;
    dot << "digraph dependencies {\n";
    for(auto & p : v)
    {
        if(p->get_name() == "snapbuilder")
        {
            continue;
        }
        dependencies_t const dependencies(p->get_trimmed_dependencies());
        if(!dependencies.empty())
        {
            dot << "\"" << p->get_name() << "\" [shape=box];\n";
            for(auto const & n : dependencies)
            {
                dot << "\"" << p->get_name() << "\" -> \"" << n << "\";\n";
            }
        }
        else
        {
            dot << "\"" << p->get_name() << "\" [shape=ellipse];\n";
        }
    }
    dot << "}\n";

    SNAP_LOG_INFO
        << "Run dot command: `dot -Tsvg`"
        << SNAP_LOG_SEND;

    cppprocess::io_data_pipe::pointer_t input(std::make_shared<cppprocess::io_data_pipe>());
    input->add_input(dot.str());

    cppprocess::io_capture_pipe::pointer_t capture(std::make_shared<cppprocess::io_capture_pipe>());
    capture->add_process_done_callback(output_captured);

    g_dot_process = std::make_shared<cppprocess::process>("dependencies");

    g_dot_process->set_command("dot");
    g_dot_process->add_argument("-Tsvg");
    g_dot_process->set_output_io(capture);
    g_dot_process->start(); // TODO: check return value for errors
}


void project::view_svg(vector_t & v, std::string const & root_path)
{
    snap::NOT_USED(v);

    std::string const svg_filename(root_path + "/BUILD/dependencies.svg");
    struct stat s;
    if(stat(svg_filename.c_str(), &s) != 0)
    {
        return;
    }
    if(s.st_size == 0)
    {
        // TODO: make that a GUI error
        //
        std::cerr << "error: dependencies.svg file is empty?!" << std::endl;
        return;
    }

    std::string cmd("display ");
    cmd += svg_filename;
    cmd += " &";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        SNAP_LOG_ERROR
            << "command \""
            << cmd
            << "\" generated an error."
            << SNAP_LOG_SEND;
    }
}


} // builder namespace
// vim: ts=4 sw=4 et
