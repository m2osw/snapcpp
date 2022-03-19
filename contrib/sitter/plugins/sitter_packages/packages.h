// Snap Websites Server -- watchdog packages
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
namespace packages
{


//enum class name_t
//{
//    SNAP_NAME_WATCHDOG_PACKAGES_CACHE_FILENAME,
//    SNAP_NAME_WATCHDOG_PACKAGES_PATH
//};
//char const * get_name(name_t name) __attribute__ ((const));



DECLARE_MAIN_EXCEPTION(packages_exception);

DECLARE_LOGIC_ERROR(packages_logic_error);

DECLARE_EXCEPTION(packages_exception, packages_exception_invalid_argument);
DECLARE_EXCEPTION(packages_exception, packages_exception_invalid_name);
DECLARE_EXCEPTION(packages_exception, packages_exception_invalid_priority);







class packages
    : public plugins::plugin
{
public:
                        packages();
                        packages(packages const & rhs) = delete;
    virtual             ~packages() override;

    packages &          operator = (packages const & rhs) = delete;

    static packages *   instance();

    // plugins::plugin implementation
    virtual QString     description() const override;
    virtual QString     dependencies() const override;
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // server signals
    void                on_process_watch(QDomDocument doc);

private:
    void                load_packages();
    void                load_xml(QString package_filename);

    watchdog_child *    f_snap = nullptr;
};

} // namespace packages
} // namespace snap
// vim: ts=4 sw=4 et
