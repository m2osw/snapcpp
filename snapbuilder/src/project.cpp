// Copyright (c) 2021-2023  Made to Order Software Corp.  All Rights Reserved
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
#include    "version.h"


// cppprocess
//
#include    <cppprocess/io_capture_pipe.h>
#include    <cppprocess/io_data_pipe.h>


// as2js
//
#include    <as2js/json.h>


// cppthread
//
#include    <cppthread/guard.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/join_strings.h>
#include    <snapdev/string_replace_many.h>
#include    <snapdev/to_lower.h>
#include    <snapdev/trim_string.h>


// Qt
//
#include    <QtWidgets>


// C++
//
#include    <algorithm>
#include    <fstream>


// C
//
#include    <curl/curl.h>
#include    <sys/stat.h>



namespace builder
{


#define guard_project   cppthread::guard lock(*cppthread::g_system_mutex)


namespace
{


// the dot process needs to survive the function so we have a global
// we don't need to run it more than once anyway (but we need to see how
// to avoid destroying an old version too soon)
//
cppprocess::process::pointer_t  g_dot_process = cppprocess::process::pointer_t();


constexpr std::string_view  g_user_agent_name = "snapbuilder";
constexpr std::string_view  g_user_agent_version = SNAPBUILDER_VERSION_STRING;
constexpr std::string_view  g_user_agent_platform = "Linux; Ubuntu; x86_64";
constexpr std::string_view  g_user_agent_curl = "curl/8.5.0+";
constexpr std::string_view  g_user_agent_wget = "wget/1.21.4+";

constexpr std::string_view  g_user_agent_space = " ";
constexpr std::string_view  g_user_agent_separator = "/";
constexpr std::string_view  g_user_agent_open_parenthesis = "(";
constexpr std::string_view  g_user_agent_close_parenthesis = ")";

constexpr std::string_view  g_curl_user_agent =
    snapdev::join_string_views<
        g_user_agent_name,
        g_user_agent_separator,
        g_user_agent_version,
        g_user_agent_space,
        g_user_agent_open_parenthesis,
        g_user_agent_platform,
        g_user_agent_close_parenthesis,
        g_user_agent_space,
        g_user_agent_curl>;

constexpr std::string_view  g_wget_user_agent =
    snapdev::join_string_views<
        g_user_agent_name,
        g_user_agent_separator,
        g_user_agent_version,
        g_user_agent_space,
        g_user_agent_open_parenthesis,
        g_user_agent_platform,
        g_user_agent_close_parenthesis,
        g_user_agent_space,
        g_user_agent_wget>;



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








/** \brief Initialize a project.
 *
 * The project is given a name (as per deps.make) and the constructor also
 * checks whether the project exists (i.e. we can find a folder with the
 * same name in the top folder or under contrib/...).
 *
 * The state goes like this:
 *
 * * `f_exists` is false, then the folder does not exist, we abandon that
 *   project altogether (not shown in the Qt table) -- this happens for
 *   the snapbuilder project
 * * `f_exists` is true, the folder exists, we will try to load it in our
 *   background thread
 * * `f_loaded` is false, either the project does not exist (see `f_exists`)
 *   or it was not loaded yet
 * * `f_loaded` is true, we successfully loaded the project once, most
 *   of the project data has been updated to what was found on disk/remotely
 * * `f_valid` is false, either the project does not exist (see `f_exists`)
 *   or we did not yet load it, or we loaded it and it failed
 * * `f_valid` is true, the project was loaded successfully
 *
 * Further, we determine a state using the `f_building` value, the `f_state`
 * value, and compare various other fields to know whther the project is
 * ready to be built, it is building now, packaging, etc.
 *
 * \param[in] parent  The snapbuilder object so we can access the root and
 * other paths.
 * \param[in] name  The name of the project as found in deps.make.
 * \param[in] deps  The dependencies found so far.
 */
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

    find_project();
}


void project::find_project()
{
    struct stat s;

    // top folder?
    //
    f_project_path = f_snap_builder->get_root_path() + "/" + f_name;
    if(stat(f_project_path.c_str(), &s) != 0)
    {
        // contrib?
        //
        f_project_path = f_snap_builder->get_root_path() + "/contrib/" + f_name;
        if(stat(f_project_path.c_str(), &s) != 0)
        {
            f_project_path.clear();
            f_exists = false;
            return;
        }
    }

    SNAP_LOG_DEBUG
        << "found project \""
        << f_name
        << "\" under: \""
        << f_project_path
        << "\"."
        << SNAP_LOG_SEND;

    f_exists = true;
}


void project::project_changed()
{
    f_snap_builder->project_changed(shared_from_this());
}


void project::load_project()
{
    SNAP_LOG_INFO
        << "Loading project "
        << f_name
        << "."
        << SNAP_LOG_SEND;

    must_be_background_thread();

    if(!retrieve_version())
    {
        return;
    }

    if(!check_state())
    {
        return;
    }

    // at this point we know about the other states
    //
    {
        guard_project;
        f_loaded = true;
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

    {
        guard_project;
        f_valid = true;
    }

    load_remote_data(true);
}


bool project::retrieve_version()
{
    std::string cmd("cd ");
    cmd += f_project_path;
    cmd += "; dpkg-parsechangelog --show-field Version";

    SNAP_LOG_TRACE
        << "retrieve version with: "
        << cmd
        << SNAP_LOG_SEND;

    FILE * p(popen(cmd.c_str(), "r"));
    char buf[256];
    if(fgets(buf, sizeof(buf) - 1, p) == nullptr)
    {
        buf[0] = '\0';
    }
    buf[sizeof(buf) - 1] = '\0';
    pclose(p);
    std::string version(buf);

    std::string::size_type tilde(version.find('~'));
    if(tilde != std::string::npos)
    {
        version = version.substr(0, tilde);
    }

    SNAP_LOG_TRACE
        << "local version: "
        << version
        << SNAP_LOG_SEND;

    set_version(version);

    return !version.empty();
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
            set_state("not committed");
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
            set_state("not pushed");
            return true;
        }
    }

    // state looks good so far
    //
    set_state("ready");
    return true;
}


bool project::get_last_commit_timestamp()
{
    std::string cmd("cd ");
    cmd += f_project_path;
    cmd += "; git log -1 --format=%ct";

    SNAP_LOG_TRACE
        << "get last commit timestamp with: "
        << cmd
        << SNAP_LOG_SEND;

    FILE * p(popen(cmd.c_str(), "r"));
    char buf[256];
    if(fgets(buf, sizeof(buf) - 1, p) == nullptr)
    {
        buf[0] = '\0';
    }
    buf[sizeof(buf) - 1] = '\0';
    pclose(p);

    time_t const last_commit(atol(buf));

    SNAP_LOG_TRACE
        << "last commit timestamp: "
        << last_commit
        << SNAP_LOG_SEND;

    guard_project;
    f_last_commit = last_commit;
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

    SNAP_LOG_TRACE
        << "get last commit hash with: "
        << cmd
        << SNAP_LOG_SEND;

    FILE * p(popen(cmd.c_str(), "r"));
    char buf[256];
    if(fgets(buf, sizeof(buf) - 1, p) == nullptr)
    {
        buf[0] = '\0';
    }
    buf[sizeof(buf) - 1] = '\0';
    pclose(p);

    std::string const last_commit_hash(snapdev::trim_string(std::string(buf)));

    SNAP_LOG_TRACE
        << "last commit hash: "
        << f_last_commit_hash
        << SNAP_LOG_SEND;

    guard_project;
    f_last_commit_hash = last_commit_hash;
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
        guard_project;
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

    // WARNING: do not call the started_building() since this very function
    //          is called about continuation, not startup and as a result
    //          it could mess up files and parameters
    //
    set_building(flag.is_open()
            ? building_t::BUILDING_COMPILING
            : building_t::BUILDING_NOT_BUILDING);
}


void project::mark_as_done_building()
{
    snapdev::NOT_USED(unlink(get_flag_filename().c_str()));
}


/** \brief Check whether the folder exists.
 *
 * When we read the deps.make file, we could end up with a project name that
 * was deleted. This function returns true if the project still exists.
 *
 * \return true if the project folder was found.
 */
bool project::exists() const
{
    // guard not necessary, this is set at construction time and never changes
    return f_exists;
}


bool project::is_valid() const
{
    guard_project;
    return f_valid;
}


/** \brief Return the name of the project.
 *
 * This function returns the name of the project as found in the deps.make
 * file.
 *
 * \warning
 * The name of the project is the name of folder, not the name of the
 * final package. Actually, the name of the packages are found inside
 * the `debian/control` file. This is important since one project may
 * generate many packages (such as the eventdispatcher).
 *
 * \warning
 * Also we have a special case with the `cmake` folder. The name of that
 * package on GIT is snapcmakemodules. So in some places we change that
 * name to make things work.
 *
 * \return The name of the project folder.
 */
std::string const & project::get_name() const
{
    // no need for a mutex, the name is set on construction and it cannot
    // be changed
    //
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

    return f_name;
}


void project::set_version(std::string const & version)
{
    guard_project;
    f_version = version;
}


std::string project::get_version() const
{
    guard_project;
    return f_version;
}


void project::clear_remote_info(std::size_t size)
{
    guard_project;
    f_remote_info.clear();
    f_remote_info.reserve(size);
}


void project::add_remote_info(project_remote_info::pointer_t info)
{
    guard_project;
    f_remote_info.push_back(info);
}


std::string project::get_remote_version() const
{
    guard_project;
    if(f_remote_info.size() == 0)
    {
        return std::string("-");
    }

    return f_remote_info[0]->get_build_version();
}


/** \brief Set the current state.
 *
 * Change the f_state variable with the specified string.
 *
 * \warning
 * This function is not symmetrical to the get_state(). This function changes
 * the f_state variable. The other returns a state that dependents on many
 * variables such as f_loaded, f_building, versions, etc.
 *
 * \param[in] state  The new state, as a string.
 */
void project::set_state(std::string const & state)
{
    guard_project;
    f_state = state;
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
    guard_project;

    // state is unknown until the project is loaded
    //
    if(!f_loaded)
    {
        return "unknown";
    }

    // building has priority
    //
    switch(f_building)
    {
    case building_t::BUILDING_COMPILING:
        return "building";

    case building_t::BUILDING_PACKAGING:
        return "packaging";

    default:
        // see below for status
        break;

    }

    // "not committed" and "not pushed" are always returned as is
    //
    if(f_state != "ready"
    && f_state != "sending")
    {
        return f_state;
    }

    // never built? (at least no info from remote)
    //
    if(get_remote_version() == "-")
    {
        return "never built";
    }

    if(get_build_status() == build_status_t::BUILD_STATUS_FAILED)
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
    guard_project;
    return f_last_commit;
}


std::string project::get_last_commit_as_string() const
{
    time_t const last_commit(get_last_commit());
    if(last_commit == 0)
    {
        return "-";
    }

    char buf[256];
    tm t;
    localtime_r(&last_commit, &t);
    buf[0] = '\0';
    strftime(buf, sizeof(buf), "%Y-%m-%d %T", &t); // use same format as in JSON
    buf[sizeof(buf) - 1];
    return buf;
}


std::string project::get_remote_build_state() const
{
    guard_project;
    if(f_remote_info.size() == 0)
    {
        return std::string("-");
    }

    return f_remote_info[0]->get_build_state();
}


std::string project::get_remote_build_date() const
{
    guard_project;
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
void project::load_remote_data(bool loading)
{
    must_be_background_thread();

    // a build is complete only once all the releases are built (or failed to)
    //
    // we can download a JSON file from launchpad that gives us the information
    // about the latest build(s)
    //
    if(loading
    || get_building() == building_t::BUILDING_COMPILING
    || f_list_of_codenames_and_archs.empty())
    {
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

            if(access(cache_filename.c_str(), R_OK) != 0)
            {
                SNAP_LOG_MAJOR
                    << "cache file \""
                    << cache_filename
                    << "\" not available even after PPA retrieval."
                    << SNAP_LOG_SEND;
                return;
            }
        }

        // read the file and save the few fields we're interested in:
        //
        //   - last build date
        //   - build state
        //   - source version
        //   - architecture
        //
        std::string json_filename(cache_filename);
        as2js::json json;
        as2js::json::json_value::pointer_t root(json.load(json_filename));
        if(root == nullptr)
        {
            SNAP_LOG_ERROR
                << "file \""
                << cache_filename
                << "\" does not represent a valid JSON file. Deleting."
                << SNAP_LOG_SEND;
            unlink(cache_filename.c_str());
            return;
        }
        if(root->get_type() != as2js::json::json_value::type_t::JSON_TYPE_OBJECT)
        {
            SNAP_LOG_ERROR
                << "JSON found in cache file \""
                << cache_filename
                << "\" does not represent an object."
                << SNAP_LOG_SEND;
            return;
        }

        as2js::json::json_value::object_t const & top_fields(root->get_object());

        // TODO: verify that the "start" field is 0

        auto const it(top_fields.find("entries"));
        if(it == top_fields.cend())
        {
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

            SNAP_LOG_ERROR
                << "JSON found in cache file \""
                << cache_filename
                << "\" has no \"entries\" field."
                << SNAP_LOG_SEND;
            return;
        }

        if(it->second->get_type() != as2js::json::json_value::type_t::JSON_TYPE_ARRAY)
        {
            SNAP_LOG_ERROR
                << "JSON found in cache file \""
                << cache_filename
                << "\" has an \"entries\" field, but it is not an array."
                << SNAP_LOG_SEND;
            return;
        }

        std::set<std::string> built_list_of_codenames_and_archs;
        f_list_of_codenames_and_archs.clear();
        set_build_status(build_status_t::BUILD_STATUS_UNKNOWN);

        as2js::json::json_value::array_t const & entries(it->second->get_array());
        clear_remote_info(entries.size());
        for(as2js::json::json_value::pointer_t const & e : entries)
        {
            // just in case, verify that the entry is an object, if not, just
            // ignore that item
            //
            if(e->get_type() != as2js::json::json_value::type_t::JSON_TYPE_OBJECT)
            {
                continue;
            }
            as2js::json::json_value::object_t build(e->get_object());

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
            if(source_package_name_it->second->get_type() != as2js::json::json_value::type_t::JSON_TYPE_STRING)
            {
                SNAP_LOG_ERROR
                    << "\"source_package_name\" is not a string."
                    << SNAP_LOG_SEND;
                continue;
            }
            if(source_package_name_it->second->get_string() != get_project_name())
            {
                SNAP_LOG_WARNING
                    << "\"source_package_name\" says \""
                    << source_package_name_it->second->get_string()
                    << "\", we expected \""
                    << get_project_name()
                    << "\" instead (this happens if you changed the name of the project and there are old references in the JSON file from launchpad)."
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
                if(date_built_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_STRING)
                {
                    date = date_built_it->second->get_string();
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
                    if(date_started_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_STRING)
                    {
                        date = date_started_it->second->get_string();
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
                    if(date_started_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_STRING)
                    {
                        date = date_started_it->second->get_string();
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
                if(build_version_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_STRING)
                {
                    build_version = build_version_it->second->get_string();
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
                if(build_arch_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_STRING)
                {
                    build_arch = build_arch_it->second->get_string();
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
            if(build_version == get_version())
            {
                f_list_of_codenames_and_archs.insert(build_codename + ':' + build_arch);
            }

            // get the build state of this entry
            //
            std::string build_state;
            auto const build_state_it(build.find("buildstate"));
            if(build_state_it != build.end())
            {
                if(build_state_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_STRING)
                {
                    build_state = build_state_it->second->get_string();

                    if(build_version == get_version())
                    {
                        if(build_state == "Successfully built")
                        {
                            built_list_of_codenames_and_archs.insert(build_codename + ':' + build_arch);

                            // set to 1 if first; then if already 1, we're good
                            // and if set to 0, we keep it in the "failed" state
                            // because at least one version failed
                            //
                            if(get_build_status() == build_status_t::BUILD_STATUS_UNKNOWN)
                            {
                                set_build_status(build_status_t::BUILD_STATUS_SUCCEEDED);
                            }
                        }
                        else if(build_state == "Failed to build"
                             || build_state == "Dependency wait")
                        {
                            built_list_of_codenames_and_archs.insert(build_codename + ':' + build_arch);
                            set_build_status(build_status_t::BUILD_STATUS_FAILED);
                        }

                        auto const build_self_link(build.find("self_link"));
                        if(build_self_link != build.end()
                        && build_self_link->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_STRING)
                        {
                            SNAP_LOG_INFO
                                << get_project_name()
                                << " v"
                                << build_version
                                << "~"
                                << build_codename
                                << " for "
                                << build_arch
                                << ": "
                                << date
                                << ": Found build state \""
                                << build_state
                                << "\" with self-link \""
                                << build_self_link->second->get_string()
                                << "\". (success? "
                                << get_build_status_string()
                                << ")"
                                << SNAP_LOG_SEND;
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

            project_remote_info::pointer_t info(std::make_shared<project_remote_info>());
            info->set_date(date);
            info->set_build_codename(build_codename);
            info->set_build_state(build_state);
            info->set_build_version(build_version);
            info->set_build_arch(build_arch);

            add_remote_info(info);
        }

        if(get_building() == building_t::BUILDING_COMPILING)
        {
            if(!f_list_of_codenames_and_archs.empty()
            && f_list_of_codenames_and_archs == built_list_of_codenames_and_archs)
            {
                // the compiling is done, switch to packaging mode
                //
                if(get_build_status() == build_status_t::BUILD_STATUS_SUCCEEDED)
                {
                    set_building(building_t::BUILDING_PACKAGING);

                    // TODO: find the location when the build process starts
                    //       because it would be cleaner to clear this list
                    //       at that point (especially if we want to show
                    //       the end user the status of each package for
                    //       a project)
                    //
                    for(auto & status : f_package_statuses)
                    {
                        status.second = false;
                    }

                    SNAP_LOG_INFO
                        << "Done compiling \""
                        << f_name
                        << "\", starting packaging."
                        << SNAP_LOG_SEND;
                }
                else
                {
                    set_building(building_t::BUILDING_NOT_BUILDING);

                    // delete the flag, we are done with it
                    //
                    mark_as_done_building();

                    SNAP_LOG_INFO
                        << "Done compiling \""
                        << f_name
                        << "\". All were not successful. Build process stopped."
                        << SNAP_LOG_SEND;
                }
            }
            else if(!f_list_of_codenames_and_archs.empty()
                 && !built_list_of_codenames_and_archs.empty())
            {
                SNAP_LOG_INFO
                    << "Still building \""
                    << f_name
                    << "\", completed list of code names & architectures: \""
                    << snapdev::join_strings(f_list_of_codenames_and_archs, ", ")
                    << "\", list of built code names & architectures: \""
                    << snapdev::join_strings(built_list_of_codenames_and_archs, ", ")
                    << "\""
                    << SNAP_LOG_SEND;
            }
        }
    }

    // WARNING: the dot_deb_exists() call takes MINUTES PER .deb file
    //          and I'm not too sure why; although it works, it's really
    //          really slow... so when loading the app. I skip that test
    //          and the package remains in "building" status until the
    //          timer comes off and then we test those files in the
    //          background (on the timer)
    //
    // TODO: with the way our backend is testing things, we could return
    //       here and have the following code in the dot_deb_exists()
    //       and call that on our watch instead of this load function
    //       (at least I'm pretty sure we could tweak things that way)
    //       this would allow us to change the "building" to "packaging"
    //       in the interface before attempting the `curl --head ...` calls
    //
    if(!loading
    && get_building() == building_t::BUILDING_PACKAGING)
    {
        // we want to make sure that the .deb are indeed available
        // before marking the system as built
        //
        if(dot_deb_exists())
        {
            set_building(building_t::BUILDING_NOT_BUILDING);

            // delete the flag, we are done with it
            //
            mark_as_done_building();

            check_state();

            SNAP_LOG_INFO
                << "Done building \""
                << f_name
                << "\", new status is: \""
                << get_build_status_string()
                << "\""
                << SNAP_LOG_SEND;
        }
    }
}

/* example of an enry
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

  example of URL to a built .deb file:
  https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+files/snapdev_1.1.34.0~jammy_amd64.deb
*/


// TODO: not used yet although I'd like to test all the files instead of
//       just the main one and the package name is what we need for that;
//       however, at the moment, the input filename is incorrect (we're
//       not in this project folder so just reading debian/control is
//       not going to work well)
//
void project::read_control()
{
    if(!f_control_info.empty())
    {
        return;
    }

    // load the control file
    //
    std::ifstream in(f_project_path + "/debian/control");
    int entry(0);
    definition_t def;
    std::string line;
    while(getline(in, line))
    {
        if(line.empty())
        {
            if(entry == 0)
            {
                f_control_info = def;
            }
            else
            {
                auto const it(def.find("package"));
                if(it == def.end())
                {
                    SNAP_LOG_ERROR
                        << "found a package section without a name in \""
                        << f_name
                        << "\" project."
                        << SNAP_LOG_SEND;
                }
                else
                {
                    f_control_packages[it->second] = def;
                }
            }
            def.clear();
            ++entry;
        }
        else
        {
            std::string::size_type const pos(line.find(':'));
            std::string const name(line.substr(0, pos));
            std::string value(line.substr(pos + 1));
            while(value.back() == '\\')
            {
                value.pop_back();
                getline(in, line);
                if(line.empty())
                {
                    SNAP_LOG_ERROR
                        << "found a field continuation followed by an empty line in \""
                        << f_name
                        << "\" project."
                        << SNAP_LOG_SEND;
                    return;
                }
                value += ' ';
                value += snapdev::trim_string(line);
            }
            def[snapdev::to_lower(name)] = snapdev::trim_string(value);
        }
    }
}


bool project::dot_deb_exists()
{
    // the URL looks like this:
    // https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+files/snapdev_1.1.34.0~jammy_amd64.deb
    // and a HEAD against it has to return a 3XX result if the file
    // exists, otherwise it returns a 404. If it returns a 5XX, then
    // we probably need to try again later
    //
    // TODO: we need to check all entries until a 3XX is returned and then
    //       stop checking that specific entry
    //
    read_control();
    std::string const remote_version(get_remote_version());
    for(auto codename_and_arch : f_list_of_codenames_and_archs)
    {
        std::unique_ptr<CURL, decltype(&::curl_easy_cleanup)> curl(curl_easy_init(), &::curl_easy_cleanup);
        if(curl == nullptr)
        {
            SNAP_LOG_ERROR
                << "could not properly initialize curl to check .deb availability for the \""
                << f_name
                << "\" project."
                << SNAP_LOG_SEND;
            return false;
        }

        std::string::size_type const pos(codename_and_arch.find(':'));
        std::string const codename(codename_and_arch.substr(0, pos));
        std::string const arch(codename_and_arch.substr(pos + 1));

        // check each package name, all the packages need to be available
        // not just the main one (although the name of the project we
        // have in our git environment is not always the name of the main
        // package as for cmake and advgetopt)
        //
        for(auto it(f_control_packages.cbegin()); it != f_control_packages.cend(); ++it)
        {
            std::string architecture(arch);
            auto const package_arch(it->second.find("architecture"));
            if(package_arch != it->second.end())
            {
                if(package_arch->second == "any")
                {
                    architecture = arch;
                }
                else if(package_arch->second == "all")
                {
                    architecture = "all";
                }
                else if(package_arch->second != arch)
                {
                    // in all other cases it is specific and if we're not
                    // compiling for that specific architecture, then that
                    // package won't be created
                    //
                    // TODO: this is not 100% true since we have wildcards
                    //       but in our case that should not matter because
                    //       we only need to specify the CPU and just "any"
                    //       is sufficient and already supported
                    //       (i.e. at the moment we do not support <os>-any
                    //       or any-<cpu>).
                    //
                    //       There is, it seems, a special "any all" case
                    //       but I think that's only in the .dsc file.
                    //
                    //       Finally, the architecture could be a list of
                    //       os/cpu separated by spaces so we should check
                    //       whether any one of them is a match
                    //
                    SNAP_LOG_TODO
                        << "skipping architecture \""
                        << package_arch->second
                        << "\" which is not a match; expected \""
                        << arch
                        << "\"."
                        << SNAP_LOG_SEND;
                    continue;
                }
            }
            else
            {
                // this cannot happen unless your control file is invalid
                // (i.e. the Architecture: ... field is mandatory)
                //
                SNAP_LOG_MAJOR
                    << "no \"Architecture: ...\" field found in control file for package \""
                    << it->first
                    << "\"; using default architecture value: \""
                    << arch
                    << "\"."
                    << SNAP_LOG_SEND;
            }
            std::string url("https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+files/");
            url += it->first;
            url += '_';
            url += remote_version;
            url += '~';
            url += codename;
            url += '_';
            url += architecture;
            url += ".deb";

            if(f_package_statuses[url])
            {
                // we already found that package, no need to check for it again
                //
                continue;
            }

            SNAP_LOG_NOTICE
                << "checking whether .deb exists with `curl --head "
                << url
                << "`"
                << SNAP_LOG_SEND;

            curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "HEAD");
            curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_DEFAULT_PROTOCOL, "https");
            curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, g_curl_user_agent.data());

            CURLcode const res(curl_easy_perform(curl.get()));
            if(res != CURLE_OK)
            {
                SNAP_LOG_ERROR
                    << "curl HEAD to \""
                    << url
                    << "\" failed. ("
                    << curl_easy_strerror(res)
                    << ")."
                    << SNAP_LOG_SEND;
                return false;
            }

            long http_code(599);
            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
            if(http_code >= 400)
            {
                SNAP_LOG_WARNING
                    << "curl HEAD to \""
                    << url
                    << "\" return HTTP error code: "
                    << http_code
                    << ". The package is not yet ready."
                    << SNAP_LOG_SEND;
                return false;
            }

            // TBD: should we verify that it is a 301, 302, 303, 306, or 307?

            f_package_statuses[url] = true;
        }
    }

    // all the .deb seem to be available at the moment
    //
    return true;
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
    must_be_background_thread();

    std::string cmd("wget -q --user-agent='");
    cmd += g_wget_user_agent;
    cmd += "' -O '";
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
        SNAP_LOG_WARNING
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


bool project::is_building() const
{
    guard_project;
    return f_building != building_t::BUILDING_NOT_BUILDING;
}


bool project::is_packaging() const
{
    guard_project;
    return f_building == building_t::BUILDING_PACKAGING;
}


void project::start_build()
{
    must_be_background_thread();

    std::string cmd(f_snap_builder->get_root_path());
    cmd += "/bin/send-to-launchpad.sh ";
    cmd += f_name;

    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        SNAP_LOG_ERROR
            << "could not properly start building project \""
            << f_name
            << "\"."
            << SNAP_LOG_SEND;
        return;
    }

    // create a <project-name>.building flag in the cache folder, as long as
    // this is there, we want to continue checking the status on launchpad
    // until the package is built or it failed
    //
    std::ofstream flag;
    flag.open(get_flag_filename());
    if(flag.is_open())
    {
        time_t const now(time(nullptr));
        tm t;
        gmtime_r(&now, &t);
        char buf[256];
        buf[0] = '\0';
        strftime(buf, sizeof(buf) - 1, "Date: %y/%m/%d %H:%M:%S", &t);
        buf[sizeof(buf) - 1] = '\0';
        flag << buf << "\n"
                "Version: " << get_version() << '\n';
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
    {
        guard_project;
        f_build_hash = f_last_commit_hash;
    }

    std::ofstream hash;
    hash.open(get_build_hash_filename());
    if(hash.is_open())
    {
        guard_project;
        hash << f_last_commit_hash << std::endl;
    }

    set_building(building_t::BUILDING_COMPILING);
}


void project::set_building(building_t building)
{
    guard_project;
    f_building = building;
}


project::building_t project::get_building() const
{
    guard_project;
    return f_building;
}


void project::set_build_status(build_status_t status)
{
    guard_project;
    f_build_status = status;
}


project::build_status_t project::get_build_status() const
{
    guard_project;
    return f_build_status;
}


char const * project::get_build_status_string() const
{
    build_status_t const build_status(get_build_status());

    if(build_status == build_status_t::BUILD_STATUS_UNKNOWN)
    {
        return "unknown";
    }

    if(build_status == build_status_t::BUILD_STATUS_SUCCEEDED)
    {
        return "Build succeeded";
    }

    return "Build failed";
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


QColor project::get_state_color() const
{
    // TODO: properly handle the default background color
    //
    QColor color(255, 255, 255);

    std::string state(get_state());
    if(state.empty())
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
        state = "?";
#pragma GCC diagnostic pop
    }

    bool found(true);
    switch(state[0])
    {
    case 'b':
        if(state == "bad version")
        {
            // there are changes in your local version but the version is
            // the same as a successful build on the remote (i.e. you need
            // to click on "Edit Changelog")
            //
            color = QColor(222, 119, 153);
        }
        else if(state == "building")
        {
            // the project is being built right now
            //
            color = QColor(211, 255, 78);
        }
        else if(state == "build failed")
        {
            // the last build failed
            //
            color = QColor(243, 140, 246);
        }
        else if(state == "built")
        {
            // the last build succeeded and we do not have changes on our end
            //
            color = QColor(240, 255, 240);
        }
        else
        {
            found = false;
        }
        break;

    case 'n':
        if(state == "never built")
        {
            // this means we never got info from the remote (or the file
            // is empty) and that means it was never built there
            //
            color = QColor(200, 200, 200);
        }
        else if(state == "not committed")
        {
            color = QColor(255, 248, 240);
        }
        else if(state == "not pushed")
        {
            color = QColor(255, 240, 230);
        }
        else
        {
            found = false;
        }
        break;

    case 'p':
        if(state == "packaging")
        {
            // the project is being packaged (built but .deb not yet available)
            //
            color = QColor(225, 255, 78);
        }
        else
        {
            found = false;
        }
        break;

    case 'r':
        if(state == "ready")
        {
            // this is the default
            //
            color = QColor(255, 255, 255);
        }
        else
        {
            found = false;
        }
        break;

    case 's':
        if(state == "sending")
        {
            // the project is being built right now
            //
            color = QColor(78, 237, 255);
        }
        else
        {
            found = false;
        }
        break;

    }

    if(!found)
    {
        SNAP_LOG_WARNING
            << "unknown (unhandled) project state -> color: \""
            << state
            << "\"."
            << SNAP_LOG_SEND;
    }

    return color;
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

        // define background color
        //
        QColor const color(p->get_state_color());
        std::stringstream style;
        style
            << "style=filled,color=black,fillcolor=\"#"
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << color.red()
            << color.green()
            << color.blue()
            << "\"";

            // The URL is not useful at the moment and probably won't be even
            // to support clicks on packages to open a popup menu
            //
            // See https://forum.qt.io/topic/99524/qsvgwidget-and-uris-can-i-emit-a-signal-by-clicking-on-a-link-in-an-svg-image/2
            //
            // A user says we can use QSvgRenderer::boundsOnElement(<id>) where
            // the <id> would be the project name in our case. Then with a
            // derived QSvgWidget of our own, we can capture clicks and check
            // against those bounds. If one clicked inside an element, open
            // a popup menu
            //
            //<< "\",URL=\"http://snapwebsites.org/project/"
            //<< p->get_name()

        dependencies_t const dependencies(p->get_trimmed_dependencies());
        if(!dependencies.empty())
        {
            dot << "\"" << p->get_name() << "\" [shape=box," << style.str() << "];\n";
            for(auto const & n : dependencies)
            {
                dot << "\"" << p->get_name() << "\" -> \"" << n << "\";\n";
            }
        }
        else
        {
            dot << "\"" << p->get_name() << "\" [shape=ellipse," << style.str() << "];\n";
        }
    }
    dot << "}\n";

//std::cerr
//    << "------------------------------------- dot file\n"
//    << dot
//    << "-------------------------------------\n";

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


std::string project::get_error() const
{
    guard_project;
    return f_error_message;
}


void project::clear_error()
{
    guard_project;
    f_error_message.clear();
}


void project::add_error(std::string const & msg)
{
    guard_project;
    f_error_message += msg;
    if(msg.back() != '\n')
    {
        f_error_message += '\n';
    }
}


void project::must_be_background_thread()
{
    if(!f_snap_builder->is_background_thread())
    {
        SNAP_LOG_FATAL
            << "this function was called from the main thread when it should only be called by the background thread."
            << SNAP_LOG_SEND;
        throw std::runtime_error("this function was called from the main thread when it should only be called by the background thread.");
    }
}



} // builder namespace
// vim: ts=4 sw=4 et
