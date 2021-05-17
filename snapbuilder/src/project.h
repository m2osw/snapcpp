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
#pragma once

// advgetopt lib
//
#include    <advgetopt/utils.h>


// C++ lib
//
#include    <memory>



namespace builder
{


class snap_builder;

class project
{
public:
    typedef std::shared_ptr<project>            pointer_t;
    typedef std::vector<pointer_t>              vector_t;

                                project(
                                      snap_builder * parent
                                    , std::string const & name
                                    , advgetopt::string_list_t const & deps);
                                project(project const & rhs) = delete;
    project                     operator = (project const & rhs) = delete;

    bool                        is_valid() const;
    std::string const &         get_name() const;
    std::string const &         get_version() const;
    std::string const &         get_state() const;
    time_t                      get_last_commit() const;
    std::string                 get_last_commit_as_string() const;
    advgetopt::string_list_t    get_dependencies() const;

    bool                        operator < (project const & rhs) const;
    static void                 sort(std::vector<pointer_t> & vector);

private:
    static bool                 compare(pointer_t a, pointer_t b);

    bool                        find_project();
    void                        load_project();
    bool                        retrieve_version();
    bool                        check_state();
    bool                        get_last_commit_timestamp();
    void                        load_remote_data();

    snap_builder *              f_snap_builder = nullptr;
    std::string                 f_name = std::string();
    std::string                 f_project_path = std::string();
    std::string                 f_state = std::string();
    std::string                 f_version = std::string();
    time_t                      f_last_commit = 0;
    bool                        f_valid = false;
    advgetopt::string_list_t    f_dependencies = advgetopt::string_list_t();
};



} // builder namespace
// vim: ts=4 sw=4 et
