// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapbuilder
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


// as2js lib
//
#include    <as2js/json.h>


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/string_replace_many.h>
#include    <snapdev/trim_string.h>


// Qt lib
//
#include    <QtWidgets>


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




void project_remote_info::set_date(std::string const & date)
{
    f_date = date;
}


void project_remote_info::set_build_codename(std::string const & codename)
{
    f_build_codename = codename;
}


void project_remote_info::set_build_state(std::string const & build_state)
{
    f_build_state = build_state;
}


void project_remote_info::set_build_version(std::string const & build_version)
{
    f_build_version = build_version;
}


void project_remote_info::set_build_arch(std::string const & build_arch)
{
    f_build_arch = build_arch;
}


std::string const & project_remote_info::get_date() const
{
    return f_date;
}


std::string const & project_remote_info::get_build_codename() const
{
    return f_build_codename;
}


std::string const & project_remote_info::get_build_state() const
{
    return f_build_state;
}


std::string const & project_remote_info::get_build_version() const
{
    return f_build_version;
}


std::string const & project_remote_info::get_build_arch() const
{
    return f_build_arch;
}








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
        SNAP_LOG_INFO
            << "found project under: \""
            << f_project_path
            << "\""
            << SNAP_LOG_SEND;

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
    SNAP_LOG_INFO
        << "Loading project "
        << f_name
        << "."
        << SNAP_LOG_SEND;

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

    if(!get_last_commit_hash())
    {
        return;
    }

    if(!get_build_hash())
    {
        return;
    }

    retrieve_building_state();

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
    //    << "get last commit timestamp with: "
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


bool project::get_last_commit_hash()
{
// Using a process would be great but this is asynchronous and at this
// point I just don't want to deal with 100% asynchronous functionality
// (i.e. I would then have to read the list of processes and then gather
// their data; as the data arrives, we could then update the table the
// user sees, it will be great, but this is just a quick builder helper
// not an end user project)
//
//    // retrieve the hash of our latest `git commit ...` so it can be
//    // compared against the last build hash
//    //
//    {
//        cppprocess::io_capture_pipe::pointer_t capture(std::make_shared<cppprocess::io_capture_pipe>());
//        capture->add_process_done_callback(std::bind(&project::last_commit, this));
//
//        -- this is the process definition from the header
//        cppprocess::process::pointer_t
//                                f_last_commit_process = cppprocess::process::pointer_t();
//
//        f_last_commit_process = std::make_shared<cppprocess::process>("last-commit");
//        f_last_commit_process->set_working_directory(f_project_path);
//        f_last_commit_process->set_command("git");
//        f_last_commit_process->add_argument("rev-parse");
//        f_last_commit_process->add_argument("HEAD");
//        f_last_commit_process->set_output_io(capture);
//        f_last_commit_process->start(); // TODO: check return value for errors
//    }

    std::string cmd("cd ");
    cmd += f_project_path;
    cmd += "; git rev-parse HEAD";

    //SNAP_LOG_INFO
    //    << "get last commit hash with: "
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

    f_last_commit_hash = snapdev::trim_string(std::string(buf));

    //SNAP_LOG_INFO
    //    << "last commit hash: "
    //    << f_last_commit_hash
    //    << SNAP_LOG_SEND;

    return !f_last_commit_hash.empty();
}


bool project::get_build_hash()
{
    // this state would need to be communicated if you have multiple
    // programmers using the snapbuilder...
    //
    std::ifstream hash;
    hash.open(get_build_hash_filename());
    if(hash.is_open())
    {
        hash >> f_build_hash;
        f_build_hash = snapdev::trim_string(f_build_hash);
    }

    return true;
}


void project::retrieve_building_state()
{
    // if the .building file exists, then that means we started a build
    // and we don't yet know whether it's finished
    //
    std::ifstream flag;
    flag.open(get_flag_filename());

    // WARNING: do not call the set_building() otherwise we may mess up the
    //          last revision UUID file, which is important to know the
    //          status of the build
    //
    f_building = flag.is_open();
}


bool project::is_valid() const
{
    return f_valid;
}


std::string const & project::get_name() const
{
    return f_name;
}


/** \brief Get the exact name as found on launchpad
 *
 * The `cmake` project is renamed `snapcmakemodules` on launchpad. This
 * function returns that name for that project.
 *
 * \return The name of the project as defined on launchpad.
 */
std::string project::get_project_name() const
{
    if(f_name == "cmake")
    {
        return "snapcmakemodules";
    }

    if(f_name == "libQtSerialization")
    {
        return "libqtserialization";
    }

    return f_name;
}


std::string const & project::get_version() const
{
    return f_version;
}


std::string project::get_remote_version() const
{
    if(f_remote_info.size() == 0)
    {
        return std::string("-");
    }

    return f_remote_info[0]->get_build_version();
}


/** \brief Compute the state of the project.
 *
 * The project has many states which are computed as follow:
 *
 * * <empty> -- this is the default state value which means that the
 * state was not yet calculated
 *
 * * not committed -- the project is not yet commit; we have changes in
 * our local files
 *
 * * not pushed -- the project was not yet pushed to the remote repository;
 * we prefer to have files pushed when we generate the source for a build
 * even though it doesn't matter to launchpad, it makes it easier if you
 * have multiple developers to have things pushed; our build system, though
 * would use files from the remote repository, so in that case it is
 * important to us
 *
 * * ready -- everything is ready for a build
 *
 * * building -- launchpad is currently building
 */
std::string project::get_state() const
{
    // building has priority
    //
    if(f_building)
    {
        return "building";
    }

    // "not committed" and "not pushed" are returned as is
    //
    if(f_state != "ready")
    {
        return f_state;
    }

    // never built? (at least no info from remote)
    //
    if(get_remote_version() == "-")
    {
        return "never built";
    }

    if(get_build_failed())
    {
        return "build failed";
    }

    // if the version did not change, but the hash did, then the programmer
    // has to edit the changelog to bump the version
    //
    if(get_version() == get_remote_version())
    {
        // the build hash may not be available (not yet in our cache)
        // which is a big problem we'll want to resolve at some point
        // (i.e. we would need the cache to be on our build server not
        // individually defined for each user)
        //
        if(f_build_hash.empty()
        || f_last_commit_hash == f_build_hash)
        {
            return "built";
        }
        else
        {
            return "bad version";
        }
    }

    // we're ready for a new build!
    //
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


std::string project::get_remote_build_state() const
{
    if(f_remote_info.size() == 0)
    {
        return std::string("-");
    }

    return f_remote_info[0]->get_build_state();
}


std::string project::get_remote_build_date() const
{
    if(f_remote_info.size() == 0)
    {
        return std::string("-");
    }

    // this may not be the build date, we find 3 dates:
    //   . creation date
    //   . start date
    //   . finished date
    //
    std::string date(f_remote_info[0]->get_date());
    std::string::size_type pos(date.find('T'));
    if(pos != std::string::npos)
    {
        date[pos] = ' ';
    }
    pos = date.find('.');
    if(pos != std::string::npos)
    {
        return date.substr(0, pos);
    }
    return date;
}


/** \brief Load the remote data from launchpad.
 *
 * This function checks whether we already have a cache of the launchpad data.
 * If so then the function does nothing. If we do not have any information
 * about the project, then we download it from launchpad. This tells us
 * whether we need to run a build or not.
 */
void project::load_remote_data()
{
    // a build is complete only once all the releases are built (or failed to)
    //
    // we can download a JSON file from launchpad that gives us the information
    // about the latest build(s)
    //

    std::string const cache_filename(get_ppa_json_filename());
    if(access(cache_filename.c_str(), R_OK) != 0)
    {
        // no cache available, load it for the first time
        //
        if(!retrieve_ppa_status())
        {
            // load failed, that's it for now on that one...
            //
            return;
        }
    }

    if(access(cache_filename.c_str(), R_OK) != 0)
    {
        SNAP_LOG_MAJOR
            << "cache file \""
            << cache_filename
            << "\" not available even after PPA retrieval. Try forcibly resetting the cache of that project."
            << SNAP_LOG_SEND;
        return;
    }

    // read the file and save the few fields we're interested in:
    //
    //   - last build date
    //   - build state
    //   - source version
    //   - architecture
    //
    as2js::String json_filename(cache_filename);
    as2js::JSON json;
    as2js::JSON::JSONValue::pointer_t root(json.load(json_filename));
    if(root->get_type() != as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT)
    {
        SNAP_LOG_ERROR
            << "JSON found in cache file \""
            << cache_filename
            << "\" does not represent an object."
            << SNAP_LOG_SEND;
        return;
    }

    as2js::JSON::JSONValue::object_t const & top_fields(root->get_object());
    if(top_fields.find("total_size") != top_fields.end())
    {
        // if not empty, we have a "total_size_link" instead
        //
        // this happens whenever we create a new project and we have not
        // yet compiled it on launchpad
        //
        SNAP_LOG_ERROR
            << "JSON found in cache file \""
            << cache_filename
            << "\" has a \"total_size\" field which means it is empty."
            << SNAP_LOG_SEND;
        return;
    }

    // TODO: verify that the "start" field is 0

    auto const it(top_fields.find("entries"));
    if(it == top_fields.cend())
    {
        SNAP_LOG_ERROR
            << "JSON found in cache file \""
            << cache_filename
            << "\" has no \"entries\" field."
            << SNAP_LOG_SEND;
        return;
    }

    if(it->second->get_type() != as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY)
    {
        SNAP_LOG_ERROR
            << "JSON found in cache file \""
            << cache_filename
            << "\" has an \"entries\" field, but it is not an array."
            << SNAP_LOG_SEND;
        return;
    }

    std::set<std::string> complete_list_of_codenames_and_archs;
    std::set<std::string> built_list_of_codenames_and_archs;
    f_built_successfully = -1;

    as2js::JSON::JSONValue::array_t const & entries(it->second->get_array());
    f_remote_info.clear();
    f_remote_info.reserve(entries.size());
    for(as2js::JSON::JSONValue::pointer_t const & e : entries)
    {
        // just in case, verify that the entry is an object, if not, just
        // ignore that item
        //
        if(e->get_type() != as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT)
        {
            continue;
        }
        as2js::JSON::JSONValue::object_t build(e->get_object());

        // verify that the project name matches this entry, if not, we
        // may need to delete the cache...
        //
        auto const source_package_name_it(build.find("source_package_name"));
        if(source_package_name_it == build.end())
        {
            SNAP_LOG_ERROR
                << "\"source_package_name\" field not found."
                << SNAP_LOG_SEND;
            continue;
        }
        if(source_package_name_it->second->get_type() != as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING)
        {
            SNAP_LOG_ERROR
                << "\"source_package_name\" is not a string."
                << SNAP_LOG_SEND;
            continue;
        }
        if(source_package_name_it->second->get_string().to_utf8() != get_project_name())
        {
            SNAP_LOG_ERROR
                << "\"source_package_name\" says \""
                << source_package_name_it->second->get_string().to_utf8()
                << "\", we expected \""
                << get_project_name()
                << "\" instead."
                << SNAP_LOG_SEND;
            continue;
        }

        // get the creation date
        //
        std::string date;
        auto const date_built_it(build.find("datebuilt"));
        if(date_built_it != build.end())
        {
            // date when it was last built, we keep that one!
            //
            if(date_built_it->second->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING)
            {
                date = date_built_it->second->get_string().to_utf8();
            }

            // TODO: get duration
        }
        if(date.empty())
        {
            auto const date_started_it(build.find("date_started"));
            if(date_started_it != build.end())
            {
                // date when it was last built, we keep that one!
                //
                if(date_started_it->second->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING)
                {
                    date = date_started_it->second->get_string().to_utf8();
                }
            }
        }
        if(date.empty())
        {
            auto const date_started_it(build.find("datecreated"));
            if(date_started_it != build.end())
            {
                // date when it was last built, we keep that one!
                //
                if(date_started_it->second->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING)
                {
                    date = date_started_it->second->get_string().to_utf8();
                }
            }
        }
        if(date.empty())
        {
            SNAP_LOG_WARNING
                << "no date found in this entry."
                << SNAP_LOG_SEND;
        }

        // get the build version
        //
        std::string build_version;
        auto const build_version_it(build.find("source_package_version"));
        if(build_version_it != build.end())
        {
            if(build_version_it->second->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING)
            {
                build_version = build_version_it->second->get_string().to_utf8();
            }
        }
        if(build_version.empty())
        {
            SNAP_LOG_ERROR
                << "no version found in this entry."
                << SNAP_LOG_SEND;
            continue;
        }

        // the version includes a codename (i.e. "....~bionic")
        // here we want to break that up so we have a version
        // and a seperated codename
        //
        std::string::size_type const pos(build_version.find('~'));
        if(pos == std::string::npos)
        {
            SNAP_LOG_ERROR
                << "no '~' found in the version, we expected a codename."
                << SNAP_LOG_SEND;
            continue;
        }
        std::string const build_codename(build_version.substr(pos + 1));
        build_version.erase(pos);

        // get the build architecture
        //
        std::string build_arch;
        auto const build_arch_it(build.find("arch_tag"));
        if(build_arch_it != build.end())
        {
            // date when it was last built, we keep that one!
            //
            if(build_arch_it->second->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING)
            {
                build_arch = build_arch_it->second->get_string().to_utf8();
            }
        }
        if(build_arch.empty())
        {
            SNAP_LOG_ERROR
                << "no architecture specified in this entry."
                << SNAP_LOG_SEND;
            continue;
        }

        // to know whether all the versions and architectures are built
        // we need a complete list of those for our given version
        //
        // TODO: this is flaky because it may take a moment for the
        //       remote system to enter all the data; at this time,
        //       though, we take 1 min. to re-read the state so we
        //       should be good... assuming no huge delay on launchpad
        //
        if(build_version == f_version)
        {
            complete_list_of_codenames_and_archs.insert(build_codename + ':' + build_arch);
        }

        // get the build state of this entry
        //
        std::string build_state;
        auto const build_state_it(build.find("buildstate"));
        if(build_state_it != build.end())
        {
            if(build_state_it->second->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING)
            {
                build_state = build_state_it->second->get_string().to_utf8();

                if(build_version == f_version)
                {
                    if(build_state == "Successfully built")
                    {
                        built_list_of_codenames_and_archs.insert(build_codename + ':' + build_arch);

                        // set to 1 if first; then if already 1, we're good
                        // and if set to 0, we keep it in the "failed" state
                        // because at least one version failed
                        //
                        if(f_built_successfully == -1)
                        {
                            f_built_successfully = 1;
                        }
                    }
                    else if(build_state == "Failed to build"
                         || build_state == "Dependency wait")
                    {
                        built_list_of_codenames_and_archs.insert(build_codename + ':' + build_arch);
                        f_built_successfully = 0;
                    }
                }
            }
        }
        if(build_state.empty())
        {
            SNAP_LOG_ERROR
                << "no build state found in this entry."
                << SNAP_LOG_SEND;
            continue;
        }

        find_remote_info(build_codename, build_arch);

        project_remote_info::pointer_t info(std::make_shared<project_remote_info>());
        info->set_date(date);
        info->set_build_codename(build_codename);
        info->set_build_state(build_state);
        info->set_build_version(build_version);
        info->set_build_arch(build_arch);

        f_remote_info.push_back(info);
    }

    if(f_building
    && !complete_list_of_codenames_and_archs.empty()
    && complete_list_of_codenames_and_archs == built_list_of_codenames_and_archs)
    {
        f_building = false;

        // delete the flag, we're done with it
        //
        snapdev::NOT_USED(unlink(get_flag_filename().c_str()));

        SNAP_LOG_INFO
            << "Done building \""
            << f_name
            << "\", new status is: \""
            << (f_built_successfully == -1
                ? "unknown"
                : (f_built_successfully == 1
                    ? "Built successfully"
                    : "Build failed"))
            << "\""
            << SNAP_LOG_SEND;
    }


/* example of an enry
{
  "self_link": "https://api.launchpad.net/devel/~snapcpp/+archive/ubuntu/ppa/+build/23113615",
  "web_link": "https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+build/23113615",
  "resource_type_link": "https://api.launchpad.net/devel/#build",
  "datecreated": "2022-02-01T03:45:14.192170+00:00",
  "date_started": "2022-02-01T03:45:28.563934+00:00",
  "datebuilt": "2022-02-01T03:47:52.315429+00:00",
  "duration": "0:02:23.751495",
  "date_first_dispatched": "2022-02-01T03:45:28.563934+00:00",
  "builder_link": "https://api.launchpad.net/devel/builders/lcy02-amd64-024",
  "buildstate": "Successfully built",
  "build_log_url": "https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+build/23113615/+files/buildlog_ubuntu-impish-amd64.snapdev_1.1.18.0~impish_BUILDING.txt.gz",
  "title": "amd64 build of snapdev 1.1.18.0~impish in ubuntu impish RELEASE",
  "dependencies": null,
  "can_be_rescored": false,
  "can_be_retried": false,
  "can_be_cancelled": false,
  "archive_link": "https://api.launchpad.net/devel/~snapcpp/+archive/ubuntu/ppa",
  "pocket": "Release",
  "upload_log_url": null,
  "distribution_link": "https://api.launchpad.net/devel/ubuntu",
  "current_source_publication_link": "https://api.launchpad.net/devel/~snapcpp/+archive/ubuntu/ppa/+sourcepub/13233258",
  "source_package_name": "snapdev",
  "source_package_version": "1.1.18.0~impish",
  "arch_tag": "amd64",
  "changesfile_url": "https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+build/23113615/+files/snapdev_1.1.18.0~impish_amd64.changes",
  "score": null,
  "external_dependencies": null,
  "http_etag": "\"6bc0b24353084b49907e42a239bf2f99c5d1a6b3-670cb7b5c2dce75465f2d7cfb3ddbe3e879544f5\""
}
*/


}


project_remote_info::pointer_t project::find_remote_info(
      std::string const & build_codename
    , std::string const & build_arch)
{
    auto const it(std::find_if(
          f_remote_info.cbegin()
        , f_remote_info.cend()
        , [build_codename, build_arch](project_remote_info::pointer_t info)
        {
            return info->get_build_codename() == build_codename
                && info->get_build_arch() == build_arch;
        }));
    if(it != f_remote_info.end())
    {
        return *it;
    }

    return project_remote_info::pointer_t();
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
 * and `d` so in other words when `a` depends on `b` it also depends on what
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


std::string project::get_ppa_json_filename() const
{
    std::string const & cache(f_snap_builder->get_cache_path());
    return cache + '/' + get_project_name() + ".json";
}


std::string project::get_flag_filename() const
{
    std::string const & cache(f_snap_builder->get_cache_path());
    return cache + '/' + get_project_name() + ".building";
}


std::string project::get_build_hash_filename() const
{
    std::string const & cache(f_snap_builder->get_cache_path());
    return cache + '/' + get_project_name() + ".hash";
}


/** \brief Load the PPA status from LaunchPad.
 *
 * This function forcibly loads a copy of this project JSON which gives us
 * the status of a build (among other things).
 *
 * The function should be called only if (1) we started a build and it is
 * still ongoing or (2) we do not have a version cached on our side.
 * Otherwise it's kind of a waste. The user will be given the ability
 * to by-pass the cache to make sure he can refresh the screen properly.
 *
 * \return true if the PPA was contacted successfully.
 */
bool project::retrieve_ppa_status()
{
    std::string cmd("wget -q -O '");
    cmd += get_ppa_json_filename();
    cmd += "' '";
    cmd += snapdev::string_replace_many(
                  f_snap_builder->get_launchpad_url()
                , {{ "@PROJECT_NAME@", get_project_name() }});
    cmd += '\'';

    SNAP_LOG_INFO
        << "Updating cache of \""
        << f_name
        << "\" with command: \""
        << cmd
        << "\"."
        << SNAP_LOG_SEND;

    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        // When within the QTimerEvent this blocks everything, so not a good
        // idea, using a log message instead
        //
        //QMessageBox msg(
        //      QMessageBox::Critical
        //    , "Error Retrieving Remote Project Data"
        //    , QString("We had trouble retrieving the remote project data from LaunchPad.")
        //    , QMessageBox::Close
        //    , const_cast<snap_builder *>(f_snap_builder)
        //    , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        //msg.exec();

        SNAP_LOG_INFO
            << "Cache of \""
            << f_name
            << "\" could not be updated (r = "
            << r
            << ")."
            << SNAP_LOG_SEND;

        return false;
    }

    SNAP_LOG_INFO
        << "Cache of \""
        << f_name
        << "\" updated successfully."
        << SNAP_LOG_SEND;

    return true;
}


bool project::get_building() const
{
    return f_building;
}


void project::set_building(bool building)
{
    // create a <project-name>.building flag in the cache folder, as long as
    // this is there, we want to continue checking the status on launchpad
    // until the package is built or it failed
    //
    if(building)
    {
        std::ofstream flag;
        flag.open(get_flag_filename());
        if(flag.is_open())
        {
            time_t now(time(nullptr));
            tm t;
            gmtime_r(&now, &t);
            char buf[256];
            strftime(buf, sizeof(buf) - 1, "Started on %y/%m/%d %H:%M:%S", &t);
            buf[sizeof(buf) - 1] = '\0';
            flag << buf << std::endl;
        }

        // gather the latest commit hash in case the programmer updated
        // a few last issues and thus the hash was updated
        //
        if(!get_last_commit_hash())
        {
            SNAP_LOG_ERROR
                << "could not gather the latest commit hash for \""
                << f_name
                << "\" when marking that project as building. Using \""
                << f_last_commit_hash
                << "\" for now."
                << SNAP_LOG_SEND;
        }
        f_build_hash = f_last_commit_hash;

        std::ofstream hash;
        hash.open(get_build_hash_filename());
        if(hash.is_open())
        {
            hash << f_last_commit_hash << std::endl;
        }
    }

    f_building = building;
}


bool project::get_build_succeeded() const
{
    // this value has 3 states:
    //    -1 -- unknown
    //     0 -- build failed
    //     1 -- build succeeded
    //
    return f_built_successfully == 1;
}


bool project::get_build_failed() const
{
    return f_built_successfully == 0;
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
    g_dot_process->set_input_io(input);
    g_dot_process->set_output_io(capture);
    g_dot_process->start(); // TODO: check return value for errors
}


void project::view_svg(vector_t & v, std::string const & root_path)
{
    snapdev::NOT_USED(v);

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
