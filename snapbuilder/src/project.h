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
#pragma once

// advgetopt
//
#include    <advgetopt/utils.h>


// eventdispatcher
//
#include    <cppprocess/process.h>


// Qt
//
#include    <QWidget>


// C++
//
#include    <memory>
#include    <set>



namespace builder
{


class snap_builder;

class project_remote_info
{
public:
    typedef std::shared_ptr<project_remote_info>    pointer_t;
    typedef std::vector<pointer_t>                  vector_t;

    void                    set_date(std::string const & date);
    void                    set_build_codename(std::string const & codename);
    void                    set_build_state(std::string const & build_state);
    void                    set_build_version(std::string const & build_version);
    void                    set_build_arch(std::string const & build_arch);

    std::string const &     get_date() const;
    std::string const &     get_build_codename() const;
    std::string const &     get_build_state() const;
    std::string const &     get_build_version() const;
    std::string const &     get_build_arch() const;

private:
    std::string             f_date = std::string();     // first of: date built, started build, created
    std::string             f_build_codename = std::string();
    std::string             f_build_state = std::string();
    std::string             f_build_version = std::string();
    std::string             f_build_arch = std::string();
};



class project
    : public std::enable_shared_from_this<project>
{
public:
    typedef std::shared_ptr<project>            pointer_t;
    typedef std::vector<pointer_t>              vector_t;
    typedef std::map<std::string, pointer_t>    map_t;
    typedef std::set<std::string>               dependencies_t;

                                project(
                                      snap_builder * parent
                                    , std::string const & name
                                    , advgetopt::string_list_t const & deps);
                                project(project const & rhs) = delete;
    project &                   operator = (project const & rhs) = delete;

    bool                        exists() const;
    bool                        is_valid() const;
    std::string                 get_error() const;
    void                        clear_error();
    std::string const &         get_name() const;
    std::string                 get_project_name() const;
    void                        set_version(std::string const & version);
    std::string                 get_version() const;
    std::string                 get_remote_version() const;
    void                        set_state(std::string const & state);
    std::string                 get_state() const;
    QColor                      get_state_color() const;
    time_t                      get_last_commit() const;
    std::string                 get_last_commit_as_string() const;
    std::string                 get_remote_build_state() const;
    std::string                 get_remote_build_date() const;
    dependencies_t              get_dependencies() const;
    dependencies_t              get_trimmed_dependencies() const;

    std::string                 get_ppa_json_filename() const;
    std::string                 get_flag_filename() const;
    void                        mark_as_done_building();
    std::string                 get_build_hash_filename() const;
    void                        load_remote_data(bool load);
    bool                        retrieve_ppa_status();
    bool                        is_building() const;
    bool                        is_packaging() const;

    bool                        operator < (project const & rhs) const;
    static void                 sort(vector_t & v);

    void                        project_changed();
    void                        load_project();
    void                        start_build();
    static void                 simplify(vector_t & v);
    static void                 generate_svg(
                                      vector_t & v
                                    , cppprocess::io::process_io_done_t output_captured);
    static void                 view_svg(vector_t & v, std::string const & root_path);

private:
    enum class building_t : std::uint8_t
    {
        BUILDING_NOT_BUILDING,      // not currently building
        BUILDING_COMPILING,         // until .json tells us "build succeeded"
        BUILDING_PACKAGING,         // until .deb are downloadable
    };

    enum class build_status_t : std::int8_t
    {
        BUILD_STATUS_UNKNOWN = -1,
        BUILD_STATUS_FAILED  = 0,
        BUILD_STATUS_SUCCEEDED = 1,
    };

    typedef std::map<std::string, std::string>          definition_t;
    typedef std::map<std::string, definition_t>         package_t;
    typedef std::map<std::string, bool>                 package_status_t;

    void                        add_dependency(std::string const & name);
    void                        add_missing_dependencies(pointer_t p, map_t & m);
    static bool                 compare(pointer_t a, pointer_t b);

    void                        clear_remote_info(std::size_t size);
    void                        add_remote_info(project_remote_info::pointer_t info);
    void                        find_project();
    bool                        retrieve_version();
    bool                        check_state();
    bool                        get_last_commit_timestamp();
    bool                        get_last_commit_hash();
    bool                        get_build_hash();
    void                        retrieve_building_state();
    project_remote_info::pointer_t
                                find_remote_info(
                                      std::string const & build_codename
                                    , std::string const & build_arch);
    bool                        dot_deb_exists();
    void                        set_building(building_t building);
    building_t                  get_building() const;
    void                        set_build_status(build_status_t status);
    build_status_t              get_build_status() const;
    char const *                get_build_status_string() const;
    void                        add_error(std::string const & msg);
    void                        must_be_background_thread();
    void                        read_control();

    snap_builder *              f_snap_builder = nullptr;
    std::string                 f_name = std::string();
    std::string                 f_project_path = std::string();
    std::string                 f_state = std::string();
    std::string                 f_error_message = std::string();
    std::string                 f_version = std::string();
    time_t                      f_last_commit = 0;
    std::string                 f_last_commit_hash = std::string();
    std::string                 f_build_hash = std::string();
    bool                        f_exists = false;
    bool                        f_loaded = false;
    bool                        f_valid = false;
    bool                        f_recursed_add_dependencies = false;
    building_t                  f_building = building_t::BUILDING_NOT_BUILDING;
    build_status_t              f_build_status = build_status_t::BUILD_STATUS_UNKNOWN;
    dependencies_t              f_dependencies = dependencies_t();
    dependencies_t              f_trimmed_dependencies = dependencies_t();
    project_remote_info::vector_t
                                f_remote_info = project_remote_info::vector_t();
    std::set<std::string>       f_list_of_codenames_and_archs = std::set<std::string>();
    definition_t                f_control_info = definition_t();
    package_t                   f_control_packages = package_t();
    package_status_t            f_package_statuses = package_status_t();
};



} // builder namespace



struct project_ptr
{
    builder::project::pointer_t     f_ptr = builder::project::pointer_t();

private:
    Q_GADGET
};

Q_DECLARE_METATYPE(project_ptr)

// vim: ts=4 sw=4 et
