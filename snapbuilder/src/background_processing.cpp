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


void job::set_next_attempt(int delay)
{
    snapdev::timespec_ex interval(delay, 0);
    snapdev::timespec_ex const now(snapdev::now());
    f_next_attempt = snapdev::now() + interval;
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
 * \param[in] w  The worker processing this job.
 *
 * \return true if the process is not finished but is on pause for a while.
 */
bool job::process(background_worker * w)
{
    switch(f_work)
    {
    case work_t::WORK_UNKNOWN:
        SNAP_LOG_FATAL
            << "worker: cannot process a job of type WORK_UNKNOWN."
            << SNAP_LOG_SEND;
        throw std::runtime_error("cannot process a job of type WORK_UNKNOWN.");

    case work_t::WORK_LOAD_PROJECT:
        return load_project(w);

    case work_t::WORK_ADJUST_COLUMNS:
        return adjust_columns();

    case work_t::WORK_GIT_PUSH:
        return git_push();

    case work_t::WORK_RETRIEVE_PPA_STATUS:
        return retrieve_ppa_status();

    case work_t::WORK_START_BUILD:
        return start_build(w);

    case work_t::WORK_WATCH_BUILD:
        return watch_build();

    }
    snapdev::NOT_REACHED();
}


bool job::load_project(background_worker * w)
{
    SNAP_LOG_DEBUG
        << "worker: read project \""
        << f_project->get_name()
        << "\"."
        << SNAP_LOG_SEND;

    f_project->load_project();
    f_project->project_changed();

    if(f_project->is_building())
    {
        // watch this build
        //
        job::pointer_t j(std::make_shared<job>(job::work_t::WORK_WATCH_BUILD));
        j->set_project(f_project);
        w->send_job(j);
    }

    return true;
}


bool job::adjust_columns()
{
    f_snap_builder->adjust_columns();

    return true;
}


bool job::git_push()
{
    f_snap_builder->process_git_push(f_project);

    return true;
}


bool job::retrieve_ppa_status()
{
    // try to get the remote data, if it fails, try again up to 5 times
    //
    bool success(f_project->retrieve_ppa_status());
    if(!success
    && f_retries < 5)
    {
        ++f_retries;
        set_next_attempt(60 * 3 * f_retries);
        return false;
    }

    // we just updated the PPA status file so we force a reload of the
    // remote data to see the results
    //
    f_project->load_remote_data(true);
    f_project->project_changed();

    return true;
}


bool job::start_build(background_worker * w)
{
    f_project->start_build();

    job::pointer_t j(std::make_shared<job>(job::work_t::WORK_WATCH_BUILD));
    j->set_project(f_project);
    w->send_job(j);

    return true;
}


bool job::watch_build()
{
    if(!f_project->is_valid())
    {
        SNAP_LOG_ERROR
            << "watch_build() called with an invalid project."
            << SNAP_LOG_SEND;
        return true;
    }

    if(!f_project->is_building())
    {
        SNAP_LOG_RECOVERABLE_ERROR
            << "watch_build() called with a project that is not being built."
            << SNAP_LOG_SEND;
        return true;
    }

    if(!f_project->retrieve_ppa_status())
    {
        // we need to continue to work on this one
        //
        // retry in 60 seconds
        //
        // TODO: look into testing one project per minute, with this
        //       implementation (like the older one) all the building
        //       projects are being checked in a row
        //
        // packaging is really slow, only check once every 5 min.
        //
        set_next_attempt(f_project->is_packaging() ? 60 * 5 : 60);
        return false;
    }

    f_project->load_remote_data(false);
    f_project->project_changed();

    if(f_project->is_building())
    {
        // as above, while building, we need to repeat the check over and
        // over until everything is done one way or the other
        //
        set_next_attempt(f_project->is_packaging() ? 60 * 5 : 60);
        return false;
    }

    // success
    //
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
            if(!j->process(this))
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
            if(f_extra_work.front()->process(this))
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
