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

// self
//
#include    "project.h"


// cppthread
//
#include    <cppthread/fifo.h>
#include    <cppthread/runner.h>



namespace builder
{



class snap_builder;


class job
{
public:
    typedef std::shared_ptr<job>    pointer_t;
    typedef std::list<pointer_t>    list_t;

    enum class work_t
    {
        WORK_UNKNOWN, // if not properly defined, default

        WORK_LOAD_PROJECT,
        WORK_ADJUST_COLUMNS,
    };

                                    job(work_t w);
                                    job(job const &) = delete;
    job &                           operator = (job const &) = delete;

    //work_t                          get_work() const;

    void                            set_snap_builder(snap_builder * sb);

    void                            set_project(project::pointer_t p);
    project::pointer_t              get_project() const;

    //void                            set_next_attempt(int seconds_from_now);
    snapdev::timespec_ex const &    get_next_attempt() const;

    bool                            process();

private:
    bool                            load_project();
    bool                            adjust_columns();

    work_t                          f_work = work_t::WORK_UNKNOWN;
    project::pointer_t              f_project = project::pointer_t();
    snap_builder *                  f_snap_builder = nullptr;
    snapdev::timespec_ex            f_next_attempt = snapdev::timespec_ex();
};


class background_worker
    : public cppthread::runner
{
public:
    typedef std::shared_ptr<background_worker>
                                pointer_t;

                                background_worker();
                                background_worker(background_worker const &) = delete;
    background_worker &         operator = (background_worker const &) = delete;

    void                        send_job(job::pointer_t j);

    // cppthread::runner implementation
    //
    virtual void                run();

    void                        stop();

private:
    typedef cppthread::fifo<job::pointer_t>    job_fifo_t;

    std::int64_t                get_timeout();

    job_fifo_t                  f_job_fifo = job_fifo_t();
    job::list_t                 f_extra_work = job::list_t();
};



} // builder namespace
// vim: ts=4 sw=4 et
