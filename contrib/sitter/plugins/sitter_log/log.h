// Snap Websites Server -- Log watchdog: report log existance/size issues.
// Copyright (c) 2018-2019  Made to Order Software Corp.  All Rights Reserved
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
#include    "snapwatchdog/log_definitions.h"
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
namespace log
{

enum class name_t
{
    SNAP_NAME_WATCHDOG_LOG_IGNORE
};
char const * get_name(name_t name) __attribute__ ((const));




DECLARE_MAIN_EXCEPTION(log_exception);

DECLARE_EXCEPTION(log_exception, log_exception_invalid_io);






class log
    : public cppthread::plugin
{
public:
                        log();
                        log(log const & rhs) = delete;
    virtual             ~log() override;

    log &               operator = (log const & rhs) = delete;

    static log *        instance();

    // plugins::plugin implementation
    virtual void        bootstrap(snap_child * snap) override;
    virtual int64_t     do_update(int64_t last_updated) override;

    // server signal
    void                on_process_watch(QDomDocument doc);

private:
    void                check_log(int index, std::string filename, watchdog_log_t const & l, QDomElement e);

    watchdog_child *    f_snap = nullptr;
    bool                f_found = false;
};

} // namespace log
} // namespace snap
// vim: ts=4 sw=4 et
