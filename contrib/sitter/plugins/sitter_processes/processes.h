// Snap Websites Server -- watchdog processes
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

// self
//
#include    "snapwatchdog/snapwatchdog.h"


// snapwebsites
//
#include    <snapwebsites/snap_child.h>


// cppthread
//
//#include    <cppthread/plugins.h>


// Qt
//
#include    <QDomDocument>



namespace snap
{
namespace processes
{


enum class name_t
{
    SNAP_NAME_WATCHDOG_PROCESSES_PATH
};
char const * get_name(name_t name) __attribute__ ((const));


DECLARE_MAIN_EXCEPTION(processes_exception);

DECLARE_EXCEPTION(processes_exception, processes_exception_invalid_argument);
DECLARE_EXCEPTION(processes_exception, processes_exception_invalid_process_name);



class processes
    : public cppthread::plugin
{
public:
                        processes();
                        processes(processes const & rhs) = delete;
    virtual             ~processes() override;

    processes &         operator = (processes const & rhs) = delete;

    static processes *  instance();

    // cppthread::plugin implementation
    virtual void        bootstrap(void * snap) override;
    virtual int64_t     do_update(int64_t last_updated) override;

    // server signals
    void                on_process_watch(QDomDocument doc);

private:
    watchdog_child *    f_snap = nullptr;
};

} // namespace processes
} // namespace snap
// vim: ts=4 sw=4 et
