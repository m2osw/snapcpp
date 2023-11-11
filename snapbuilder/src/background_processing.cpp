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
#include    "background_processing.h"

#include    "snap_builder.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// C++
//



namespace builder
{



job::job(work_t w)
    : f_work(w)
{
}


void job::set_snap_builder(snap_builder * sb)
{
    f_snap_builder = sb;
}


void job::set_project(project::pointer_t p)
{
    f_project = p;
}


project::pointer_t job::get_project() const
{
    return f_project;
}


snapdev::timespec_ex const & job::get_next_attempt() const
{
    return f_next_attempt;
}


/** \brief Process this job.
 *
 * This function is used to process this job.
 *
 * If the function returns true, then there is more work to be done. For
 * example, when we build a package, there are several stages. One single
 * `job` object manages all the states by returning `true` here (i.e. more
 * work has to be done) and setting the f_next_attempt parameter to when
 * the additional work should happen.
 *
 * \return true if the process is not finished but is on pause for a while.
 */
bool job::process()
{
    switch(f_work)
    {
    case work_t::WORK_UNKNOWN:
        SNAP_LOG_FATAL
            << "worker: cannot process a job of type WORK_UNKNOWN."
            << SNAP_LOG_SEND;
        throw std::runtime_error("cannot process a job of type WORK_UNKNOWN.");

    case work_t::WORK_LOAD_PROJECT:
        return load_project();

    case work_t::WORK_ADJUST_COLUMNS:
        return adjust_columns();

    }
    snapdev::NOT_REACHED();
}


bool job::load_project()
{
    SNAP_LOG_DEBUG
        << "worker: read project \""
        << f_project->get_name()
        << "\"."
        << SNAP_LOG_SEND;

    f_project->load_project();

    if(f_project->is_building())
    {
        // we need to continue to work on this one
        //
        // retry in 60 seconds
        //
        // TODO: look into testing one project per minute, with this
        //       implementation (like the older one) all the building
        //       projects are being checked in a row
        //
        snapdev::timespec_ex pause(60, 0);
        if(f_project->is_packaging())
        {
            // packaging is really slow, only check once every 5 min.
            //
            pause.tv_sec= 60 * 5;
        }
        snapdev::timespec_ex const now(snapdev::now());
        f_next_attempt = snapdev::now() + pause;
        return false;
    }

    return true;
}


bool job::adjust_columns()
{
    f_snap_builder->adjust_columns();

    return true;
}














background_worker::background_worker()
    : runner("worker")
{
}


void background_worker::send_job(job::pointer_t j)
{
    f_job_fifo.push_back(j);
}


void background_worker::run()
{
    while(continue_running())
    {
        std::int64_t const usecs(get_timeout());
        job::pointer_t j;
        if(f_job_fifo.pop_front(j, usecs))
        {
            if(!j->process())
            {
                f_extra_work.push_back(j);
            }
        }
        else
        {
            if(f_job_fifo.is_done())
            {
                // quitting, ignore anything else
                //
                break;
            }

            // the pop_front() function timed out, check the next piece
            // of work to process
            //
            if(f_extra_work.empty())
            {
                throw std::runtime_error("somehow f_work_fifo returned false when it is not done and there isn't extra work.");
            }
            if(f_extra_work.front()->process())
            {
                // done with that one
                //
                f_extra_work.erase(f_extra_work.begin());
            }
        }
    }
}


std::int64_t background_worker::get_timeout()
{
    std::int64_t usecs(-1);
    if(!f_extra_work.empty())
    {
        f_extra_work.sort([](job::pointer_t a, job::pointer_t b)
            {
                return a->get_next_attempt() < b->get_next_attempt();
            });
        snapdev::timespec_ex earliest(f_extra_work.front()->get_next_attempt());
        snapdev::timespec_ex now(snapdev::now());
        earliest -= now;
        usecs = std::max(earliest.to_usec(), 0L);
    }

    return usecs;
}


void background_worker::stop()
{
    f_job_fifo.done(false);
}




} // builder namespace
// vim: ts=4 sw=4 et
