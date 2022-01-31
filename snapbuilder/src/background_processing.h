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
#pragma once

// self
//
#include    "snap_builder.h"


// eventdispatcher lib
//
#include    <eventdispatcher/timer.h>









namespace builder
{



class background_processing
    : public ed::timer_connection
{
public:
                                background_processing(snap_builder * sb);
                                background_processing(background_processing const &) = delete;
    background_processing &     operator = (background_processing const &) = delete;

    void                        add_project(project::pointer_t p);

    // timer implementation
    void                        process_timeout();

private:
    snap_builder *              f_snap_builder = nullptr;
    project::deque_t            f_projects = project::deque_t();
};



} // builder namespace
// vim: ts=4 sw=4 et
