// Snap Websites Server -- Memory watchdog: record memory usage over time.
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

// self
//
#include "snapwatchdog.h"


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
namespace memory
{


enum class name_t
{
    SNAP_NAME_WATCHDOG_MEMORY_NAME
};
char const * get_name(name_t name) __attribute__ ((const));


DECLARE_LOGIC_ERROR(memory_logic_error);

DECLARE_MAIN_EXCEPTION(memory_exception);

DECLARE_EXCEPTION(memory_exception, memory_exception_invalid_argument);



class memory
    : public cppthread::plugin
{
public:
                        memory();
                        memory(memory const & rhs) = delete;
    virtual             ~memory() override;

    memory &            operator = (memory const & rhs) = delete;

    static memory *     instance();

    // cppthread::plugin implementation
    virtual void        bootstrap(void * snap) override;
    virtual int64_t     do_update(int64_t last_updated) override;

    // server signal
    void                on_process_watch(QDomDocument doc);

private:
    watchdog_child *    f_snap = nullptr;
};

} // namespace memory
} // namespace snap
// vim: ts=4 sw=4 et
