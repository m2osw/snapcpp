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

// self
//
#include    "background_processing.h"


// C++ lib
//



namespace builder
{



background_processing::background_processing(snap_builder * sb)
    : timer(-1)
    , f_snap_builder(sb)
{
    // by default we do nothing
    //
    set_enable(false);
}


void background_processing::add_project(project::pointer_t p)
{
    f_projects.push_back(p);

    if(!is_enabled())
    {
        // we have at least one project to work on, enable the timer
        // this time it will time out immediately; depending on the
        // future tasks, the timeout may be much longer
        //
        set_enable(true);
        timeval now = {};
        gettimeofday(&now, nullptr);
        set_timeout_date(now.tv_sec * 1'000'000 + now.tv_usec);
    }
}


void background_processing::process_timeout()
{
}



} // builder namespace
// vim: ts=4 sw=4 et
