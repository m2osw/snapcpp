// Snap Websites Server -- APT watchdog: record apt-check results
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
// Snap Websites Server -- APT watchdog: record apt-check results
#pragma once

// sitter
//
#include    "sitter/sitter.h"


// cppthread
//
//#include    <cppthread/plugins.h>



namespace sitter
{
namespace apt
{


enum class name_t
{
    SNAP_NAME_SITTER_APT_NAME
};
char const * get_name(name_t name) __attribute__ ((const));



DECLARE_LOGIC_ERROR(apt_logic_error);

DECLARE_MAIN_EXCEPTION(apt_exception);

DECLARE_EXCEPTION(apt_exception, apt_exception_invalid_argument);



class apt
    : public cppthread::plugin
{
public:
                        apt();
                        apt(apt const & rhs) = delete;
    virtual             ~apt() override;

    apt &               operator = (apt const & rhs) = delete;

    static apt *        instance();

    // cppthread::plugin implementation
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(void * snap) override;

    // server signal
    void                on_process_watch(QDomDocument doc);

private:
    sitter *            f_snap = nullptr;
};


} // namespace apt
} // namespace snap
// vim: ts=4 sw=4 et
