// Snap Websites Server -- manage the snapmanager.cgi and snapmanagerdaemon
// Copyright (C) 2016  Made to Order Software Corp.
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

#include "lib/manager.h"
#include "lib/plugin_base.h"

#include <snapwebsites/plugins.h>

#include <QDomDocument>

namespace snap
{
namespace self
{


enum class name_t
{
    SNAP_NAME_SNAPMANAGERCGI_SELF_NAME
};
char const * get_name(name_t name) __attribute__ ((const));



class self_exception : public snap_exception
{
public:
    self_exception(char const *        what_msg) : snap_exception("self", what_msg) {}
    self_exception(std::string const & what_msg) : snap_exception("self", what_msg) {}
    self_exception(QString const &     what_msg) : snap_exception("self", what_msg) {}
};

class self_exception_invalid_argument : public self_exception
{
public:
    self_exception_invalid_argument(char const *        what_msg) : self_exception(what_msg) {}
    self_exception_invalid_argument(std::string const & what_msg) : self_exception(what_msg) {}
    self_exception_invalid_argument(QString const &     what_msg) : self_exception(what_msg) {}
};





class self
        : public snap_manager::plugin_base
{
public:
                            self();
    virtual                 ~self() override;

    // plugins::plugin implementation
    static self *           instance();
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // manager overload
    virtual bool            display_value(QDomElement parent, snap_manager::status_t const & s) override;

    // server signal
    void                    on_retrieve_status(snap_manager::server_status & server_status);

private:
    snap_manager::manager * f_snap = nullptr;
};

} // namespace self
} // namespace snap
// vim: ts=4 sw=4 et
