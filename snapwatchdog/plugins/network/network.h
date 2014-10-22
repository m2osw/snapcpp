// Snap Websites Server -- Network watchdog
// Copyright (C) 2013-2014  Made to Order Software Corp.
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

#include <snapwebsites/plugins.h>
#include <snapwebsites/snap_child.h>

#include <QDomDocument>

namespace snap
{
namespace network
{

class network_exception : public snap_exception
{
public:
    network_exception(char const *       what_msg) : snap_exception("network", what_msg) {}
    network_exception(std::string const& what_msg) : snap_exception("network", what_msg) {}
    network_exception(QString const&     what_msg) : snap_exception("network", what_msg) {}
};

class network_exception_invalid_argument : public network_exception
{
public:
    network_exception_invalid_argument(char const *       what_msg) : network_exception(what_msg) {}
    network_exception_invalid_argument(std::string const& what_msg) : network_exception(what_msg) {}
    network_exception_invalid_argument(QString const&     what_msg) : network_exception(what_msg) {}
};





enum name_t
{
    SNAP_NAME_WATCHDOG_NETWORK_NAME
};
char const *get_name(name_t name) __attribute__ ((const));


class network : public plugins::plugin
{
public:
                        network();
                        ~network();

    static network *    instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    void                on_init();
    void                on_process_watch(QDomDocument doc);

private:
    zpsnap_child_t      f_snap;
    QString             f_network_data_path;
};

} // namespace network
} // namespace snap
// vim: ts=4 sw=4 et
