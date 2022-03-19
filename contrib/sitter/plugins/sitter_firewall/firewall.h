// Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved.
//
// https://snapwebsites.org/project/sitter
// contact@m2osw.com
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// Snap Websites Server -- watchdog firewall
#pragma once

// sitter
//
#include    <sitter/sitter.h>


// cppthread
//
//#include    <cppthread/plugins.h>



namespace snap
{
namespace firewall
{

//enum class name_t
//{
//    SNAP_NAME_WATCHDOG_FIREWALL_NAME
//};
//char const * get_name(name_t name) __attribute__ ((const));







class firewall
    : public cppthread::plugin
{
public:
                        firewall();
                        firewall(firewall const & rhs) = delete;
    virtual             ~firewall() override;

    firewall &          operator = (firewall const & rhs) = delete;

    static firewall *   instance();

    // cppthread::plugin implementation
    virtual void        bootstrap(void * snap) override;
    virtual int64_t     do_update(int64_t last_updated) override;

    // server signals
    void                on_process_watch(QDomDocument doc);

private:
    watchdog_child *    f_snap = nullptr;
};

} // namespace firewall
} // namespace snap
// vim: ts=4 sw=4 et
