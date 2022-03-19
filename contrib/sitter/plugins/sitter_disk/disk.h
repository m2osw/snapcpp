// Snap Websites Server -- Disk watchdog: report disk usage over time.
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

// sitter
//
#include    <sitter/sitter.h>


// cppthread
//
//#include    <cppthread/plugins.h>



namespace snap
{
namespace disk
{

//enum class name_t
//{
//    SNAP_NAME_WATCHDOG_DISK_IGNORE
//};
//char const * get_name(name_t name) __attribute__ ((const));


DECLARE_LOGIC_ERROR(disk_logic_error);

DECLARE_MAIN_EXCEPTION(disk_exception);

DECLARE_EXCEPTION(disk_exception, disk_exception_invalid_io);



class disk
    : public cppthread::plugin
{
public:
                        disk();
                        disk(disk const & rhs) = delete;
    virtual             ~disk() override;

    disk &              operator = (disk const & rhs) = delete;

    static disk *       instance();

    // plugins::plugin implementation
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(void * snap) override;

    // server signal
    void                on_process_watch(QDomDocument doc);

private:
    watchdog_child *    f_snap = nullptr;
};

} // namespace disk
} // namespace snap
// vim: ts=4 sw=4 et
